/*
 * Copyright (c) 2009-2010 Nate Lawson <nate@root.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#ifndef XUM1541_H
#define XUM1541_H

#include <usb.h>

#include "opencbm.h"
#include "xum1541_types.h"

/*
 * Compile-time assert to make sure CBM_FILE is large enough.
 * Perhaps this should be in the global opencbm.h
 */
#ifndef CTASSERT
#define CTASSERT(x)         _CTASSERT(x, __LINE__)
#define _CTASSERT(x, y)     __CTASSERT(x, y)
#define __CTASSERT(x, y)    typedef char __assert ## y[(x) ? 1 : -1]
#endif

CTASSERT(sizeof(CBM_FILE) >= sizeof(usb_dev_handle *));

/*
 * Make our control transfer timeout 10% later than the device itself
 * times out. This is used for both the INIT and RESET messages since
 * INIT can do its own reset if it finds the drive in the middle of
 * a previous aborted transaction.
 */
#define USB_TIMEOUT         ((int)(XUM1541_TIMEOUT * 1100))

// libusb value for "wait forever" (signed int)
#define LIBUSB_NO_TIMEOUT   0x7fffffff

// the maximum value for all allowed xum1541 serial numbers
#define MAX_ALLOWED_XUM1541_SERIALNUM 255

// Disk/tape mode
#define DeviceDriveMode_NoTapeSupport  -1 // Firmware has no tape support
#define DeviceDriveMode_Uninit          0 // Uninitialized
#define DeviceDriveMode_Disk            1 // Disk drive mode (only communication to disk drives allowed)
#define DeviceDriveMode_Tape            2 // Tape drive mode (only communication to tape drive allowed)

const char *xum1541_device_path(int PortNumber);
int xum1541_init(usb_dev_handle **HandleXum1541, int PortNumber);
void xum1541_close(usb_dev_handle *HandleXum1541);
int xum1541_control_msg(usb_dev_handle *HandleXum1541, unsigned int cmd);
int xum1541_ioctl(usb_dev_handle *HandleXum1541, unsigned int cmd,
    unsigned int addr, unsigned int secaddr);

// Read/write data in normal CBM and speeder protocol modes
int xum1541_write(usb_dev_handle *HandleXum1541, unsigned char mode,
    const unsigned char *data, size_t size);
int xum1541_write_ext(usb_dev_handle *HandleXum1541, unsigned char mode,
    const unsigned char *data, size_t size, int *Status, int *BytesWritten);
int xum1541_read(usb_dev_handle *HandleXum1541, unsigned char mode,
    unsigned char *data, size_t size);
int xum1541_read_ext(usb_dev_handle *HandleXum1541, unsigned char mode,
    unsigned char *data, size_t size, int *Status, int *BytesRead);

int xum1541_tap_break(usb_dev_handle *HandleXum1541);

#endif // XUM1541_H
