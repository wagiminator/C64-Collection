/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2005 Michael Klein <michael.klein@puffin.lb.shuttle.de>
*/

#ifndef LIBTRANS_INT_H
#define LIBTRANS_INT_H

#include "opencbm.h"
#include "libtrans.h"

#include "arch.h"

#ifdef LIBOCT_STATE_DEBUG
# define DEBUG_STATEDEBUG
#endif
#include "statedebug.h"

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#define DBG_PROGNAME "LIBOPENCBMTRANSFER.DLL"

#include "debug.h"


#if 0

#define BLOCKSIZE 256
#define GCRBUFSIZE 326

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
    int  (*read_block)(unsigned char,unsigned char,char*);
    int  (*write_block)(unsigned char,unsigned char,const char*,int,int);
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

typedef struct {
    int (*upload)    (CBM_FILE fd, unsigned char drive);
    int (*init)      (CBM_FILE fd, unsigned char drive);
    int (*read1byte) (CBM_FILE fd, unsigned char *c1);
    int (*read2byte) (CBM_FILE fd, unsigned char *c1, unsigned char *c2);
    int (*readblock) (CBM_FILE fd, unsigned char *p, unsigned int length);
    int (*write1byte)(CBM_FILE fd, unsigned char c1);
    int (*write2byte)(CBM_FILE fd, unsigned char c1, unsigned char c2);
    int (*writeblock)(CBM_FILE fd, unsigned char *p, unsigned int length);
} transfer_funcs;

#define DECLARE_TRANSFER_FUNCS(_name_) \
    transfer_funcs libopencbmtransfer_ ## _name_ = \
    { \
        upload,     \
        init,       \
        read1byte,  \
        read2byte,  \
        readblock,  \
        write1byte, \
        write2byte, \
        writeblock  \
    }

extern transfer_funcs libopencbmtransfer_s1;
extern transfer_funcs libopencbmtransfer_s2;
extern transfer_funcs libopencbmtransfer_pp;

#endif /* #ifndef LIBTRANS_INT_H */
