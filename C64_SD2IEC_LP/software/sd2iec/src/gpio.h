// ===================================================================================
// Basic GPIO Functions for ATmega164/324/644/1284(P)                         * v1.0 *
// ===================================================================================
//
// Pins must be defined as PA0, PA1, .., PD7 - e.g.:
// #define PIN_LED PA0      // LED on pin PA0
//
// PIN functions available:
// ------------------------
// PIN_input(PIN)           set PIN as INPUT (*)
// PIN_output(PIN)          set PIN as OUTPUT
// PIN_pullup(PIN)          enable internal PULLUP resistor on PIN (if input)
// PIN_pulloff(PIN)         disable internal PULLUP resistor on PIN (*)
//
// PIN_low(PIN)             set PIN output value to LOW (*)
// PIN_high(PIN)            set PIN output value to HIGH
// PIN_toggle(PIN)          toggle PIN output value
// PIN_read(PIN)            read PIN input value
// PIN_write(PIN, val)      write PIN output value (0 = LOW / 1 = HIGH)
//
// PIN_PORT(PIN)            PORTx register of the pin (for assembler and fast code)
// PIN_DDR(PIN)             DDRx  register of the pin
// PIN_PINREG(PIN)          PINx  register of the pin
// PIN_BIT(PIN)             bit number of the pin within its port (0..7)
//
// Notes:
// ------
// - (*) reset state
// - With constant pin names the macros compile to single sbi/cbi/sbis instructions.
// - PIN_pullup() and PIN_high() set the same bit: on an input it switches the pull-up
//   on, on an output it drives the pin high.
//
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <avr/io.h>

#undef PA0
#undef PA1
#undef PA2
#undef PA3
#undef PA4
#undef PA5
#undef PA6
#undef PA7
#undef PB0
#undef PB1
#undef PB2
#undef PB3
#undef PB4
#undef PB5
#undef PB6
#undef PB7
#undef PC0
#undef PC1
#undef PC2
#undef PC3
#undef PC4
#undef PC5
#undef PC6
#undef PC7
#undef PD0
#undef PD1
#undef PD2
#undef PD3
#undef PD4
#undef PD5
#undef PD6
#undef PD7

#define PA0 0
#define PA1 1
#define PA2 2
#define PA3 3
#define PA4 4
#define PA5 5
#define PA6 6
#define PA7 7
#define PB0 8
#define PB1 9
#define PB2 10
#define PB3 11
#define PB4 12
#define PB5 13
#define PB6 14
#define PB7 15
#define PC0 16
#define PC1 17
#define PC2 18
#define PC3 19
#define PC4 20
#define PC5 21
#define PC6 22
#define PC7 23
#define PD0 24
#define PD1 25
#define PD2 26
#define PD3 27
#define PD4 28
#define PD5 29
#define PD6 30
#define PD7 31

#define _GPIO_PIN(x)        (&PINA)[((x)>>3)*3]
#define _GPIO_DDR(x)        (&DDRA)[((x)>>3)*3]
#define _GPIO_PORT(x)       (&PORTA)[((x)>>3)*3]
#define _GPIO_BIT(x)        (1<<((x)&7))

#define PIN_input(x)        _GPIO_DDR(x)  &= ~_GPIO_BIT(x)
#define PIN_output(x)       _GPIO_DDR(x)  |=  _GPIO_BIT(x)
#define PIN_pullup(x)       _GPIO_PORT(x) |=  _GPIO_BIT(x)
#define PIN_pulloff(x)      _GPIO_PORT(x) &= ~_GPIO_BIT(x)
#define PIN_low(x)          _GPIO_PORT(x) &= ~_GPIO_BIT(x)
#define PIN_high(x)         _GPIO_PORT(x) |=  _GPIO_BIT(x)
#define PIN_toggle(x)       _GPIO_PIN(x)   =  _GPIO_BIT(x)
#define PIN_read(x)         (_GPIO_PIN(x) &   _GPIO_BIT(x))
#define PIN_write(x, v)     ((v) ? (PIN_high(x)) : (PIN_low(x)))

// Register addresses (I/O space, for assembler: _SFR_IO_ADDR not needed)
#define PIN_PINREG(x)       (0x00 + ((x)>>3)*3)
#define PIN_DDR(x)          (0x01 + ((x)>>3)*3)
#define PIN_PORT(x)         (0x02 + ((x)>>3)*3)
#define PIN_BIT(x)          ((x)&7)

#ifdef __cplusplus
};
#endif
