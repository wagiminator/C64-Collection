/*
 * Utility routines for working with xum1541 devices
 *
 * Copyright 2011 Nate Lawson <nate@root.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

//#include "xum1541.h"
// XXX
#define USB_TIMEOUT         ((int)(XUM1541_TIMEOUT * 1100))
#define MAX_ALLOWED_XUM1541_SERIALNUM 255
#define XUM1541_VID                 0x16d0
#define XUM1541_PID                 0x0504
#define XUM1541_VERSION             7

#include "usb.h"

#include "util.h"

// XXX Fix for Linux/Mac build, should be moved
#ifndef LIBUSB_PATH_MAX
#define LIBUSB_PATH_MAX 512
#endif

static int
usbGetStringAscii(usb_dev_handle *dev, int index, int langid,
    char *buf, int buflen)
{
    char buffer[256];
    int rval, i;

    rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR,
        (USB_DT_STRING << 8) + index, langid,
        buffer, sizeof(buffer), 1000);
    if (rval < 0)
        return rval;

    if (buffer[1] != USB_DT_STRING)
        return 0;
    if ((unsigned char)buffer[0] < rval)
        rval = (unsigned char)buffer[0];

    rval /= 2;
    /* lossy conversion to ISO Latin1 */
    for (i = 1; i < rval; i++) {
        if (i > buflen)  /* destination buffer overflow */
            break;
        buf[i-1] = buffer[2 * i];
        if (buffer[2 * i + 1] != 0)  /* outside of ISO Latin1 range */
            buf[i-1] = '?';
    }
    buf[i-1] = 0;
    return i - 1;
}

void
verbose_print(char *msg, ...)
{
    va_list args;
    extern int verbose;

    if (!verbose)
        return;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
}

// Cleanup after a failure
static void
xum1541_cleanup(usb_dev_handle **usbHandle, char *msg, ...)
{
    va_list args;

    if (msg != NULL) {
        va_start(args, msg);
        fprintf(stderr, msg, args);
        va_end(args);
    }
    if (*usbHandle != NULL)
        usb_close(*usbHandle);
    *usbHandle = NULL;
}

typedef int (*DeviceValidateFn_t)(struct usb_device *dev, void *arg);

// 0: skip, 1: weak match, 2: claim
#define USBDEV_IGNORE   0
#define USBDEV_WEAK     1
#define USBDEV_CLAIM    2

// XXX need to reinit each time
static int leastSerial = MAX_ALLOWED_XUM1541_SERIALNUM + 1;


static int
xum1541_validate_device(struct usb_device *dev, void *arg)
{
    static char xumProduct[] = "xum1541"; // Start of USB product string id
    usb_dev_handle *handle;
    char string[256];
    int len, serialnum, PortNumber;

    // Fetch serial of any specific device instance to open
    PortNumber = *(int *)arg;

    // First, find our vendor and product id
    if (dev->descriptor.idVendor != XUM1541_VID ||
        dev->descriptor.idProduct != XUM1541_PID)
        return USBDEV_IGNORE;

    verbose_print("    found xu/xum1541 version %04x, device %s\n",
        dev->descriptor.bcdDevice, dev->filename);
    if ((handle = usb_open(dev)) == NULL) {
        fprintf(stderr, "error: Cannot open USB device: %s\n",
            usb_strerror());
        return USBDEV_IGNORE;
    }

    // Get device product name and try to match against "xum1541".
    len = usbGetStringAscii(handle, dev->descriptor.iProduct,
        0x0409, string, sizeof(string) - 1);
    if (len < 0) {
        xum1541_cleanup(&handle,
            "error: cannot query product name: %s\n", usb_strerror());
        return USBDEV_IGNORE;
    }
    string[len] = '\0';
    if (len < sizeof(xumProduct) - 1 || strstr(string, xumProduct) == NULL) {
        xum1541_cleanup(&handle,
    "error: xum1541 device \"%s\" doesn't match product string\n", xumProduct);
        return USBDEV_IGNORE;
    }
    verbose_print("    xum1541 name: %s\n", string);

    len = usbGetStringAscii(handle,
        dev->descriptor.iSerialNumber, 0x0409,
        string, sizeof(string) - 1);
    if (len < 0 && PortNumber != 0) {
        // we need the serial number, when PortNumber is not 0
        xum1541_cleanup(&handle,
            "error: cannot query serial number: %s\n",
            usb_strerror());
        return USBDEV_IGNORE;
    }
    xum1541_cleanup(&handle, NULL);

    serialnum = 0;
    if (len > 0 && len <= 3) {
        string[len] = '\0';
        serialnum = atoi(string);
    }
    if (PortNumber != serialnum) {
        // keep in mind the handle, if the device's
        // serial number is less than previous ones
        if (serialnum < leastSerial) {
            leastSerial = serialnum;
            return USBDEV_WEAK;
        } else
            return USBDEV_IGNORE;
    }

    verbose_print("    xum1541 serial number: %3u\n", serialnum);
    return USBDEV_CLAIM;
}

// USB bus enumeration
static int
xum1541_enumerate(usb_dev_handle **usbHandle, DeviceValidateFn_t validateFn,
    void *arg)
{
    struct usb_bus *bus;
    struct usb_device *dev, *preferredDefaultHandle;
    int ret;

    assert(usbHandle != NULL);
    assert(validateFn != NULL);
    verbose_print("scanning usb ...\n");

    // Enumerate all devices (again)
    usb_find_busses();
    usb_find_devices();

    /*
     * usb_find_devices() sets errno if some devices don't reply 100%
     * correctly. We ignore this as this has nothing to do with our device.
     */
    errno = 0;

    *usbHandle = NULL;
    preferredDefaultHandle = NULL;
    for (bus = usb_get_busses(); bus; bus = bus->next) {
        verbose_print("scanning bus %s\n", bus->dirname);
        for (dev = bus->devices; dev; dev = dev->next) {
            verbose_print("  device %04x:%04x at %s\n",
                dev->descriptor.idVendor, dev->descriptor.idProduct,
                dev->filename);
            ret = validateFn(dev, arg);
            if (ret == USBDEV_IGNORE) {
                continue;
            } else {
                preferredDefaultHandle = dev;
                if (ret == USBDEV_CLAIM)
                    break;
            }
        }
    }

    // No matching device found
    if (preferredDefaultHandle == NULL) {
        verbose_print("no xum1541 device found\n");
        return -1;
    }

    // If any device was selected (even weakly), open and return its handle.
    if ((*usbHandle = usb_open(preferredDefaultHandle)) == NULL) {
        fprintf(stderr, "error: Cannot reopen USB device: %s\n",
            usb_strerror());
        return -1;
    }
    return 0;
}

int
xum1541_get_model_version(usb_dev_handle *handle, int *model, int *version)
{
    struct usb_device *dev;
    int revision;

    dev = usb_device(handle);
    if (!dev)
        return -1;
    // XXX endianness?
    revision = dev->descriptor.bcdDevice;
    *model = revision >> 8;
    *version = revision & 0xff;
    return 0;
}

usb_dev_handle *
xum1541_find_device(int PortNumber, char *devNameBuf, int nameBufSize)
{
    usb_dev_handle *usbHandle;
    struct usb_device *dev;
    int len, ret;

    ret = xum1541_enumerate(&usbHandle, xum1541_validate_device, &PortNumber);
    if (ret != 0 || usbHandle == NULL)
        return NULL;
    dev = usb_device(usbHandle);
    if (!dev)
        return NULL;
    len = usbGetStringAscii(usbHandle, dev->descriptor.iProduct,
        0x0409, devNameBuf, nameBufSize - 1);
    if (len < 0) {
        xum1541_cleanup(&usbHandle,
            "error: cannot query product name: %s\n", usb_strerror());
        return NULL;
    }
    devNameBuf[len] = '\0';

#if 0
    // Select first and only device configuration.
    if (usb_set_configuration(usbHandle, 1) != 0) {
        xum1541_cleanup(&usbHandle, "USB error: %s\n", usb_strerror());
        return NULL;
    }

    /*
     * Get exclusive access to interface 0.
     * After this point, do cleanup using xum1541_close() instead of
     * xum1541_cleanup().
     */
    if (usb_claim_interface(usbHandle, 0) != 0) {
        xum1541_cleanup(&usbHandle, "USB error: %s\n", usb_strerror());
        return NULL;
    }
#endif

    return usbHandle;
}
