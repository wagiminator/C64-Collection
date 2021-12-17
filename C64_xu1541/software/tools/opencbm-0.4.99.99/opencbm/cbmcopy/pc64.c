/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
*/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "inputfiles.h"

static int probe(FILE *file, const char *fname, cbmcopy_message_cb msg_cb)
{
    char sig[8];

    msg_cb( sev_debug, "checking for pc64" );

    if(fread( sig, sizeof(sig), 1, file ) == 1 &&
       strncmp( sig, "C64File", 8 ) == 0)
    {
        msg_cb( sev_debug, "PC64 file detected: %s", fname );
        return 1;
    }
    rewind( file );
    return 0;
}


static int read(FILE *file, const char *fname, int entry,
                char *cbmname, char *type,
                unsigned char **data, size_t *size,
                cbmcopy_message_cb msg_cb)
{
    const char *ext;

    struct
    {
        unsigned char sig[8];
        unsigned char cbmname[16];
        unsigned char zero;
        unsigned char reclen;
    } pc64header;

    rewind( file );

    if(entry != 0)
    {
        msg_cb( sev_warning, "invalid PC64 entry" );
        return 1;
    }

    if(fread( &pc64header, sizeof(pc64header), 1, file ) != 1)
    {
        msg_cb( sev_warning, "could not read PC64 header" );
        return 1;
    }

    if(strlen( fname ) > 4)
    {
        ext = fname + strlen(fname) - 3;
        if(isdigit(ext[1]) && isdigit(ext[2]) &&
           strchr("PSDU", toupper(*ext)))
        {
            *type = (char) toupper(*ext);
        }
        else
        {
            msg_cb( sev_warning, "could not guess PC64 filetype: %s", fname );
            *type = 'P';
        }
    }
    else
    {
        msg_cb( sev_warning,
                "name too short to guess PC64 filetype: %s", fname );
        *type = 'P';
    }
    memcpy(cbmname, pc64header.cbmname, 16 );

    *data = NULL;
    if(fseek(file, 0L, SEEK_END) == 0)
    {
        *size = ftell(file) - sizeof(pc64header);
        if(*size)
        {
            *data = malloc(*size);
            if(*data)
            {
                if(fseek(file, sizeof(pc64header), SEEK_SET) == 0 &&
                   fread(*data, *size, 1, file) == 1)
                {
                    return 0;
                }
                free(*data);
            }
        }
        else
        {
            return 0;
        }
    }
    return 1;
}

DECLARE_INPUT_READER(pc64);
