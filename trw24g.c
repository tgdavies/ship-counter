#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "avrutils.h"
#include "trw24g.h"

void init_trw24g() {
    red(1);
    // CE and CS are output
    CE_PORT = 0;
    CS_PORT = 0;
    CE_DDR = 1;
    CS_DDR = 1;
    // CLK1 and DATA are output
    CLK1_PORT = 0; // must start low
    DATA_PORT = 0;
    CLK1_DDR = 1;
    DATA_DDR = 1; // will be input in some modes
    
    MCUCR = 0x03; // look for rising edge on INT0
    // enable interrupt INT0 for RX signal
    GIMSK |= (1 << INT0);
    
    _delay_ms(4); // wait for > Tpd2sby
    red(0); 
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
    _delay_us(200); // wait > Tcs2data or Tce2data
}
static uint8_t channel;
static uint8_t address;



static uint8_t* buffer;
static void (*recv_callback)();
static uint8_t i; // loop counter

void config_trw24g(uint8_t addr, uint8_t chan, uint8_t* buf, void (*callback)()) {
    address = addr;
    channel = chan;
    buffer = buf;
    recv_callback = callback;
    send_config(RX);
}

void send_config(uint8_t tx_rx) {
    
    DATA_DDR = 1;
    // send TEST section
    set_mode_trw24g(MODE_CONFIG);

    send_byte_trw24g(0x8E);
    send_byte_trw24g(0x08);
    send_byte_trw24g(0x1C); // PLL control bits zero
    // send configuration
    send_byte_trw24g(DATA_W); // channel 2 (unused)
    send_byte_trw24g(DATA_W); // channel 1 bits per packet
    for (i = 0; i < 9; ++i) { // all zeroes of address for channel 2, leading zeroes of address for channel 1
        send_byte_trw24g(0);
    }
    send_byte_trw24g(address);
    send_byte_trw24g((ADDR_W << 2) | 3); // address width and enable 16 bit CRC
    send_byte_trw24g(CM_SHOCKBURST | RFDR_SB_250K | XO_F_16M | RF_PWR_0);
    send_byte_trw24g((channel << 1) | tx_rx); // receive mode
    set_mode_trw24g(MODE_STANDBY);
}

void set_txrx_trw24g(uint8_t mode) {
    send_config(mode);
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
    _delay_ms(20); // wait for send, probably unnecessary
    sei(); // enable interrupts
}

void start_recv_trw24g() {
    set_txrx_trw24g(RX);
    set_mode_trw24g(MODE_ACTIVE);
    sei();
}

ISR(INT0_vect) {
    green(1);
    DATA_DDR = 0; // DATA is an input
    set_mode_trw24g(MODE_STANDBY);
    static uint8_t i;
    for (i = 0; i < DATA_W / 8; ++i) {
        buffer[i] = read_byte_trw24g();
    }
    (*recv_callback)();
    set_mode_trw24g(MODE_ACTIVE);
    //green(0);
}


