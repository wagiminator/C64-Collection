/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2011 Thomas Winkler
 *  Copyright 2011 Wolfgang Moser
*/

#ifndef CBMCOPY_INT_H
#define CBMCOPY_INT_H

#include "opencbm.h"
#include "cbmcopy.h"

#ifdef LIBCBMCOPY_DEBUG
# define DEBUG_STATEDEBUG
#endif
#include "statedebug.h"

#define SA_READ     0
#define SA_WRITE    1

/* mandatory functions for the transfer function modules */
typedef struct {
    int  (*write_blk)(CBM_FILE,const void *,unsigned char,cbmcopy_message_cb);
    int  (*read_blk)(CBM_FILE,void *,size_t,cbmcopy_message_cb);
    int  (*check_error)(CBM_FILE,int);
    int  (*upload_turbo)(CBM_FILE, unsigned char, enum cbm_device_type_e,int);
    int  (*start_turbo)(CBM_FILE,int);
    void (*exit_turbo)(CBM_FILE,int);
} transfer_funcs;

/* callbacks from generic block handlers to the transfer function modules */
typedef int           (*write_byte_t)(CBM_FILE,unsigned char);
typedef unsigned char (*read_byte_t)(CBM_FILE);

/* generic block handlers to the transfer data with single byte transfers */
int write_block_generic(CBM_FILE,const void *,unsigned char,write_byte_t,cbmcopy_message_cb);
int read_block_generic(CBM_FILE,void *,size_t,read_byte_t,cbmcopy_message_cb);

#define DECLARE_TRANSFER_FUNCS(x) \
    transfer_funcs cbmcopy_ ## x = {write_blk, read_blk, check_error, \
                        upload_turbo, start_turbo, exit_turbo}

#endif
