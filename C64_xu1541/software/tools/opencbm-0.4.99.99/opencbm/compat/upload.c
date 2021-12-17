/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
*/

#include <unistd.h>
#include <stdio.h>

#include "cbm4linux.h"

int cbm_upload(int f, unsigned char dev, int adr, unsigned char *prog, int size)
{
    int c, i, rv = 0;
    char cmd[40];
    
    for(i = 0; i < size; i+=32) {
        cbm_listen(f, dev, 15);
        c = size - i;
        if(c > 32) c = 32;
        sprintf(cmd, "M-W%c%c%c", adr%256, adr/256, c);
        adr += c;
        write(f, cmd, 6);
        write(f, prog, c);
        prog += c;
        rv   += c;
        cbm_unlisten(f);
    }
    return rv;
}
