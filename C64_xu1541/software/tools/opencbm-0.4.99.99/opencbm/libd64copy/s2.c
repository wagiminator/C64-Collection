/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2008 Spiro Trikaliotis
*/

#include "opencbm.h"
#include "d64copy_int.h"

#include <stdlib.h>

#include "arch.h"

#include "opencbm-plugin.h"

static opencbm_plugin_s2_read_n_t * opencbm_plugin_s2_read_n = NULL;

static opencbm_plugin_s2_write_n_t * opencbm_plugin_s2_write_n = NULL;

static const unsigned char s2_drive_prog[] = {
#include "s2.inc"
};

static CBM_FILE fd_cbm;
static int two_sided;

static int s2_read_byte(CBM_FILE fd, unsigned char *c)
{
    int i;
    *c = 0;
    for(i=4; i>0; i--) {
                                                                        SETSTATEDEBUG(DebugBitCount=i*2);
#ifndef USE_CBM_IEC_WAIT
        while(cbm_iec_get(fd, IEC_CLOCK));
                                                                        SETSTATEDEBUG((void)0);
        *c = (*c>>1) | (cbm_iec_get(fd, IEC_DATA) ? 0x80 : 0);
#else
        *c = (*c>>1) | ((cbm_iec_wait(fd, IEC_CLOCK, 0) & IEC_DATA) ? 0x80 : 0);
#endif
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_release(fd, IEC_ATN);
                                                                        SETSTATEDEBUG(DebugBitCount--);
#ifndef USE_CBM_IEC_WAIT
        while(!cbm_iec_get(fd,IEC_CLOCK));
                                                                        SETSTATEDEBUG((void)0);
        *c = (*c>>1) | (cbm_iec_get(fd, IEC_DATA) ? 0x80 : 0);
#else
        *c = (*c>>1) | ((cbm_iec_wait(fd, IEC_CLOCK, 1) & IEC_DATA) ? 0x80 : 0);
#endif
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_set(fd, IEC_ATN);
    }
                                                                        SETSTATEDEBUG(DebugBitCount=-1);
    return 0;
}

static int s2_write_byte_nohs(CBM_FILE fd, unsigned char c)
{
    int i;
    for(i=4; ; i--) {
                                                                        SETSTATEDEBUG(DebugBitCount=i*2);
        c & 1 ? cbm_iec_set(fd, IEC_DATA) : cbm_iec_release(fd, IEC_DATA);
        c >>= 1;
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_release(fd, IEC_ATN);
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

        if(i<=1) return 0;

                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
        while(!cbm_iec_get(fd, IEC_CLOCK));
#else
        cbm_iec_wait(fd, IEC_CLOCK, 1);
#endif
    }
}

/* read_n redirects USB reads to the external reader if required */
static void read_n(unsigned char *data, int size) 
{
    int i;

    if (opencbm_plugin_s2_read_n)
    {
        opencbm_plugin_s2_read_n(fd_cbm, data, size);
        return;
    }

    for(i=0;i<size;i++)
	s2_read_byte(fd_cbm, data++);
}

static int s2_write_byte(CBM_FILE fd, unsigned char c)
{
                                                                        SETSTATEDEBUG((void)0);
    s2_write_byte_nohs(fd, c);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
    while(!cbm_iec_get(fd, IEC_CLOCK));
#else
    cbm_iec_wait(fd, IEC_CLOCK, 1);
#endif
                                                                        SETSTATEDEBUG(DebugBitCount=-1);
    cbm_iec_release(fd, IEC_DATA);
                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

/* write_n redirects USB writes to the external reader if required */
static void write_n(const unsigned char *data, int size) 
{
    int i;

    if (opencbm_plugin_s2_write_n)
    {
        opencbm_plugin_s2_write_n(fd_cbm, data, size);
        return;
    }

    for(i=0;i<size;i++)
	s2_write_byte(fd_cbm, *data++);
}

static int read_block(unsigned char tr, unsigned char se, unsigned char *block)
{
    unsigned char status;

                                                                        SETSTATEDEBUG((void)0);
    write_n(&tr, 1);
                                                                        SETSTATEDEBUG((void)0);
    write_n(&se, 1);
#ifndef USE_CBM_IEC_WAIT
    arch_usleep(20000);
#endif
                                                                        SETSTATEDEBUG((void)0);
    read_n(&status, 1);
                                                                        SETSTATEDEBUG(DebugByteCount=0);
    read_n(block, BLOCKSIZE);
                                                                        SETSTATEDEBUG(DebugByteCount=-1);

    return status;
}

static int write_block(unsigned char tr, unsigned char se, const unsigned char *blk, int size, int read_status)
{
    unsigned char status;
                                                                        SETSTATEDEBUG((void)0);
    write_n(&tr, 1);
                                                                        SETSTATEDEBUG((void)0);
    write_n(&se, 1);
                                                                        SETSTATEDEBUG(DebugByteCount=0);
    write_n(blk, size);
                                                                        SETSTATEDEBUG(DebugByteCount=-1);
#ifndef USE_CBM_IEC_WAIT
    if(size == BLOCKSIZE) {
        arch_usleep(20000);
    }
#endif
                                                                        SETSTATEDEBUG((void)0);
    read_n(&status, 1);
                                                                        SETSTATEDEBUG((void)0);
    return status;
}

static int open_disk(CBM_FILE fd, d64copy_settings *settings,
                     const void *arg, int for_writing,
                     turbo_start start, d64copy_message_cb message_cb)
{
    unsigned char d = (unsigned char)(ULONG_PTR)arg;

    fd_cbm = fd;
    two_sided = settings->two_sided;

    opencbm_plugin_s2_read_n = cbm_get_plugin_function_address("opencbm_plugin_s2_read_n");

    opencbm_plugin_s2_write_n = cbm_get_plugin_function_address("opencbm_plugin_s2_write_n");

                                                                        SETSTATEDEBUG((void)0);
    cbm_upload(fd_cbm, d, 0x700, s2_drive_prog, sizeof(s2_drive_prog));
                                                                        SETSTATEDEBUG((void)0);
    start(fd, d);
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_release(fd_cbm, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);
    while(!cbm_iec_get(fd_cbm, IEC_CLOCK));
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_set(fd_cbm, IEC_ATN);
    arch_usleep(20000);
    
                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

static void close_disk(void)
{
                                                                        SETSTATEDEBUG((void)0);
    s2_write_byte(fd_cbm, 0);
                                                                        SETSTATEDEBUG((void)0);
    s2_write_byte_nohs(fd_cbm, 0);
    arch_usleep(100);
                                                                        SETSTATEDEBUG(DebugBitCount=-1);
    cbm_iec_release(fd_cbm, IEC_DATA);
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_release(fd_cbm, IEC_ATN);
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_set(fd_cbm, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);

    opencbm_plugin_s2_read_n = NULL;

    opencbm_plugin_s2_write_n = NULL;
}

static int send_track_map(unsigned char tr, const char *trackmap, unsigned char count)
{
    int i;
    int size;
    unsigned char *data;

                                                                        SETSTATEDEBUG((void)0);
    size = d64copy_sector_count(two_sided, tr);
    data = malloc(2+size);

    data[0] = tr;
    data[1] = count;

    /* build track map */
    for(i = 0; i < size; i++)
	data[2+i] = !NEED_SECTOR(trackmap[i]);
    
    write_n(data, size+2);
    free(data);
                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

static int read_gcr_block(unsigned char *se, unsigned char *gcrbuf)
{
    unsigned char s;

                                                                        SETSTATEDEBUG((void)0);
    read_n(&s, 1);
    *se = s;
                                                                        SETSTATEDEBUG((void)0);
    read_n(&s, 1);

    if(s) {
        return s;
    }
                                                                        SETSTATEDEBUG(DebugByteCount=0);
    read_n(gcrbuf, GCRBUFSIZE);									
                                                                        SETSTATEDEBUG(DebugByteCount=-1);
    return 0;
}

DECLARE_TRANSFER_FUNCS_EX(s2_transfer, 1, 1);
