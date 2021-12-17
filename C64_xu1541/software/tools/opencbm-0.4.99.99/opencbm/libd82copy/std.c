/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Modifications for openCBM Copyright 2011-2011 Thomas Winkler
*/

#include "opencbm.h"
#include "d82copy_int.h"

#include <stdio.h>
#include <stdlib.h>

static unsigned char drive = 0;
static CBM_FILE fd_cbm = (CBM_FILE) -1;

static int read_block(unsigned char tr, unsigned char se, unsigned char *block)
{
    char cmd[48];
    int rv = 1;

    sprintf(cmd, "U1:2 0 %d %d", tr, se);
    if(cbm_exec_command(fd_cbm, drive, cmd, 0) == 0) {
        rv = cbm_device_status(fd_cbm, drive, cmd, sizeof(cmd));
        if(rv == 0) {
            if(cbm_exec_command(fd_cbm, drive, "B-P2 0", 0) == 0) {
                if(cbm_talk(fd_cbm, drive, 2) == 0) {
                                                                        SETSTATEDEBUG(debugLibD82ByteCount=0);
                    rv = cbm_raw_read(fd_cbm, block, BLOCKSIZE) != BLOCKSIZE;
                                                                        SETSTATEDEBUG(debugLibD82ByteCount=-1);
                    cbm_untalk(fd_cbm);
                }
            }
        }
    }
    return rv;
}

static int write_block(unsigned char tr, unsigned char se, const unsigned char *blk, int size, int read_status)
{
    char cmd[48];
    int  rv = 1;

    if(cbm_exec_command(fd_cbm, drive, "B-P2 0", 0) == 0)
    {
        if(cbm_listen(fd_cbm, drive, 2) == 0)
        {
                                                                        SETSTATEDEBUG(debugLibD82ByteCount=0);
            rv = cbm_raw_write(fd_cbm, blk, size) != size;
                                                                        SETSTATEDEBUG(debugLibD82ByteCount=-1);
            cbm_unlisten(fd_cbm);
            if(rv == 0)
            {
                sprintf(cmd ,"U2:2 0 %d %d", tr, se);
                cbm_exec_command(fd_cbm, drive, cmd, 0);
                rv = cbm_device_status(fd_cbm, drive, cmd, sizeof(cmd));
            }
        }
    }
    return rv;
}

static int open_disk(CBM_FILE fd, d82copy_settings *settings,
                     const void *arg, int for_writing,
                     turbo_start start, d82copy_message_cb message_cb)
{
    char buf[48];
    int rv;

    if(settings->end_track > D82_TRACKS && !settings->two_sided)
    {
        message_cb(0, 
                   "standard transfer doesn't handle extended track images");
        return 99;
    }

    drive = (unsigned char)(ULONG_PTR)arg;

    fd_cbm = fd;

    cbm_open(fd_cbm, drive, 2, "#", 1);

    rv = cbm_device_status(fd_cbm, drive, buf, sizeof(buf));
    if(rv)
    {
        message_cb(0, "drive %02d: %s", drive, buf);
    }
    return rv;
}

static void close_disk(void)
{
    cbm_close(fd_cbm, drive, 2);
}

DECLARE_TRANSFER_FUNCS(std_transfer, 1, 0);
