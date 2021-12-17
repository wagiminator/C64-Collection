/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2005      Tim Schürmann
 *  Copyright 2007,2009 Spiro Trikaliotis
 *  Copyright 2009      Arnd <arnd(at)jonnz(dot)de>
*/

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "opencbm.h"
#include "cbm_module.h"

#if defined(__linux__)
#include <linux/errno.h>  /* error codes */
#else
#include <errno.h>  /* error codes */
#endif


/*linux functions needed by parallel burst */

unsigned char opencbm_plugin_parallel_burst_read(CBM_FILE f)
{
    return ioctl(f, CBMCTRL_PARBURST_READ);
}

void opencbm_plugin_parallel_burst_write(CBM_FILE f, unsigned char c)
{
    ioctl(f, CBMCTRL_PARBURST_WRITE, c);
}

int opencbm_plugin_parallel_burst_read_track(CBM_FILE f, unsigned char *buffer, unsigned int length)
{
    int retval;

    PARBURST_RW_VALUE mv;
    mv.buffer=buffer;
    mv.length=length; /* only needed in write_track */
    retval=ioctl(f, CBMCTRL_PARBURST_READ_TRACK, &mv);  /* we have to catch retval to check on EFAULT */
    if(retval==(-EFAULT)) {printf("cbm4linux: cbm.c: cbm_parallel_burst_read_track: ioctl returned -EFAULT"); return 0;}
    return retval;
}

int opencbm_plugin_parallel_burst_read_track_var(CBM_FILE f, unsigned char *buffer, unsigned int length)
{
    int retval;

    PARBURST_RW_VALUE mv;
    mv.buffer=buffer;
    mv.length=length; /* only needed in write_track */
    retval=ioctl(f, CBMCTRL_PARBURST_READ_TRACK_VAR, &mv);  /* we have to catch retval to check on EFAULT */
    if(retval==(-EFAULT)) {printf("cbm4linux: cbm.c: cbm_parallel_burst_read_track_var: ioctl returned -EFAULT"); return 0;}
    return retval;
}

int opencbm_plugin_parallel_burst_write_track(CBM_FILE f,  unsigned char *buffer, unsigned int length)
{
    int retval;

    PARBURST_RW_VALUE mv;
    mv.buffer=buffer;
    mv.length=length;
    retval=ioctl(f, CBMCTRL_PARBURST_WRITE_TRACK, &mv);
    if(retval==(-EFAULT)) {printf("cbm4linux: cbm.c: cbm_parallel_burst_write_track: ioctl returned -EFAULT"); return 0;}
    return retval;
}
