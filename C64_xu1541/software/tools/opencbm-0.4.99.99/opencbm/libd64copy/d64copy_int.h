/*
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version
 *    2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
*/

#ifndef D64COPY_INT_H
#define D64COPY_INT_H

#include "opencbm.h"
#include "d64copy.h"
#include "gcr.h"

#include "arch.h"

#ifdef LIBD64COPY_DEBUG
# define DEBUG_STATEDEBUG
#endif
#include "statedebug.h"

/* standard .d64 track count */
#define STD_TRACKS   35
/* "standard" 40-track .d64 */
#define EXT_TRACKS   40
/* absolute limit. may not work with all drives */
#define TOT_TRACKS   42

/* .d71 track count */
#define D71_TRACKS   70

#define STD_BLOCKS  683
#define D71_BLOCKS  (STD_BLOCKS*2)

#define MAX_SECTORS  21

#define NEED_SECTOR(b) ((((b)==bs_error)||((b)==bs_must_copy))?1:0)

typedef int(*turbo_start)(CBM_FILE,unsigned char);

typedef struct {
    int  (*open_disk)(CBM_FILE,d64copy_settings*,const void*,int,
                      turbo_start,d64copy_message_cb);
    int  (*read_block)(unsigned char,unsigned char,unsigned char*);
    int  (*write_block)(unsigned char,unsigned char,const unsigned char*,int,int);
    void (*close_disk)(void);
    int  is_cbm_drive;
    int  needs_turbo;
    int  (*send_track_map)(unsigned char,const char*,unsigned char);
    int  (*read_gcr_block)(unsigned char*,unsigned char*);
} transfer_funcs;

#define DECLARE_TRANSFER_FUNCS(x,c,t) \
    transfer_funcs d64copy_ ## x = {open_disk, \
                        read_block, \
                        write_block, \
                        close_disk, \
                        c, \
                        t, \
                        NULL, \
                        NULL}

#define DECLARE_TRANSFER_FUNCS_EX(x,c,t) \
    transfer_funcs d64copy_ ## x = {open_disk, \
                        read_block, \
                        write_block, \
                        close_disk, \
                        c, \
                        t, \
                        send_track_map, \
                        read_gcr_block}

#endif
