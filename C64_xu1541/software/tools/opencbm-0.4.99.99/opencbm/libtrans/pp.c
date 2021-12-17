/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2005 Michael Klein <michael.klein@puffin.lb.shuttle.de>
*/

#include "opencbm.h"
#include "libtrans_int.h"

#include <stdlib.h>

#include "arch.h"

static const unsigned char pp1541_drive_prog[] = {
#include "pp1541.inc"
};

static const unsigned char pp1571_drive_prog[] = {
#include "pp1571.inc"
};

static int pp_write(CBM_FILE fd, unsigned char c1, unsigned char c2)
{
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
    while(!cbm_iec_get(fd, IEC_DATA));
#else
    cbm_iec_wait(fd, IEC_DATA, 1);
#endif
                                                                        SETSTATEDEBUG((void)0);
    cbm_pp_write(fd, c1);
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_release(fd, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
    while(cbm_iec_get(fd, IEC_DATA));
#else
    cbm_iec_wait(fd, IEC_DATA, 0);
#endif

                                                                        SETSTATEDEBUG((void)0);
    cbm_pp_write(fd, c2);
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_set(fd, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
    while(!cbm_iec_get(fd, IEC_DATA));
#else
    cbm_iec_wait(fd, IEC_DATA, 1);
#endif

                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

static int pp_read(CBM_FILE fd, unsigned char *c1, unsigned char *c2)
{
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_release(fd, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
    while(cbm_iec_get(fd, IEC_DATA));
#else
    cbm_iec_wait(fd, IEC_DATA, 0);
#endif
                                                                        SETSTATEDEBUG((void)0);
    *c1 = cbm_pp_read(fd);

                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_set(fd, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
    while(!cbm_iec_get(fd, IEC_DATA));
#else
    cbm_iec_wait(fd, IEC_DATA, 1);
#endif
                                                                        SETSTATEDEBUG((void)0);
    *c2 = cbm_pp_read(fd);

                                                                        SETSTATEDEBUG((void)0);
    return 0;
}


static int
upload(CBM_FILE fd, unsigned char drive)
{
    enum cbm_device_type_e driveType;
    const unsigned char *pp_drive_prog = 0;
    unsigned int pp_drive_prog_length = 0;
    unsigned int bytesWritten;

    if (cbm_identify(fd, drive, &driveType, NULL))
        return 1;

    switch (driveType)
    {
    case cbm_dt_cbm1581:
        DBG_ERROR((DBG_PREFIX "1581 not supported!"));
        return 1;

    case cbm_dt_cbm1541:
        DBG_PRINT((DBG_PREFIX "recognized 1541."));
        pp_drive_prog = pp1541_drive_prog;
        pp_drive_prog_length = sizeof(pp1541_drive_prog);
        break;

    case cbm_dt_cbm1570:
    case cbm_dt_cbm1571:
        DBG_PRINT((DBG_PREFIX "recognized 1571."));
        pp_drive_prog = pp1571_drive_prog;
        pp_drive_prog_length = sizeof(pp1571_drive_prog);
        break;

    case cbm_dt_unknown:
        /* FALL THROUGH */

    default:
        DBG_ERROR((DBG_PREFIX "unknown device type!"));
        return 1;
    }

    bytesWritten = cbm_upload(fd, drive, 0x700, pp_drive_prog, pp_drive_prog_length);

    if (bytesWritten != pp_drive_prog_length)
    {
        DBG_ERROR((DBG_PREFIX "wanted to write %u bytes, but only %u "
            "bytes could be written", pp_drive_prog_length, bytesWritten));

        return 1;
    }

    return 0;
}

static int
init(CBM_FILE fd, unsigned char drive)
{
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_set(fd, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_wait(fd, IEC_DATA, 1);

                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

static int
read1byte(CBM_FILE fd, unsigned char *c1)
{
    unsigned char dummy;
    int ret;
                                                                        SETSTATEDEBUG(DebugByteCount = -6401);
    ret = pp_read(fd, c1, &dummy);
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
    return ret;
}

static int
read2byte(CBM_FILE fd, unsigned char *c1, unsigned char *c2)
{
    int ret;
                                                                        SETSTATEDEBUG(DebugByteCount = -12801);
    ret = pp_read(fd, c1, c2);
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
    return ret;
}

static int
readblock(CBM_FILE fd, unsigned char *p, unsigned int length)
{
    unsigned char c1;
    unsigned char c2;
                                                                        SETSTATEDEBUG(DebugByteCount = 0);
    for (; length < 0x100; length++)
    {
                                                                        SETSTATEDEBUG(DebugByteCount += 2);
        if (pp_read(fd, &c1, &c2))
        {
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
            return 1;
        }

        *p++ = c1; length++;

        if (length < 0x100)
            *p++ = c2;
    }
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
    // perform end-handshake
    pp_read(fd, &c1, &c2);
                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

static int
write1byte(CBM_FILE fd, unsigned char c1)
{
    int ret;
                                                                        SETSTATEDEBUG(DebugByteCount = -6401);
    ret = pp_write(fd, c1, 0);
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
    return ret;
}

static int
write2byte(CBM_FILE fd, unsigned char c1, unsigned char c2)
{
    int ret;
                                                                        SETSTATEDEBUG(DebugByteCount = -12801);
    ret = pp_write(fd, c1, c2);
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
    return ret;
}

static int
writeblock(CBM_FILE fd, unsigned char *p, unsigned int length)
{
                                                                        SETSTATEDEBUG(DebugByteCount = 0);
    for (; length < 0x100; length += 2, p += 2)
    {
                                                                        SETSTATEDEBUG(DebugByteCount += 2);
        if (pp_write(fd, p[0], p[1]))
        {
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
            return 1;
        }
    }

                                                                        SETSTATEDEBUG(DebugByteCount = -1);
    return 0;
}

DECLARE_TRANSFER_FUNCS(pp);
