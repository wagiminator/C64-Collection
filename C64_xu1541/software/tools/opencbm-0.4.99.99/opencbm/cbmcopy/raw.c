/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
*/

#include <stdlib.h>
#include <string.h>

#include "opencbm.h"

#include "inputfiles.h"

static int probe(FILE *file, const char *fname, cbmcopy_message_cb msg_cb)
{
    return 1;
}


static int read(FILE *file, const char *fname, int entry,
                char *cbmname, char *type,
                unsigned char **data, size_t *size,
                cbmcopy_message_cb msg_cb)
{
    char *tail;
    char *tail2;

    tail = strrchr(fname, '/');
    tail2 = strrchr(fname, '\\');

    if (tail2)
    {
        if (tail2 > tail)
        {
            tail = tail2;
        }
    }

    strncpy(cbmname, tail ? tail+1 : fname, 16);
    for(tail = cbmname; *tail; tail++)
    {
        switch(*tail)
        {
            case '_':
                *tail = ' ';
                break;
            default:
                *tail = cbm_ascii2petscii_c(*tail);
                break;
        }
    }
    *type = 'P';
    *data = NULL;

    if(entry == 0 && fseek( file, 0L, SEEK_END ) == 0 )
    {
        *size = ftell(file);
        if(*size)
        {
            *data = malloc(*size);
            if(*data)
            {
                rewind(file);
                if(fread(*data, *size, 1, file) == 1)
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

DECLARE_INPUT_READER(raw);
