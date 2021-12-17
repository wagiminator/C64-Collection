/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999 Michael Klein <michael.klein@puffin.lb.shuttle.de>
 */

/*
 * patched version: Wolfgang Moser, 20050508-19:24
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
#include "frm_analyzer.inc"
};

static void help()
{
    printf(
"Usage: frm_analyzer [OPTION]... DRIVE NAME,ID\n"
"Fast CBM-1541 disk formatter\n"
"\n"
"  -h, --help               display this help and exit\n"
"  -V, --version            display version information and exit\n"
"  -@, --adapter=plugin:bus tell OpenCBM which backend plugin and bus to use\n"
"\n"
"  -n, --no-bump            do not bump drive head\n"
"  -x, --extended           format 40 track disk\n"
//"  -o, --original           fill sectors with the original pattern (0x4b, 0x01...)\n"
"  -f, --fill               fill sectors with a defined pattern (x, x...)\n"
"                           instead of zeroes\n"
"  -s, --status             display drive status after formatting\n"
"  -p, --progress           display progress indicator\n"
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
//    unsigned char drive, tracks = 35, bump = 1, orig = 0, show_progress = 0;
    unsigned char drive, tracks = 35, bump = 1, orig = 0x4b, show_progress = 0;
    char cmd[40], name[20], *arg;
    int erroroccured = 0;
    char *adapter = NULL;
    int option;

    struct option longopts[] =
    {
        { "help"       , no_argument      , NULL, 'h' },
        { "version"    , no_argument      , NULL, 'V' },
        { "adapter"    , required_argument, NULL, '@' },
        { "no-bump"    , no_argument      , NULL, 'n' },
        { "extended"   , no_argument      , NULL, 'x' },
//        { "original"   , no_argument      , NULL, 'o' },
{ "fill"  , required_argument, NULL, 'f' },

        { "status"     , no_argument      , NULL, 's' },
        { "progress"   , no_argument      , NULL, 'p' },

        /* undocumented */
        { "end-track"  , required_argument, NULL, 't' },
        { NULL         , 0                , NULL, 0   }
    };

//    const char shortopts[] ="hVnxospt:";
    const char shortopts[] ="hVnxf:spt:";


    while((option = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
    {
        switch(option)
        {
            case 'n': bump = 0;
                      break;
            case 'f': //orig = 0x4b;
orig=arch_atoc(optarg);
                      break;
            case 's': status = 1;
                      break;
            case 'x': tracks = 40;
                      break;
            case 'h': help();
                      return 0;
            case 'V': printf("frm_analyzer %s\n", OPENCBM_VERSION);
                      return 0;
            case 'p': show_progress = 1;
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
        cbm_upload(fd, drive, 0x0300, dskfrmt, sizeof(dskfrmt));
        sprintf(cmd, "M-E%c%c%c%c%c%c0:%s", 3, 3, tracks + 1, 
                orig, bump, show_progress, name);
        cbm_exec_command(fd, drive, cmd, 11+strlen(name));
#if 0
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
#endif
        erroroccured = cbm_device_status(fd, drive, cmd, sizeof(cmd));

        if(erroroccured && status)
        {
            printf("%s\n", cmd);
        }

        if(!erroroccured && (tracks > 35))
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

        if(!erroroccured && status)
        {
            cbm_device_status(fd, drive, cmd, sizeof(cmd));
            printf("%s\n", cmd);
        }
#if 1   // verbose output
        {
#if 0 // @TODO unused variables
            float RPMval;
            int sectors, virtGAPsze, remainder, trackTailGAP, flags, retry, lastTr;
            const char *vrfy;
#endif
            unsigned char data[0x100];
            if (cbm_download(fd, drive, 0x0500, data, sizeof(data)) == sizeof(data))
            {


#if 1
                // TODO, Pattern analyzer, get the lenght of the PLL synchronization period
                //
                    // search the last byte triple consisting of: 0x49, 0x24, 0x92
                    // 
                int k;
                const unsigned char pattern[]={0x49, 0x24, 0x92};
                // const unsigned char pattern[]={0xdb, 0x6d, 0xb6};

                for(k=sizeof(data)-3; k>=0; --k)
                {
                    if(data[k]==pattern[0] && data[k+1]==pattern[1] && data[k+2]==pattern[2]) break;
                }
                if(k<0)
                {
                    // no part of the written sequence was found
                    k=sizeof(data);
                }
                else
                {    
                        // now search the beginning of that "010010010010010010010010..." bit stream
                    while(k>=0 && data[k]==pattern[0] && data[k+1]==pattern[1] && data[k+2]==pattern[2])
                    {
                        k-=3;
                    }
                    k+=3;
    
                        // do single byte decreases
                    if(k>=1 && data[k-1]==pattern[2]) --k;
                    if(k>=1 && data[k-1]==pattern[1]) --k;
                    if(k>=1 && data[k-1]==pattern[0]) --k;
                }

                printf("Result with Pattern: 0x%02X / %3d, formatted on track %2d, PLL synchronization length: %3d\n", orig, orig, tracks, k);


                for (i=0; i < sizeof(data); i++)
                {
                    /*
                    if(data[i] == 0)
                    {
                        printf("\n");
                        break;
                    }
                    */
                    printf(" %02X", data[i]);
                    if((i&0x0f) == 0x0f) printf("\n");
                }
#else
                int i;
                printf("Track|Retry|sctrs|slctd|| GAP |modulo |modulo|tail| Verify  | RPM  |\n"
                       "     |     |     | GAP ||adjst|complmt| dvsr |GAP |         |      |\n"
                       "-----+-----+-----+-----++-----+-------+------+----+---------+------+\n");

                lastTr=-1;
                for (i=0; i < sizeof(data); i+=4)
                {
                    if(data[i]==0) break;    // no more data is available


                    if(data[i+3]>=0x40 && data[i]>42){
                            // logging continuation line
                        printf("     |     | debug log || $%02X | $%02X $%02X $%02X\n",
                               data[i], data[i+1], data[i+2], data[i+3]);
                        continue;    // proceed with next loop run
                        }


                    if(data[i]==lastTr) retry++;
                    else                retry=0;
                    lastTr=data[i];

                    if(data[i]>=25)            // preselect track dependent constants
                    {
                        if(data[i]>=31) sectors=17, RPMval=60000000.0f/16;
                        else            sectors=18, RPMval=60000000.0f/15;
                    }
                    else
                    {
                        if(data[i]>=18) sectors=19, RPMval=60000000.0f/14;
                        else            sectors=21, RPMval=60000000.0f/13;
                    }

                        // separate some flags
                    flags=(data[i+3]>>6)&0x03;
                    data[i+3]&=0x3f;

                    switch(flags)
                    {
                        case 0x01: vrfy="SYNC fail"; break;
                        case 0x02: vrfy="verify OK"; break;
                        case 0x03: vrfy="vrfy fail"; break;
                        default:   vrfy="   ./.   ";
                    }

                        // recalculation of the track tail GAP out of the
                        // choosen GAP for this track, the new GAP size
                        // adjustment and the complement of the remainder
                        // of the adjustment division

                    virtGAPsze=data[i+1]         -3;    // virtual GAP increase to
                        // prevent reformatting, when only one byte is missing


                    remainder=((data[i+2]==0xff) ? virtGAPsze : sectors)
                                 - data[i+3];


                    trackTailGAP=((data[i+2]==0xff) ? 0 : data[i+2]*sectors + virtGAPsze)
                                    + remainder;

                        // the following constants are nybble based (double the
                        // size of the well known constants for SYNC lengths,
                        // block header size, data block GAP and data block)
                        //
                        // (0x01&data[i+1]&sectors) is a correction term, if "half
                        // GAPs" are written and the number of sectors is odd
                        //
                    // RPMval / (sectors * (10+20+18+10 + 650 + data[i+1]) - (0x01&data[i+1]&sectors) + trackTailGAP - data[i+1])

                    RPMval = (flags != 0x01) ?
                        RPMval / (sectors * (10+20+18+10 + 650 + data[i+1]) - (0x01&data[i+1]&sectors) + trackTailGAP - data[i+1])
                        : 0;

                    printf(" %3u | ", data[i]);
                    if(retry>0) printf("%3u", retry);
                    else        printf("   ");
                   /*      " |sctrs |slctd  || GAP   |modulo   |modulo  |tail | Verify  | RPM  |\n"
                    *      " |      | GAP   ||adjst  |complmt  |        |GAP  |         |      |\n"
                    *      "-+----- +-----  ++-----  +-------  +------  +---- +---------+------+\n"
                    */
                    printf(" |  %2u | $%02X || $%02X |   $%02X |  $%02X |$%03X|%9s|%6.2f|\n",
                           sectors, data[i+1], data[i+2], data[i+3],
                           remainder, trackTailGAP, vrfy, RPMval);
                }
                printf("\n  *) Note: All GAP based numbers shown here (sedecimal values) are\n"
                         "           nybble based (4 GCR Bits) instead of GCR byte based.\n");
#endif
            }
            else
            {
                fprintf(stderr, "error reading data!\n");
            }
        }
#endif
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
