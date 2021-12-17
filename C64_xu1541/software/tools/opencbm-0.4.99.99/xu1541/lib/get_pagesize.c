#include "xu1541lib.h"
#include "xu1541_types.h"

#include <usb.h>

#include <stdio.h>
#include <string.h>

#include "arch.h"

int xu1541lib_get_pagesize(usb_dev_handle *handle) {
  unsigned char retval[2];
  int nBytes;

  nBytes = usb_control_msg(handle,
	   USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
	   USBBOOT_FUNC_GET_PAGESIZE, 0, 0,
	   (char*)retval, sizeof(retval), 1000);

  if (nBytes != sizeof(retval)) {
    fprintf(stderr, "Error getting page size: %s\n", usb_strerror());
    return -1;
  }

  return 256 * retval[0] + retval[1];
}
