/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
*/

#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "cbm4linux.h"
#include "cbm_module.h"

const char cbm_dev[] = "/dev/cbm";

int cbm_listen(int f, unsigned char dev, unsigned char secadr)
{
    return ioctl(f, CBMCTRL_LISTEN, (dev<<8) | secadr);
}

int cbm_talk(int f, unsigned char dev, unsigned char secadr)
{
    return ioctl(f, CBMCTRL_TALK, (dev<<8) | secadr);
}

int cbm_open(int f, unsigned char dev, unsigned char secadr)
{
    return ioctl(f, CBMCTRL_OPEN, (dev<<8) | secadr);
}

int cbm_close(int f, unsigned char dev, unsigned char secadr)
{
    return ioctl(f, CBMCTRL_CLOSE, (dev<<8) | secadr);
}

int cbm_unlisten(int f)
{
    return ioctl(f, CBMCTRL_UNLISTEN);
}

int cbm_untalk(int f)
{
    return ioctl(f, CBMCTRL_UNTALK);
}

int cbm_get_eoi(int f)
{
    return ioctl(f, CBMCTRL_GET_EOI);
}

int cbm_reset(int f)
{
    return ioctl(f, CBMCTRL_RESET);
}

unsigned char cbm_pp_read(int f)
{
    return ioctl(f, CBMCTRL_PP_READ);
}

void cbm_pp_write(int f, unsigned char c)
{
    ioctl(f, CBMCTRL_PP_WRITE, c);
}

int cbm_iec_poll(int f)
{
    return ioctl(f, CBMCTRL_IEC_POLL);
}

int cbm_iec_get(int f, int line)
{
    return (ioctl(f, CBMCTRL_IEC_POLL) & line) != 0;
}

void cbm_iec_set(int f, int line)
{
    ioctl(f, CBMCTRL_IEC_SET, line);
}

void cbm_iec_release(int f, int line)
{
    ioctl(f, CBMCTRL_IEC_RELEASE, line);
}

int cbm_iec_wait(int f, int line, int state)
{
    return ioctl(f, CBMCTRL_IEC_WAIT, (line<<8) | state);
}

int cbm_device_status(int f, int drv, char *buf,  int bufsize)
{
    strncpy(buf, "99, DRIVER ERROR,00,00\r", bufsize);
    if(cbm_talk(f, drv, 15) == 0) {
        int bytes_read = read(f, buf, bufsize);
        if(bytes_read == bufsize) {
            bytes_read--;
        }
        buf[bytes_read] = '\0';
        cbm_untalk(f);
    }
    return atoi(buf);
}

int cbm_exec_command(int f, int drv, char *cmd, int len)
{
    int rv;
    rv = cbm_listen(f, drv, 15);
    if(rv == 0) {
        if(len == 0) {
            len = strlen(cmd);
        }
        rv = write(f, cmd, len) != len;
        cbm_unlisten(f);
    }
    return rv;
}
