// ===================================================================================
// Basic SPI Master Functions for ATmega48/88/168/328(P)                      * v1.0 *
// ===================================================================================
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include "spi.h"

// Init SPI as master, mode 0, slow clock
void SPI_init(void) {
  DDRB  |= (1<<2) | (1<<3) | (1<<5);            // SS (PB2), MOSI (PB3), SCK (PB5) as output
  PORTB |= (1<<4);                              // pull-up on MISO (PB4)
  SPI_slow();                                   // master, mode 0, F_CPU/64
}
