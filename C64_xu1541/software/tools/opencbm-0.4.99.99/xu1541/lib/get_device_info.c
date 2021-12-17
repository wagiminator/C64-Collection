#include "xu1541lib.h"
#include "xu1541_types.h"

#include <usb.h>

#include <stdio.h>
#include <string.h>

#include "arch.h"

int xu1541lib_get_device_info(usb_dev_handle *handle, xu1541_device_info_t *device_info, unsigned int device_info_size) {
  int nBytes;
  unsigned char reply[6];

  /* check if the size is correct */
  if (device_info_size != sizeof *device_info) {
    return 0;
  }

  memset(device_info, 0, device_info_size);

  nBytes = usb_control_msg(handle,
	   USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
	   XU1541_INFO, 0, 0, (char*)reply, sizeof(reply), 1000);

  if(nBytes < 0) {
    fprintf(stderr, "USB request for XU1541 info failed: %s!\n",
	    usb_strerror());
    return 0;
  }
  else if((nBytes != sizeof(reply)) && (nBytes != 4)) {
    fprintf(stderr, "Unexpected number of bytes (%d) returned\n", nBytes);
    return 0;
  }

  if (nBytes > 4) {
    device_info->BiosVersionAvailable = 1;
    device_info->BiosVersionMajor = reply[4];
    device_info->BiosVersionMinor = reply[5];
  }

  device_info->FirmwareVersionMajor = reply[0];
  device_info->FirmwareVersionMinor = reply[1];
  device_info->Capabilities = *(unsigned short*)(reply+2);
  if (device_info->FirmwareVersionMajor != 0xff && device_info->FirmwareVersionMinor != 0xff) {
     device_info->FirmwareVersionAvailable = 1;
  }

  if (device_info->Capabilities & XU1541_CAP_BOOTLOADER) {
     device_info->BootloaderMode = 1;
  }

  return 1;
}
