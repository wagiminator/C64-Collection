/*
 * xum1541 external message types and defines
 * Copyright (c) 2009-2010 Nate Lawson <nate@root.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#ifndef _XUM1541_TYPES_H
#define _XUM1541_TYPES_H

// Vendor and product ID. These are owned by Nate Lawson, do not reuse.
#define XUM1541_VID                 0x16d0
#define XUM1541_PID                 0x0504

// XUM1541_INIT reports this versions
#define XUM1541_VERSION             7

// USB parameters for descriptor configuration
#define XUM_BULK_IN_ENDPOINT        3
#define XUM_BULK_OUT_ENDPOINT       4
#define XUM_ENDPOINT_0_SIZE         8

// control transactions
#define XUM1541_ECHO                0
#define XUM1541_INIT                (XUM1541_ECHO + 1)
#define XUM1541_RESET               (XUM1541_ECHO + 2)
#define XUM1541_SHUTDOWN            (XUM1541_ECHO + 3)
#define XUM1541_ENTER_BOOTLOADER    (XUM1541_ECHO + 4)
#define XUM1541_TAP_BREAK           (XUM1541_ECHO + 5)

// Adapter capabilities, but device may not support them
#define XUM1541_CAP_CBM             0x01 // supports CBM commands
#define XUM1541_CAP_NIB             0x02 // parallel nibbler
#define XUM1541_CAP_NIB_SRQ         0x04 // 1571 serial nibbler
#ifdef IEEE_SUPPORT
#define XUM1541_CAP_IEEE488         0x08 // GPIB (PET) parallel bus
#else
#define XUM1541_CAP_IEEE488         0
#endif
#ifdef TAPE_SUPPORT
#define XUM1541_CAP_TAP             0x10 // 153x tape support
#else
#define XUM1541_CAP_TAP             0
#endif

#define XUM1541_CAPABILITIES        (XUM1541_CAP_CBM |      \
                                     XUM1541_CAP_NIB |      \
                                     XUM1541_CAP_TAP |      \
                                     XUM1541_CAP_IEEE488)

// Actual auto-detected status
#define XUM1541_DOING_RESET         0x01 // no clean shutdown, will reset now
#define XUM1541_NO_DEVICE           0x02 // no IEC device present yet
#define XUM1541_IEEE488_PRESENT     0x10 // IEEE-488 device connected
#define XUM1541_TAPE_PRESENT        0x20 // 153x tape device connected

// Sizes for commands and responses in bytes
#define XUM_CMDBUF_SIZE             4 // Command block (out)
#define XUM_STATUSBUF_SIZE          3 // Waiting status value (in)
#define XUM_DEVINFO_SIZE            8 // Response to XUM1541_INIT msg (in)

/*
 * Control msg command timeout. Since the longest command we run in this
 * mode is XUM1541_RESET (or INIT if it has to run RESET), we chose a
 * time greater than the maximum a device has to respond after ATN
 * goes active. The IEC spec lists Tat as 1 ms assuming the device is
 * alive, so we're being very generous.
 */
#define XUM1541_TIMEOUT             1.5

// Read/write commands, protocol type defined below
#define XUM1541_READ                8
#define XUM1541_WRITE               (XUM1541_READ + 1)

/*
 * Maximum size for USB transfers (read/write commands, all protocols).
 * This should be ok for the raw USB protocol. I haven't tested this much
 * but at least 8192 works (e.g. for nib protocol reads. For longer
 * transfers, the usermode code should break it up into chunks this size.
 */
#define XUM_MAX_XFER_SIZE           32768

/*
 * Individual control commands. Those that can take a while and thus
 * report async status are marked with "async".
 */
#define XUM1541_IOCTL               16
#define XUM1541_GET_EOI             (XUM1541_IOCTL + 7)
#define XUM1541_CLEAR_EOI           (XUM1541_IOCTL + 8)
#define XUM1541_PP_READ             (XUM1541_IOCTL + 9)
#define XUM1541_PP_WRITE            (XUM1541_IOCTL + 10)
#define XUM1541_IEC_POLL            (XUM1541_IOCTL + 11)
#define XUM1541_IEC_WAIT            (XUM1541_IOCTL + 12)
#define XUM1541_IEC_SETRELEASE      (XUM1541_IOCTL + 13)
#define XUM1541_PARBURST_READ       (XUM1541_IOCTL + 14)
#define XUM1541_PARBURST_WRITE      (XUM1541_IOCTL + 15)
#define XUM1541_SRQBURST_READ       (XUM1541_IOCTL + 16)
#define XUM1541_SRQBURST_WRITE      (XUM1541_IOCTL + 17)
#define XUM1541_TAP_MOTOR_ON            (XUM1541_IOCTL + 50)
#define XUM1541_TAP_GET_VER             (XUM1541_IOCTL + 51)
#define XUM1541_TAP_PREPARE_CAPTURE     (XUM1541_IOCTL + 52)
#define XUM1541_TAP_PREPARE_WRITE       (XUM1541_IOCTL + 53)
#define XUM1541_TAP_GET_SENSE           (XUM1541_IOCTL + 54)
#define XUM1541_TAP_WAIT_FOR_STOP_SENSE (XUM1541_IOCTL + 55)
#define XUM1541_TAP_WAIT_FOR_PLAY_SENSE (XUM1541_IOCTL + 56)
#define XUM1541_TAP_MOTOR_OFF           (XUM1541_IOCTL + 57)

#define IS_CMD_ASYNC(x)             ((x) == XUM1541_IEC_WAIT)

/*
 * Status return values for async commands.
 * Status response also contains a second data word, 16-bits little-endian.
 * This value, accessed via XUM_GET_STATUS_VAL(), usually gives a length
 * but is command-specific.
 */
#define XUM1541_IO_BUSY             1
#define XUM1541_IO_READY            2
#define XUM1541_IO_ERROR            3

// Tape/disk mode error return values for xum1541_ioctl, xum1541_read, xum1541_write.
#define XUM1541_Error_NoTapeSupport      -100
#define XUM1541_Error_NoDiskTapeMode     -101
#define XUM1541_Error_TapeCmdInDiskMode  -102
#define XUM1541_Error_DiskCmdInTapeMode  -103

// Macros to retrive the status and extended value (usually a length).
#define XUM_GET_STATUS(buf)         (buf[0])
#define XUM_GET_STATUS_VAL(buf)     (((buf[2]) << 8) | (buf[1]))

/*
 * Basic CBM and special protocols for use with XUM1541_READ/WRITE
 * We use the upper nibble for the protocol type so we can use the
 * lower nibble for additional protocol-specific flags.
 */
#define XUM_RW_PROTO(x)             ((x) & 0xf0)
#define XUM_RW_FLAGS(x)             ((x) & 0x0f)
#define XUM1541_CBM                 (1 << 4) // Standard CBM protocol
#define XUM1541_S1                  (2 << 4) // serial1
#define XUM1541_S2                  (3 << 4) // serial2
#define XUM1541_PP                  (4 << 4) // Parallel
#define XUM1541_P2                  (5 << 4) // 2-byte parallel
#define XUM1541_NIB                 (6 << 4) // burst nibbler parallel
#define XUM1541_NIB_COMMAND         (7 << 4) // BN parallel commands
#define XUM1541_NIB_SRQ             (8 << 4) // 1571 serial nibbler
#define XUM1541_NIB_SRQ_COMMAND     (9 << 4) // Serial commands
#define XUM1541_TAP                (10 << 4) // tape read/write
#define XUM1541_TAP_CONFIG         (11 << 4) // tape send/receive configuration

// Flags for use with write and XUM1541_CBM protocol
#define XUM_WRITE_TALK              (1 << 0)
#define XUM_WRITE_ATN               (1 << 1)

// Request an early exit from nib read via burst_read_track_var()
#define XUM1541_NIB_READ_VAR        0x8000

#endif // _XUM1541_TYPES_H
