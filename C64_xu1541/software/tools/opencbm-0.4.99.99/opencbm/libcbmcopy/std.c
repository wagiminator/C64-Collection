/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2011 Thomas Winkler
 *  Copyright 2011 Wolfgang Moser
 */

#include "opencbm.h"
#include "cbmcopy_int.h"

#include <stdlib.h>

#include "arch.h"

/*! \brief write a data block of a file to the OpenCBM backend

 \param HandleDevice  
   Pointer to a CBM_FILE which will contain the file handle of the OpenCBM backend

 \param Buffer
    Pointer to buffer which contains the data to be written to the OpenCBM backend

 \param Count
    The number of bytes to be transferred from the buffer to the OpenCBM backend,
    or 255, to transfer 254 bytes from the buffer and tell the turbo write routine
    that more blocks are following

 \param msg_cb
    Handle to cbmcopy's log message handler

 \return
    The number of bytes actually written, <0 on OpenCBM backend error.
*/
static int write_blk(CBM_FILE HandleDevice, const void *Buffer, unsigned char Count, cbmcopy_message_cb msg_cb)
{
    if( Count == 255 )
    {
        /* standard routines don't know how to use the "block is following" flag */
        Count--;
    }
    return cbm_raw_write(HandleDevice, Buffer, Count);
}

/*! \brief read a data block of a file from the OpenCBM backend

 \param HandleDevice  
   Pointer to a CBM_FILE which will contain the file handle of the OpenCBM backend

 \param Buffer
    Pointer to a buffer to store the bytes read from  the OpenCBM backend 

 \param Count
    The maximum size of the buffer

 \param msg_cb
    Handle to cbmcopy's log message handler

 \return
    The number of bytes actually read (0 to 254), <0 on OpenCBM backend error.
*/
static int read_blk(CBM_FILE HandleDevice, void *Buffer, size_t Count, cbmcopy_message_cb msg_cb)
{
    int rv;

    /* non-turbo methods don't send a header byte specifying the block length */
    rv = cbm_raw_read(HandleDevice, Buffer, Count);
    if( rv == 254 )
    {
        /* when a full block was read, return that more blocks are following (255) */
        /* even if this is wrong. If so, a count of 0 is returned in the next call */
        rv++;
    }
    return rv;
}

static int check_error(CBM_FILE fd, int write)
{
    /* No explicit error check with  cbm_device_status  can be done since */
    /* the device is still listening or talking. The former  write_byte   */
    /* and  read_byte  function return the library's (error) return codes */
    return 0;
}

static int upload_turbo(CBM_FILE fd, unsigned char drive,
                        enum cbm_device_type_e drive_type, int write)
{
    if(write)
    {
        cbm_listen(fd, drive, SA_WRITE);
    }
    else
    {
        cbm_talk(fd, drive, SA_READ);
    }
    return 0;
}


static int start_turbo(CBM_FILE fd, int write)
{
    /* no special handshake signalisation needed */
    return 0;
}


static void exit_turbo(CBM_FILE fd, int write)
{
    if(write)
    {
        cbm_unlisten(fd);
    }
    else
    {
        cbm_untalk(fd);
    }
}

DECLARE_TRANSFER_FUNCS(std_transfer);
