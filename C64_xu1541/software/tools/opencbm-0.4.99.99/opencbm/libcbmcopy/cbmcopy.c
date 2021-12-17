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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cbmcopy_int.h"

#include "arch.h"

static const unsigned char turboread1541[] = {
#include "turboread1541.inc"
};

static const unsigned char turboread1571[] = {
#include "turboread1571.inc"
};

static const unsigned char turboread1581[] = {
#include "turboread1581.inc"
};

static const unsigned char turbowrite1541[] = {
#include "turbowrite1541.inc"
};

static const unsigned char turbowrite1571[] = {
#include "turbowrite1571.inc"
};

static const unsigned char turbowrite1581[] = {
#include "turbowrite1581.inc"
};

extern transfer_funcs cbmcopy_s1_transfer,
                      cbmcopy_s2_transfer,
                      cbmcopy_pp_transfer,
                      cbmcopy_std_transfer;

static struct _transfers
{
    const transfer_funcs *trf;
    const char *name, *abbrev;
}
transfers[] =
{
    { &cbmcopy_s1_transfer, "auto", "a%1" },
    { &cbmcopy_s1_transfer, "serial1", "s1" },
    { &cbmcopy_s2_transfer, "serial2", "s2" },
    { &cbmcopy_pp_transfer, "parallel", "p%" },
    { &cbmcopy_std_transfer, "original", "o%" },
    { NULL, NULL, NULL }
};

static int check_drive_type(CBM_FILE fd, unsigned char drive,
                            cbmcopy_settings *settings,
                            cbmcopy_message_cb msg_cb)
{
    const char *type_str;

    if( settings->drive_type == cbm_dt_unknown )
    {
        if(cbm_identify( fd, drive, &settings->drive_type, &type_str ))
        {
            msg_cb( sev_warning, "could not identify drive, using no turbo" );
        }
        else
        {
            msg_cb( sev_info, "identified a %s drive", type_str );
        }
    }
    return 0;
}


static int send_turbo(CBM_FILE fd, unsigned char drive, int write,
                      const cbmcopy_settings *settings,
                      const unsigned char *turbo, size_t turbo_size,
                      const unsigned char *start_cmd, size_t cmd_len,
                      cbmcopy_message_cb msg_cb)
{
    const transfer_funcs *trf;

    trf = transfers[settings->transfer_mode].trf;
    /*
     * minimum requirement for any transfer mode for a given device
     * is that a transfer mode shutdown handler does exist
     */
    if(trf->exit_turbo != NULL)
    {
        if(turbo_size)
        {
            cbm_upload( fd, drive, 0x500, turbo, turbo_size );
            msg_cb( sev_debug, "uploading %d bytes turbo code", turbo_size );
            if(trf->upload_turbo(fd, drive, settings->drive_type, write) == 0)
            {
                cbm_exec_command( fd, drive, start_cmd, cmd_len );
                msg_cb( sev_debug, "initializing transfer code" );
                if(trf->start_turbo(fd, write) == 0)
                {
                    msg_cb( sev_debug, "done" );
                    return 0;
                }
                else
                {
                    msg_cb( sev_fatal, "could not start turbo" );
                }
            }
        }
        else
        {
            msg_cb( sev_debug, "no turbo code upload is required", turbo_size );
            /* nevertheless the transfer must be initialised */
            trf->upload_turbo(fd, drive, settings->drive_type, write);
            trf->start_turbo(fd, write);
            return 0;
        }
    }
    else
    {
        msg_cb( sev_fatal, "Unsupported transfer mode for this device" );
    }
    return -1;
}


static int cbmcopy_read(CBM_FILE fd,
                        cbmcopy_settings *settings,
                        unsigned char drive,
                        int track, int sector,
                        const char *cbmname,
                        int cbmname_len,
                        unsigned char **filedata,
                        size_t *filedata_size,
                        cbmcopy_message_cb msg_cb,
                        cbmcopy_status_cb status_cb)
{
    int rv;
    int i;
    int turbo_size;
    int error;
    unsigned char buf[48];
    const unsigned char *turbo;
    const transfer_funcs *trf;
    int blocks_read;

    *filedata = NULL;
    *filedata_size = 0;

    msg_cb( sev_debug, "using transfer mode '%s'",
            transfers[settings->transfer_mode].name);
    trf = transfers[settings->transfer_mode].trf;

    if(check_drive_type( fd, drive, settings, msg_cb ))
    {
        return -1;
    }

    switch(settings->drive_type)
    {
        case cbm_dt_cbm1541:
            turbo = turboread1541;
            turbo_size = sizeof(turboread1541);
            break;
        case cbm_dt_cbm1570:
        case cbm_dt_cbm1571:
            cbm_exec_command( fd, drive, "U0>M1", 0 );
            turbo = turboread1571;
            turbo_size = sizeof(turboread1571);
            break;
        case cbm_dt_cbm1581:
            turbo = turboread1581;
            turbo_size = sizeof(turboread1581);
            break;
        default: /* unreachable */
            msg_cb( sev_warning, "*** unknown drive type" );
            /* fall through */
        case cbm_dt_cbm4040:
        case cbm_dt_cbm8050:
        case cbm_dt_cbm8250:
        case cbm_dt_sfd1001:
            turbo = NULL;
            turbo_size = 0;
            break;
    }

    if(transfers[settings->transfer_mode].abbrev[0] == 'o')
    {
        /* if "original" transfer mode - no drive code can be used */
        turbo = NULL;
        turbo_size = 0;
    }

    if(cbmname)
    {
        /* start by file name */
        track = 0;
        sector = 0;
        cbm_open( fd, drive, SA_READ, NULL, 0 );
        if(cbmname_len == 0) cbmname_len = strlen( cbmname );
        cbm_raw_write( fd, cbmname, cbmname_len );
        cbm_unlisten( fd );
    }
    else
    {
        /* start by track/sector */
        cbm_open( fd, drive, SA_READ, "#", 1 );
    }
    rv = cbm_device_status( fd, drive, (char*)buf, sizeof(buf) );

    if(rv)
    {
        msg_cb( sev_fatal, "could not open file for reading: %s", buf );
        return rv;
    }

    blocks_read = 0;
    error = 0;

    if(track)
    {
        msg_cb( sev_debug, "start read at %d/%d", track, sector );
    }
    sprintf( (char*)buf, "U4:%c%c", (unsigned char)track, (unsigned char)sector );

    SETSTATEDEBUG((void)0);    // pre send_turbo condition
    if(send_turbo(fd, drive, 0, settings,
                  turbo, turbo_size, buf, 5, msg_cb) == 0)
    {
        msg_cb( sev_debug, "start of copy" );
        status_cb( blocks_read );

        /*
         * FIXME: Find and eliminate the real protocol races to
         *        eliminate the rare hangups with the 1581 based
         *        turbo routines (bugs suspected in 6502 code)
         *
         * Hotfix proposion for the 1581 protocols:
         *    add a little delay after the turbo start
         */
        arch_usleep(1000);

        SETSTATEDEBUG(DebugBlockCount=0);   // turbo sent condition

        while( (error = trf->check_error(fd, 0)) == 0 )
        {
            SETSTATEDEBUG((void)0); // after check_error condition
            arch_usleep(1000);      // fix for Tim's environment

            SETSTATEDEBUG(DebugBlockCount++);    // preset condition

            /* @SRT: FIXME! the next statement is dangerous: If there 
             * is no memory block large enough for reallocating, the
             * memory block is not freed, but realloc() returns NULL,
             * thus, we have a memory leak.
             */
             /* reserve additional memory to hold to up an additional full block */
            *filedata = realloc(*filedata, *filedata_size + 254);
            SETSTATEDEBUG((void)0);    // after check_error condition
            if(*filedata)
            {
                /* read block, let the block reader also handle the initial length byte */
                i = trf->read_blk( fd, (*filedata) + blocks_read * 254, 254, msg_cb);
                msg_cb( sev_debug, "number of bytes read for block %d: %d", blocks_read, i );

                SETSTATEDEBUG((void)0);    // afterread condition

                /*
                 * FIXME: Find and eliminate the real protocol races to
                 *        eliminate the rare hangups with the 1581 based
                 *        turbo routines (bugs suspected in 6502 code)
                 *
                 * Hotfix proposion for the 1581 protocols:
                 *    add a little delay at the end of the loop
                 *       "hmmmm, if we know that the drive is busy
                 *        now, shouldn't we wait for it then?"
                 *    add a little delay after the turbo start
                 */
                arch_usleep(1000);

                if( i < 255)
                {
                    if( i >= 0 )
                    {
                        /* in case of original transfers, there is no extra length byte transfer,    */
                        /* whenever 254 bytes are read from a block a count value of 255 is returned */
                        /* and if this was the last block, 0 bytes are read with the next block call */
                        *filedata_size += i;
                    }
                    else
                    {
                        rv = -1;
                    }
                    break;
                }
                else
                {
                    /* more blocks are following, a full block was transferred */
                    *filedata_size += 254;
                }

                SETSTATEDEBUG((void)0);    // afterread condition
                status_cb( ++blocks_read );
            }
            else
            {
                SETSTATEDEBUG((void)0);    // afterread condition
                /* FIXME */
            }
        }
        msg_cb( sev_debug, "done" );
        SETSTATEDEBUG(DebugBlockCount=-1);   // turbo sent condition
        trf->exit_turbo( fd, 0 );
        SETSTATEDEBUG((void)0);    // turbo exited condition

        /*
         * FIXME: Find and eliminate the real protocol race to
         *        eliminate a rare "99, DRIVER ERROR,00,00" with the
         *        1571 disk drive and the serial-2 read turbo
         *
         * Hotfix proposion for the 1571 protocol:
         *    add a listen/unlisten sequence, just doing a decent
         *    arch_usleep() and waiting some time did not work
         */
        cbm_listen(fd, drive, SA_READ);
        cbm_unlisten(fd);
        SETSTATEDEBUG((void)0);    // turbo exited, waited for drive

        rv = cbm_device_status( fd, drive, (char*)buf, sizeof(buf) );
        if(rv)
        {
            msg_cb( sev_warning, "file copy ended with error status: %s", buf );
        }
    }

    cbm_close( fd, drive, SA_READ );
    return rv;
}


char *cbmcopy_get_transfer_modes()
{
    const struct _transfers *t;
    int size;
    char *buf;
    char *dst;

    size = 1; /* for terminating '\0' */
    for(t = transfers; t->name; t++)
    {
        size += (strlen(t->name) + 1);
    }

    buf = malloc(size);

    if(buf)
    {
        dst = buf;
        for(t = transfers; t->name; t++)
        {
            strcpy(dst, t->name);
            dst += (strlen(t->name) + 1);
        }
        *dst = '\0';
    }

    return buf;
}


int cbmcopy_get_transfer_mode_index(const char *name)
{
    const struct _transfers *t;
    int i;
    int abbrev_len;
    int tm_len;

    if(NULL == name)
    {
        /* default transfer mode */
        return 0;
    }

    tm_len = strlen(name);
    for(i = 0, t = transfers; t->name; i++, t++)
    {
        if(strcmp(name, t->name) == 0)
        {
            /* full match */
            return i;
        }
        if(t->abbrev[strlen(t->abbrev)-1] == '%')
        {
            abbrev_len = strlen(t->abbrev) - 1;
            if(abbrev_len <= tm_len && strncmp(t->name, name, tm_len) == 0)
            {
                return i;
            }
        }
        else
        {
            if(strcmp(name, t->abbrev) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

int cbmcopy_check_auto_transfer_mode(CBM_FILE cbm_fd, int auto_transfermode, int drive)
{
    /* We assume auto is the first transfer mode */
    assert(strcmp(transfers[0].name, "auto") == 0);

    if (auto_transfermode == 0)
    {
        enum cbm_cable_type_e cable_type;
        enum cbm_device_type_e device_type;
        unsigned char testdrive;

        /*
         * Test the cable
         */

        if (cbm_identify_xp1541(cbm_fd, (unsigned char)drive, NULL, &cable_type) == 0)
        {
            if (cable_type == cbm_ct_xp1541)
            {
                /*
                 * We have a parallel cable, use that
                 */
                return cbmcopy_get_transfer_mode_index("parallel");
            }
        }

        /*
         * lookup drivetyp, if IEEE-488 drive, use original
         */

        if (cbm_identify(cbm_fd, (unsigned char)drive, &device_type, NULL) == 0)
        {
            switch(device_type)
            {
                case cbm_dt_cbm4040:
                case cbm_dt_cbm8050:
                case cbm_dt_cbm8250:
                case cbm_dt_sfd1001:
                    /*
                     * We are using an IEEE-488 drive, use original transfer mode
                     */
                    return cbmcopy_get_transfer_mode_index("original");

                default:
                    /*
                     * We are using an IEC drive, everything is o.k.
                     */
                    break;
            }
        }

        /*
         * We do not have a parallel cable. Check if we are the only drive
         * on the bus, so we can use serial2, at least.
         */

        for (testdrive = 4; testdrive < 31; ++testdrive)
        {
            /* of course, the drive to be transfered to is present! */
            if (testdrive == drive)
                continue;

            if (cbm_identify(cbm_fd, testdrive, &device_type, NULL) == 0)
            {
                /*
                 * My bad, there is another drive -> only use serial1
                 */
                return cbmcopy_get_transfer_mode_index("serial1");
            }
        }

        /*
         * If we reached here with transfermode 0, we are the only
         * drive, thus, use serial2.
         */
        return cbmcopy_get_transfer_mode_index("serial2");
    }

    return auto_transfermode;
}

cbmcopy_settings *cbmcopy_get_default_settings(void)
{
    cbmcopy_settings *settings;

    settings = malloc(sizeof(cbmcopy_settings));

    if(NULL != settings)
    {
        settings->drive_type    = cbm_dt_unknown; /* auto detect later on */
        settings->transfer_mode = 0;
    }
    return settings;
}



int cbmcopy_write_file(CBM_FILE fd,
                       cbmcopy_settings *settings,
                       int drivei,
                       const char *cbmname,
                       int cbmname_len,
                       const unsigned char *filedata,
                       int filedata_size,
                       cbmcopy_message_cb msg_cb,
                       cbmcopy_status_cb status_cb)
{
    int rv;
    int i;
    int turbo_size;
    unsigned char drive = (unsigned char) drivei; //! \todo Find better solution
    int error;
    unsigned char buf[48];
    const unsigned char *turbo;
    const transfer_funcs *trf;
    int blocks_written;

    msg_cb( sev_debug, "using transfer mode `%s'",
            transfers[settings->transfer_mode].name);
    trf = transfers[settings->transfer_mode].trf;

    if(check_drive_type( fd, drive, settings, msg_cb ))
    {
        return -1;
    }

    switch(settings->drive_type)
    {
        case cbm_dt_cbm1541:
            turbo = turbowrite1541;
            turbo_size = sizeof(turbowrite1541);
            break;
        case cbm_dt_cbm1570:
        case cbm_dt_cbm1571:
            cbm_exec_command( fd, drive, "U0>M1", 0 );
            turbo = turbowrite1571;
            turbo_size = sizeof(turbowrite1571);
            break;
        case cbm_dt_cbm1581:
            turbo = turbowrite1581;
            turbo_size = sizeof(turbowrite1581);
            break;
        default: /* unreachable */
            msg_cb( sev_warning, "*** unknown drive type" );
            /* fall through */
        case cbm_dt_cbm4040:
        case cbm_dt_cbm8050:
        case cbm_dt_cbm8250:
        case cbm_dt_sfd1001:
            turbo = NULL;
            turbo_size = 0;
            break;
    }

    if(transfers[settings->transfer_mode].abbrev[0] == 'o')
    {
        /* if "original" transfer mode - no drive code can be used */
        turbo = NULL;
        turbo_size = 0;
    }

    cbm_open( fd, drive, SA_WRITE, NULL, 0 );
    if(cbmname_len == 0) cbmname_len = strlen( cbmname );
    cbm_raw_write( fd, cbmname, cbmname_len );
    cbm_unlisten( fd );
    rv = cbm_device_status( fd, drive, buf, sizeof(buf) );

    if(rv)
    {
        msg_cb( sev_fatal, "could not open file for writing: %s", buf );
        return rv;
    }

    blocks_written = 0;
    error = 0;

    SETSTATEDEBUG((void)0);    // pre send_turbo condition
    if(send_turbo(fd, drive, 1, settings,
                  turbo, turbo_size, (unsigned char*)"U4:", 3, msg_cb) == 0)
    {
        msg_cb( sev_debug, "start of copy" );
        status_cb( blocks_written );

        /*
         * FIXME: Find and eliminate the real protocol races to
         *        eliminate the rare hangups with the 1581 based
         *        turbo routines (bugs suspected in 6502 code)
         *
         * Hotfix proposion for the 1581 protocols:
         *    add a little delay after the turbo start
         */
        arch_usleep(1000);

        SETSTATEDEBUG(DebugBlockCount=0);   // turbo sent condition

        while( filedata_size > 0 )
        {
            /* if more blocks are following (more than 254 bytes) set the count value to 255 */
            i = filedata_size <= 254 ? filedata_size : 255;

            SETSTATEDEBUG(DebugBlockCount++);

            /* write block, let the block writer also handle the initial length byte */
            i = trf->write_blk( fd, filedata, (unsigned char)i, msg_cb );
            if ( (i != 254) && (i != filedata_size) )
            {
                /* normally blocks of 254 bytes are written, even if the count byte is at value 255 */
                /* for the last block of a file, only filedata_size bytes need to be transferred    */
                rv = -1;
                break;
            }

            SETSTATEDEBUG((void)0);
            if ( trf->check_error( fd, 1 ) != 0 )
            {
                rv = -1;
                break;
            }

            /*
             * FIXME: Find and eliminate the real protocol races to
             *        eliminate the rare hangups with the 1581 based
             *        turbo routines (bugs suspected in 6502 code)
             *
             * Hotfix proposion for the 1581 protocols:
             *    add a little delay at the end of the loop
             *       "hmmmm, if we know that the drive is busy
             *        now, shouldn't we wait for it then?"
             *    add a little delay after the turbo start
             */
            arch_usleep(1000);

            status_cb( ++blocks_written );
            SETSTATEDEBUG((void)0);

            filedata_size -= 254;
            filedata += 254;
        }
        msg_cb( sev_debug, "done" );

        SETSTATEDEBUG((void)0);
        trf->exit_turbo( fd, 1 );
        SETSTATEDEBUG((void)0);

        rv = cbm_device_status( fd, drive, (char*)buf, sizeof(buf) );
        if(rv)
        {
            msg_cb( sev_warning, "file copy ended with error status: %s", buf );
        }
    }
    cbm_close( fd, drive, SA_WRITE );
    return rv;
}


/* just a wrapper */
int cbmcopy_read_file_ts(CBM_FILE fd,
                         cbmcopy_settings *settings,
                         int drive,
                         int track, int sector,
                         unsigned char **filedata,
                         size_t *filedata_size,
                         cbmcopy_message_cb msg_cb,
                         cbmcopy_status_cb status_cb)
{
    return cbmcopy_read(fd, settings, (unsigned char) drive,
                        track, sector,
                        NULL, 0,
                        filedata, filedata_size,
                        msg_cb, status_cb);
}


/* just a wrapper */
int cbmcopy_read_file(CBM_FILE fd,
                      cbmcopy_settings *settings,
                      int drive,
                      const char *cbmname,
                      int cbmname_len,
                      unsigned char **filedata,
                      size_t *filedata_size,
                      cbmcopy_message_cb msg_cb,
                      cbmcopy_status_cb status_cb)
{
    return cbmcopy_read(fd, settings, (unsigned char) drive,
                        0, 0,
                        cbmname, cbmname_len,
                        filedata, filedata_size,
                        msg_cb, status_cb);
}

/*! \brief write a data block of a file with a sequence of byte transfers

 \param HandleDevice  
   Pointer to a CBM_FILE which will contain the file handle of the OpenCBM backend

 \param wb_func
    Callback to the write_byte function 

 \param data
    Pointer to buffer which contains the data to be written to the OpenCBM backend

 \param size
    The number of bytes to be transferred from the buffer to the OpenCBM backend,
    or 255, to transfer 254 bytes from the buffer and tell the turbo write routine
    that more blocks are following

 \param msg_cb
    Handle to cbmcopy's log message handler

 \return
    The number of bytes actually written, 0 on OpenCBM backend error.
    If there is a fatal error, returns -1.
*/
int write_block_generic(CBM_FILE HandleDevice, const void *data, unsigned char size, write_byte_t wb_func, cbmcopy_message_cb msg_cb)
{
    int rv = 0;
    const unsigned char *pbuffer = data;

    SETSTATEDEBUG((void)0);
#ifdef LIBCBMCOPY_DEBUG
    msg_cb( sev_debug, "send byte count: %d", size );
#endif
    if ( (pbuffer == NULL) || (wb_func( HandleDevice, size ) != 0) )
    {
        return -1;
    }

    SETSTATEDEBUG(DebugByteCount=0);
    if( size == 0xff )
    {
        size--;
    }

#ifdef LIBCBMCOPY_DEBUG
    msg_cb( sev_debug, "send block data" );
#endif 
    while( size>0 )
    {
        SETSTATEDEBUG(DebugByteCount++);
        if ( wb_func( HandleDevice, *(pbuffer++) ) != 0 )
        {
            break;
        }
        rv++;
        size--;
    }

    /* (drive is busy now) */
    SETSTATEDEBUG(DebugByteCount=-1);

    return rv;
}

/*! \brief read a data block of a file with a sequence of byte transfers

 \param HandleDevice  
   Pointer to a CBM_FILE which will contain the file handle of the OpenCBM backend

 \param data
    Pointer to a buffer to store the read bytes within

 \param size
    The maximum size of the buffer

 \param rb_func
    Callback to the read_byte function 

 \param msg_cb
    Handle to cbmcopy's log message handler

 \return
    The number of bytes actually read (1 to 254), 0 on OpenCBM backend error,
    255, if more blocks are following within this file chain.
    If there is a fatal error, returns -1.
*/
int read_block_generic(CBM_FILE HandleDevice, void *data, size_t size, read_byte_t rb_func, cbmcopy_message_cb msg_cb)
{
    int rv = 0;
    unsigned char c;
    unsigned char *pbuffer = data;

    SETSTATEDEBUG((void)0);
    /* get the number of bytes that need to be transferred for this block */
    c = rb_func( HandleDevice );
    SETSTATEDEBUG((void)0);
#ifdef LIBCBMCOPY_DEBUG
    msg_cb( sev_debug, "received byte count: %d", c );
#endif 

    rv = c;
    if( c == 0xff )
    {
        /* this is a flag that further bytes are following, so get a full block of 254 bytes */
        c--;
    }

    if( (pbuffer == NULL) || (c > size) )
    {
        /* If the block size if greater than the available buffer, return with
         * a fatal error since the turbo handlers always need to transfer a
         * complete block. If there is no buffer allocated at all, fail also.
         */
        return -1;
    }

    SETSTATEDEBUG(DebugByteCount=0);
#ifdef LIBCBMCOPY_DEBUG
    msg_cb( sev_debug, "receive block data (%d)", c );
#endif 
    while( c>0 )
    {
        SETSTATEDEBUG(DebugByteCount++);
        *(pbuffer++) = rb_func( HandleDevice );
        c--;
    }

    /* (drive is busy now) */
    SETSTATEDEBUG(DebugByteCount=-1);

    return rv;
}
