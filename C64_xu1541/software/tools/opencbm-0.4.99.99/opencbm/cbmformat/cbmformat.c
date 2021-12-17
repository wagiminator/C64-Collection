/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
*/

#include "opencbm.h"

#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch.h"
#include "libmisc.h"

static unsigned char dskfrmt[] = {
#include "cbmformat.inc"
};

static void help()
{
    printf(
"Usage: cbmformat [OPTION]... DRIVE NAME,ID\n"
"Fast CBM-1541 disk formatter\n"
"\n"
"  -h, --help                 display this help and exit\n"
"  -V, --version              display version information and exit\n"
"  -@, --adapter=plugin:bus   tell OpenCBM which backend plugin and bus to use\n"
"\n"
"  -n, --no-bump              do not bump drive head\n"
"  -x, --extended             format 40 track disk\n"
"  -c, --clear                clear (demagnetize) this disk.\n"
"                             this is highly recommended if this disk\n"
"                             is used for the first time.\n"
"  -v, --verify               verify each track after it is written\n"
"  -o, --original             fill sectors with the original pattern\n"
"                             (0x4b, 0x01...) instead of zeroes\n"
"  -s, --status               display drive status after formatting\n"
"  -p, --progress             display progress indicator\n"
"\n"
);
}

static void hint(char *s)
{
    fprintf(stderr, "Try `%s' -h for more information.\n", s);
}

int ARCH_MAINDECL main(int argc, char *argv[])
{
    int status = 0, id_ofs = 0, name_len, i;
    CBM_FILE fd;
    unsigned char drive, tracks = 35, bump = 1, orig = 0, show_progress = 0;
    unsigned char verify = 0;
    unsigned char demagnetize = 0;
    char cmd[40], name[20], *arg;
    int err = 0;
    char *adapter = NULL;
    int option;

    struct option longopts[] =
    {
        { "help"       , no_argument      , NULL, 'h' },
        { "version"    , no_argument      , NULL, 'V' },
        { "adapter"    , required_argument, NULL, '@' },
        { "no-bump"    , no_argument      , NULL, 'n' },
        { "extended"   , no_argument      , NULL, 'x' },
        { "original"   , no_argument      , NULL, 'o' },
        { "status"     , no_argument      , NULL, 's' },
        { "progress"   , no_argument      , NULL, 'p' },
        { "verify"     , no_argument      , NULL, 'v' },
        { "clear"      , no_argument      , NULL, 'c' },

        /* undocumented */
        { "end-track"  , required_argument, NULL, 't' },
        { NULL         , 0                , NULL, 0   }
    };

    const char shortopts[] ="hVnxospvct:@:";
    while((option = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
    {
        switch(option)
        {
            case 'n': bump = 0;
                      break;
            case 'o': orig = 1;
                      break;
            case 's': status = 1;
                      break;
            case 'x': tracks = 40;
                      break;
            case 'h': help();
                      return 0;
            case 'V': printf("cbmformat %s\n", OPENCBM_VERSION);
                      return 0;
            case 'p': show_progress = 1;
                      break;
            case 'v': verify = 1;
                      break;
            case 'c': demagnetize = 1;
                      break;
            case 't': tracks = arch_atoc(optarg);
                      break;
            case '@': if (adapter == NULL)
                          adapter = cbmlibmisc_strdup(optarg);
                      else
                      {
                          fprintf(stderr, "--adapter/-@ given more than once.");
                          hint(argv[0]);
                          return 1;
                      }
                      break;
            default : hint(argv[0]);
                      return 1;
        }
    }

    if(optind + 2 != argc)
    {
        fprintf(stderr, "Usage: %s [OPTION]... DRIVE NAME,ID\n", argv[0]);
        hint(argv[0]);
        return 1;
    }

    arg = argv[optind++];
    drive = arch_atoc(arg);
    if(drive < 8 || drive > 11)
    {
        fprintf(stderr, "Invalid drive number (%s)\n", arg);
        return 1;
    }
    
    arg      = argv[optind++];
    name_len = 0;
    while(*arg)
    {
        unsigned char c;
        c = (unsigned char) toupper(*arg);
        if(c == ',')
        {
            if(id_ofs)
            {
                fprintf(stderr, "More than one `,' in disk name\n");
                return 1;
            }
            id_ofs = name_len;
        }
        name[name_len++] = c;
        if(name_len > 19)
        {
            fprintf(stderr, "Disk name too long\n");
            return 1;
        }
        arg++;
    }
    name[name_len] = 0;

    if(cbm_driver_open_ex(&fd, adapter) == 0)
    {
        cbm_upload(fd, drive, 0x0500, dskfrmt, sizeof(dskfrmt));
        sprintf(cmd, "M-E%c%c%c%c%c%c%c%c0:%s", 3, 5, tracks + 1, 
            orig, bump, show_progress, demagnetize, verify, name);
        cbm_exec_command(fd, drive, cmd, 13+strlen(name));

        if(show_progress)
        {
            /* do some handshake */
            cbm_iec_release(fd, IEC_CLOCK);
            for(i = 1; i <= tracks; i++)
            {
                cbm_iec_wait(fd, IEC_DATA, 1);
                cbm_iec_set(fd, IEC_CLOCK);
                cbm_iec_wait(fd, IEC_DATA, 0);
                cbm_iec_release(fd, IEC_CLOCK);

                printf("#");
                fflush(stdout);
            }
            printf("\n");
        }

        err = cbm_device_status(fd, drive, cmd, sizeof(cmd));

        if(err && status)
        {
            printf("%s\n", cmd);
        }

        if(!err && (tracks > 35))
        {
            cbm_open(fd, drive, 2, "#", 1);
            cbm_exec_command(fd, drive, "U1:2 0 18 0", 11);
            cbm_exec_command(fd, drive, "B-P2 192", 8);
            cbm_listen(fd, drive, 2);
            while(tracks > 35)
            {
                cbm_raw_write(fd, "\021\377\377\001", 4);
                tracks--;
            }
            cbm_unlisten(fd);
            cbm_exec_command(fd, drive, "U2:2 0 18 0", 11);
            cbm_close(fd, drive, 2);
        }

        if(!err && status)
        {
            cbm_device_status(fd, drive, cmd, sizeof(cmd));
            printf("%s\n", cmd);
        }
        cbm_driver_close(fd);
        cbmlibmisc_strfree(adapter);
        return 0;
    }
    else
    {
        arch_error(0, arch_get_errno(), "%s", cbm_get_driver_name_ex(adapter));
        cbmlibmisc_strfree(adapter);
        return 1;
    }
}
