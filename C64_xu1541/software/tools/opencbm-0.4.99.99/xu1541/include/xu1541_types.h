/*
  xu1541_types.h - xu1541
*/

#ifndef XU1541_TYPES_H
#define XU1541_TYPES_H

/* usb bootloader command constants */
#define USBBOOT_FUNC_LEAVE_BOOT 1
#define USBBOOT_FUNC_WRITE_PAGE 2
#define USBBOOT_FUNC_GET_PAGESIZE 3

/* special commands used for testing and debugging */
#define XU1541_ECHO                  0xff
#define XU1541_GET_EVENT             0xfe
#define XU1541_IRQ_PAUSE             0xfd

/* normal commands */
#define XU1541_INFO                  0

#define XU1541_CAP_CBM               0x0001   /* supports CBM commands */
#define XU1541_CAP_LL                0x0002   /* supports low level io */
#define XU1541_CAP_PP                0x0004   /* supports 8 bit port */
#define XU1541_CAP_NIB               0x0008   /* supports nibbler */
#define XU1541_CAP_PROTO_S1          0x0010   /* supports serial1 protocol */
#define XU1541_CAP_PROTO_S2          0x0020   /* supports serial2 protocol */
#define XU1541_CAP_PROTO_PP          0x0040   /* supports parallel protocol */
#define XU1541_CAP_PROTO_P2          0x0080   /* supports parallel2 protocol */

#define XU1541_CAP_BOOTLOADER        0x4000   /* device is in bootloader mode */

#define XU1541_CAPABILIIES  (XU1541_CAP_CBM | XU1541_CAP_LL | XU1541_CAP_PP  | XU1541_CAP_PROTO_S1 | XU1541_CAP_PROTO_S2 | XU1541_CAP_PROTO_PP | XU1541_CAP_PROTO_P2)

#define XU1541_READ                  1
#define XU1541_WRITE                 2
#define XU1541_IOCTL                 3

/* constants for valid ioctl commands */
#define XU1541_TALK	             (XU1541_IOCTL + 0)
#define XU1541_LISTEN	             (XU1541_IOCTL + 1)
#define XU1541_UNTALK                (XU1541_IOCTL + 2)
#define XU1541_UNLISTEN              (XU1541_IOCTL + 3)
#define XU1541_OPEN                  (XU1541_IOCTL + 4)
#define XU1541_CLOSE                 (XU1541_IOCTL + 5)
#define XU1541_RESET                 (XU1541_IOCTL + 6)
#define XU1541_GET_EOI               (XU1541_IOCTL + 7)
#define XU1541_CLEAR_EOI             (XU1541_IOCTL + 8)

/* support commands for async read/write */
#define XU1541_REQUEST_READ          (XU1541_IOCTL + 9)
#define XU1541_GET_RESULT            (XU1541_IOCTL + 10)

#define XU1541_PP_READ               (XU1541_IOCTL + 11)
#define XU1541_PP_WRITE              (XU1541_IOCTL + 12)
#define XU1541_IEC_POLL              (XU1541_IOCTL + 13)
#define XU1541_IEC_WAIT              (XU1541_IOCTL + 14)
#define XU1541_IEC_SETRELEASE        (XU1541_IOCTL + 15)

/* constants needed by parallel burst (nibbler) */
#define XU1541_PARBURST_READ         (XU1541_IOCTL + 16)
#define XU1541_PARBURST_WRITE        (XU1541_IOCTL + 17)
#define XU1541_PARBURST_READ_TRACK   (XU1541_IOCTL + 18)
#define XU1541_PARBURST_WRITE_TRACK  (XU1541_IOCTL + 19)

/* from version x.09 on, one can write eeprom data */
#define XU1541_EEPROM_READ           (XU1541_IOCTL + 20)
#define XU1541_EEPROM_WRITE          (XU1541_IOCTL + 21)

/* start the flash bootloader */
#define XU1541_FLASH                 (XU1541_IOCTL + 22)

/* special protocol codes begin at 32 */
#define XU1541_S1                    (32)
#define XU1541_S2                    (XU1541_S1 + 1)
#define XU1541_PP                    (XU1541_S1 + 2)
#define XU1541_P2                    (XU1541_S1 + 3)

/* io state codes */
#define XU1541_IO_IDLE           0
#define XU1541_IO_READ           1
#define XU1541_IO_READ_DONE      2
#define XU1541_IO_WRITE_PREPARED 3
#define XU1541_IO_WRITE          4
#define XU1541_IO_IRQ_PAUSE      5
#define XU1541_IO_RESULT         6
#define XU1541_IO_ASYNC          7

/* max time to wait for device */
#define XU1541_W4L_TIMEOUT        20  /* seconds, max. 60 possible here */
#define XU1541_IO_BUFFER_SIZE    128

#endif /* XU1541_TYPES_H */
