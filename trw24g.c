#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "avrutils.h"
#include "trw24g.h"

void init_trw24g() {
    // CLK1 and DATA are output
    CLK1_DDR = 1;
    DATA_DDR = 1; // will be input in some modes
    CLK1_PORT = 0; // must start low
    
    // CE and CS are output
    CE_DDR = 1;
    CS_DDR = 1;
    
    MCUCR = 0x03; // look for rising edge on INT0
    // enable interrupt INT0 for RX signal
    GIMSK |= (1 << INT0);
    
    _delay_ms(4); // wait for > Tpd2sby
    red(1);
    green(1);
    
}

#define send_bit(byte, n) \
        DATA_PORT = (byte >> n) & 1; \
        _delay_us(1); \
        CLK1_PORT = 1; \
        _delay_us(1); \
        CLK1_PORT = 0;

// CLK1 must be low on entry
// DATA must be an output
void send_byte_trw24g(uint8_t byte) {
    send_bit(byte, 7);
    send_bit(byte, 6);
    send_bit(byte, 5);
    send_bit(byte, 4);
    send_bit(byte, 3);
    send_bit(byte, 2);
    send_bit(byte, 1);
    send_bit(byte, 0);
}

// CLK1 must be low on entry
// DATA must be an input
uint8_t read_byte_trw24g() {
    static uint8_t data;
    static uint8_t i;
    data = 0;
    for (i = 0; i < 8; ++i) {
        data <<= 1;
        CLK1_PORT = 1;
        _delay_us(1);
        data |= DATA_PIN;
        CLK1_PORT = 0;
        _delay_us(1);
    }
    return data;
}

void set_mode_trw24g(uint8_t mode) {
    CE_PORT = (mode & 0x2) >> 1;
    CS_PORT = (mode & 0x1);
    _delay_us(10); // wait > Tcs2data or Tce2data
}

void set_txrx_trw24g(uint8_t mode) {
    set_mode_trw24g(MODE_CONFIG);
    DATA_DDR = 1;
    send_bit(mode, 0);
    set_mode_trw24g(MODE_STANDBY);
}

static uint8_t* buffer;
static void (*recv_callback)();
static uint8_t i; // loop counter

void config_trw24g(uint8_t addr, uint8_t channel, uint8_t* buf, void (*callback)()) {
    buffer = buf;
    recv_callback = callback;
    DATA_DDR = 1;
    // send TEST section
    send_byte_trw24g(0x8E);
    send_byte_trw24g(0x08);
    send_byte_trw24g(0x1C); // PLL control bits zero
    // send configuration
    send_byte_trw24g(DATA_W); // channel 2 (unused)
    send_byte_trw24g(DATA_W); // channel 1 bits per packet
    for (i = 0; i < 9; ++i) { // all zeroes of address for channel 2, leading zeroes of address for channel 1
        send_byte_trw24g(0);
    }
    send_byte_trw24g(addr);
    send_byte_trw24g((ADDR_W << 2) | 3); // address width and enable 16 bit CRC
    send_byte_trw24g(0x4F); // shockburst mode, 250 kbps, 16MHz crystal, 0dBm transmit power
    send_byte_trw24g((channel << 1) | 1); // receive mode
    set_mode_trw24g(MODE_STANDBY);
}

// assumes data is already set up in buffer
// note that we are in standby mode after this completes, you need to call
// start_recv_trw24g() after calling this.
void send_trw24g(uint8_t addr) {
    cli(); // disable interrupts while we send TODO just disable the receive interrupt?
    set_txrx_trw24g(TX);
    set_mode_trw24g(MODE_ACTIVE);
    send_byte_trw24g(addr);
    for (i = 0; i < DATA_W/8; ++i) {
        send_byte_trw24g(buffer[i]);
    }
    set_mode_trw24g(MODE_STANDBY);
    _delay_ms(2); // wait for send, probably unnecessary
    sei(); // enable interrupts
}

void start_recv_trw24g() {
    set_txrx_trw24g(RX);
    set_mode_trw24g(MODE_ACTIVE);
    sei();
}

ISR(INT0_vect) {
    DATA_DDR = 0; // DATA is an input
    set_mode_trw24g(MODE_STANDBY);
    static uint8_t i;
    for (i = 0; i < DATA_W / 8; ++i) {
        buffer[i] = read_byte_trw24g();
    }
    (*recv_callback)();
    set_mode_trw24g(MODE_ACTIVE);
}


