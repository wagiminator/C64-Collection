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
#include "cbmcopy_int.h"

#include <stdlib.h>

#include "opencbm-plugin.h"

static opencbm_plugin_s1_read_n_t * opencbm_plugin_s1_read_n = NULL;

static opencbm_plugin_s1_write_n_t * opencbm_plugin_s1_write_n = NULL;

static const unsigned char s1r15x1[] = {
#include "s1r.inc"
};

static const unsigned char s1w15x1[] = {
#include "s1w.inc"
};

static const unsigned char s1r1581[] = {
#include "s1r-1581.inc"
};

static const unsigned char s1w1581[] = {
#include "s1w-1581.inc"
};

static struct drive_prog
{
    const unsigned char *prog;
    size_t size;
} drive_progs[] =
{
    { s1r15x1, sizeof(s1r15x1) },
    { s1w15x1, sizeof(s1w15x1) },
    { s1r1581, sizeof(s1r1581) },
    { s1w1581, sizeof(s1w1581) }
};

static int write_byte(CBM_FILE,unsigned char);
static unsigned char read_byte(CBM_FILE);

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
    The number of bytes actually written, 0 on OpenCBM backend error.
    If there is a fatal error, returns -1.
*/
static int write_blk(CBM_FILE HandleDevice, const void *Buffer, unsigned char Count, cbmcopy_message_cb msg_cb)
{
    if (opencbm_plugin_s1_write_n)
    {
#ifdef LIBCBMCOPY_DEBUG
        msg_cb( sev_debug, "send byte count: %d", Count );
#endif
        SETSTATEDEBUG((void)0);
        if ( (Buffer == NULL) || (opencbm_plugin_s1_write_n( HandleDevice, &Count, 1 ) != 1) )
        {
            return -1;
        }
        SETSTATEDEBUG((void)0);

        if( Count == 0xff )
        {
            Count--;
        }

#ifdef LIBCBMCOPY_DEBUG
        msg_cb( sev_debug, "send block data" );
#endif 
        return opencbm_plugin_s1_write_n( HandleDevice, Buffer, Count );
    }
    else
    {
        SETSTATEDEBUG((void)0);
        /* call generic byte oriented block handler */
        return write_block_generic(HandleDevice, Buffer, Count, &write_byte, msg_cb);
    }
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
    The number of bytes actually read (1 to 254), 0 on OpenCBM backend error,
    255, if more blocks are following within this file chain.
    If there is a fatal error, returns -1.
*/
static int read_blk(CBM_FILE HandleDevice, void *Buffer, size_t Count, cbmcopy_message_cb msg_cb)
{
    unsigned char c;
    int rv = 0;

    if (opencbm_plugin_s1_read_n)
    {
        SETSTATEDEBUG((void)0);
        /* get the number of bytes that need to be transferred for this block */
        if( opencbm_plugin_s1_read_n(HandleDevice, &c, 1) != 1 )
        {
            return -1;
        }
#ifdef LIBCBMCOPY_DEBUG
        msg_cb( sev_debug, "received byte count: %d", c );
#endif 
        SETSTATEDEBUG((void)0);
        rv = c;

        if( c == 0xff )
        {
            /* this is a flag that further bytes are following, so get a full block of 254 bytes */
            c--;
        }
        if( (Buffer == NULL) || (c > Count) )
        {
            /* If the block size if greater than the available buffer, return with
             * a fatal error since the turbo handlers always need to transfer a
             * complete block. If there is no buffer allocated at all, fail also.
             */
            return -1;
        }
#ifdef LIBCBMCOPY_DEBUG
        msg_cb( sev_debug, "receive block data (%d)", c );
#endif 
        return (opencbm_plugin_s1_read_n(HandleDevice, Buffer, c) != c )? -1 : rv;
        /* (drive is busy now) */
    }
    else
    {
        /* call generic byte oriented block handler */
    return read_block_generic(HandleDevice, Buffer, Count, &read_byte, msg_cb);
    }
}

static int write_byte(CBM_FILE fd, unsigned char c)
{
    int b, i;
                                                                        SETSTATEDEBUG((void)0);
    for(i=7; i>=0; i--) {
        b=(c >> i) & 1;
                                                                        SETSTATEDEBUG(DebugBitCount=i);
        if(b) cbm_iec_set(fd, IEC_DATA); else cbm_iec_release(fd, IEC_DATA);
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_release(fd, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
        while(!cbm_iec_get(fd, IEC_CLOCK));
#else
        cbm_iec_wait(fd, IEC_CLOCK, 1);
#endif
                                                                        SETSTATEDEBUG((void)0);
        if(b) cbm_iec_release(fd, IEC_DATA); else cbm_iec_set(fd, IEC_DATA);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
        while(cbm_iec_get(fd, IEC_CLOCK));
#else
        cbm_iec_wait(fd, IEC_CLOCK, 0);
#endif
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_release(fd, IEC_DATA);
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_set(fd, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
        while(!cbm_iec_get(fd, IEC_DATA));
#else
        cbm_iec_wait(fd, IEC_DATA, 1);
#endif
    }
                                                                        SETSTATEDEBUG(DebugBitCount=-1);
    return 0;
}

static unsigned char read_byte(CBM_FILE fd)
{
    int b=0, i;
    unsigned char c;
                                                                        SETSTATEDEBUG((void)0);
    c = 0;
    for(i=7; i>=0; i--) {
                                                                        SETSTATEDEBUG(DebugBitCount=i);
#ifndef USE_CBM_IEC_WAIT
        while(cbm_iec_get(fd, IEC_DATA));
#else        
        cbm_iec_wait(fd, IEC_DATA, 0);
#endif
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_release(fd, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);
        b = cbm_iec_get(fd, IEC_CLOCK);
        c = (c >> 1) | (b ? 0x80 : 0);
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_set(fd, IEC_DATA);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
        while(b == cbm_iec_get(fd, IEC_CLOCK));
#else        
        cbm_iec_wait(fd, IEC_CLOCK, !b);
#endif
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_release(fd, IEC_DATA);
                                                                        SETSTATEDEBUG((void)0);
#ifndef USE_CBM_IEC_WAIT
        while(!cbm_iec_get(fd, IEC_DATA));
#else        
        cbm_iec_wait(fd, IEC_DATA, 1);
#endif
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_set(fd, IEC_CLOCK);
    }
                                                                        SETSTATEDEBUG(DebugBitCount=-1);
    return c;
}

static int check_error(CBM_FILE fd, int write)
{
    int error;

                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_release(fd, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);
    cbm_iec_wait(fd, IEC_DATA, 0);
                                                                        SETSTATEDEBUG((void)0);
    error = cbm_iec_get(fd, IEC_CLOCK) == 0;

    if(!error)
    {
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_set(fd, IEC_DATA);
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_wait(fd, IEC_CLOCK, 0);
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_release(fd, IEC_DATA);
        if(write)
        {
                                                                        SETSTATEDEBUG((void)0);
            cbm_iec_set(fd, IEC_CLOCK);
        }
        else
        {
                                                                        SETSTATEDEBUG((void)0);
            cbm_iec_wait(fd, IEC_DATA, 1);
                                                                        SETSTATEDEBUG((void)0);
            cbm_iec_set(fd, IEC_CLOCK);
        }
                                                                        SETSTATEDEBUG((void)0);
    }
                                                                        SETSTATEDEBUG((void)0);
    return error;
}

static int upload_turbo(CBM_FILE fd, unsigned char drive,
                        enum cbm_device_type_e drive_type, int write)
{
    const struct drive_prog *p;
    int dt;

    opencbm_plugin_s1_read_n = cbm_get_plugin_function_address("opencbm_plugin_s1_read_n");
    opencbm_plugin_s1_write_n = cbm_get_plugin_function_address("opencbm_plugin_s1_write_n");

    dt = (drive_type == cbm_dt_cbm1581);
    p = &drive_progs[dt * 2 + (write != 0)];

                                                                        SETSTATEDEBUG((void)0);
    cbm_upload(fd, drive, 0x680, p->prog, p->size);
                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

static int start_turbo(CBM_FILE fd, int write)
{
    if(write)
    {
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_wait(fd, IEC_DATA, 1);
    }
    else
    {
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_release(fd, IEC_CLOCK);
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_wait(fd, IEC_DATA, 1);
                                                                        SETSTATEDEBUG((void)0);
        cbm_iec_set(fd, IEC_CLOCK);
    }
                                                                        SETSTATEDEBUG((void)0);
    return 0;
}

static void exit_turbo(CBM_FILE fd, int write)
{
                                                                        SETSTATEDEBUG((void)0);
//    cbm_iec_wait(fd, IEC_DATA, 0);
                                                                        SETSTATEDEBUG((void)0);

    opencbm_plugin_s1_read_n = NULL;

    opencbm_plugin_s1_write_n = NULL;
}

DECLARE_TRANSFER_FUNCS(s1_transfer);
