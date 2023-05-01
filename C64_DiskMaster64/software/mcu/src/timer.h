// ===================================================================================
// Timer Functions for CH55x
// ===================================================================================

#pragma once
#include "ch554.h"

// Calculate number of clock cycles per microsecond(us)
#define T0_CLK_US           F_CPU/1000000

// Init Timer0 in 8-bit reload mode at timer freq = sys freq
#define T0_init()           TMOD = bT0_M1; T2MOD = (bTMR_CLK | bT0_CLK)

// Start Timer0
#define T0_start()          TF0 = 0; TR0 = 1

// Stop Timer0
#define T0_stop()           TR0 = 0

// Set Timer0 period to specified microseconds(us)
#define T0_setperiod(us)    TH0 = (uint8_t)(256-(us*T0_CLK_US)); TL0 = (uint8_t)(256-(us*T0_CLK_US))&0xFF

// Set Timer0 period and start timer
#define T0_startperiod(us)  T0_stop(); T0_setperiod(us); T0_start()

// Wait for end of current timer period
#define T0_waitperiod()     while(!TF0); TF0 = 0

