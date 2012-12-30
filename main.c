#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "avrutils.h"
#include "trw24g.h"

#define MY_ADDR (0x01)
#define OTHER_ADDR (0x02)

uint8_t buffer[DATA_W/8];

void packet_received() {
    green(1);
}

int main(void) {
    static uint8_t count = 0;
    // set the clock prescaler to 1 to get us an 8MHz clock -- the CKDIV8 fuse is programmed by default, so initial prescaler is /8
    CLKPR = 0x80;
    CLKPR = 0x00;
    PORTA_byte.val = 0;
    PORTB_byte.val = 0;
    setup_leds();
    red(0);
    green(0);
    init_trw24g();
    config_trw24g(MY_ADDR, 0x20, buffer, packet_received);
    start_recv_trw24g();
    if (MY_ADDR == 0x02) {
        _delay_ms(500);
    }
    for (;;) {
        count++;
        if (MY_ADDR == 0x01) {
                _delay_ms(100);
        }

        _delay_ms(500);
        red(1);
        _delay_ms(500);
        for (uint8_t i = 0; i < DATA_W/8; ++i) {
            buffer[i] = count;
        }
        send_trw24g(OTHER_ADDR);
        red(0);
        green(0);
        start_recv_trw24g();
    }
    return 0; /* never reached */
}

