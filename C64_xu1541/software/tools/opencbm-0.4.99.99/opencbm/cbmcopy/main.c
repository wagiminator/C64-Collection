/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2001-2004 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2004-2007 Spiro Trikaliotis
 *  Copyright 2011      Thomas Winkler
 *  Copyright 2011      Wolfgang Moser (http://d81.de)
 */

#include <ctype.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch.h"
#include "libmisc.h"

#include "opencbm.h"
#include "cbmcopy.h"
#include "inputfiles.h"

#ifdef LIBCBMCOPY_DEBUG
# define DEBUG_STATEDEBUG
#endif
#include "statedebug.h"


/* global, because of signal handler */
static CBM_FILE fd_cbm;

static cbmcopy_severity_e verbosity = sev_info;
static int no_progress = 0;

static void my_message_cb(cbmcopy_severity_e severity,
                          const char *format, ...)
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

static int my_status_cb(int blocks_written)
{
    char *statstr;

    // if(!no_progress) printf("."); fflush(stdout);
    if(!no_progress)
    {
        switch (blocks_written%4)
        {
            case 3: statstr="\010-"; break;
            case 2: statstr="\010/"; break;
            case 1: statstr="\010|"; break;
            default: statstr="\010.\\";
        }
        printf("%s",statstr);
        fflush(stdout);
    }
    return 0;
}


static void help(const char *prog)
{
    printf(
"Usage: %s [OPTION]... [DRIVE] [FILE]...\n"
"Copy files to a CBM-15[478]1 or compatible drive and vice versa\n"
"\n"
"Options:\n"
"  -r, --read                 transfer 15x1->PC\n"
"                             (default when started as 'cbmread')\n"
"  -w, --write                transfer PC->15x1\n"
"                             (default when started as 'cbmwrite')\n"
"                             -r and -w are mutually exclusive\n"
"\n"
"  -h, --help                 display this help and exit\n"
"  -V, --version              display version information and exit\n"
"  -@, --adapter=plugin:bus   tell OpenCBM which backend plugin and bus to use\n"
"  -q, --quiet                quiet output\n"
"  -v, --verbose              control verbosity (repeatedly, up to 3 times)\n"
"  -n, --no-progress          do not display progress information\n"
"\n"
"  -t, --transfer=TRANSFER    set transfermode; valid modes:\n"
"                             auto (default)\n"
"                               original or o  (slowest)\n"
"                               serial1 or s1\n"
"                               serial2 or s2\n"
"                               parallel       (fastest)\n"
"                             (can be abbreviated, if unambiguous)\n"
"                             `serial1' should work in any case;\n"
"                             `serial2' won't work if more than one device is\n"
"                             connected to the IEC bus;\n"
"                             `parallel' needs a XP1541/XP1571 cable in addition\n"
"                             to the serial one.\n"
"                             `auto' tries to determine the best option.\n"
"  -d, --drive-type=TYPE      specify drive type, one of:\n"
"                               1541, 1570, 1571, 1581\n"
"  -a, --address=ADDRESS      override file start address\n"
"  -o, --output=NAME          specifies target name (ASCII, even for writing).\n"
"\n"
"Options for writing:\n"
"  -f, --file-type            specify CBM file type (D,P,S,U)\n"
"  -R, --raw                  skip test for PC64 (.p00) and T64 input file\n"
"\n", prog);
}

static void hint(char *prog)
{
    printf("Try `%s' --help for more information.\n", prog);
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

    cbm_reset(fd_cbm_local);
    cbm_driver_close(fd_cbm_local);
    exit(1);
}


extern input_reader cbmwrite_raw;
extern input_reader cbmwrite_pc64;
extern input_reader cbmwrite_t64;


static void char_star_opt_once(const char **arg,
                               const char *long_name,
                               char **argv)
{
    if(*arg)
    {
        my_message_cb(sev_fatal, "%s given more than once.", long_name);
        hint(argv[0]);
        exit(1);
    }
    *arg = optarg;
}


int ARCH_MAINDECL main(int argc, char **argv)
{
    CBM_FILE fd;
    FILE *file;
    char *fname;

    int mode;
    int option;
    unsigned char *filedata;
    size_t filesize;
    char buf[48];
    int num_entries;
    int num_files;
    int rv;
    int i;
    int write;
    cbmcopy_settings *settings;
    char auto_name[17];
    char auto_type = '\0';
    char output_type = '\0';
    char *tail;
    char *ext;
    char *adapter = NULL;

    unsigned char drive;
    const char *tm = NULL;
    const char *dt = NULL;
    int force_raw = 0;
    int address = -1;
    const char *output_name = NULL;
    const char *address_str = NULL;
    char *fs_name;

    input_reader *readers[] =
    {
        &cbmwrite_raw,   /* must be first, as it is default */
        &cbmwrite_pc64,
        &cbmwrite_t64,
        NULL
    };

    input_reader *rd;

    struct option longopts[] =
    {
        { "help"            , no_argument      , NULL, 'h' },
        { "verbose"         , no_argument      , NULL, 'v' },
        { "adapter"         , required_argument, NULL, '@' },
        { "quiet"           , no_argument      , NULL, 'q' },
        { "version"         , no_argument      , NULL, 'V' },
        { "no-progress"     , no_argument      , NULL, 'n' },
        { "read"            , no_argument      , NULL, 'r' },
        { "write"           , no_argument      , NULL, 'w' },
        { "transfer"        , required_argument, NULL, 't' },
        { "drive-type"      , required_argument, NULL, 'd' },
        { "file-type"       , required_argument, NULL, 'f' },
        { "output"          , required_argument, NULL, 'o' },
        { "raw"             , no_argument      , NULL, 'R' },
        { "address"         , no_argument      , NULL, 'a' },
        { NULL              , 0                , NULL, 0   }
    };

    const char shortopts[] ="hVqvrwnt:d:f:o:Ra:@:";

    if(NULL == (tail = strrchr(argv[0], '/')))
    {
        tail = argv[0];
    }
    else
    {
        tail++;
    }
    if(strcmp(tail, "cbmread") == 0)
    {
        mode = 'r'; /* read */
    }
    else if(strcmp(tail, "cbmwrite") == 0)
    {
        mode = 'w'; /* write */
    }
    else
    {
        mode = EOF; /* mode must be given later */
    }

    settings = cbmcopy_get_default_settings();

    /* loop over cmd line opts */
    while((option = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
    {
        switch(option)
        {
            case 'h': /* --help */
                help(argv[0]);
                return 0;
            case 'V': /* --version */
                printf("cbmcopy %s\n", OPENCBM_VERSION);
                return 0;
            case 'q': /* -- quiet */
                if(verbosity > sev_fatal)
                    verbosity--;
                break;
            case 'v': /* --verbose */
                if(verbosity < sev_debug)
                    verbosity++;
                break;
            case 'n': /* --no-progress */
                no_progress = 1;
                break;

            case 'r': /* --read */
            case 'w': /* --write */
                if(mode != EOF)
                {
                    my_message_cb(sev_fatal, "-r/-w given more than once");
                    hint(argv[0]);
                    return 1;
                }
                mode = option;
                break;

            case 't': /* --transfer */
                char_star_opt_once(&tm, "--transfer", argv);
                break;
            case 'd': /* --drive-type */
                char_star_opt_once(&dt, "--drive-type", argv);
                break;
            case 'o': /* --output */
                char_star_opt_once(&output_name, "--output", argv);
                break;
            case 'f': /* --file-type */
                output_type = (char) toupper(*optarg);
                break;
            case 'R': /* --raw */
                force_raw = 1;
                break;
            case 'a': /* override-address */
                char_star_opt_once(&address_str, "--address", argv);
                break;
            case '@': /* choose adapter */
                if (adapter == NULL)
                    adapter = cbmlibmisc_strdup(optarg);
                else {
                    my_message_cb(sev_fatal, "--adapter/-@ given more than once.");
                    hint(argv[0]);
                    exit(1);
                }
                break;
            default : /* unknown */
                hint(argv[0]);
                return 1;
        }
    }

    /* check -r/-w */
    switch(mode)
    {
        case 'r' :
            write = 0;
            break;
        case 'w' :
            write = 1;
            break;
        default:
            my_message_cb(sev_fatal,
                          "-r or -w must be given when started as `%s'",
                          argv[0]);
            hint(argv[0]);
            return 1;
    }

    /* check transfer mode */
    settings->transfer_mode = cbmcopy_get_transfer_mode_index(tm);
    if(settings->transfer_mode < 0)
    {
        my_message_cb(sev_fatal, "Unknown transfer mode: %s", tm);
        return 1;
    }

    /* check device type */
    if(dt)
    {
        const struct
        {
            const char *str;
            enum cbm_device_type_e type;
        }
        *p, types[] =
        {
            { "1541", cbm_dt_cbm1541 }, { "1571", cbm_dt_cbm1571 },
            { "1570", cbm_dt_cbm1570 }, { "1581", cbm_dt_cbm1581 },
            { NULL  , cbm_dt_unknown }
        };

        for(p = types; p->str && strcmp(dt, p->str); p++)
            ; /* nothing */

        if(!p->str)
        {
            my_message_cb(sev_fatal, "Unknown drive type: %s", dt);
            return 1;
        }
        settings->drive_type = p->type;
    }

    /* check CBM file type */
    if(output_type)
    {
        if(write)
        {
            if(strchr("DSPU", output_type) == NULL)
            {
                my_message_cb(sev_fatal, "Invalid file type : %c", output_type);
            }
        }
        else
        {
            my_message_cb(sev_warning, "--file-type ignored");
        }
    }

    /* check load address override */
    if(address_str)
    {
        address = strtol(address_str, &tail, 0);
        if(*tail || address < 0 || address > 0xffff)
        {
            my_message_cb(sev_fatal, "--address invalid: %s", address_str);
            hint(argv[0]);
            return 1;
        }
    }

    /* first non-option is device number */
    if(optind == argc)
    {
        my_message_cb(sev_fatal, "%s: No drive number given", argv[0]);
        hint(argv[0]);
        return 1;
    }

    drive = (unsigned char) strtol(argv[optind], &tail, 0);
    if(drive < 8 || drive > 11 || *tail)
    {
        my_message_cb(sev_fatal, "invalid drive: `%s'", argv[optind]);
        return 1;
    }

    /* remaining args are file names */
    num_files = argc - optind - 1;

    if(num_files == 0)
    {
        my_message_cb(sev_fatal, "%s: No files?", argv[0]);
        hint(argv[0]);
        return 1;
    }

    /* more than one file name given, avoid -o option */
    if(num_files > 1 && output_name)
    {
        my_message_cb(sev_fatal, "--output requires exactly one file name");
        return 1;
    }

    rv = cbm_driver_open_ex( &fd, adapter );
    cbmlibmisc_strfree(adapter);

    if(0 == rv)
    {
        fd_cbm = fd;

        /*
         * If the user specified auto transfer mode, find out
         * which transfer mode to use.
         */
        settings->transfer_mode = 
            cbmcopy_check_auto_transfer_mode(fd_cbm,
                settings->transfer_mode,
                drive);

        arch_set_ctrlbreak_handler(reset);

        while(++optind < argc)
        {
            fname = argv[optind];
            if(write)
            {
                rd = readers[0];

                file = fopen(fname, "rb");
                if(file)
                {
                    num_entries = 0;
                    if(!force_raw)
                    {
                        /* try to detect file format */
                        for(i = 1; readers[i] && !num_entries; i++)
                        {
                            num_entries =
                                readers[i]->probe( file, fname, my_message_cb );
                            if(num_entries)
                                rd = readers[i];
                        }
                    }
                    if(!num_entries) num_entries = 1; /* raw file */

                    for(i = 0; i < num_entries; i++)
                    {
                        my_message_cb( sev_debug,
                                       "processing entry %d from %s",
                                       i, fname );
                        if(rd->read(file, fname, i,
                                    auto_name, &auto_type,
                                    &filedata, &filesize, my_message_cb ) == 0)
                        {
                            buf[16] = '\0';
                            if(output_name)
                            {
                                strncpy(buf, output_name, 16);
                                cbm_ascii2petscii(buf);
                            }
                            else
                            {
                                /* no charset conversion */
                                strncpy(buf, auto_name, 16);
                            }
                            strcat(buf, ",x");
                            buf[strlen(buf)-1] =
                                output_type ? output_type : auto_type;
                            strcat(buf, ",W");

                            my_message_cb( sev_info,
                                           "writing %s -> %s", fname, buf );

                            if(address >= 0 && filesize > 1)
                            {
                                filedata[0] = address % 0x100;
                                filedata[1] = address / 0x100;

                                my_message_cb( sev_debug, 
                                               "override address: $%02x%02x",
                                               filedata[1], filedata[0] );

                            }
                            if(cbmcopy_write_file(fd, settings, drive,
                                                  buf, strlen(buf),
                                                  filedata, filesize,
                                                  my_message_cb,
                                                  my_status_cb) == 0)
                            {
                                printf("\n");
                                rv = cbm_device_status( fd, drive,
                                                        buf, sizeof(buf) );
                                my_message_cb( rv ?  sev_warning : sev_info,
                                               "%s", buf );
                            }
                            else
                                printf("\n");

                            if(filedata)
                            {
                                free(filedata);
                            }
                        }
                        else
                        {
                            my_message_cb( sev_warning,
                                           "error processing entry %d from %s",
                                           i, fname );
                        }
                    }
                }
                else
                {
                    my_message_cb( sev_warning,
                                   "warning could not read %s: %s",
                                   fname, arch_strerror(arch_get_errno()) );
                }
            }
            else
            {
                strncpy(buf, fname, 16);
                buf[16] = '\0';
                cbm_ascii2petscii(buf);

                if(output_name)
                {
                    fs_name = arch_strdup(output_name);
                }
                else
                {
                    tail = strrchr(fname, ',');

                    ext = "prg"; /* default */

                    if(tail)
                    {
                        *tail++ = '\0';
                        switch(*tail)
                        {
                            case 'd': ext = "del"; break;
                            case 's': ext = "seq"; break;
                            case 'u': ext = "usr"; break;
                        }
                    }
                    fs_name = malloc(strlen(fname) + strlen(ext) + 2);
                    if(fs_name) sprintf(fs_name, "%s.%s", fname, ext);
                }

                if(fs_name)
                {
                    for(tail = fs_name; *tail; tail++)
                    {
                        if(*tail == '/') *tail = '_';
                    }
                }
                else
                {
                    /* should not happen... */
                    cbm_driver_close( fd );
                    my_message_cb(sev_fatal, "Out of memory");
                    exit(1);
                }

                my_message_cb( sev_info, "reading %s -> %s", buf, fs_name );

                if(cbmcopy_read_file(fd, settings, drive, buf, strlen(buf),
                                     &filedata, &filesize,
                                     my_message_cb, my_status_cb) == 0)
                {
                    rv = cbm_device_status( fd, drive, buf, sizeof(buf) );
                    my_message_cb( rv ? sev_warning : sev_info, "%s", buf );

                    file = fopen(fs_name, "wb");
                    if(file)
                    {
                        if(filedata)
                        {
                            if(address >= 0 && filesize > 1)
                            {
                                filedata[0] = address % 0x100;
                                filedata[1] = address / 0x100;

                                my_message_cb( sev_debug, 
                                               "override address: $%02x%02x",
                                               filedata[1], filedata[0] );
                            }
                            if(fwrite(filedata, filesize, 1, file) != 1)
                            {
                                my_message_cb(sev_warning,
                                              "could not write %s: %s",
                                              fs_name, arch_strerror(arch_get_errno()));
                            }
                        }
                        fclose(file);
                    }
                    else
                    {
                        my_message_cb(sev_warning,
                                      "could not open %s: %s",
                                      fs_name, arch_strerror(arch_get_errno()));
                    }

                    if(filedata)
                    {
                        free(filedata);
                    }
                }
                else
                {
                    my_message_cb(sev_warning, "error reading %s", buf);
                }
                if(fs_name)
                {
                    free(fs_name);
                }
            }
        }
        cbm_driver_close( fd );

        if(rv)
        {
            my_message_cb(sev_warning, "there was at least one error" );
        }
    }

    return rv;
}
