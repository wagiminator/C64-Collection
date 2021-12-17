#include "xu1541lib.h"
#include "xu1541_types.h"

#include <usb.h>

#include <stdio.h>
#include <string.h>

#include "arch.h"

/* try to set xu1541 into boot mode */
int xu1541lib_set_to_boot_mode(usb_dev_handle *handle)
{
  printf("Setting xu1541 into boot mode... \n");

  usb_control_msg(handle,
        USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
        XU1541_FLASH, 0, 0, 0, 0, 1000);

  xu1541lib_wait(handle);

  return 0;
}
