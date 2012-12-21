
#define CLK1_PORT  PORTB_byte.b0
#define CLK1_DDR  DDRB_byte.b0

#define DATA_PORT    PORTA_byte.b7 //PA7
#define DATA_DDR   DDRA_byte.b7 //DDA7
#define DATA_PIN  PINA_byte.b7 //PINA

#define DR1_PORT     PORTB_byte.b2 //PB2 //(INT0)
#define DR1_DDR     DDRB_byte.b2
#define DR1_PIN    PINB_byte.b2

#define CS_PORT      PORTA_byte.b3 //PA3
#define CS_DDR      DDRA_byte.b3
#define CS_PIN      PINA_byte.b3

#define CE_PORT      PORTA_byte.b2 //PA2
#define CE_DDR     DDRA_byte.b2
#define CE_PIN     PINA_byte.b2

extern void init_trw24g();

#define MODE_ACTIVE     (0x2)
#define MODE_CONFIG     (0x1)
#define MODE_STANDBY     (0x0)

extern void set_mode_trw24g(uint8_t mode);

#define ADDR_W  (8) // number of bits for address
#define DATA_W  (256-ADDR_W-16) // bits per packet assuming 16 bit CRC

extern void config_trw24g(uint8_t addr, uint8_t channel, uint8_t* buf, void (*callback)());

#define TX (1)
#define RX (0)

extern void send_trw24g(uint8_t addr);
