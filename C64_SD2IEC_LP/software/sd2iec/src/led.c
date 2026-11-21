// ===================================================================================
// LEDs and Buttons                                                           * v1.0 *
// ===================================================================================
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include "system.h"
#include "gpio.h"
#include "led.h"
#include "../config.h"

#if LED_ACTIVE_LOW > 0
  #define LED_on(p)       PIN_low(p)
  #define LED_off(p)      PIN_high(p)
#else
  #define LED_on(p)       PIN_high(p)
  #define LED_off(p)      PIN_low(p)
#endif

static volatile uint8_t LED_state;      // bit 0: dirty, bit 1: error
static volatile uint8_t KEY_event;      // last button event
#define KEY_SLEEP_TIME    2000          // long press time in ms (sleep mode)

void LED_init(void) {
  LED_off(PIN_LED_BUSY);  PIN_output(PIN_LED_BUSY);
  LED_off(PIN_LED_ERROR); PIN_output(PIN_LED_ERROR);
  PIN_input(PIN_KEY_NEXT); PIN_pullup(PIN_KEY_NEXT);
  PIN_input(PIN_KEY_PREV); PIN_pullup(PIN_KEY_PREV);
}

void LED_busy(uint8_t on) {
  if(on) LED_on(PIN_LED_BUSY); else LED_off(PIN_LED_BUSY);
}

void LED_dirty(uint8_t on) {
  if(on) LED_state |= 1; else LED_state &= ~1;
  if(on) LED_on(PIN_LED_ERROR); else if(!(LED_state & 2)) LED_off(PIN_LED_ERROR);
}

void LED_error(uint8_t on) {
  if(on) LED_state |= 2; else LED_state &= ~2;
  if(!LED_state) LED_off(PIN_LED_ERROR);
}

// Called every millisecond
void LED_tick(void) {
  static uint16_t blink, count;
  static uint8_t keys;
  uint8_t k;

  // error LED blinks (2Hz) on errors, steady while data is not written
  if(++blink >= 250) {
    blink = 0;
    if(LED_state == 2) PIN_toggle(PIN_LED_ERROR);
  }

  // buttons: event when released after at least 30ms, sleep after 2s
  k = (PIN_read(PIN_KEY_NEXT) ? 0 : 1) | (PIN_read(PIN_KEY_PREV) ? 0 : 2);
  if(k) {
    keys |= k;
    if(++count == KEY_SLEEP_TIME) { KEY_event = KEY_SLEEP; keys = 0x80; }   // long press
  }
  else {
    if(!(keys & 0x80) && keys && count >= 30) KEY_event = keys;
    keys = 0;
    count = 0;
  }
}

// Get a button event (1 NEXT, 2 PREV, 3 both, 4 long press) or 0
uint8_t KEY_get(void) {
  uint8_t k;
  INT_ATOMIC_BLOCK { k = KEY_event; KEY_event = 0; }
  return k;
}
