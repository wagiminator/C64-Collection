#include "xu1541lib.h"
#include "xu1541_types.h"

#include <usb.h>

#include <stdio.h>
#include <string.h>

#include "arch.h"

void xu1541lib_display_device_info(usb_dev_handle *handle) {
  xu1541_device_info_t device_info;

  if (xu1541lib_get_device_info(handle, &device_info, sizeof device_info)) {
    if (device_info.BiosVersionAvailable)
      printf("Device reports BIOS version %x.%02x.\n", device_info.BiosVersionMajor, device_info.BiosVersionMinor);

    if (device_info.FirmwareVersionAvailable)
      printf("Device reports firmware version %x.%02x.\n", device_info.FirmwareVersionMajor, device_info.FirmwareVersionMinor);
    else
      printf("Device reports: No firmware available.\n");

    printf("Device reports capabilities 0x%04x.\n", device_info.Capabilities);
    printf("Device is %sin bootloader mode.\n", device_info.BootloaderMode ? "" : "not ");
  }
}
