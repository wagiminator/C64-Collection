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
** \file libmisc/dynlibusb.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Allow for libusb (0.1) to be loaded dynamically
**        (Currently, this is used on Windows only)
**
****************************************************************/

#ifndef OPENCBM_LIBMISC_DYNLIBUSB_H
#define OPENCBM_LIBMISC_DYNLIBUSB_H

#include <usb.h>

#include "getpluginaddress.h"

#ifndef LIBUSB_APIDECL
# define LIBUSB_APIDECL
#endif

typedef
struct usb_dll_s {
    SHARED_OBJECT_HANDLE shared_object_handle;

    /*
     * these definitions are taken directly from libusb.h. 
     * commented out entries are not currently unused.
     */

    usb_dev_handle * (LIBUSB_APIDECL *open)(struct usb_device *dev);
    int (LIBUSB_APIDECL *close)(usb_dev_handle *dev);
//    int (LIBUSB_APIDECL *get_string)(usb_dev_handle *dev, int index, int langid, char *buf, size_t buflen);
//    int (LIBUSB_APIDECL *get_string_simple)(usb_dev_handle *dev, int index, char *buf, size_t buflen);

//    int (LIBUSB_APIDECL *get_descriptor_by_endpoint)(usb_dev_handle *udev, int ep, unsigned char type, unsigned char index, void *buf, int size);
//    int (LIBUSB_APIDECL *get_descriptor)(usb_dev_handle *udev, unsigned char type, unsigned char index, void *buf, int size);

    int (LIBUSB_APIDECL *bulk_write)(usb_dev_handle *dev, int ep, const char *bytes, int size, int timeout);
    int (LIBUSB_APIDECL *bulk_read)(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);
//    int (LIBUSB_APIDECL *interrupt_write)(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);
//    int (LIBUSB_APIDECL *interrupt_read)(usb_dev_handle *dev, int ep, char *bytes, int size, int timeout);
    int (LIBUSB_APIDECL *control_msg)(usb_dev_handle *dev, int requesttype, int request, int value, int index, char *bytes, int size, int timeout);
    int (LIBUSB_APIDECL *set_configuration)(usb_dev_handle *dev, int configuration);
    int (LIBUSB_APIDECL *claim_interface)(usb_dev_handle *dev, int interface);
    int (LIBUSB_APIDECL *release_interface)(usb_dev_handle *dev, int interface);
//    int (LIBUSB_APIDECL *set_altinterface)(usb_dev_handle *dev, int alternate);
    int (LIBUSB_APIDECL *resetep)(usb_dev_handle *dev, unsigned int ep);
    int (LIBUSB_APIDECL *clear_halt)(usb_dev_handle *dev, unsigned int ep);
//    int (LIBUSB_APIDECL *reset)(usb_dev_handle *dev);

    char * (LIBUSB_APIDECL *strerror)(void);

    void (LIBUSB_APIDECL *init)(void);
//    void (LIBUSB_APIDECL *set_debug)(int level);
    int (LIBUSB_APIDECL *find_busses)(void);
    int (LIBUSB_APIDECL *find_devices)(void);
    struct usb_device * (LIBUSB_APIDECL *device)(usb_dev_handle *dev);
    struct usb_bus * (LIBUSB_APIDECL *get_busses)(void);

} usb_dll_t;

extern usb_dll_t usb;

extern int dynlibusb_init(void);
extern void dynlibusb_uninit(void);

#endif /* #ifndef OPENCBM_LIBMISC_DYNLIBUSB_H */
