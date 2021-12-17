/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright (C) 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright (C) 2007      Wolfgang Moser (http://d81.de)
*/

#include "opencbm.h"
#include "d64copy.h"
#include "d64copy_int.h"

#include "arch.h"
#include "libmisc.h"

#ifdef LIBD64COPY_DEBUG
# define DEBUG_STATEDEBUG
#endif
#include "statedebug.h"

#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern transfer_funcs d64copy_pp_transfer,
                      d64copy_s1_transfer,
                      d64copy_s2_transfer;

const unsigned char weakstart[] = {
    	0x55, 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01, 0x00, 0xff, 0x7f, 0xbf, 0xdf, 0xef, 0xf7 };
const unsigned char syncmark[] = {
    	0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99, 0x9a, 0xff, 0x59, 0x99, 0x99, 0x99, 0x99, 0x99, 0x99 };

/* setable via command line */
static d64copy_severity_e verbosity = sev_warning;

/* other globals */
static CBM_FILE fd_cbm;


static int is_cbm(char *name)
{
    return((strcmp(name, "8" ) == 0) || (strcmp(name, "9" ) == 0) ||
           (strcmp(name, "10") == 0) || (strcmp(name, "11") == 0) );
}


static void help()
{
    printf(
"Usage: weaktest [OPTION]... [TARGET]\n"
"\n"
"Options:\n"
"  -h, --help               display this help and exit\n"
"  -V, --version            display version information and exit\n"
"  -@, --adapter=plugin:bus tell OpenCBM which backend plugin and bus to use\n"
"\n"
);
}

static void hint(char *s)
{
    fprintf(stderr, "Try `%s' --help for more information.\n", s);
}

static void my_message_cb(int severity, const char *format, ...)
{
    va_list args;

    static const char *severities[4] =
    {
        "Fatal",
        "Warning",
        "Info",
        "Debug"
    };

    if(verbosity >= severity)
    {
        fprintf(stderr, "[%s] ", severities[severity]);
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
}

static void ARCH_SIGNALDECL reset(int dummy)
{
    CBM_FILE fd_cbm_local;

    /*
     * remember fd_cbm, and make the global one invalid
     * so that no routine can call a cbm_...() routine
     * once we have cancelled another one
     */
    fd_cbm_local = fd_cbm;
    fd_cbm = CBM_FILE_INVALID;

    fprintf(stderr, "\nSIGINT caught X-(  Resetting IEC bus...\n");

    DEBUG_PRINTDEBUGCOUNTERS();

    d64copy_cleanup();
    cbm_reset(fd_cbm_local);
    cbm_driver_close(fd_cbm_local);
    exit(1);
}

static int turbo_routine_starter(CBM_FILE fd, unsigned char drive)
{
    SETSTATEDEBUG((void)0);
    return cbm_exec_command(fd, drive, "U4:", 3);
}

static const unsigned char warp_read_1541[] =
{
#include "warpread1541.inc"
};

static const unsigned char warp_write_1541[] =
{
#include "warpwrite1541.inc"
};

static const unsigned char warp_read_1571[] =
{
#include "warpread1571.inc"
};

static const unsigned char warp_write_1571[] =
{
#include "warpwrite1571.inc"
};

static const unsigned char turbo_read_1541[] =
{
#include "turboread1541.inc"
};

static const unsigned char turbo_write_1541[] =
{
#include "turbowrite1541.inc"
};

static const unsigned char turbo_read_1571[] =
{
#include "turboread1571.inc"
};

static const unsigned char turbo_write_1571[] =
{
#include "turbowrite1571.inc"
};

static const struct drive_prog
{
    int size;
    const unsigned char *prog;
} drive_progs[] =
{
    {sizeof(turbo_read_1541), turbo_read_1541},
    {sizeof(turbo_write_1541), turbo_write_1541},
    {sizeof(warp_read_1541), warp_read_1541},
    {sizeof(warp_write_1541), warp_write_1541},
    {sizeof(turbo_read_1571), turbo_read_1571},
    {sizeof(turbo_write_1571), turbo_write_1571},
    {sizeof(warp_read_1571), warp_read_1571},
    {sizeof(warp_write_1571), warp_write_1571}
};

static int send_turbo(CBM_FILE fd, unsigned char drv, int write, int warp, int drv_type)
{
    const struct drive_prog *prog;

    prog = &drive_progs[drv_type * 4 + warp * 2 + write];

    SETSTATEDEBUG((void)0);
    return cbm_upload(fd, drv, 0x500, prog->prog, prog->size);
}

static void printGcrBuffer(unsigned char* data, int omitzeroes)
{
    int i=0;
    for(i=0; i<GCRBUFSIZE-1;)
    {
        if( !(i & 0x1f))
        {
            printf("\n0x");
        }
        if(0==data[i] && omitzeroes)
        {
            printf("  ");
        }
        else
        {
            printf("%02X", data[i]);
        }
        i++;
    }
    printf("\n");
}

static void xorBufferWithTestpattern(unsigned char buffer[])
{
	  int i, j;
    for(i=0; i<sizeof(weakstart); i++)
    {
        buffer[i] ^= weakstart[i];	// copy over the weak bit area start sequence
    }
    for(i=16; i<(GCRBUFSIZE-64); i++)
    {
        buffer[i] ^= 0;	// follow with weak bits only
    }
    for(; i<(GCRBUFSIZE-22); i++)
    {
        buffer[i] ^= 0x55; // start writing the test vector tail marker
    }
    for(j=0; j<sizeof(syncmark); i++, j++)
    {
        buffer[i] ^= syncmark[j]; // copy the bitsync marker
    }
    for(; i<GCRBUFSIZE; i++)
    {
        buffer[i] ^= 0x55; // then proceed with GAPs
    }
}

static int testLastDirectorySectorWithWeakBits(
    unsigned char cbm_drive, d64copy_settings setup, const transfer_funcs *target,
    unsigned char track, unsigned char se)
{
    unsigned char gcr[GCRBUFSIZE];
    char trackmap[21+1];
    int st, i;

    SETSTATEDEBUG((void)0);
    // warp write
    send_turbo(fd_cbm, cbm_drive, 1, 1, setup.drive_type == cbm_dt_cbm1541 ? 0 : 1);

    SETSTATEDEBUG((void)0);
    if(target->open_disk(fd_cbm, &setup, (void*)(ULONG_PTR)cbm_drive, 1,
                      turbo_routine_starter, my_message_cb) == 0)
    {
        for(i=0; i<GCRBUFSIZE; i++)
        {
            gcr[i] = 0;
        }
        xorBufferWithTestpattern(gcr);

        // print the buffer
        printf("Input test vector");
        printGcrBuffer(gcr, 0);

        SETSTATEDEBUG((void)0);
        st = target->write_block(track, se, gcr, GCRBUFSIZE-1, 0);
        target->close_disk();

        if(st)
        {
            my_message_cb(1, "failed to write block (%d)", st);
            // return -1;
        }
 
        SETSTATEDEBUG((void)0);

        // warp read
        send_turbo(fd_cbm, cbm_drive, 0, 1, setup.drive_type == cbm_dt_cbm1541 ? 0 : 1);

        SETSTATEDEBUG((void)0);
        if(target->open_disk(fd_cbm, &setup, (void*)(ULONG_PTR)cbm_drive, 0,
                          turbo_routine_starter, my_message_cb) == 0)
        {
            // set up the map with sectors to copy
            memset(trackmap, bs_dont_copy, sizeof(trackmap));
            trackmap[se] = bs_must_copy;
            SETSTATEDEBUG((void)0);
            target->send_track_map(track, trackmap, 1);

            SETSTATEDEBUG((void)0);
            st = target->read_gcr_block(&se, gcr);
            target->close_disk();

            if(st)
            {
                my_message_cb(1, "failed to read back block (%d)", st);
                return -1;
            }
    
            // print the buffer
            printf("\nRead back weak bit area");
            printGcrBuffer(gcr, 0);

            xorBufferWithTestpattern(gcr);
            printf("\nRead back weak bit area XOR'ed with input vector");
            printGcrBuffer(gcr, 1);

            return 0;
        }
    }
    my_message_cb(0, "can't open target");
    return -1;
}

static int testWeakBitsBehavior(
    unsigned char cbm_drive, d64copy_settings setup)
{
    const transfer_funcs *target;
    unsigned char stbuf[32];
    int rv;

    if(setup.drive_type == cbm_dt_unknown )
    {
        my_message_cb( 2, "Trying to identify drive type" );
        if( cbm_identify( fd_cbm, cbm_drive, &setup.drive_type, NULL ) )
        {
            my_message_cb( 0, "could not identify device" );
        }
    }

    switch( setup.drive_type )
    {
        case cbm_dt_cbm1581:
            my_message_cb( 0, "1581 drives are not supported" );
            return -1;
        case cbm_dt_cbm1571:
        case cbm_dt_cbm1570:
            // switch to 1541 mode, if it should be a 1571 or 1570
            SETSTATEDEBUG((void)0);
            cbm_exec_command(fd_cbm, cbm_drive, "U0>M0", 0);
            // switch to 1571 side one
            SETSTATEDEBUG((void)0);
            cbm_exec_command(fd_cbm, cbm_drive, "U0>H0", 0);
            // initialise drive/disk side
            SETSTATEDEBUG((void)0);
            cbm_exec_command(fd_cbm, cbm_drive, "I0:", 0);
            break;
        default:
            my_message_cb( 1, "Unknown drive, assuming 1541" );
            setup.drive_type = cbm_dt_cbm1541;
        case cbm_dt_cbm1541:
            SETSTATEDEBUG((void)0);
            cbm_exec_command(fd_cbm, cbm_drive, "I0:", 0);
    }

    switch(setup.transfer_mode)
    {
        case 0:
        case 1:
        default:
            my_message_cb( 0, "Non warp'able transfer modes are not supported" );
            return -1;

        case 2:
            target = &d64copy_s1_transfer;
            break;
        case 3:
            target = &d64copy_s2_transfer;
            break;
        case 4:
            target = &d64copy_pp_transfer;
            break;
    }


    rv = testLastDirectorySectorWithWeakBits(
        cbm_drive, setup, target, 18, 15);

    if(0==rv && cbm_dt_cbm1571 == setup.drive_type )
    {
        // switch to 1571 side two
        SETSTATEDEBUG((void)0);
        cbm_exec_command(fd_cbm, cbm_drive, "U0>H1", 0);
        // initialise drive/disk side
        SETSTATEDEBUG((void)0);
        cbm_exec_command(fd_cbm, cbm_drive, "I0:", 0);
        if(0==cbm_device_status(fd_cbm, cbm_drive, stbuf, sizeof(stbuf)))
        {
            printf("\n\nSwitched to 1571 second side:\n\n");
            rv = testLastDirectorySectorWithWeakBits(
                cbm_drive, setup, target, 18, 15);
        }
    }

    return rv;
}

int ARCH_MAINDECL main(int argc, char *argv[])
{
    d64copy_settings *settings = d64copy_get_default_settings();
    char *dst_arg;
    int  rv = 1;
    int option;
    int dst_is_cbm;
    unsigned char drive;
    char *adapter = NULL;

    struct option longopts[] =
    {
        { "help"       , no_argument      , NULL, 'h' },
        { "version"    , no_argument      , NULL, 'V' },
        { "adapter"    , required_argument, NULL, '@' },
        { NULL         , 0                , NULL, 0   }
    };
    const char shortopts[] ="hV";

    while((option = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
    {
        switch(option)
        {
            case 'h': help();
                      return 0;
            case 'V': printf("weaktest %s\n", OPENCBM_VERSION);
                      return 0;
            case '@': if (adapter == NULL)
                          adapter = cbmlibmisc_strdup(optarg);
                      else
                      {
                          fprintf(stderr, "--adapter/-@ given more than once.");
                          hint(argv[0]);
                          return 1;
                      }
                      break;
            case 0:   break; // needed for --no-warp
            default : hint(argv[0]);
                      return 1;
        }
    }

    if(optind + 1 != argc)
    {
        fprintf(stderr, "Usage: %s [OPTION]... [TARGET]\n", argv[0]);
        hint(argv[0]);
        return 1;
    }

    dst_arg = argv[optind];
    dst_is_cbm = is_cbm(dst_arg);
    if(0 == dst_is_cbm)
    {
        my_message_cb(0, "target must be a CBM drive");
        return 1;
    }

    if(cbm_driver_open_ex(&fd_cbm, adapter) == 0)
    {
        drive = (unsigned char)atoi(dst_arg);
        settings->warp = 1;
        /*
         * If the user specified auto transfer mode, find out
         * which transfer mode to use.
         */
        settings->transfer_mode = 
            d64copy_check_auto_transfer_mode(fd_cbm, 0, drive);

        my_message_cb(3, "decided to use transfer mode %d", settings->transfer_mode );

        arch_set_ctrlbreak_handler(reset);

				rv = testWeakBitsBehavior(
				    drive, *settings);

        cbm_driver_close(fd_cbm);
        // rv = 0;
    }

    if(rv!=0)
    {
        arch_error(0, arch_get_errno(), "%s", cbm_get_driver_name_ex(adapter));
    }

    cbmlibmisc_strfree(adapter);
    free(settings);
    
    return rv;
}
