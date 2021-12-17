/*
 * Name: s2.c
 * Project: xu1541
 * Author: Till Harbaum
 * Tabsize: 4
 * Copyright: (c) 2007 by Till Harbaum <till@harbaum.org>
 * License: GPL
 *
 */

/* This file contains the "serial2" helper functions for opencbm */
/* changes in the protocol must be reflected here. */

#include "xum1541.h"

void
s2_write_byte(uint8_t c)
{
    uint8_t i;

    for (i = 4; i != 0; i--) {
        // Send first bit, releasing ATN and waiting for CLK release ack.
        if ((c & 1) != 0)
            iec_set(IO_DATA);
        else
            iec_release(IO_DATA);
        IEC_DELAY();
        c >>= 1;
        iec_release(IO_ATN);
        while (iec_get(IO_CLK)) {
            if (!TimerWorker())
                return;
        }

        // Send second bit, setting ATN and waiting for CLK set ack.
        if ((c & 1) != 0)
            iec_set(IO_DATA);
        else
            iec_release(IO_DATA);
        IEC_DELAY();
        c >>= 1;
        iec_set(IO_ATN);
        while (!iec_get(IO_CLK)) {
            if (!TimerWorker())
                return;
        }
    }

    iec_release(IO_DATA);
    IEC_DELAY();
}

uint8_t
s2_read_byte(void)
{
    uint8_t c, i;

    c = 0;
    for (i = 4; i != 0; i--) {
        // Receive first bit, waiting for CLK and releasing ATN to ack.
        while (iec_get(IO_CLK)) {
            if (!TimerWorker())
                return -1;
        }
        // Pause each time CLK changes to be sure DATA is stable.
        IEC_DELAY();
        c = (c >> 1) | (iec_get(IO_DATA) ? 0x80 : 0);
        iec_release(IO_ATN);

        // Receive second bit, waiting for CLK to be released and setting ATN
        // to ack.
        while (!iec_get(IO_CLK)) {
            if (!TimerWorker())
                return -1;
        }
        // Pause each time CLK changes to be sure DATA is stable.
        IEC_DELAY();
        c = (c >> 1) | (iec_get(IO_DATA) ? 0x80 : 0);
        iec_set(IO_ATN);
    }

    return c;
}
