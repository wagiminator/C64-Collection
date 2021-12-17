/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2001-2003 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
*/

#include <stdlib.h>
#include <string.h>

#include "opencbm.h"
#include "inputfiles.h"

static int probe(FILE *file, const char *fname, cbmcopy_message_cb msg_cb)
{
    /* signature list stolen from cbmconvert */
    const char signatures[][32] =
    {
        "C64 tape image file\0\0\0\0\0\0\0\0\0\0\0\0\0",
        "C64S tape file\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",
        "C64S tape image file\0\0\0\0\0\0\0\0\0\0\0\0"
    };
    struct
    {
        char sig[32];
        unsigned char major;
        unsigned char minor;
        unsigned char max_entries_l;
        unsigned char max_entries_h;
        unsigned char used_entries_l;
        unsigned char used_entries_h;
        char pad[2];
        char name[24];  /* careful: not '\0'-terminated! */
    } t64header;

    size_t i;
    int used_entries;
    
    msg_cb( sev_debug, "checking for t64" );

    if(fread( &t64header, sizeof(t64header), 1, file) == 1)
    {
        for(i = 0; i < sizeof(signatures) / 32; i++)
        {
            if(memcmp(signatures[i], t64header.sig, 32) == 0)
            {
                used_entries =
                    t64header.used_entries_l +
                    t64header.used_entries_h * 256;
                msg_cb( sev_debug, "found t64 signature '%s'", signatures[i] );
                msg_cb( sev_debug, "tape version %u.%u",
                        t64header.major, t64header.minor );
                msg_cb( sev_debug, "tape contains %d file%c",
                        used_entries, used_entries > 1 ? 's' : '\0' );
                return used_entries;
            }
        }
    }
    rewind( file );
    return 0;
}


static int read(FILE *file, const char *fname, int entry,
                char *cbmname, char *type,
                unsigned char **data, size_t *size,
                cbmcopy_message_cb msg_cb)
{
    struct
    {
        unsigned char c64s_type;
        unsigned char c1541_type;
        unsigned char start_l;
        unsigned char start_h;
        unsigned char end_l;
        unsigned char end_h;
        unsigned char unused1[2];
        unsigned char offset[4];  /* l to h */
        unsigned char unused2[4];
        unsigned char name[16];
    } t64entry;
    long offset;
    int i;
    int start;
    int end;

    if(fseek( file, 0x40 + 32 * entry, SEEK_SET ) == 0 &&
       fread( &t64entry, sizeof(t64entry), 1, file ) == 1)
    {
        memcpy( cbmname, t64entry.name, 16 );
        for( i = 15; i >= 0 && cbmname[i] == 0x20; i-- )
        {
            cbmname[i] = (char) 0xa0;
        }
        switch(t64entry.c1541_type)
        {
            case 0x01:
            case 0x80:
            case 'D' :
                *type = 'D';
                break;
            case 0x81:
            case 'S' :
                *type = 'S';
                break;
            case 0x82:
            case 'P' :
                *type = 'P';
                break;
            case 0x83:
            case 'U' :
                *type = 'U';
                break;
            default:
                msg_cb( sev_info, "unknown 1541 file type, using default" );
                *type = 'P';
                break;
        }
        for(offset = 0, i = 3; i >= 0; i--)
        {
            offset = offset * 0x100 + t64entry.offset[i];
        }
        msg_cb( sev_debug, "data offset = %08lx", offset );
        start = t64entry.start_l + 0x100 * t64entry.start_h;
        end = t64entry.end_l + 0x100 * t64entry.end_h;

        if(fseek(file, offset, SEEK_SET) == 0)
        {
            *size = 2 + (end - start);
            *data = malloc(*size);
            if(*data)
            {
                (*data)[0] = t64entry.start_l;
                (*data)[1] = t64entry.start_h;
                if(fread(&(*data)[2], (*size)-2, 1, file) == 1)
                {
                    return 0;
                }
                else
                {
                    msg_cb( sev_warning, "could not read file data" );
                }
                free(*data);
            }
            else
            {
                msg_cb( sev_warning, "no memory" );
            }
        }
        else
        {
            msg_cb( sev_warning, "could not seek to file data" );
        }
    }
    else
    {
        msg_cb( sev_warning, "could not seek to directory entry" );
    }
    return 1;
}

DECLARE_INPUT_READER(t64);
