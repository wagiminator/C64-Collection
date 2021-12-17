/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2009 Arnd Menge <arnd(at)jonnz(dot)de>
*/

#ifndef CBM_MODULE_H
#define CBM_MODULE_H

#if defined(__linux__)
#include <linux/ioctl.h>
#include <linux/types.h>
#else
#if defined(__FreeBSD__)
#include <sys/ioctl.h>
#endif
#endif

#define CBMCTRL_BASE        0xcb

#define CBMCTRL_TALK        _IO(CBMCTRL_BASE, 0)
#define CBMCTRL_LISTEN      _IO(CBMCTRL_BASE, 1)
#define CBMCTRL_UNTALK      _IO(CBMCTRL_BASE, 2)
#define CBMCTRL_UNLISTEN    _IO(CBMCTRL_BASE, 3)
#define CBMCTRL_OPEN        _IO(CBMCTRL_BASE, 4)
#define CBMCTRL_CLOSE       _IO(CBMCTRL_BASE, 5)
#define CBMCTRL_RESET       _IO(CBMCTRL_BASE, 6)
#define CBMCTRL_GET_EOI     _IO(CBMCTRL_BASE, 7)
#define CBMCTRL_CLEAR_EOI   _IO(CBMCTRL_BASE, 8)

#define CBMCTRL_PP_READ     _IO(CBMCTRL_BASE, 10)
#define CBMCTRL_PP_WRITE    _IO(CBMCTRL_BASE, 11)
#define CBMCTRL_IEC_POLL    _IO(CBMCTRL_BASE, 12)
#define CBMCTRL_IEC_SET     _IO(CBMCTRL_BASE, 13)
#define CBMCTRL_IEC_RELEASE _IO(CBMCTRL_BASE, 14)
#define CBMCTRL_IEC_WAIT    _IO(CBMCTRL_BASE, 15)
#define CBMCTRL_IEC_SETRELEASE _IO(CBMCTRL_BASE, 16)

/* linux constants needed by parallel burst */
#define CBMCTRL_PARBURST_READ    _IO(CBMCTRL_BASE, 17)
#define CBMCTRL_PARBURST_WRITE   _IO(CBMCTRL_BASE, 18)
#define CBMCTRL_PARBURST_READ_TRACK        _IO(CBMCTRL_BASE, 19)
#define CBMCTRL_PARBURST_WRITE_TRACK _IO(CBMCTRL_BASE, 20)
#define CBMCTRL_PARBURST_READ_TRACK_VAR    _IO(CBMCTRL_BASE, 21)

/* all values needed by PARBURST_READ_TRACK and PARBURST_WRITE_TRACK */
typedef struct PARBURST_RW_VALUE {
	unsigned char *buffer;
	int length;
} PARBURST_RW_VALUE;

#endif
