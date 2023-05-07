// ===================================================================================
// Timer Functions for DiskMaster64
// ===================================================================================

#pragma once
#include "ch554.h"

// ===================================================================================
// Timer0 used as a 6502 clock counter
// ===================================================================================

// Calculate number of clock cycles per microsecond(us)
#define T0_CLK_US           F_CPU/1000000

// Init Timer0 in 8-bit reload mode at timer freq = sys freq
#define T0_init()           TMOD |= bT0_M1; T2MOD |= (bTMR_CLK | bT0_CLK)

// Start Timer0
#define T0_start()          TF0 = 0; TR0 = 1

// Stop Timer0
#define T0_stop()           TR0 = 0

// Set Timer0 period to specified microseconds(us)
#define T0_setperiod(us)    TH0 = (uint8_t)(256-(us*T0_CLK_US)); TL0 = (uint8_t)(256-(us*T0_CLK_US))

// Set Timer0 period and start timer
#define T0_startperiod(us)  T0_stop(); T0_setperiod(us); T0_start()

// Wait for end of current timer period
#define T0_waitperiod()     while(!TF0); TF0 = 0

// ===================================================================================
// Timer1 used as a watchdog with longer period
// ===================================================================================

// Total period in seconds
#define T1_PERIOD           4

// Calculate number of timer cycles for total period
#define T1_CYCLES           (uint8_t)(F_CPU * T1_PERIOD / 12 / 65526)

// Init Timer1 in 16-bit mode at timer freq = sys freq / 12
#define T1_init()           TMOD |= bT1_M0

// Start Timer1
#define T1_start()          TF1 = 0; TR1 = 1; ET1 = 1

// Stop Timer1
#define T1_stop()           TR1 = 0

// Reset Timer1
#define T1_reset()          TH1 = 0; TL1 = 0
