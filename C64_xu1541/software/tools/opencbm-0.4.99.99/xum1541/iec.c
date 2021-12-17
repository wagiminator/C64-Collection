/*
 * xum1541 IEC routines
 * Copyright (c) 2009-2010 Nate Lawson <nate@root.org>
 *
 * Based on: firmware/xu1541.c
 * Copyright: (c) 2007 by Till Harbaum <till@harbaum.org>
 *
 * Imported at revision:
 * Revision 1.16    2009/01/24 14:51:01    strik
 * New version 1.17;
 * Do not return data for XUM1541_READ after an EOI occurred.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#include "xum1541.h"

/*
 * Table of constants from IEC timing diagram
 */
#define IEC_T_AT    1000 // Max ATN response required time (us)
//      IEC_T_H     inf  // Max listener hold-off time
#define IEC_T_NE    40   // Typical non-EOI response to RFD time (us)
#define IEC_T_S     20   // Min talker bit setup time (us, 70 typical)
#define IEC_T_V     20   // Min data valid time (us, 20 typical)
#define IEC_T_F     1000 // Max frame handshake time (us, 20 typical)
#define IEC_T_R     20   // Min frame to release of ATN time (us)
#define IEC_T_BB    100  // Min time between bytes (us)
#define IEC_T_YE    200  // Min EOI response time (us, 250 typical)
#define IEC_T_EI    60   // Min EOI response hold time (us)
#define IEC_T_RY    60   // Max talker response limit (us, 30 typical)
#define IEC_T_PR    20   // Min byte acknowledge hold time (us, 30 typical)
#define IEC_T_TK    -1   // 20/30/100 talk-attention release time (us)
//      IEC_T_DC    inf  // Talk-attention acknowledge time, 0 - inf
#define IEC_T_DA    80   // Min talk-attention ack hold time (us)
#define IEC_T_FR    60   // Min EOI acknowledge time (us)

static void iec_reset(bool forever);
static uint16_t iec_raw_write(uint16_t len, uint8_t flags);
static uint16_t iec_raw_read(uint16_t len);
static bool iec_wait(uint8_t line, uint8_t state);
static uint8_t iec_poll(void);
static void iec_setrelease(uint8_t set, uint8_t release);

static struct ProtocolFunctions iecFunctions = {
    .cbm_reset = iec_reset,
    .cbm_raw_write = iec_raw_write,
    .cbm_raw_read = iec_raw_read,
    .cbm_wait = iec_wait,
    .cbm_poll = iec_poll,
    .cbm_setrelease = iec_setrelease,
};

/* fast conversion between logical and physical mapping */
static const uint8_t iec2hw_table[] PROGMEM = {
    0,
    IO_DATA,
              IO_CLK,
    IO_DATA | IO_CLK,
                       IO_ATN,
    IO_DATA |          IO_ATN,
              IO_CLK | IO_ATN,
    IO_DATA | IO_CLK | IO_ATN,
                                IO_RESET,
    IO_DATA |                   IO_RESET,
              IO_CLK |          IO_RESET,
    IO_DATA | IO_CLK |          IO_RESET,
                       IO_ATN | IO_RESET,
    IO_DATA |          IO_ATN | IO_RESET,
              IO_CLK | IO_ATN | IO_RESET,
    IO_DATA | IO_CLK | IO_ATN | IO_RESET,
};

static uint8_t
iec2hw(uint8_t iec)
{
    return pgm_read_byte(iec2hw_table + iec);
}

// Initialize all IEC lines to idle
struct ProtocolFunctions *
iec_init()
{
    iec_release(IO_ATN | IO_CLK | IO_DATA | IO_RESET);
    DELAY_US(10);
    return &iecFunctions;
}

/*
 * All exit paths have to take ~200 us total for the timing in
 * wait_for_free_bus() to be correct. Once we're past the point of finding
 * an active drive (DATA set after ATN is set), we have hit 200 us.
 */
static uint8_t
check_if_bus_free(void)
{
    // Let go of all lines and wait for the drive to have time to react.
    iec_release(IO_ATN | IO_CLK | IO_DATA | IO_RESET);
    DELAY_US(50);

    // If DATA is held, drive is not yet ready.
    if (iec_get(IO_DATA)) {
        DELAY_US(150);
        return 0;
    }

    /*
     * DATA is free, now make sure it is stable for 50 us. Nate has seen
     * it glitch if DATA is stable for < 38 us before we pull ATN.
     */
    DELAY_US(50);
    if (iec_get(IO_DATA)) {
        DELAY_US(100);
        return 0;
    }

    /*
     * Assert ATN and wait for the drive to have time to react. It typically
     * does so almost immediately.
     */
    iec_set(IO_ATN);
    DELAY_US(100);

    // If DATA is still unset, no drive answered.
    if (!iec_get(IO_DATA)) {
        iec_release(IO_ATN);
        return 0;
    }

    // Good, at least one drive reacted. Now, test releasing ATN.
    iec_release(IO_ATN);
    DELAY_US(100);

    /*
     * The drive released DATA, so we're done.
     *
     * Nate noticed on a scope that the drive pulls DATA for 60 us,
     * 150-500 us after releasing it in response to when we release ATN.
     */
    return !iec_get(IO_DATA);
}

/*
 * Wait up to 1.5 secs to see if any drive answers ATN toggle.
 * Technically, we only have to wait up to IEC_T_AT (1 ms) but we're
 * being generous here.
 */
static void
wait_for_free_bus(bool forever)
{
    uint16_t i;

    for (i = (uint16_t)(XUM1541_TIMEOUT * 5000); i != 0 || forever; i--) {
        /*
         * We depend on the internal delays within this function to be sure
         * the whole loop takes long enough. If there is no device, this
         * takes 200 us per try (1.5 sec total timeout).
         *
         * In the minimal case (DATA held the whole time), it only delays
         * for 50 us per try (375 ms total timeout). This is still much
         * more than IEC_T_AT.
         */
        if (check_if_bus_free())
            return;

        // Bail out early if host signalled an abort.
        if (!TimerWorker())
            return;
    }
    DEBUGF(DBG_ERROR, "wait4free bus to\n");
}

static void
iec_reset(bool forever)
{
    DEBUGF(DBG_ALL, "reset\n");
    iec_release(IO_DATA | IO_ATN | IO_CLK);

    /*
     * Hold the device in reset a while. 20 ms was too short and it didn't
     * fully reset (e.g., motor did not run). Nate checked with a scope
     * and his 1541-B grabs DATA exactly 25 ms after RESET goes active.
     * 30 ms seems good. It takes about 1.2 seconds before the drive answers
     * by grabbing DATA.
     *
     * There is a small glitch at 25 ms after grabbing RESET where RESET out
     * goes inactive for 1 us. This corresponds with the drive grabbing CLK
     * and DATA, and for about 40 ns, ATN also. Nate assumes this is
     * crosstalk from the VIAs being setup by the 6502.
     */
    iec_set(IO_RESET);
    /*
     * For one of Womo's drives a reset low hold time of 30 ms was too short.
     * Therefore the value was increased to 100ms. The drive already worked
     * with a value of 45 ms, but it was more than doubled to 100 ms to get a
     * safety margin for even very worse cabling or input circuit conditions.
     */
    DELAY_MS(100);
    iec_release(IO_RESET);

    wait_for_free_bus(forever);
}

// Wait up to 2 ms for any of the masked lines to become active.
static uint8_t
iec_wait_timeout_2ms(uint8_t mask, uint8_t state)
{
    uint8_t count = 200;

    while ((iec_poll_pins() & mask) == state && count-- != 0)
        DELAY_US(10);

    return ((iec_poll_pins() & mask) != state);
}

// Wait up to 400 us for CLK to be pulled by the drive.
static void
iec_wait_clk(void)
{
    uint8_t count = 200;

    while (iec_get(IO_CLK) == 0 && count-- != 0)
        DELAY_US(2);
}

/*
 * Send a byte, one bit at a time via the IEC protocol.
 *
 * The minimum spec setup (Ts) and hold times (Tv) are both 20 us. However,
 * Nate found that the 16 Mhz AT90USB162 was not recognized by his
 * 1541 when using these intervals.
 *
 * It appears the spec is much too optimistic. The typical setup time (Ts)
 * of 70 us is also not quite long enough. Increasing the setup time to
 * 72 us appears to work consistently, but he chose the value 75 us to
 * give more margin. The 1541 consistently takes 120 us for Ts and
 * 70 us for Tv, which is probably why no one noticed this before.
 *
 * The hold time did not appear to have any effect. In fact, reducing the
 * hold time to 15 us still worked fine.
 */
static uint8_t
send_byte(uint8_t b)
{
    uint8_t i, ack = 0;

    for (i = 8; i != 0; i--) {
        // Wait for Ts (setup) with additional padding
        DELAY_US(IEC_T_S + 55);

        // Set the bit value on the DATA line and wait for it to settle.
        if (!(b & 1)) {
            iec_set(IO_DATA);
            IEC_DELAY();
        }

        // Trigger clock edge and hold valid for spec minimum time (Tv).
        iec_release(IO_CLK);
        DELAY_US(IEC_T_V);

        iec_set_release(IO_CLK, IO_DATA);
        b >>= 1;
    }

    /*
     * Wait up to 2 ms for DATA to be driven by device (IEC_T_F).
     * It takes around 70-80 us on Nate's 1541.
     */
    ack = iec_wait_timeout_2ms(IO_DATA, IO_DATA);
    if (!ack) {
        DEBUGF(DBG_ERROR, "sndbyte nak\n");
    }

    return ack;
}

/*
 * Wait for listener to release DATA line. We wait forever.
 * This is because the listener hold-off time (Th) is allowed to be
 * infinite (e.g., for printers or other slow equipment).
 *
 * Nate's 1541 responds in about 670 us for an OPEN from idle.
 * It responds in about 65 us between bytes of a transaction.
 */
static bool
wait_for_listener(void)
{
    // Release CLK to indicate that we are ready to send.
    iec_release(IO_CLK);

    // Wait forever (IEC_T_H) for device to do the same with the DATA line.
    while (iec_get(IO_DATA)) {
        // If we got an abort, bail out of this loop.
        if (!TimerWorker())
            return false;
    }
    return true;
}

/*
 * Write bytes to the drive via the CBM default protocol.
 * Returns number of successful written bytes or 0 on error.
 */
static uint16_t
iec_raw_write(uint16_t len, uint8_t flags)
{
    uint8_t atn, talk, data;
    uint16_t rv;

    rv = len;
    atn = flags & XUM_WRITE_ATN;
    talk = flags & XUM_WRITE_TALK;
    eoi = 0;

    DEBUGF(DBG_INFO, "cwr %d, atn %d, talk %d\n", len, atn, talk);
    if (len == 0)
        return 0;

    usbInitIo(len, ENDPOINT_DIR_OUT);

    /*
     * First, check if any device is present on the bus.
     * If ATN and RST are both low (active), we know that at least one
     * drive is attached but none are powered up. In this case, we
     * bail out early. Otherwise, we'd get stuck in wait_for_listener().
     */
    if (!iec_wait_timeout_2ms(IO_ATN|IO_RESET, 0)) {
        DEBUGF(DBG_ERROR, "write: no devs on bus\n");
        usbIoDone();
        return 0;
    }

    iec_release(IO_DATA);
    iec_set(IO_CLK | (atn ? IO_ATN : 0));
    IEC_DELAY();

    // Wait for any device to pull data after we set CLK. This is actually
    // IEC_T_AT (1 ms) but we allow a bit longer.
    if (!iec_wait_timeout_2ms(IO_DATA, IO_DATA)) {
        DEBUGF(DBG_ERROR, "write: no devs\n");
        iec_release(IO_CLK | IO_ATN);
        usbIoDone();
        return 0;
    }

    /*
     * Wait a short while for drive to be ready for us to release CLK.
     * This uses the typical value for IEC_T_NE. Even though it has no
     * minimum, the transfer starts to be unreliable for Tne somewhere
     * below 10 us.
     */
    DELAY_US(IEC_T_NE);

    // Respond with data as soon as device is ready (max time Tne, 200 us).
    while (len != 0) {
        // Be sure DATA line has been pulled by device. If not, we timed
        // out without a device being ready.
        if (!iec_get(IO_DATA)) {
            DEBUGF(DBG_ERROR, "write: dev not pres\n");
            rv = 0;
            break;
        }

        // Release CLK and wait forever for listener to release data.
        if (!wait_for_listener()) {
            DEBUGF(DBG_ERROR, "write: w4l abrt\n");
            rv = 0;
            break;
        }

        /*
         * Signal EOI by waiting so long (IEC_T_YE, > 200 us) that
         * listener pulls DATA, then wait for it to be released.
         * The device will do so in IEC_T_EI, >= 60 us.
         *
         * If we're not signalling EOI, we must set CLK (below) in less
         * than 200 us after wait_for_listener() (IEC_T_RY).
         */
        if (len == 1 && !atn) {
            iec_wait_timeout_2ms(IO_DATA, IO_DATA);
            iec_wait_timeout_2ms(IO_DATA, 0);
        }
        iec_set(IO_CLK);

        // Get a data byte from host, quitting if it signalled an abort.
        if (usbRecvByte(&data) != 0) {
            rv = 0;
            break;
        }
        if (send_byte(data)) {
            len--;
            DELAY_US(IEC_T_BB);
        } else {
            DEBUGF(DBG_ERROR, "write: io err\n");
            rv = 0;
            break;
        }

        wdt_reset();
    }
    usbIoDone();

    /*
     * We rely on the per-byte IEC_T_BB delay (above) being more than
     * the minimum time before releasing ATN (IEC_T_R).
     */
    if (rv != 0) {
        // Talk-ATN turn around (talker and listener exchange roles).
        if (talk) {
            // Hold DATA and release ATN, waiting talk-ATN release time.
            iec_set_release(IO_DATA, IO_ATN);
            DELAY_US(IEC_T_TK);

            // Now release CLK and wait for device to grab it.
            iec_release(IO_CLK);
            IEC_DELAY();

            // Wait forever for device (IEC_T_DC).
            while (!iec_get(IO_CLK)) {
                if (!TimerWorker()) {
                    rv = 0;
                    break;
                }
            }
        } else
            iec_release(IO_ATN);
    } else {
        /*
         * If there was an error, release all lines before returning.
         * Delay the minimum time to releasing ATN after frame, just in
         * case the IEC_T_BB delay (above) was skipped. It is only performed
         * if send_byte() succeeded and not in this error case.
         */
        DELAY_US(IEC_T_R);
        iec_release(IO_CLK | IO_ATN);
    }

    DEBUGF(DBG_INFO, "wrv=%d\n", rv);
    return rv;
}

static uint16_t
iec_raw_read(uint16_t len)
{
    uint8_t ok, bit, b;
    uint16_t to, count;

    DEBUGF(DBG_INFO, "crd %d\n", len);
    usbInitIo(len, ENDPOINT_DIR_IN);
    count = 0;
    do {
        to = 0;

        /* wait for clock to be released. typically times out during: */
        /* directory read */
        while (iec_get(IO_CLK)) {
            if (to >= 50000 || !TimerWorker()) {
                /* 1.0 (50000 * 20us) sec timeout */
                DEBUGF(DBG_ERROR, "rd to\n");
                usbIoDone();
                return 0;
            }
            to++;
            DELAY_US(20);
        }

        // XXX is this right? why treat EOI differently here?
        if (eoi) {
            usbIoDone();
            return 0;
        }

        /* release DATA line */
        iec_release(IO_DATA);

        /* use special "timer with wait for clock" */
        iec_wait_clk();

        // Is the talking device signalling EOI?
        if (iec_get(IO_CLK) == 0) {
            eoi = 1;
            iec_set(IO_DATA);
            DELAY_US(70);
            iec_release(IO_DATA);
        }

        /*
         * Disable IRQs to make sure the byte transfer goes uninterrupted.
         * This isn't strictly needed since the only interrupt we use is the
         * one for USB control transfers.
         */
        cli();

        // Wait up to 2 ms for CLK to be asserted
        ok = iec_wait_timeout_2ms(IO_CLK, IO_CLK);

        // Read all 8 bits of a byte
        for (bit = b = 0; bit < 8 && ok; bit++) {
            // Wait up to 2 ms for CLK to be released
            ok = iec_wait_timeout_2ms(IO_CLK, 0);
            if (ok) {
                b >>= 1;
                if (iec_get(IO_DATA) == 0)
                    b |= 0x80;

                // Wait up to 2 ms for CLK to be asserted
                ok = iec_wait_timeout_2ms(IO_CLK, IO_CLK);
            }
        }

        sei();

        if (ok) {
            // Acknowledge byte received ok
            iec_set(IO_DATA);

            // Send the data byte to host, quitting if it signalled an abort.
            if (usbSendByte(b))
                break;
            count++;
            DELAY_US(50);
        }

        wdt_reset();
    } while (count != len && ok && !eoi);

    if (!ok) {
        DEBUGF(DBG_ERROR, "read io err\n");
        count = 0;
    }

    DEBUGF(DBG_INFO, "rv=%d\n", count);
    usbIoDone();
    return count;
}

/* wait forever for a specific line to reach a certain state */
static bool
iec_wait(uint8_t line, uint8_t state)
{
    uint8_t hw_mask, hw_state;

    /* calculate hw mask and expected state */
    hw_mask = iec2hw(line);
    hw_state = state ? hw_mask : 0;

    while ((iec_poll_pins() & hw_mask) == hw_state) {
        if (!TimerWorker())
            return false;
        DELAY_US(10);
    }

    return true;
}

static uint8_t
iec_poll(void)
{
    uint8_t iec_state, rv = 0;

    iec_state = iec_poll_pins();
    if ((iec_state & IO_DATA) == 0)
        rv |= IEC_DATA;
    if ((iec_state & IO_CLK) == 0)
        rv |= IEC_CLOCK;
    if ((iec_state & IO_ATN) == 0)
        rv |= IEC_ATN;

    return rv;
}

static void
iec_setrelease(uint8_t set, uint8_t release)
{
    if (release == 0)
        iec_set(iec2hw(set));
    else if (set == 0)
        iec_release(iec2hw(release));
    else
        iec_set_release(iec2hw(set), iec2hw(release));
}
