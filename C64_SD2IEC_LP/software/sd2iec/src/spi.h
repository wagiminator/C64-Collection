// ===================================================================================
// Basic SPI Master Functions for ATmega164/324/644/1284(P)                   * v1.0 *
// ===================================================================================
//
// Functions available:
// --------------------
// SPI_init()               Init SPI as master, mode 0, slow clock (F_CPU/64)
// SPI_slow()               Set slow clock (F_CPU/64, 125kHz at 8MHz)
// SPI_fast()               Set fast clock (F_CPU/2 or F_CPU/4, see SD_FAST_CLOCK)
// SPI_transfer(d)          Transmit and receive one data byte
// SPI_write(d)             Transmit one data byte
// SPI_read()               Receive one data byte (sends 0xFF)
//
// SPI pins: MOSI = PB5, MISO = PB6, SCK = PB7, SS = PB4 (must be output or high)
//
// Notes:
// ------
// - Chip select pins must be defined and controlled by the application.
//
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "system.h"
#include "../config.h"

// SPI Functions
void SPI_init(void);                    // init SPI master

#define SPI_slow()          do { SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR1); SPSR = 0; } while(0)
#if SD_FAST_CLOCK > 0
  #define SPI_fast()        do { SPCR = (1<<SPE) | (1<<MSTR); SPSR = (1<<SPI2X); } while(0)
#else
  #define SPI_fast()        do { SPCR = (1<<SPE) | (1<<MSTR); SPSR = 0; } while(0)
#endif

// Transmit and receive one data byte
static inline uint8_t SPI_transfer(uint8_t data) {
  SPDR = data;                          // start transfer
  while(!(SPSR & (1<<SPIF)));           // wait until done
  return SPDR;                          // return received byte
}

#define SPI_write(d)        SPI_transfer(d)
#define SPI_read()          SPI_transfer(0xFF)

#ifdef __cplusplus
};
#endif
