// ===================================================================================
// IEC Bus (CBM Serial Bus) Device Functions                                  * v1.0 *
// ===================================================================================
//
// Functions available:
// --------------------
// iec_init()               Init bus pins, ATN interrupt and device address
// iec_mainloop()           Handle the bus forever (calls the DOS and the fast loaders)
//
// Bus line macros (for the fast loaders):
// ---------------------------------------
// IEC_bus()                Read the bus (IEC_BIT_ATN/DATA/CLOCK set = line released)
// IEC_ATN, IEC_DATA,       Line levels (non-zero = released/high)
// IEC_CLOCK
// set_data(s), set_clock(s)  Release (s = 1) or pull (s = 0) DATA / CLOCK
// set_atn_irq(on)          Enable / disable the ATN interrupt (acknowledges ATN by
//                          pulling DATA immediately, see fastloader_ll.S)
// set_clock_irq(on)        Enable / disable the CLOCK interrupt (DreamLoad)
//
// Notes:
// ------
// - The lines are driven open collector style through the data direction register:
//   input = released (pulled up by the computer), output low = pulled.
// - All three lines must be on the same port (see config.h).
// - The protocol follows the 1541 ROM; JiffyDOS is detected during the ATN sequence.
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "gpio.h"
#include "../config.h"

#if (PIN_IEC_ATN >> 3) != (PIN_IEC_DATA >> 3) || (PIN_IEC_ATN >> 3) != (PIN_IEC_CLOCK >> 3)
  #error "IEC lines must be on the same port"
#endif

// Registers and bits of the bus port
#define IEC_PIN_REG       _GPIO_PIN(PIN_IEC_ATN)
#define IEC_DDR_REG       _GPIO_DDR(PIN_IEC_ATN)
#define IEC_PORT_REG      _GPIO_PORT(PIN_IEC_ATN)
#define IEC_BIT_ATN       _GPIO_BIT(PIN_IEC_ATN)
#define IEC_BIT_DATA      _GPIO_BIT(PIN_IEC_DATA)
#define IEC_BIT_CLOCK     _GPIO_BIT(PIN_IEC_CLOCK)

// Pin change interrupt of the bus port (the interrupt routine is in fastloader_ll.S)
#define IEC_PCIE          (1 << (PIN_IEC_ATN >> 3))
#if   (PIN_IEC_ATN >> 3) == 0
  #define IEC_PCMSK       PCMSK0
#elif (PIN_IEC_ATN >> 3) == 1
  #define IEC_PCMSK       PCMSK1
#elif (PIN_IEC_ATN >> 3) == 2
  #define IEC_PCMSK       PCMSK2
#else
  #define IEC_PCMSK       PCMSK3            // not contiguous with PCMSK0..2
#endif

#define IEC_bus()         (IEC_PIN_REG)
#define IEC_ATN           (IEC_PIN_REG & IEC_BIT_ATN)
#define IEC_DATA          (IEC_PIN_REG & IEC_BIT_DATA)
#define IEC_CLOCK         (IEC_PIN_REG & IEC_BIT_CLOCK)

static inline __attribute__((always_inline)) void set_data(uint8_t state) {
  if(state) IEC_DDR_REG &= ~IEC_BIT_DATA;
  else      IEC_DDR_REG |=  IEC_BIT_DATA;
}

static inline __attribute__((always_inline)) void set_clock(uint8_t state) {
  if(state) IEC_DDR_REG &= ~IEC_BIT_CLOCK;
  else      IEC_DDR_REG |=  IEC_BIT_CLOCK;
}

static inline __attribute__((always_inline)) void set_atn_irq(uint8_t on) {
  if(on) IEC_PCMSK |=  IEC_BIT_ATN;
  else   IEC_PCMSK &= ~IEC_BIT_ATN;
}

static inline __attribute__((always_inline)) void set_clock_irq(uint8_t on) {
  if(on) IEC_PCMSK |=  IEC_BIT_CLOCK;
  else   IEC_PCMSK &= ~IEC_BIT_CLOCK;
}

// Bus state
typedef struct {
  uint8_t flags;                        // IEC_EOI_RECVD, IEC_COMMAND_RECVD, IEC_JIFFY_*
  uint8_t bus_state;                    // state of the main loop
  uint8_t device_state;                 // DEVICE_IDLE, DEVICE_LISTEN, DEVICE_TALK
  uint8_t secondary;                    // secondary address of the current transaction
} iec_data_t;

#define IEC_EOI_RECVD     0x01
#define IEC_COMMAND_RECVD 0x02
#define IEC_JIFFY_ACTIVE  0x04
#define IEC_JIFFY_LOAD    0x08

extern iec_data_t iec_data;
extern uint8_t jiffy_enabled;           // JiffyDOS support on (XJ)
extern uint8_t vc20mode;                // slower timing for the VIC-20 (UI-)

void iec_init(void);
void iec_mainloop(void);
uint8_t iec_check_atn(void);            // 1 if ATN changed (state updated)

#ifdef __cplusplus
};
#endif
