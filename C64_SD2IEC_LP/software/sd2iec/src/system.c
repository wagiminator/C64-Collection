// ===================================================================================
// Basic System Functions for ATmega164/324/644/1284(P)                       * v1.0 *
// ===================================================================================
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include "system.h"

// ===================================================================================
// System Tick (MIL) Functions
// ===================================================================================
static volatile uint32_t MIL_counter = 0;       // milliseconds since start

// Start system tick: timer 2 in CTC mode, prescaler 64, 1kHz interrupt
void MIL_init(void) {
  TCCR2A = (1<<WGM21);                          // CTC mode
  TCCR2B = (1<<CS22);                           // prescaler 64
  OCR2A  = (F_CPU / 64 / 1000) - 1;             // 1kHz
  TIMSK2 = (1<<OCIE2A);                         // enable compare match interrupt
}

// Read milliseconds (16-bit)
uint16_t MIL_read(void) {
  uint16_t result;
  INT_ATOMIC_BLOCK { result = (uint16_t)MIL_counter; }
  return result;
}

// Read milliseconds (32-bit)
uint32_t MIL_read32(void) {
  uint32_t result;
  INT_ATOMIC_BLOCK { result = MIL_counter; }
  return result;
}

// Interrupt service routine: other interrupts may interrupt this one
ISR(TIMER2_COMPA_vect, ISR_NOBLOCK) {
  MIL_counter++;
  #if MIL_CALLBACK > 0
  MIL_callback();
  #endif
}
