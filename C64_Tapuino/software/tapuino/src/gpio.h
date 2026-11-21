// ===================================================================================
// Basic GPIO Functions for ATmega48/88/168/328(P)                            * v1.0 *
// ===================================================================================
//
// Pins must be defined as PB0, PB1, .., PD7 - e.g.:
// #define PIN_LED PB5      // LED on pin PB5
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
// PORT functions available:
// -------------------------
// PORTB_input()            set complete PORT as input
// PORTB_output()           set complete PORT as output
// PORTB_write(val)         write PORT output value
// PORTB_read()             read PORT input value
// (the same for PORTC and PORTD)
//
// Notes:
// ------
// - (*) reset state
// - With constant pin names the macros compile to single sbi/cbi/sbis instructions.
// - PIN_pullup() and PIN_high() set the same bit: on an input it switches the pull-up
//   on, on an output it drives the pin high.
// - PB6/PB7 are used by the crystal, PC6 is RESET.
//
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <avr/io.h>

// ===================================================================================
// Enumerate PIN Designators (use these designators to define pins)
// ===================================================================================
// avr-libc defines PB0..PD7 as bit numbers (0..7); they are replaced by pin numbers
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

enum { PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7,
       PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7,
       PD0, PD1, PD2, PD3, PD4, PD5, PD6, PD7 };

// Port registers: PINx, DDRx and PORTx follow each other for B, C and D
#define _GPIO_PIN(x)        (&PINB)[((x)>>3)*3]
#define _GPIO_DDR(x)        (&DDRB)[((x)>>3)*3]
#define _GPIO_PORT(x)       (&PORTB)[((x)>>3)*3]
#define _GPIO_BIT(x)        (1<<((x)&7))

// ===================================================================================
// PIN Functions
// ===================================================================================
#define PIN_input(x)        _GPIO_DDR(x)  &= ~_GPIO_BIT(x)
#define PIN_output(x)       _GPIO_DDR(x)  |=  _GPIO_BIT(x)
#define PIN_pullup(x)       _GPIO_PORT(x) |=  _GPIO_BIT(x)
#define PIN_pulloff(x)      _GPIO_PORT(x) &= ~_GPIO_BIT(x)
#define PIN_low(x)          _GPIO_PORT(x) &= ~_GPIO_BIT(x)
#define PIN_high(x)         _GPIO_PORT(x) |=  _GPIO_BIT(x)
#define PIN_toggle(x)       _GPIO_PIN(x)   =  _GPIO_BIT(x)
#define PIN_read(x)         (_GPIO_PIN(x) &   _GPIO_BIT(x))
#define PIN_write(x, v)     ((v) ? (PIN_high(x)) : (PIN_low(x)))

// ===================================================================================
// PORT Functions
// ===================================================================================
#define PORTB_input()       DDRB  = 0x00
#define PORTB_output()      DDRB  = 0xff
#define PORTB_write(v)      PORTB = (v)
#define PORTB_read()        (PINB)

#define PORTC_input()       DDRC  = 0x00
#define PORTC_output()      DDRC  = 0xff
#define PORTC_write(v)      PORTC = (v)
#define PORTC_read()        (PINC)

#define PORTD_input()       DDRD  = 0x00
#define PORTD_output()      DDRD  = 0xff
#define PORTD_write(v)      PORTD = (v)
#define PORTD_read()        (PIND)

#ifdef __cplusplus
};
#endif
