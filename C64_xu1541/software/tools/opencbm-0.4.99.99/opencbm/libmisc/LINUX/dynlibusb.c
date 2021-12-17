/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2010 Spiro Trikaliotis
 *
*/

/*! ************************************************************** 
** \file libmisc/LINUX/dynlibusb.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Allow for libusb (0.1) to be loaded dynamically
**        (Currently, this is used on Windows only)
****************************************************************/

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "opencbm.h"

#include "arch.h"
#include "dynlibusb.h"
#include "getpluginaddress.h"

usb_dll_t usb = {
    .shared_object_handle = NULL,
    .open = usb_open, 
    .close = usb_close, 
    .bulk_write = usb_bulk_write,
    .bulk_read = usb_bulk_read,
    .control_msg = usb_control_msg,
    .set_configuration = usb_set_configuration,
    .claim_interface = usb_claim_interface,
    .release_interface = usb_release_interface,
    .clear_halt = usb_clear_halt,
    .strerror = usb_strerror, 
    .init = usb_init, 
    .find_busses = usb_find_busses, 
    .find_devices = usb_find_devices, 
    .device = usb_device,
    .get_busses = usb_get_busses
};

int dynlibusb_init(void) {
    int error = 0;

    return error;
}

void dynlibusb_uninit(void) {
}
