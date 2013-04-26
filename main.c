#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "avrutils.h"
#include "trw24g.h"
#include "spi_via_usi_driver.h"
#include "usi_uart.h"

#ifdef NODE_1
#define MY_ADDR (0x02)
#define OTHER_ADDR (0x01)
#else
#define MY_ADDR (0x01)
#define OTHER_ADDR (0x02)
#endif

#define SEND_HEX_DIGIT(v)         USI_UART_Transmit_Byte((v) > 9 ? 'A' - 9 + (v) : (v) + '0');


// first byte of data packet is 1 for data, 0 for ack
// second byte is packet number, or number being acknowledged
uint8_t rx_buffer[DATA_W / 8];
uint8_t tx_buffer[DATA_W / 8];
volatile uint8_t last_sent_packet_no = 0;
volatile uint8_t last_ack_received_packet_no = 0;
volatile uint8_t last_received_packet_no = 0;
volatile uint8_t ack_sent = 1;
volatile uint8_t packet_ready = 0;
uint8_t data_buffer[DATA_W / 8 - 2];
#define TIMEOUT_COUNT (20)
uint8_t time_since_send = 0;

void reportPacket(uint8_t type, uint8_t seq, uint8_t data) {
#ifdef NODE_1
        USI_UART_Transmit_Byte(type);
        SEND_HEX_DIGIT(seq >> 4);
        SEND_HEX_DIGIT(seq & 0x0f);
        USI_UART_Transmit_Byte('-');
        USI_UART_Transmit_Byte(data);
        USI_UART_Transmit_Byte(' ');
#endif
}

/**
 * This is being called from an ISR, so don't do any work
 */
void packet_received() {
    //red(1);
    if (rx_buffer[0] == 0x01) { // data
        if (last_received_packet_no != rx_buffer[1]) {
                green(1);
                last_received_packet_no = rx_buffer[1];
                // this was a new packet
                packet_ready = rx_buffer[2];
        }
        ack_sent = 0; // either acking a new packet or resending the ack for the previous packet
    } else if (rx_buffer[0] == 0x00) { // ack
        last_ack_received_packet_no = rx_buffer[1];
        red(1);
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
    start_recv_trw24g();
}

void send_ack() {
    tx_buffer[0] = 0x00;
    tx_buffer[1] = last_received_packet_no;
    for (int i = 0; i < DATA_W / 8 - 2; ++i) {
        tx_buffer[i+2] = 0;
    }
    send_trw24g(OTHER_ADDR);
    start_recv_trw24g();
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
    //red(0);
    //green(0);
    // do we need to send an ack?
    if (last_received_packet_no != 0 && ack_sent == 0) {
        reportPacket('A', last_received_packet_no, ' ');
        send_ack(OTHER_ADDR, last_received_packet_no);
        ack_sent = 1;
    }
    ++time_since_send;
    if (!packet_acked() && time_since_send > TIMEOUT_COUNT) {
        reportPacket('s', last_sent_packet_no, data_buffer[0]);
        send_packet();
        time_since_send = 0;
    }
}

static uint8_t char_to_send = 0;

int main(void) {
    // set the clock prescaler to 1 to get us an 8MHz clock -- the CKDIV8 fuse is programmed by default, so initial prescaler is /8
    CLKPR = 0x80;
    CLKPR = 0x00;
    PORTA_byte.val = 0;
    PORTB_byte.val = 0;
    setup_leds();
    red(1);
    green(1);
    _delay_ms(500);


    init_trw24g();
    config_trw24g(MY_ADDR, 0x30, rx_buffer, tx_buffer, packet_received);
    start_recv_trw24g();

#ifdef NODE_1
    USI_UART_Transmit_Byte('A');
    USI_UART_Transmit_Byte('B');
    USI_UART_Transmit_Byte('C');
#endif

    for (;;) {
#ifdef NODE_1
        if (USI_UART_Data_In_Receive_Buffer()) {
            char_to_send = USI_UART_Receive_Byte();
        }
#endif

        if (packet_ready) {
            reportPacket('R', last_received_packet_no, packet_ready);
#ifndef NODE_1
            char_to_send = packet_ready;
#endif
            packet_ready = 0;
        }

        if (packet_acked() && char_to_send) {
            data_buffer[0] = char_to_send;
            char_to_send = 0;
            ++last_sent_packet_no;
            reportPacket('S', last_sent_packet_no, data_buffer[0]);
            send_packet();
        }
        poll_protocol();
       _delay_ms(100);
       green(0);
       red(0);
    }
    return 0; /* never reached */
}

