/*
 * Name: s1.c
 * Project: xu1541
 * Author: Till Harbaum
 * Tabsize: 4
 * Copyright: (c) 2007 by Till Harbaum <till@harbaum.org>
 * License: GPL
 *
 */

/* This file contains the "serial1" helper functions for opencbm */
/* changes in the protocol must be reflected here. */

#include "xum1541.h"

void
s1_write_byte(uint8_t c)
{
    uint8_t i;

    for (i = 8; i != 0; i--, c <<= 1) {
        // Send first bit, releasing CLK and waiting for drive to ack by
        // setting CLK.
        if ((c & 0x80) != 0)
            iec_set(IO_DATA);
        else
            iec_release(IO_DATA);
        IEC_DELAY();

        // Be sure DATA is stable before CLK is released. Also, delay after
        // releasing CLK before polling for the drive setting it.
        iec_release(IO_CLK);
        IEC_DELAY();
        while (!iec_get(IO_CLK)) {
            if (!TimerWorker())
                return;
        }

        // Send bit a second time. First, wait for drive to release CLK.
        // Then, release DATA and set CLK and wait for drive to ack by
        // setting DATA.
        if ((c & 0x80) != 0)
            iec_release(IO_DATA);
        else
            iec_set(IO_DATA);
        while (iec_get(IO_CLK)) {
            if (!TimerWorker())
                return;
        }

        // Delay after releasing DATA before polling for the drive setting it.
        iec_set_release(IO_CLK, IO_DATA);
        IEC_DELAY();
        while (!iec_get(IO_DATA)) {
            if (!TimerWorker())
                return;
        }
    }
}

uint8_t
s1_read_byte(void)
{
    uint8_t i, b, c;

    c = 0;
    for (i = 8; i != 0; i--) {
        while (iec_get(IO_DATA)) {
            if (!TimerWorker())
                return -1;
        }
        iec_release(IO_CLK);
        IEC_DELAY();
        b = iec_get(IO_CLK);
        c = (c >> 1) | (b ? 0x80 : 0);
        iec_set(IO_DATA);
        while (b == iec_get(IO_CLK)) {
            if (!TimerWorker())
                return -1;
        }

        iec_release(IO_DATA);
        IEC_DELAY();
        while (!iec_get(IO_DATA)) {
            if (!TimerWorker())
                return -1;
        }
        iec_set(IO_CLK);
    }

    return c;
}
