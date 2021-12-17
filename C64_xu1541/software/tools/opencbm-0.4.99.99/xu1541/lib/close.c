#include "xu1541lib.h"
#include "xu1541_types.h"

#include <usb.h>

#include <stdio.h>
#include <string.h>

#include "arch.h"

void xu1541lib_close(usb_dev_handle *handle) {
  /* release exclusive access to device */
  if(usb_release_interface(handle, 0))
    fprintf(stderr, "USB error: %s\n", usb_strerror());

  /* close usb device */
  usb_close(handle);
}
