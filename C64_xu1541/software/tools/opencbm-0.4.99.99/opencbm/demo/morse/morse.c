/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
*/

#include "opencbm.h"

#include "arch.h"

#include <fcntl.h>
#include <stdlib.h>

static unsigned char morse[] = {
#include "morse.inc"
};

int ARCH_MAINDECL main(int argc, char *argv[])
{
    unsigned char drv = argc > 1 ? arch_atoc(argv[1]) : 8;
    CBM_FILE fd;
    
    if(cbm_driver_open_ex(&fd, NULL) == 0)
    {
        cbm_upload(fd, drv, 0x0500, morse, sizeof(morse));
        cbm_listen(fd, drv, 15);
        cbm_raw_write(fd, "U3:SOS", 6);
        cbm_unlisten(fd);
        cbm_driver_close(fd);
        return 0;
    }
    return 1;
}
