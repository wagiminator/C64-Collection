// ===================================================================================
// Basic System Functions for ATmega164/324/644/1284(P)                       * v1.0 *
// ===================================================================================
//
// System tick (MIL) functions available (timer 2, 1kHz):
// -------------------------------------------------------
// MIL_init()               start the system tick (1ms, interrupt)
// MIL_read()               milliseconds since start (16-bit, wraps after 65s)
// MIL_read32()             milliseconds since start (32-bit)
// MIL_callback()           function called by the tick every millisecond (defined
//                          by the application, see MIL_CALLBACK below)
//
// Microsecond timeout (TMR) functions available (timer 0, 1us steps at 8MHz):
// -------------------------------------------------------------------------
// TMR_init()               start timer 0 (prescaler 8)
// TMR_start(us)            start a timeout of 1..256 microseconds
// TMR_expired()            returns non-zero when the timeout has expired
//
// Delay (DLY) functions available:
// --------------------------------
// DLY_us(n)                delay n microseconds
// DLY_ms(n)                delay n milliseconds
//
// Watchdog Timer (WDT) and Reset (RST) functions available:
// ---------------------------------------------------------
// WDT_stop()               stop watchdog timer (call early after a watchdog reset)
// RST_now()                conduct software reset (via watchdog)
//
// Interrupt (INT) functions available:
// ------------------------------------
// INT_enable()             global interrupt enable
// INT_disable()            global interrupt disable
// INT_ATOMIC_BLOCK { }     execute block without being interrupted
//
// EEPROM functions available:
// ---------------------------
// EEPROM_read(a)           read  8-bit byte from EEPROM address a
// EEPROM_write(a,b)        write (update) 8-bit byte b to EEPROM address a
// EEPROM_readBlock(d,a,n)  read n bytes from EEPROM address a to d
// EEPROM_writeBlock(s,a,n) write (update) n bytes from s to EEPROM address a
//
// Notes:
// ------
// - F_CPU must be defined before system.h is included (e.g. in the makefile). The
//   clock source is set by the fuses (8MHz crystal on the SD2IEC).
// - The TMR timeout assumes F_CPU = 8MHz (other clocks are scaled at compile time).
// - Timer 2 is used by the system tick, timer 0 by the TMR timeouts. The tick interrupt allows other interrupts
//   while it runs (ISR_NOBLOCK), so that time-critical interrupts are not delayed.
// - DLY_us() and DLY_ms() expect compile-time constants.
//
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef F_CPU
  #define F_CPU             8000000     // default: 8MHz crystal
#endif

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <util/atomic.h>

// ===================================================================================
// System Tick (MIL) Functions
// ===================================================================================
#define MIL_CALLBACK        1           // 1: call MIL_callback() every millisecond

void MIL_init(void);                    // start system tick
uint16_t MIL_read(void);                // read milliseconds (16-bit)
uint32_t MIL_read32(void);              // read milliseconds (32-bit)
#if MIL_CALLBACK > 0
void MIL_callback(void);                // to be defined by the application
#endif

// ===================================================================================
// Microsecond Timeout (TMR) Functions
// ===================================================================================
#define TMR_init()          TCCR0B = (1<<CS01)
#define TMR_start(us)       do { TCNT0 = 256 - (uint8_t)((F_CPU / 8000000.0) * (us)); \
                                 TIFR0 = (1<<TOV0); } while(0)
#define TMR_expired()       (TIFR0 & (1<<TOV0))

// ===================================================================================
// Delay (DLY) Functions
// ===================================================================================
#define DLY_us(n)           _delay_us(n)
#define DLY_ms(n)           _delay_ms(n)

// ===================================================================================
// Watchdog Timer (WDT) and Reset (RST) Functions
// ===================================================================================
#define WDT_stop()          do { MCUSR = 0; wdt_disable(); } while(0)
#define RST_now()           do { wdt_enable(WDTO_15MS); while(1); } while(0)

// ===================================================================================
// Interrupt (INT) Functions
// ===================================================================================
#define INT_enable()        sei()
#define INT_disable()       cli()
#define INT_ATOMIC_BLOCK    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)

// ===================================================================================
// EEPROM Functions
// ===================================================================================
#define EEPROM_read(a)      eeprom_read_byte((const uint8_t*)(uint16_t)(a))
#define EEPROM_write(a,b)   eeprom_update_byte((uint8_t*)(uint16_t)(a), (b))
#define EEPROM_readBlock(d,a,n)  eeprom_read_block((d), (const void*)(uint16_t)(a), (n))
#define EEPROM_writeBlock(s,a,n) eeprom_update_block((s), (void*)(uint16_t)(a), (n))

#ifdef __cplusplus
};
#endif
