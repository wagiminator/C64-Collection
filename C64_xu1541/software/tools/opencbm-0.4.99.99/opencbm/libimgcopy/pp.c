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
#include "imgcopy_int.h"

#include <stdlib.h>

#include "arch.h"

#include "opencbm-plugin.h"

static opencbm_plugin_pp_dc_read_n_t * opencbm_plugin_pp_dc_read_n = NULL;

static opencbm_plugin_pp_dc_write_n_t * opencbm_plugin_pp_dc_write_n = NULL;

enum pp_direction_e
{
    PP_READ, PP_WRITE
};

static CBM_FILE fd_cbm;
static int two_sided;

static const unsigned char pp1541_drive_prog[] = {
#include "pp1541.inc"
};

static const unsigned char pp1571_drive_prog[] = {
#include "pp1571.inc"
};

static void pp_check_direction(enum pp_direction_e dir)
{
    static enum pp_direction_e direction = PP_READ;
    if(direction != dir)
    {
        arch_usleep(100);
        direction = dir;
    }
}

static int pp_write(CBM_FILE fd, char c1, char c2)
{
                                                                        SETSTATEDEBUG((void)0);
    pp_check_direction(PP_WRITE);
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
    return 0;
}

/* write_n redirects USB writes to the external reader if required */
static void write_n(const unsigned char *data, int size) 
{
    int i;

    if (opencbm_plugin_pp_dc_write_n)
    {
        opencbm_plugin_pp_dc_write_n(fd_cbm, data, size);
        return;
    }

    for(i=0;i<size/2;i++,data+=2)
	pp_write(fd_cbm, data[0], data[1]);
}

static int pp_read(CBM_FILE fd, unsigned char *c1, unsigned char *c2)
{
                                                                        SETSTATEDEBUG((void)0);
    pp_check_direction(PP_READ);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
    while(!cbm_iec_get(fd, IEC_DATA));
#else
    cbm_iec_wait(fd, IEC_DATA, 1);
#endif
                                                                        SETSTATEDEBUG((void)0);
    *c1 = cbm_pp_read(fd);
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_release(fd, IEC_CLOCK);

                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
    while(cbm_iec_get(fd, IEC_DATA));
#else
    cbm_iec_wait(fd, IEC_DATA, 0);
#endif
                                                                        SETSTATEDEBUG((void)0);
    *c2 = cbm_pp_read(fd);
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_set(fd, IEC_CLOCK);

                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

/* read_n redirects USB reads to the external reader if required */
static void read_n(unsigned char *data, int size) 
{
    int i;

    if (opencbm_plugin_pp_dc_read_n)
    {
        opencbm_plugin_pp_dc_read_n(fd_cbm, data, size);
        return;
    }

    for(i=0;i<size/2;i++,data+=2)
	pp_read(fd_cbm, data, data+1);
}

static int read_block(unsigned char tr, unsigned char se, unsigned char *block)
{
    unsigned char status[2];
                                                                        SETSTATEDEBUG((void)0);

    status[0] = tr; status[1] = se;
    write_n(status, 2);

#ifndef USE_CBM_IEC_WAIT    
    arch_usleep(20000);
#endif
                                                                        SETSTATEDEBUG((void)0);
    read_n(status, 2);

                                                                        SETSTATEDEBUG(debugLibImgByteCount=0);
    read_n(block, BLOCKSIZE);
                                                                        SETSTATEDEBUG(debugLibImgByteCount=-1);

                                                                        SETSTATEDEBUG((void)0);
    return status[1];
}

static int write_block(unsigned char tr, unsigned char se, const unsigned char *blk, int size, int read_status)
{
    int i = 0;
    unsigned char status[2];

                                                                        SETSTATEDEBUG((void)0);
    status[0] = tr; status[1] = se;
    write_n(status, 2);

                                                                        SETSTATEDEBUG((void)0);
    /* send first byte twice if length is odd */
    if(size % 2) {
        write_n(blk, 2);
        i = 1;
    }
                                                                        SETSTATEDEBUG(debugLibImgByteCount=0);
    write_n(blk+i, size-i);

                                                                        SETSTATEDEBUG(debugLibImgByteCount=-1);
#ifndef USE_CBM_IEC_WAIT    
    if(size == BLOCKSIZE) {
        arch_usleep(20000);
    }
#endif

                                                                        SETSTATEDEBUG((void)0);
    read_n(status, 2);

                                                                        SETSTATEDEBUG((void)0);
    return status[1];
}

static int open_disk(CBM_FILE fd, imgcopy_settings *settings,
                     const void *arg, int for_writing,
                     turbo_start start, imgcopy_message_cb message_cb)
{
    unsigned char d = (unsigned char)(ULONG_PTR)arg;
    const unsigned char *drive_prog;
    int prog_size;

    fd_cbm    = fd;
    two_sided = settings->two_sided;

    opencbm_plugin_pp_dc_read_n = cbm_get_plugin_function_address("opencbm_plugin_pp_dc_read_n");

    opencbm_plugin_pp_dc_write_n = cbm_get_plugin_function_address("opencbm_plugin_pp_dc_write_n");

    if(settings->drive_type != cbm_dt_cbm1541)
    {
        drive_prog = pp1571_drive_prog;
        prog_size  = sizeof(pp1571_drive_prog);
    }
    else
    {
        drive_prog = pp1541_drive_prog;
        prog_size  = sizeof(pp1541_drive_prog);
    }

                                                                        SETSTATEDEBUG((void)0);
    /* make sure the XP1541 portion of the cable is in input mode */
    cbm_pp_read(fd_cbm);

                                                                        SETSTATEDEBUG((void)0);
    cbm_upload(fd_cbm, d, 0x700, drive_prog, prog_size);
                                                                        SETSTATEDEBUG((void)0);
    start(fd, d);
                                                                        SETSTATEDEBUG((void)0);
    pp_check_direction(PP_READ);
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_set(fd_cbm, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_wait(fd_cbm, IEC_DATA, 1);
                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

static void close_disk(void)
{
                                                                        SETSTATEDEBUG((void)0);
    pp_write(fd_cbm, 0, 0);
    arch_usleep(100);
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_wait(fd_cbm, IEC_DATA, 0);

    /* make sure the XP1541 portion of the cable is in input mode */
                                                                        SETSTATEDEBUG((void)0);
    cbm_pp_read(fd_cbm);
                                                                        SETSTATEDEBUG((void)0);

    opencbm_plugin_pp_dc_read_n = NULL;

    opencbm_plugin_pp_dc_write_n = NULL;
}

static int send_track_map(imgcopy_settings *settings, unsigned char tr, const char *trackmap, unsigned char count)
{
    int i, size;
    unsigned char *data;

    size = imgcopy_sector_count(settings, tr);
    data = malloc(2+2*size);

    data[0] = tr;
    data[1] = count;

    /* build track map */
    for(i = 0; i < size; i++)
	data[2+2*i] = data[2+2*i+1] = !NEED_SECTOR(trackmap[i]);
    
    write_n(data, 2*size+2);
    free(data);
                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

static int read_gcr_block(unsigned char *se, unsigned char *gcrbuf)
{
    unsigned char s[2];
                                                                        SETSTATEDEBUG((void)0);
    read_n(s, 2);
    *se = s[1];
                                                                        SETSTATEDEBUG((void)0);
    read_n(s, 2);

    if(s[1]) {
        return s[1];
    }
                                                                        SETSTATEDEBUG(debugLibImgByteCount=0);
    read_n(gcrbuf, GCRBUFSIZE);
                                                                        SETSTATEDEBUG(debugLibImgByteCount=-1);

                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

DECLARE_TRANSFER_FUNCS_EX(pp_transfer, 1, 1);
