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

static const unsigned char s2_drive_prog[] = {
#include "s2.inc"
};

static int s2_read_byte(CBM_FILE fd, unsigned char *c)
{
    int i;
    *c = 0;
    for(i=4; i>0; i--) {
                                                                        SETSTATEDEBUG(DebugBitCount = i*2);
        cbm_iec_release(fd, IEC_ATN);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
        while(cbm_iec_get(fd, IEC_CLOCK));
                                                                        SETSTATEDEBUG((void)0);
        *c = (*c>>1) | (cbm_iec_get(fd, IEC_DATA) ? 0x80 : 0);
#else
        *c = (*c>>1) | ((cbm_iec_wait(fd, IEC_CLOCK, 0) & IEC_DATA) ? 0x80 : 0);
#endif
                                                                        SETSTATEDEBUG(DebugBitCount--);
        cbm_iec_set(fd, IEC_ATN);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
        while(!cbm_iec_get(fd,IEC_CLOCK));
                                                                        SETSTATEDEBUG((void)0);
        *c = (*c>>1) | (cbm_iec_get(fd, IEC_DATA) ? 0x80 : 0);
#else
        *c = (*c>>1) | ((cbm_iec_wait(fd, IEC_CLOCK, 1) & IEC_DATA) ? 0x80 : 0);
#endif
    }
                                                                        SETSTATEDEBUG(DebugBitCount = -1);
    cbm_iec_release(fd, IEC_ATN);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
    while(cbm_iec_get(fd, IEC_CLOCK));
#else
    cbm_iec_wait(fd, IEC_CLOCK, 0);
#endif
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_set(fd, IEC_ATN);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
    while(!cbm_iec_get(fd, IEC_CLOCK));
#else
    cbm_iec_wait(fd, IEC_CLOCK, 1);
#endif
                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

static int s2_write_byte(CBM_FILE fd, unsigned char c)
{
    int i;
    for(i=4; i>0; i--) {
                                                                        SETSTATEDEBUG(DebugBitCount = i*2);
        c & 1 ? cbm_iec_set(fd, IEC_DATA) : cbm_iec_release(fd, IEC_DATA);
        c >>= 1;
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_release(fd, IEC_ATN);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
        while(cbm_iec_get(fd, IEC_CLOCK));
#else
        cbm_iec_wait(fd, IEC_CLOCK, 0);
#endif
                                                                        SETSTATEDEBUG(DebugBitCount--);
        c & 1 ? cbm_iec_set(fd, IEC_DATA) : cbm_iec_release(fd, IEC_DATA);
        c >>= 1;
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_set(fd, IEC_ATN);

                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
        while(!cbm_iec_get(fd, IEC_CLOCK));
#else
        cbm_iec_wait(fd, IEC_CLOCK, 1);
#endif
    }
                                                                        SETSTATEDEBUG(DebugBitCount = -1);
    cbm_iec_release(fd, IEC_DATA);
                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

static int
upload(CBM_FILE fd, unsigned char drive)
{
    unsigned int bytesWritten;

    bytesWritten = cbm_upload(fd, drive, 0x700, s2_drive_prog, sizeof(s2_drive_prog));

    if (bytesWritten != sizeof(s2_drive_prog))
    {
        DBG_ERROR((DBG_PREFIX "wanted to write %u bytes, but only %u "
            "bytes could be written", sizeof(s2_drive_prog), bytesWritten));

        return 1;
    }

    return 0;
}

static int
init(CBM_FILE fd, unsigned char drive)
{
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_release(fd, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);
    while(!cbm_iec_get(fd, IEC_CLOCK));
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_set(fd, IEC_ATN);
                                                                        SETSTATEDEBUG((void)0);
    arch_usleep(20000);

    return 0;
}

static int
read1byte(CBM_FILE fd, unsigned char *c1)
{
    int ret;
                                                                        SETSTATEDEBUG(DebugByteCount = -6401);
    ret = s2_read_byte(fd, c1);
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
    return ret;
}

static int
read2byte(CBM_FILE fd, unsigned char *c1, unsigned char *c2)
{
    int ret = 0;
                                                                        SETSTATEDEBUG(DebugByteCount = -25601);
    ret = s2_read_byte(fd, c1);
                                                                        SETSTATEDEBUG(DebugByteCount = -12802);
    if (ret == 0)
        ret = s2_read_byte(fd, c2);
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
    return 1;
}

static int
readblock(CBM_FILE fd, unsigned char *p, unsigned int length)
{
                                                                        SETSTATEDEBUG(DebugByteCount = 0);
    for (; length < 0x100; length++)
    {
                                                                        SETSTATEDEBUG(DebugByteCount++);
        if (s2_read_byte(fd, p++))
        {
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
            return 1;
        }
    }
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
    return 0;
}

static int
write1byte(CBM_FILE fd, unsigned char c1)
{
    int ret;
                                                                        SETSTATEDEBUG(DebugByteCount = -6401);
    ret = s2_write_byte(fd, c1);
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
    return ret;
}

static int
write2byte(CBM_FILE fd, unsigned char c1, unsigned char c2)
{
    int ret = 0;
                                                                        SETSTATEDEBUG(DebugByteCount = -12801);
    ret = s2_write_byte(fd, c1);
                                                                        SETSTATEDEBUG(DebugByteCount = -12802);
    if (ret == 0)
        ret = s2_write_byte(fd, c2);
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
    return 1;
}

static int
writeblock(CBM_FILE fd, unsigned char *p, unsigned int length)
{
                                                                        SETSTATEDEBUG(DebugByteCount = 0);
    for (; length < 0x100; length++)
    {
                                                                        SETSTATEDEBUG(DebugByteCount++);
        if (s2_write_byte(fd, *p++))
        {
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
            return 1;
        }
    }
                                                                        SETSTATEDEBUG(DebugByteCount = -1);
    return 0;
}

DECLARE_TRANSFER_FUNCS(s2);
