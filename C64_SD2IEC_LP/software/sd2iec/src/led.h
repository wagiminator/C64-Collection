// ===================================================================================
// LEDs and Buttons                                                           * v1.0 *
// ===================================================================================
//
// Functions available:
// --------------------
// LED_init()               Init LED and button pins
// LED_busy(on)             BUSY LED: channels are open
// LED_dirty(on)            ERROR LED steady: data not yet written to the card
// LED_error(on)            ERROR LED blinking: error on the status channel
// LED_tick()               Called every millisecond (blinking, button debouncing)
// KEY_get()                Get a button event (KEY_NEXT, KEY_PREV, KEY_HOME,
//                          KEY_SLEEP or 0)
//
// Notes:
// ------
// - Pins and polarity are set in config.h.
// - NEXT and PREV pressed together give KEY_HOME (first image of the swap list).
// - A button held for two seconds gives KEY_SLEEP (sleep mode on/off).
//
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

enum { KEY_NEXT = 1, KEY_PREV = 2, KEY_HOME = 3, KEY_SLEEP = 4 };

void    LED_init(void);
void    LED_busy(uint8_t on);
void    LED_dirty(uint8_t on);
void    LED_error(uint8_t on);
void    LED_tick(void);
uint8_t KEY_get(void);

#ifdef __cplusplus
};
#endif
