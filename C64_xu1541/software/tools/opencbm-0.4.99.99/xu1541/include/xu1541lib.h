#include <usb.h>

/* vendor and product id */
#define XU1541_VID  0x0403
#define XU1541_PID  0xc632

#include <stdint.h>

typedef struct {
        uint8_t FirmwareVersionMajor;
        uint8_t FirmwareVersionMinor;
        uint16_t Capabilities;
        uint8_t BiosVersionMajor;
        uint8_t BiosVersionMinor;

        uint8_t FirmwareVersionAvailable;
        uint8_t BiosVersionAvailable;
        uint8_t BootloaderMode;
} xu1541_device_info_t;

extern usb_dev_handle * xu1541lib_find(void);
extern usb_dev_handle * xu1541lib_find_in_bootmode(unsigned int *p_soft_bootloader_mode);
extern void             xu1541lib_close(usb_dev_handle *handle);
extern int              xu1541lib_get_device_info(usb_dev_handle *handle, xu1541_device_info_t *device_info, unsigned int device_info_length);
extern void             xu1541lib_display_device_info(usb_dev_handle *handle);
extern int              xu1541lib_is_in_bootloader_mode(usb_dev_handle *handle);
extern void             xu1541lib_wait(usb_dev_handle *handle);
extern int              xu1541lib_set_to_boot_mode(usb_dev_handle *handle);
extern int              xu1541lib_get_pagesize(usb_dev_handle *handle);


