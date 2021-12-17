/*
 * Name: pp.c
 * Project: xu1541
 * Author: Till Harbaum
 * Tabsize: 4
 * Copyright: (c) 2007 by Till Harbaum <till@harbaum.org>
 * License: GPL
 *
 */

/* This file contains the "parallel" helper functions for opencbm */
/* changes in the protocol must be reflected here. */

#include "xum1541.h"

/*
 * Send 2 bytes to the drive.
 *
 * The delays after setting the parallel data are to be sure it is
 * ready on the port before signaling by toggling CLK. The parallel
 * lines transition quickly because there is little load due to the
 * drive keeping them in hi-Z. Thus the delays are not strictly needed,
 * at least at 16 MHz or below.
 */
void
pp_write_2_bytes(uint8_t *c)
{
    while (!iec_get(IO_DATA)) {
        if (!TimerWorker())
            return;
    }
    iec_pp_write(*c++);
    DELAY_US(0.5);
    iec_release(IO_CLK);

    while (iec_get(IO_DATA)) {
        if (!TimerWorker())
            return;
    }
    iec_pp_write(*c);
    DELAY_US(0.5);
    iec_set(IO_CLK);
}

void
pp_read_2_bytes(uint8_t *c)
{
    while (!iec_get(IO_DATA)) {
        if (!TimerWorker())
            return;
    }
    *c++ = iec_pp_read();
    iec_release(IO_CLK);

    while (iec_get(IO_DATA)) {
        if (!TimerWorker())
            return;
    }
    *c = iec_pp_read();
    iec_set(IO_CLK);
}
