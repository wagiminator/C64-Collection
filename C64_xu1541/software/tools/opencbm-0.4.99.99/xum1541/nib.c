/*
 * MNIB/nibtools compatible parallel read/write routines
 * Copyright (c) 2009-2010 Nate Lawson <nate@root.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include "xum1541.h"

/*
 * ATN/DATA handshaked read from the drive.
 *
 * It takes about 2 us for the drive to pull DATA once we set ATN.
 * Sometimes DATA is already held before we pull ATN.
 * Then it takes about 13 us for it to release DATA once a byte is ready.
 * Once we release ATN, it takes another 2 us to release DATA, if it is
 * at the end of the sequence.
 */
uint8_t
nib_parburst_read()
{
    uint8_t data;

    // Set ATN and wait for drive to release DATA
    iec_set_release(IO_ATN, IO_DATA|IO_CLK);
    DELAY_US(5);
    while (iec_get(IO_DATA))
        ;

    // Byte ready -- read it and release ATN
    DELAY_US(1);
    data = iec_pp_read();
    iec_release(IO_ATN);

    // Wait for the drive to pull DATA again. Delay for a bit afterwards
    // to keep the next read from being too close together.
    while (!iec_get(IO_DATA))
        ;
    DELAY_US(5);
    return data;
}

// Caller must release IO_DATA before calling this function
int8_t
nib_read_handshaked(uint8_t *data, uint8_t toggle)
{
    // Wait for a byte to be ready (data toggle matches expected value).
    while (iec_get(IO_DATA) != toggle)
        ;

    // Read it directly from the port without debouncing.
    *data = iec_pp_read();
    return 0;
}

/*
 * ATN/DATA handshaked write to the drive.
 *
 * It takes about 13 us for the drive to release DATA once we set ATN.
 * Once we release ATN, it takes the drive 2-5 us to set DATA.
 * However, we need to keep the data valid for a while after releasing
 * ATN so the drive can register it. 5 us always fails but 10 us works.
 */
void
nib_parburst_write(uint8_t data)
{

    iec_set_release(IO_ATN, IO_DATA|IO_CLK);
    DELAY_US(5);
    while (iec_get(IO_DATA))
        ;

    iec_pp_write(data);
    DELAY_US(1);
    iec_release(IO_ATN);

    /*
     * Hold parallel data ready until the drive can read it. Even though
     * the drive is supposed to grab DATA after it has gotten the byte ok,
     * it seems to do so prematurely. Thus we have to add this idle loop
     * to keep the data valid, and by that point, DATA has been set for
     * a while.
     */
    DELAY_US(10);
    while (!iec_get(IO_DATA))
        ;

    // Read from parallel port, making the outputs inputs. (critical)
    data = iec_pp_read();
}

// Caller must release IO_DATA before calling this function
int8_t
nib_write_handshaked(uint8_t data, uint8_t toggle)
{
    // Wait for drive to be ready (data toggle matches expected value).
    while (iec_get(IO_DATA) != toggle)
        ;

    // Write out the data value via parallel.
    iec_pp_write(data);
    return 0;
}

#ifdef SRQ_NIB_SUPPORT
// Handshaked read of a byte via fast serial
uint8_t
nib_srqburst_read()
{
    uint8_t data;

    // Set ATN, and wait for drive to start SRQ transfer
    iec_set_release(IO_ATN, IO_SRQ | IO_CLK | IO_DATA);
    DELAY_US(1);

    // Read 8 bits via fast serial
    data = iec_srq_read();
    iec_release(IO_ATN);

    // Wait for the drive to release CLK.
    while (iec_get(IO_CLK))
        ;

    return data;
}

// Handshaked write of a byte via fast serial
void
nib_srqburst_write(uint8_t data)
{
    // Set ATN and wait for drive to start SRQ transfer
    iec_set_release(IO_ATN, IO_SRQ | IO_CLK | IO_DATA);
    DELAY_US(5);

    // Wait for the drive to set CLK.
    while (!iec_get(IO_CLK))
        ;

    // Send data byte via fast serial
    iec_srq_write(data);
    DELAY_US(1);

    // Release ATN again.
    iec_release(IO_ATN);

    // Wait for the drive to release CLK.
    while (iec_get(IO_CLK))
        ;
}

uint8_t
nib_srq_write_handshaked(uint8_t data, uint8_t toggle)
{
    uint16_t write_timeout = 0;

    // Wait for the drive to toggle CLK, break on timeout error.
    while (iec_get(IO_CLK) != toggle) {
        // Timing for 16MHz CPU (ATMega32U4). XXX use proper delay method.
        if (++write_timeout >= 100)
            return 0xff;
    }

    // Write out the data value via SRQ.
    iec_srq_write(data);

    return 0;
}
#endif // SRQ_NIB_SUPPORT
