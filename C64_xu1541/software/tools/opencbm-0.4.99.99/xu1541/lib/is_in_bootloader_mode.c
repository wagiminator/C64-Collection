#include "xu1541lib.h"
#include "xu1541_types.h"

#include <usb.h>

#include <stdio.h>
#include <string.h>

#include "arch.h"

int xu1541lib_is_in_bootloader_mode(usb_dev_handle *handle) {
  xu1541_device_info_t device_info;

  if (xu1541lib_get_device_info(handle, &device_info, sizeof device_info)) {
    return device_info.BootloaderMode ? 1 : 0;
  }
  return 0;
}


