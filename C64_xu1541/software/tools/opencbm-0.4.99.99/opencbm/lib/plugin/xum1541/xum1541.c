/*
 * xum1541 driver for bulk and control messages
 * Copyright 2009-2010 Nate Lawson <nate@root.org>
 * Copyright 2010 Spiro Trikaliotis
 * Copyright 2012 Arnd Menge
 *
 * Incorporates some code from the xu1541 driver by:
 * Copyright 2007 Till Harbaum <till@harbaum.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

/*! **************************************************************
** \file lib/plugin/xum1541/xum1541.c \n
** \author Nate Lawson \n
** \n
** \brief libusb-based xum1541 access routines
****************************************************************/

// This XUM1541 plugin has tape support.
#define TAPE_SUPPORT 1

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "opencbm.h"

#include "arch.h"
#include "dynlibusb.h"
#include "getpluginaddress.h"
#include "xum1541.h"

// XXX Fix for Linux/Mac build, should be moved
#ifndef LIBUSB_PATH_MAX
#define LIBUSB_PATH_MAX 512
#endif

static int debug_level = -1; /*!< \internal \brief the debugging level for debugging output */

unsigned char DeviceDriveMode; // Temporary disk/tape mode hack until usb device handle context is there.

/*! \internal \brief Output debugging information for the xum1541

 \param level
   The output level; output will only be produced if this level is less or equal the debugging level

 \param msg
   The printf() style message to be output
*/
static void
xum1541_dbg(int level, char *msg, ...)
{
    va_list argp;

    /* determine debug mode if not yet known */
    if (debug_level == -1) {
        char *val = getenv("XUM1541_DEBUG");
        if (val)
            debug_level = atoi(val);
    }

    if (level <= debug_level) {
        fprintf(stderr, "[XUM1541] ");
        va_start(argp, msg);
        vfprintf(stderr, msg, argp);
        va_end(argp);
        fprintf(stderr, "\n");
    }
}

/*! \internal \brief Get a char* string from the device's Unicode descriptors
    Some data will be lost in this conversion, but we are ok with that.

 \param dev
    libusb device handle

 \param index
    Descriptor string index

 \param langid
    Language code

 \param buf
    Where to store the string. The result is nul-terminated.

 \param buflen
    Length of the output buffer.

 \return
    Returns the length of the string read or 0 on error.
*/
static int
usbGetStringAscii(usb_dev_handle *dev, int index, int langid,
    char *buf, int buflen)
{
    char buffer[256];
    int rval, i;

    rval = usb.control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR,
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

// Cleanup after a failure
static void
xum1541_cleanup(usb_dev_handle **HandleXum1541, char *msg, ...)
{
    va_list args;

    if (msg != NULL) {
        va_start(args, msg);
        fprintf(stderr, msg, args);
        va_end(args);
    }
    if (*HandleXum1541 != NULL)
        usb.close(*HandleXum1541);
    *HandleXum1541 = NULL;
}

// USB bus enumeration
static void
xum1541_enumerate(usb_dev_handle **HandleXum1541, int PortNumber)
{
    static char xumProduct[] = "xum1541"; // Start of USB product string id
    static int prodLen = sizeof(xumProduct) - 1;
    struct usb_bus *bus;
    struct usb_device *dev, *preferredDefaultHandle;
    char string[256];
    int len, serialnum, leastserial;

    if (PortNumber < 0 || PortNumber > MAX_ALLOWED_XUM1541_SERIALNUM) {
        // Normalise the Portnumber for invalid values
        PortNumber = 0;
    }

    xum1541_dbg(0, "scanning usb ...");

    usb.init();
    usb.find_busses();
    usb.find_devices();

    /* usb.find_devices sets errno if some devices don't reply 100% correct. */
    /* make lib ignore this as this has nothing to do with our device */
    errno = 0;

    *HandleXum1541 = NULL;
    preferredDefaultHandle = NULL;
    leastserial = MAX_ALLOWED_XUM1541_SERIALNUM + 1;
    for (bus = usb.get_busses(); !*HandleXum1541 && bus; bus = bus->next) {
        xum1541_dbg(1, "scanning bus %s", bus->dirname);
        for (dev = bus->devices; !*HandleXum1541 && dev; dev = dev->next) {
            xum1541_dbg(1, "device %04x:%04x at %s",
                dev->descriptor.idVendor, dev->descriptor.idProduct,
                dev->filename);

            // First, find our vendor and product id
            if (dev->descriptor.idVendor != XUM1541_VID ||
                dev->descriptor.idProduct != XUM1541_PID)
                continue;

            xum1541_dbg(0, "found xu/xum1541 version %04x on bus %s, device %s",
                dev->descriptor.bcdDevice, bus->dirname, dev->filename);
            if ((*HandleXum1541 = usb.open(dev)) == NULL) {
                fprintf(stderr, "error: Cannot open USB device: %s\n",
                    usb.strerror());
                continue;
            }

            // Get device product name and try to match against "xum1541".
            // If no match, it could be an xum1541 so don't report an error.
            len = usbGetStringAscii(*HandleXum1541, dev->descriptor.iProduct,
                0x0409, string, sizeof(string) - 1);
            if (len < 0) {
                xum1541_cleanup(HandleXum1541,
                    "error: cannot query product name: %s\n", usb.strerror());
                continue;
            }
            string[len] = '\0';
            if (len < prodLen || strstr(string, xumProduct) == NULL) {
                xum1541_cleanup(HandleXum1541, NULL);
                continue;
            }
            xum1541_dbg(0, "xum1541 name: %s", string);

            len = usbGetStringAscii(*HandleXum1541,
                dev->descriptor.iSerialNumber, 0x0409,
                string, sizeof(string) - 1);
            if (len < 0 && PortNumber != 0){
                // we need the serial number, when PortNumber is not 0
                xum1541_cleanup(HandleXum1541,
                    "error: cannot query serial number: %s\n",
                    usb.strerror());
                continue;
            }
            serialnum = 0;
            if (len > 0 && len <=3 ) {
                string[len] = '\0';
                serialnum = atoi(string);
            }
            if (PortNumber != serialnum) {
                // keep in mind the handle, if the device's
                // serial number is less than previous ones
                if(serialnum < leastserial) {
                    leastserial = serialnum;
                    preferredDefaultHandle = dev;
                }
                xum1541_cleanup(HandleXum1541, NULL);
                continue;
            }

            xum1541_dbg(0, "xum1541 serial number: %3u", serialnum);
            return;
        }
    }
    // if no default device was found because only specific devices were present,
    // determine the default device from the specific ones and open it
    if(preferredDefaultHandle != NULL) {
        if ((*HandleXum1541 = usb.open(preferredDefaultHandle)) == NULL) {
            fprintf(stderr, "error: Cannot reopen USB device: %s\n",
                usb.strerror());
        }
    }
}

// Check for a firmware version compatible with this plugin
static int
xum1541_check_version(int version)
{
    xum1541_dbg(0, "firmware version %d, library version %d", version,
        XUM1541_VERSION);
    if (version < XUM1541_VERSION) {
        fprintf(stderr, "xum1541 firmware version too low (%d < %d)\n",
            version, XUM1541_VERSION);
        fprintf(stderr, "please update your xum1541 firmware\n");
        return -1;
    } else if (version > XUM1541_VERSION) {
        fprintf(stderr, "xum1541 firmware version too high (%d > %d)\n",
            version, XUM1541_VERSION);
        fprintf(stderr, "please update your OpenCBM plugin\n");
        return -1;
    }
    return 0;
}

/*! \brief Query unique identifier for the xum1541 device
  This function tries to find an unique identifier for the xum1541 device.

  \param PortNumber
   The device's serial number to search for also. It is not considered, if set to 0.

  \return
    0 on success, -1 on error. On error, the handle is cleaned up if it
    was already active.

  \remark
    On success, xum1541_handle contains a valid handle to the xum1541 device.
    In this case, the device configuration has been set and the interface
    been claimed. xum1541_close() should be called when the user is done
    with it.
*/
const char *
xum1541_device_path(int PortNumber)
{
#define PREFIX_OFFSET   (sizeof("libusb/xum1541:") - 1)
    usb_dev_handle *HandleXum1541;
    static char dev_path[PREFIX_OFFSET + LIBUSB_PATH_MAX] = "libusb/xum1541:";

    dev_path[PREFIX_OFFSET + 1] = '\0';
    xum1541_enumerate(&HandleXum1541, PortNumber);

    if (HandleXum1541 != NULL) {
        strcpy(dev_path, (usb.device(HandleXum1541))->filename);
        xum1541_close(HandleXum1541);
    } else {
        fprintf(stderr, "error: no xum1541 device found\n");
    }

    return dev_path;
}
#undef PREFIX_OFFSET

static int
xum1541_clear_halt(usb_dev_handle *handle)
{
    int ret;

    ret = usb.clear_halt(handle, XUM_BULK_IN_ENDPOINT | USB_ENDPOINT_IN);
    if (ret != 0) {
        fprintf(stderr, "USB clear halt request failed for in ep: %s\n",
            usb.strerror());
        return -1;
    }
    ret = usb.clear_halt(handle, XUM_BULK_OUT_ENDPOINT);
    if (ret != 0) {
        fprintf(stderr, "USB clear halt request failed for out ep: %s\n",
            usb.strerror());
        return -1;
    }

#ifdef __APPLE__
    /*
     * The Darwin libusb implementation calls ClearPipeStall() in
     * usb_clear_halt(). While that clears the host data toggle and resets
     * its endpoint, it does not send the CLEAR_FEATURE(halt) control
     * request to the device. The ClearPipeStallBothEnds() function does
     * do this.
     *
     * We manually send this control request here on Mac systems.
     */
    ret = usb.control_msg(handle, USB_RECIP_ENDPOINT, USB_REQ_CLEAR_FEATURE,
        0, XUM_BULK_IN_ENDPOINT | USB_ENDPOINT_IN, NULL, 0, USB_TIMEOUT);
    if (ret != 0) {
        fprintf(stderr, "USB clear control req failed for in ep: %s\n",
            usb.strerror());
        return -1;
    }
    ret = usb.control_msg(handle, USB_RECIP_ENDPOINT, USB_REQ_CLEAR_FEATURE,
        0, XUM_BULK_OUT_ENDPOINT, NULL, 0, USB_TIMEOUT);
    if (ret != 0) {
        fprintf(stderr, "USB clear control req failed for out ep: %s\n",
            usb.strerror());
        return -1;
    }
#endif // __APPLE__

    return 0;
}

/*! \brief Initialize the xum1541 device
  This function tries to find and identify the xum1541 device.

  \param HandleXum1541
   Pointer to a XUM1541_HANDLE which will contain the file handle of the USB device.

  \param PortNumber
   The device's serial number to search for also. It is not considered, if set to 0.

  \return
    0 on success, -1 on error. On error, the handle is cleaned up if it
    was already active.

  \remark
    On success, xum1541_handle contains a valid handle to the xum1541 device.
    In this case, the device configuration has been set and the interface
    been claimed. xum1541_close() should be called when the user is done
    with it.
*/
int
xum1541_init(usb_dev_handle **HandleXum1541, int PortNumber)
{
    unsigned char devInfo[XUM_DEVINFO_SIZE], devStatus;
    int len;

    // Place after "xum1541_usb_handle" allocation:
    /*uh->*/DeviceDriveMode = DeviceDriveMode_Uninit;

    xum1541_enumerate(HandleXum1541, PortNumber);

    if (*HandleXum1541 == NULL) {
        fprintf(stderr, "error: no xum1541 device found\n");
        return -1;
    }

    // Select first and only device configuration.
    if (usb.set_configuration(*HandleXum1541, 1) != 0) {
        xum1541_cleanup(HandleXum1541, "USB error: %s\n", usb.strerror());
        return -1;
    }

    /*
     * Get exclusive access to interface 0.
     * After this point, do cleanup using xum1541_close() instead of
     * xum1541_cleanup().
     */
    if (usb.claim_interface(*HandleXum1541, 0) != 0) {
        xum1541_cleanup(HandleXum1541, "USB error: %s\n", usb.strerror());
        return -1;
    }

    // Check the basic device info message for firmware version
    memset(devInfo, 0, sizeof(devInfo));
    len = usb.control_msg(*HandleXum1541, USB_TYPE_CLASS | USB_ENDPOINT_IN,
        XUM1541_INIT, 0, 0, (char*)devInfo, sizeof(devInfo), USB_TIMEOUT);
    if (len < 2) {
        fprintf(stderr, "USB request for XUM1541 info failed: %s\n",
            usb.strerror());
        xum1541_close(*HandleXum1541);
        return -1;
    }
    if (xum1541_check_version(devInfo[0]) != 0) {
        xum1541_close(*HandleXum1541);
        return -1;
    }
    if (len >= 4) {
        xum1541_dbg(0, "device capabilities %02x status %02x",
            devInfo[1], devInfo[2]);
    }

    // Check for the xum1541's current status. (Not the drive.)
    devStatus = devInfo[2];
    if ((devStatus & XUM1541_DOING_RESET) != 0) {
        fprintf(stderr, "previous command was interrupted, resetting\n");
        // Clear the stalls on both endpoints
        if (xum1541_clear_halt(*HandleXum1541) < 0) {
            xum1541_close(*HandleXum1541);
            return -1;
        }
    }

    //  Enable disk or tape mode.
	if (devInfo[1] & XUM1541_CAP_TAP)
	{
		if (devInfo[2] & XUM1541_TAPE_PRESENT)
		{
			/*uh->*/DeviceDriveMode = DeviceDriveMode_Tape;
            xum1541_dbg(1, "[xum1541_init] Tape supported, tape mode entered.");
		}
		else
		{
			/*uh->*/DeviceDriveMode = DeviceDriveMode_Disk;
            xum1541_dbg(1, "[xum1541_init] Tape supported, disk mode entered.");
		}
	}
	else
	{
		DeviceDriveMode = (unsigned char) DeviceDriveMode_NoTapeSupport;
        xum1541_dbg(1, "[xum1541_init] No tape support.");
	}

    return 0;
}
/*! \brief close the xum1541 device

 \param HandleXum1541
   Pointer to a XUM1541_HANDLE which will contain the file handle of the USB device.

 \remark
    This function releases the interface and closes the xum1541 handle.
*/
void
xum1541_close(usb_dev_handle *HandleXum1541)
{
    int ret;

    xum1541_dbg(0, "Closing USB link");

    ret = usb.control_msg(HandleXum1541, USB_TYPE_CLASS | USB_ENDPOINT_OUT,
        XUM1541_SHUTDOWN, 0, 0, NULL, 0, 1000);
    if (ret < 0) {
        fprintf(stderr,
            "USB request for XUM1541 close failed, continuing: %s\n",
            usb.strerror());
    }
    if (usb.release_interface(HandleXum1541, 0) != 0)
        fprintf(stderr, "USB release intf error: %s\n", usb.strerror());

    if (usb.close(HandleXum1541) != 0)
        fprintf(stderr, "USB close error: %s\n", usb.strerror());
}

/*! \brief  Handle synchronous USB control messages, e.g. for RESET.
    xum1541_ioctl() is used for bulk messages.

 \param HandleXum1541
   A XUM1541_HANDLE which contains the file handle of the USB device.

 \param cmd
   The command to run.

 \return
   Returns the value the USB device sent back.
*/
int
xum1541_control_msg(usb_dev_handle *HandleXum1541, unsigned int cmd)
{
    int nBytes;

    xum1541_dbg(1, "control msg %d", cmd);

    nBytes = usb.control_msg(HandleXum1541, USB_TYPE_CLASS | USB_ENDPOINT_OUT,
        cmd, 0, 0, NULL, 0, USB_TIMEOUT);
    if (nBytes < 0) {
        fprintf(stderr, "USB error in xum1541_control_msg: %s\n",
            usb.strerror());
        exit(-1);
    }

    return nBytes;
}

static int
xum1541_wait_status(usb_dev_handle *HandleXum1541)
{
    int nBytes, deviceBusy, ret;
    unsigned char statusBuf[XUM_STATUSBUF_SIZE];

    xum1541_dbg(2, "xum1541_wait_status checking for status");
    deviceBusy = 1;
    while (deviceBusy) {
        nBytes = usb.bulk_read(HandleXum1541,
            XUM_BULK_IN_ENDPOINT | USB_ENDPOINT_IN,
            (char*)statusBuf, XUM_STATUSBUF_SIZE, LIBUSB_NO_TIMEOUT);
        if (nBytes == XUM_STATUSBUF_SIZE) {
            switch (XUM_GET_STATUS(statusBuf)) {
            case XUM1541_IO_BUSY:
                xum1541_dbg(2, "device busy, waiting");
                break;
            case XUM1541_IO_ERROR:
                fprintf(stderr, "device reports error\n");
                /* FALLTHROUGH */
            case XUM1541_IO_READY:
                deviceBusy = 0;
                break;
            default:
                fprintf(stderr, "unknown status value: %d\n",
                    XUM_GET_STATUS(statusBuf));
                exit(-1);
            }
        } else {
            fprintf(stderr, "USB error in xum1541_wait_status: %s\n",
                usb.strerror());
            exit(-1);
        }
    }

    // Once we have a valid response (done ok), get extended status
    if (XUM_GET_STATUS(statusBuf) == XUM1541_IO_READY)
        ret = XUM_GET_STATUS_VAL(statusBuf);
    else
        ret = -1;

    xum1541_dbg(2, "return val = %x", ret);
    return ret;
}

// Macro to enforce disk/tape mode.
// Checks if xum1541_ioctl/xum1541_read/xum1541_write command is allowed in currently set disk/tape mode.
#define RefuseToWorkInWrongMode \
    {                                                                                                    \
        if (/*uh->*/DeviceDriveMode == DeviceDriveMode_Uninit)                                               \
        {                                                                                                \
            xum1541_dbg(1, "[RefuseToWorkInWrongMode] cmd blocked - No disk or tape mode set.");         \
            return XUM1541_Error_NoDiskTapeMode;                                                         \
        }                                                                                                \
                                                                                                         \
        if (isTapeCmd)                                                                                   \
        {                                                                                                \
            if (/*uh->*/DeviceDriveMode == DeviceDriveMode_NoTapeSupport)                                \
            {                                                                                            \
                xum1541_dbg(1, "[RefuseToWorkInWrongMode] cmd blocked - Firmware has no tape support."); \
                return XUM1541_Error_NoTapeSupport;                                                      \
            }                                                                                            \
                                                                                                         \
            if (/*uh->*/DeviceDriveMode == DeviceDriveMode_Disk)                                             \
            {                                                                                            \
                xum1541_dbg(1, "[RefuseToWorkInWrongMode] cmd blocked - Tape cmd in disk mode.");        \
                return XUM1541_Error_TapeCmdInDiskMode;                                                  \
            }                                                                                            \
        }                                                                                                \
        else /*isDiskCmd*/                                                                               \
        {                                                                                                \
            if (/*uh->*/DeviceDriveMode == DeviceDriveMode_Tape)                                             \
            {                                                                                            \
                xum1541_dbg(1, "[RefuseToWorkInWrongMode] cmd blocked - Disk cmd in tape mode.");        \
                return XUM1541_Error_DiskCmdInTapeMode;                                                  \
            }                                                                                            \
        }                                                                                                \
    }

/*! \brief Perform an ioctl on the xum1541, which is any command other than
    read/write or special device management commands such as INIT and RESET.

 \param HandleXum1541
   A XUM1541_HANDLE which contains the file handle of the USB device.

 \param cmd
   The command to run.

 \param addr
   The IEC device to use or 0 if not needed.

 \param secaddr
   The IEC secondary address to use or 0 if not needed.

 \return
   Returns the device status byte, which is 0 if the command does not
   have a return value. For some commands, the status byte gives
   info from the device such as the active IEC lines.
*/
int
xum1541_ioctl(usb_dev_handle *HandleXum1541, unsigned int cmd, unsigned int addr, unsigned int secaddr)
{
    int nBytes, ret;
    unsigned char cmdBuf[XUM_CMDBUF_SIZE];
    BOOL isTapeCmd = ((XUM1541_TAP_MOTOR_ON <= cmd) && (cmd <= XUM1541_TAP_MOTOR_OFF));

    xum1541_dbg(1, "ioctl %d for device %d, sub %d", cmd, addr, secaddr);

    RefuseToWorkInWrongMode; // Check if command allowed in current disk/tape mode.

    cmdBuf[0] = (unsigned char)cmd;
    cmdBuf[1] = (unsigned char)addr;
    cmdBuf[2] = (unsigned char)secaddr;
    cmdBuf[3] = 0;

    // Send the 4-byte command block
    nBytes = usb.bulk_write(HandleXum1541,
        XUM_BULK_OUT_ENDPOINT | USB_ENDPOINT_OUT,
        (char *)cmdBuf, sizeof(cmdBuf), LIBUSB_NO_TIMEOUT);
    if (nBytes < 0) {
        fprintf(stderr, "USB error in xum1541_ioctl cmd: %s\n",
            usb.strerror());
        exit(-1);
    }

    // If we have a valid response, return extended status
    ret = xum1541_wait_status(HandleXum1541);
    xum1541_dbg(2, "return val = %x", ret);
    return ret;
}

/*! \brief Send tape operations abort command to the xum1541 device

 \param HandleXum1541
   A XUM1541_HANDLE which contains the file handle of the USB device.

 \return
   Returns the value the USB device sent back.
*/
int
xum1541_tap_break(usb_dev_handle *HandleXum1541)
{
    BOOL isTapeCmd = TRUE;
    RefuseToWorkInWrongMode; // Check if command allowed in current disk/tape mode.

    xum1541_dbg(1, "[xum1541_tap_break] Sending tape break command.");

    return xum1541_control_msg(HandleXum1541, XUM1541_TAP_BREAK);
}

/*! \brief Write data to the xum1541 device

 \param HandleXum1541
   A XUM1541_HANDLE which contains the file handle of the USB device.

 \param mode
    Drive protocol to use to read the data from the device (e.g,
    XUM1541_CBM is normal IEC wire protocol).

 \param data
    Pointer to buffer which contains the data to be written to the xum1541

 \param size
    The number of bytes to write to the xum1541

 \return
    The number of bytes actually written, 0 on device error. If there is a
    fatal error, returns -1.
*/
int
xum1541_write(usb_dev_handle *HandleXum1541, unsigned char modeFlags, const unsigned char *data, size_t size)
{
    int wr, mode, ret;
    size_t bytesWritten, bytes2write;
    unsigned char cmdBuf[XUM_CMDBUF_SIZE];
    BOOL isTapeCmd = ((modeFlags == XUM1541_TAP) || (modeFlags == XUM1541_TAP_CONFIG));

    mode = modeFlags & 0xf0;
    xum1541_dbg(1, "write %d %d bytes from address %p flags %x",
        mode, size, data, modeFlags & 0x0f);

    RefuseToWorkInWrongMode; // Check if command allowed in current disk/tape mode.

    // Send the write command
    cmdBuf[0] = XUM1541_WRITE;
    cmdBuf[1] = modeFlags;
    cmdBuf[2] = size & 0xff;
    cmdBuf[3] = (size >> 8) & 0xff;
    wr = usb.bulk_write(HandleXum1541,
        XUM_BULK_OUT_ENDPOINT | USB_ENDPOINT_OUT,
        (char *)cmdBuf, sizeof(cmdBuf), LIBUSB_NO_TIMEOUT);
    if (wr < 0) {
        fprintf(stderr, "USB error in write cmd: %s\n",
            usb.strerror());
        return -1;
    }

    bytesWritten = 0;
    while (bytesWritten < size) {
        bytes2write = size - bytesWritten;
        if (bytes2write > XUM_MAX_XFER_SIZE)
            bytes2write = XUM_MAX_XFER_SIZE;
        wr = usb.bulk_write(HandleXum1541,
            XUM_BULK_OUT_ENDPOINT | USB_ENDPOINT_OUT,
            (char *)data, bytes2write, LIBUSB_NO_TIMEOUT);
        if (wr < 0) {
            if (isTapeCmd)
            {
                if (usb.resetep(HandleXum1541, XUM_BULK_OUT_ENDPOINT | USB_ENDPOINT_OUT) < 0)
                    fprintf(stderr, "USB reset ep request failed for out ep (tape stall): %s\n", usb.strerror());
                if (usb.control_msg(HandleXum1541, USB_RECIP_ENDPOINT, USB_REQ_CLEAR_FEATURE, 0, XUM_BULK_OUT_ENDPOINT, NULL, 0, USB_TIMEOUT) < 0)
                    fprintf(stderr, "USB error in xum1541_control_msg (tape stall): %s\n", usb.strerror());
                return bytesWritten;
            }
            fprintf(stderr, "USB error in write data: %s\n",
                usb.strerror());
            return -1;
        } else if (wr > 0)
            xum1541_dbg(2, "wrote %d bytes", wr);

        data += wr;
        bytesWritten += wr;

        /*
         * If we wrote less than we requested (or 0), the transfer is done
         * even if we had more data to write still.
         */
        if (wr < (int)bytes2write)
            break;
    }

    // If this is the CBM protocol, wait for the status message.
    if (mode == XUM1541_CBM) {
        ret = xum1541_wait_status(HandleXum1541);
        if (ret >= 0)
            xum1541_dbg(2, "wait done, extended status %d", ret);
        else
            xum1541_dbg(2, "wait done with error");
        bytesWritten = ret;
    }

    xum1541_dbg(2, "write done, got %d bytes", bytesWritten);
    return bytesWritten;
}

/*! \brief Wrapper for xum1541_write() forcing xum1541_wait_status(), with additional parameters:

 \param Status
   The return status.

 \param BytesWritten
   The number of bytes written.

 \return
     1 : Finished successfully.
    <0 : Fatal error.
*/

int
xum1541_write_ext(usb_dev_handle *HandleXum1541, unsigned char modeFlags, const unsigned char *data, size_t size, int *Status, int *BytesWritten)
{
    xum1541_dbg(1, "[xum1541_write_ext]");
    *BytesWritten = xum1541_write(HandleXum1541, modeFlags, data, size);
    if (*BytesWritten < 0)
        return *BytesWritten;
    xum1541_dbg(2, "[xum1541_write_ext] BytesWritten = %d", *BytesWritten);
    *Status = xum1541_wait_status(HandleXum1541);
    xum1541_dbg(2, "[xum1541_write_ext] Status = %d", *Status);
    return 1;
}

/*! \brief Wrapper for xum1541_read() forcing xum1541_wait_status(), with additional parameters:

 \param Status
   The return status.

 \param BytesRead
   The number of bytes read.

 \return
     1 : Finished successfully.
    <0 : Fatal error.
*/

int
xum1541_read_ext(usb_dev_handle *HandleXum1541, unsigned char mode, unsigned char *data, size_t size, int *Status, int *BytesRead)
{
    xum1541_dbg(1, "[xum1541_read_ext]");
    *BytesRead = xum1541_read(HandleXum1541, mode, data, size);
    if (*BytesRead < 0)
        return *BytesRead;
    xum1541_dbg(2, "[xum1541_read_ext] BytesRead = %d", *BytesRead);
    *Status = xum1541_wait_status(HandleXum1541);
    xum1541_dbg(2, "[xum1541_read_ext] Status = %d", *Status);
    return 1;
}

/*! \brief Read data from the xum1541 device

 \param HandleXum1541
   A XUM1541_HANDLE which contains the file handle of the USB device.

 \param mode
    Drive protocol to use to read the data from the device (e.g,
    XUM1541_CBM is normal IEC wire protocol).

 \param data
    Pointer to a buffer which will contain the data read from the xum1541

 \param size
    The number of bytes to read from the xum1541

 \return
    The number of bytes actually read, 0 on device error. If there is a
    fatal error, returns -1.
*/
int
xum1541_read(usb_dev_handle *HandleXum1541, unsigned char mode, unsigned char *data, size_t size)
{
    int rd;
    size_t bytesRead, bytes2read;
    unsigned char cmdBuf[XUM_CMDBUF_SIZE];
    BOOL isTapeCmd = ((mode == XUM1541_TAP) || (mode == XUM1541_TAP_CONFIG));

    xum1541_dbg(1, "read %d %d bytes to address %p",
               mode, size, data);

    RefuseToWorkInWrongMode; // Check if command allowed in current disk/tape mode.

    // Send the read command
    cmdBuf[0] = XUM1541_READ;
    cmdBuf[1] = mode;
    cmdBuf[2] = size & 0xff;
    cmdBuf[3] = (size >> 8) & 0xff;
    rd = usb.bulk_write(HandleXum1541,
        XUM_BULK_OUT_ENDPOINT | USB_ENDPOINT_OUT,
        (char *)cmdBuf, sizeof(cmdBuf), LIBUSB_NO_TIMEOUT);
    if (rd < 0) {
        fprintf(stderr, "USB error in read cmd: %s\n",
            usb.strerror());
        return -1;
    }

    // Read the actual data now that it's ready.
    bytesRead = 0;
    while (bytesRead < size) {
        bytes2read = size - bytesRead;
        if (bytes2read > XUM_MAX_XFER_SIZE)
            bytes2read = XUM_MAX_XFER_SIZE;
        rd = usb.bulk_read(HandleXum1541,
            XUM_BULK_IN_ENDPOINT | USB_ENDPOINT_IN,
            (char *)data, bytes2read, LIBUSB_NO_TIMEOUT);
        if (rd < 0) {
            fprintf(stderr, "USB error in read data(%p, %d): %s\n",
               data, (int)size, usb.strerror());
            return -1;
        } else if (rd > 0)
            xum1541_dbg(2, "read %d bytes", rd);

        data += rd;
        bytesRead += rd;

        /*
         * If we read less than we requested (or 0), the transfer is done
         * even if we had more data to read still.
         */
        if (rd < (int)bytes2read)
            break;
    }

    xum1541_dbg(2, "read done, got %d bytes", bytesRead);
    return bytesRead;
}
