#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include "avrutils.h"
#include "trw24g.h"

uint8_t buffer[DATA_W/8];

void packet_received() {
    green(1);
}

int main(void) {
    static uint8_t count = 0;
    // set the clock prescaler to 1 to get us an 8MHz clock -- the CKDIV8 fuse is programmed by default, so initial prescaler is /8
    CLKPR = 0x80;
    CLKPR = 0x00;
    setup_leds();
    red(0);
    green(0);
    init_trw24g();
    config_trw24g(0x01, 0x02, buffer, packet_received);
    for (;;) {
        count++;
        _delay_ms(500);
        red(count % 2);
        _delay_ms(500);
        for (uint8_t i = 0; i < DATA_W/8; ++i) {
            buffer[i] = count;
        }
        send_trw24g(0x02);
    }
    return 0; /* never reached */
}

