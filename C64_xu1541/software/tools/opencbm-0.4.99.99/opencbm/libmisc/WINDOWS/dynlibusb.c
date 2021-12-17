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
** \file libmisc/WINDOWS/dynlibusb.h \n
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

usb_dll_t usb = { NULL };

int dynlibusb_init(void) {
    int error = 1;

    do {
        usb.shared_object_handle = plugin_load("libusb0.dll");
        if ( ! usb.shared_object_handle ) {
            break;
        }

#define READ(_x) \
    usb._x = plugin_get_address(usb.shared_object_handle, "usb_" ## #_x); \
    if (usb._x == NULL) { \
        break; \
    }

        READ(open);
        READ(close);
//        READ(get_string);
//        READ(get_string_simple);
//        READ(get_descriptor_by_endpoint);
//        READ(get_descriptor);
        READ(bulk_write);
        READ(bulk_read);
//        READ(interrupt_write);
//        READ(interrupt_read);
        READ(control_msg);
        READ(set_configuration);
        READ(claim_interface);
        READ(release_interface);
//        READ(set_altinterface);
        READ(resetep);
        READ(clear_halt);
//        READ(reset);
        READ(strerror);
        READ(init);
//        READ(set_debug);
        READ(find_busses);
        READ(find_devices);
        READ(device);
        READ(get_busses);

        error = 0;
    } while (0);

    return error;
}

void dynlibusb_uninit(void) {

    do {
        if (usb.shared_object_handle == NULL) {
            break;
        }

        plugin_unload(usb.shared_object_handle);

        memset(&usb, 0, sizeof usb);

    } while (0);

}
