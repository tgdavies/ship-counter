#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "avrutils.h"
#include "trw24g.h"
#include "spi_via_usi_driver.h"


#ifdef NODE_1
#define MY_ADDR (0x02)
#define OTHER_ADDR (0x01)
#else
#define MY_ADDR (0x01)
#define OTHER_ADDR (0x02)
#endif

// first byte of data packet is 1 for data, 0 for ack
// second byte is packet number, or number being acknowledged
uint8_t rx_buffer[DATA_W / 8];
uint8_t tx_buffer[DATA_W / 8];
uint8_t last_sent_packet_no = 0;
uint8_t last_ack_received_packet_no = 0;
uint8_t last_received_packet_no = 0;
uint8_t ack_sent = 1;
uint8_t data_buffer[DATA_W / 8 - 2];
#define TIMEOUT_COUNT (5)
uint8_t time_since_send = 0;

/**
 * This is being called from an ISR, so don't do any work
 */
void packet_received() {
    if (rx_buffer[0] == 0x01) { // data
        if (last_received_packet_no != rx_buffer[1]) {
                //green(1);
                last_received_packet_no = rx_buffer[1];
                // this was a new packet
        }
        ack_sent = 0; // either acking a new packet or resending the ack for the previous packet
    } else if (rx_buffer[0] == 0x00) { // ack
        last_ack_received_packet_no = rx_buffer[1];
        red(0);
    }
}

/**
 * Call with buffer filled -- the first two bytes will be overwritten here
 */
void send_packet() {
    tx_buffer[0] = 0x01;
    tx_buffer[1] = last_sent_packet_no;
    for (int i = 0; i < DATA_W / 8 - 2; ++i) {
        tx_buffer[i+2] = data_buffer[i];
    }
    send_trw24g(OTHER_ADDR);
    time_since_send = 0;
}

void send_ack() {
    tx_buffer[0] = 0x00;
    tx_buffer[1] = last_received_packet_no;
    for (int i = 0; i < DATA_W / 8 - 2; ++i) {
        tx_buffer[i+2] = 0;
    }
    send_trw24g(OTHER_ADDR);
}

/**
 * Must not send another packet until we have an ack for the previous one
 * @return 
 */
uint8_t packet_acked() {
    return last_ack_received_packet_no == last_sent_packet_no;
}

/**
 * Called periodically to send acks and resend packages
 */
void poll_protocol() {
    red(0);
    // do we need to send an ack?
    if (last_received_packet_no != 0 && ack_sent == 0) {
        send_ack(OTHER_ADDR, last_received_packet_no);
        ack_sent = 1;
    }
    ++time_since_send;
    if (!packet_acked() && time_since_send > 5) {
        //red(1);
        send_packet();
        time_since_send = 0;
        //red(0);
    }
}

static uint8_t count = 0;

int main(void) {
    // set the clock prescaler to 1 to get us an 8MHz clock -- the CKDIV8 fuse is programmed by default, so initial prescaler is /8
    CLKPR = 0x80;
    CLKPR = 0x00;
    PORTA_byte.val = 0;
    PORTB_byte.val = 0;
    setup_leds();
    red(0);
    green(0);
    spiX_initslave(0);
    init_trw24g();
    config_trw24g(MY_ADDR, 0x30, rx_buffer, tx_buffer, packet_received);
    start_recv_trw24g();
    if (MY_ADDR == 0x02) {
        _delay_ms(500);
    }
    for (;;) {
        count++;
        sei();
        red(1);
        spiX_wait();
        red(0);
        uint8_t c = (count & 0x0f) | 0x30;
        green(1);
        spiX_put(c);
        
        //spiX_get();
        if (MY_ADDR == 0x01) {
            _delay_ms(90);
        }
        _delay_ms(100);
        //red(1);
        _delay_ms(100);
        if (packet_acked()) {
            for (uint8_t i = 0; i < DATA_W / 8 - 2; ++i) {
                data_buffer[i] = count;
            }
            ++last_sent_packet_no;
            send_packet();
        }
        poll_protocol();
        //red(0);
        green(0);
        start_recv_trw24g();
    }
    return 0; /* never reached */
}

