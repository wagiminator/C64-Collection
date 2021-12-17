/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2008 Spiro Trikaliotis
 *  Copyright 2011 Thomas Winkler
*/

#include "opencbm.h"
#include "imgcopy_int.h"

#include <stdio.h>

#include <stdlib.h>

#include "arch.h"

#include "opencbm-plugin.h"

static opencbm_plugin_s3_read_n_t * opencbm_plugin_s3_read_n = NULL;

static opencbm_plugin_s3_write_n_t * opencbm_plugin_s3_write_n = NULL;

//
// drive code
//
static const unsigned char s3_drive_prog_1541[] = {
#include "s3.inc"
};
static const unsigned char s3_drive_prog_1581[] = {
#include "s3-1581.inc"
};

static CBM_FILE fd_cbm;
static int two_sided;

//#define DEBUG

/*
static int s3_write_byte_nohs(CBM_FILE fd, unsigned char c)
{
#ifdef DEBUG
    printf("+++ s3_write_byte_nohs()\n");
#endif
    return 0;
}

static int s3_write_byte(CBM_FILE fd, unsigned char c)
{
#ifdef DEBUG
    printf("+++ s3_write_byte()\n");
#endif
    return 0;
}
*/

/* write_n redirects USB writes to the external reader if required */
static void write_n(const unsigned char *data, int size)
{
    if (opencbm_plugin_s3_write_n)
    {
#ifdef DEBUG
    printf("s3_write_n(%d)\n", size);
#endif
        opencbm_plugin_s3_write_n(fd_cbm, data, size);
        return;
    }

#ifdef DEBUG
    printf("+++ s3_write_n()  bytewise\n");
#endif
}

/* read_n redirects USB reads to the external reader if required */
static void read_n(unsigned char *data, int size)
{
    if (opencbm_plugin_s3_read_n)
    {
#ifdef DEBUG
    printf("s3_read_n(%d)  \n", size);
#endif
        opencbm_plugin_s3_read_n(fd_cbm, data, size);
        return;
    }
#ifdef DEBUG
    printf("+++ s3_read_n()  bytewise\n");
#endif
}

static int read_block(unsigned char tr, unsigned char se, unsigned char *block)
{
    unsigned char status;
    unsigned char buf[2];

#ifdef DEBUG
    printf("s3_read_block() :: track=%d, sector=%d \n", tr, se);
#endif

    buf[0] = tr;
    buf[1] = se;
    write_n(buf, 2);

    read_n(&status, 1);
    printf("s3_read_block() :: status=%d \n", status);

    read_n(block, 256);
    return status;
}

static int write_block(unsigned char tr, unsigned char se, const unsigned char *blk, int size, int read_status)
{
    unsigned char status;

#ifdef DEBUG
    printf("s3_write_block() :: track=%d, sector=%d \n", tr, se);
#endif
    write_n(&tr, 1);
    write_n(&se, 1);

    write_n(blk, size);
    read_n(&status, 1);

    return status;
}

static int open_disk(CBM_FILE fd, imgcopy_settings *settings,
                     const void *arg, int for_writing,
                     turbo_start start, imgcopy_message_cb message_cb)
{
    unsigned char d = (unsigned char)(ULONG_PTR)arg;

#ifdef DEBUG
    printf("s3_open_disk() \n");
#endif

    fd_cbm = fd;
    two_sided = settings->two_sided;

    opencbm_plugin_s3_read_n = cbm_get_plugin_function_address("opencbm_plugin_s3_read_n");
    opencbm_plugin_s3_write_n = cbm_get_plugin_function_address("opencbm_plugin_s3_write_n");

    switch(settings->drive_type)
    {
        case cbm_dt_cbm1541:
        case cbm_dt_cbm1570:
        case cbm_dt_cbm1571:
            cbm_upload(fd_cbm, d, 0x700, s3_drive_prog_1541, sizeof(s3_drive_prog_1541));
            break;

        case cbm_dt_cbm1581:
            cbm_upload(fd_cbm, d, 0x700, s3_drive_prog_1581, sizeof(s3_drive_prog_1581));
            break;

        case cbm_dt_cbm2040:
        case cbm_dt_cbm2031:
        case cbm_dt_cbm3040:
        case cbm_dt_cbm4040:
        case cbm_dt_cbm4031:
        case cbm_dt_cbm8050:
        case cbm_dt_cbm8250:
        case cbm_dt_sfd1001:
        case cbm_dt_unknown:
        default:
            // drive type not allowed
            return -1;
    }
    start(fd, d);

    printf("starting code...) \n");
    cbm_iec_release(fd, IEC_CLOCK);
    arch_usleep(300);

    return 0;
}

static void close_disk(void)
{
    unsigned char buf[2];

#ifdef DEBUG
    printf("s3_close_disk() \n");
#endif

    buf[0] = 0;
    buf[1] = 0;
    write_n(buf, 2);

    opencbm_plugin_s3_read_n = NULL;
    opencbm_plugin_s3_write_n = NULL;
}

static int send_track_map(imgcopy_settings *settings, unsigned char tr, const char *trackmap, unsigned char count)
{
    int i, size;
    unsigned char *data;

#ifdef DEBUG
    printf("s3_send_trackmap() \n");
#endif

    size = imgcopy_sector_count(settings, tr);
    data = malloc(size+2);

    data[0] = tr;
    data[1] = count;

    /* build track map */
    for(i = 0; i < size; i++)
    data[2+i] = !NEED_SECTOR(trackmap[i]);

    write_n(data, size+2);
    free(data);
    return 0;
}

static int read_gcr_block(unsigned char *se, unsigned char *gcrbuf)
{
    unsigned char s;

#ifdef DEBUG
    printf("s3_read_gcr_block() \n");
#endif

    read_n(&s, 1);
    *se = s;
    read_n(&s, 1);

    if(s) {
        return s;
    }

    read_n(gcrbuf, GCRBUFSIZE);
    return 0;
}

DECLARE_TRANSFER_FUNCS_EX(s3_transfer, 1, 1);
