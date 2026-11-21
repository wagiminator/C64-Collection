// ===================================================================================
// Basic SPI Master Functions for ATmega164/324/644/1284(P)                   * v1.0 *
// ===================================================================================
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include "spi.h"

// Init SPI as master, mode 0, slow clock
void SPI_init(void) {
  PORTB |= (1<<4);                              // SS (PB4) high
  DDRB  |= (1<<4) | (1<<5) | (1<<7);            // SS (PB4), MOSI (PB5), SCK (PB7) as output
  PORTB |= (1<<6);                              // pull-up on MISO (PB6)
  SPI_slow();                                   // master, mode 0, F_CPU/64
}
