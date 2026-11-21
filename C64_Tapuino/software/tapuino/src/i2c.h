// ===================================================================================
// Basic I2C Master Functions for ATmega48/88/168/328(P)                      * v1.0 *
// ===================================================================================
//
// Functions available:
// --------------------
// I2C_init()               Init I2C with defined clock rate (see below)
// I2C_start(addr)          I2C start transmission, addr must contain R/W bit
// I2C_write(b)             I2C transmit one data byte via I2C
// I2C_stop()               I2C stop transmission
// I2C_error()              1 if a device did not answer or the bus hung since init
//
// I2C pins: SDA = PC4, SCL = PC5
//
// Notes:
// ------
// - The internal pull-ups are switched on; external pull-up resistors (4k7 - 10k)
//   are recommended for 400kHz.
// - All waits have a timeout, so a missing device does not stop the program.
//
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "system.h"
#include "../config.h"

// I2C Parameters
#ifndef I2C_CLOCK
  #define I2C_CLOCK     400000          // I2C bus clock rate (Hz)
#endif

// I2C Functions
void I2C_init(void);                    // I2C init function
void I2C_start(uint8_t addr);           // I2C start transmission, addr must contain R/W bit
void I2C_write(uint8_t data);           // I2C transmit one data byte via I2C
void I2C_stop(void);                    // I2C stop transmission
uint8_t I2C_error(void);                // check for bus errors since init

#ifdef __cplusplus
};
#endif
