/*
 * CPU initialization and timer routines for the USBKEY devkit (at90usb1287)
 * Copyright (c) 2009-2010 Nate Lawson <nate@root.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#ifndef _CPU_USBKEY_H
#define _CPU_USBKEY_H

// Initialize the CPU (clock rate, UART)
static inline void
cpu_init(void)
{
    // Disable clock division. This takes us from 1 MHz -> 8 MHz.
    clock_prescale_set(clock_div_1);

    // Enable watchdog timer and set for 1 second.
    wdt_enable(WDTO_1S);
}

static inline void
cpu_bootloader_start(void)
{
    // XXX This does not work yet, use the hardware buttons
    clock_prescale_set(clock_div_8);
    TCCR1B = 0;
    OCR1A = 0;
    __asm__ __volatile__ ("jmp 0xf000" "\n\t");
}

// Timer and delay functions
#define DELAY_MS(x) _delay_ms(x)
#define DELAY_US(x) _delay_us(x)

#endif // _CPU_USBKEY_H
