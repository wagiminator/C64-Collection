#include "xu1541lib.h"
#include "xu1541_types.h"

#include <usb.h>

#include <stdio.h>
#include <string.h>

#include "arch.h"

/* do not recognize other xu1541-BIOS adopted firmwares */
/* comment out to enable "foreign" firmwares            */
#define RECOGNIZE_TRUE_XU1541_ONLY

const static struct recognized_usb_devices_t {
  unsigned short vid;
  unsigned short pid;
} recognized_usb_devices[] = {
  { XU1541_VID, XU1541_PID }   /* xu1541 vendor and product id (donated by ftdi) */
#if !defined(RECOGNIZE_TRUE_XU1541_ONLY)
  , { 0x16c0, 0x05dc } /* USBasp vendor and product id */
#endif
};

static int usb_get_string_ascii(usb_dev_handle *handle, int index,
				 char *string, int size) {
  unsigned char buffer[64], *c;
  int len, i;

  if((len = usb_control_msg(handle, USB_ENDPOINT_IN,
			    USB_REQ_GET_DESCRIPTOR, index, 0x0409,
			    (char*)buffer, sizeof(buffer), 1000)) < 0)
    return 0;

  /* string is shorter than the number of bytes returned? */
  if(buffer[0] < len)
    len = buffer[0];

  /* not a string? */
  if(buffer[1] != USB_DT_STRING)
    return 0;

  /* length is in unicode 16 bit chars and includes 2 byte header */
  len = (len-2)/2;

  /* string is longer than buffer provided? */
  if(len > size-1)
    len = size-1;

  /* take only lower byte for simple unicode to ascii conversion */
  for(i=0,c = buffer+2;i<len;i++,c+=2){
    if(!(*(c+1)))
      string[i] = *c;
    else
      string[i] = '_';
  }

  /* terminate string and return length */
  string[i] = 0;
  return 1;
}

/* check for known firmwares with xu1541 BIOS abilities */
static int is_xu1541bios_device(unsigned short vid, unsigned short pid) {
  int i;
  for(i=0;i<sizeof(recognized_usb_devices);++i) {
    if( recognized_usb_devices[i].vid == vid &&
        recognized_usb_devices[i].pid == pid ) {
      return 1;
    }
  }
  return 0;
}

/* find and open xu1541 device */
static usb_dev_handle *find_internal(unsigned int displaydeviceinfo, unsigned int in_bootmode, unsigned int print_error) {
  struct usb_bus      *bus;
  struct usb_device   *dev;
  usb_dev_handle      *handle = 0;

  usb_find_busses();
  usb_find_devices();

  for(bus=usb_get_busses(); bus && !handle; bus=bus->next){
    for(dev=bus->devices; dev && !handle; dev=dev->next){
      if(is_xu1541bios_device(dev->descriptor.idVendor,
                              dev->descriptor.idProduct)) {
	char string[32];

	/* we need to open the device in order to query strings */
	handle = usb_open(dev);
	if(!handle){
	  fprintf(stderr, "Warning: cannot open USB device: %s\n",
		  usb_strerror());
	  continue;
	}

	if(!usb_get_string_ascii(handle,
		 (USB_DT_STRING << 8) | dev->descriptor.iProduct,
		 string, sizeof(string))) {
	  fprintf(stderr, "Error: Cannot query product name "
		  "for device: %s\n", usb_strerror());
	  if(handle) usb_close(handle);
	  handle = NULL;
	}

        if (displaydeviceinfo) {
          xu1541lib_display_device_info(handle);
        }

	if(in_bootmode && strcmp(string, "xu1541boot") != 0 && ! xu1541lib_is_in_bootloader_mode(handle)) {
          if (in_bootmode)  {
            /* try to set xu1541 into boot mode */
            xu1541lib_set_to_boot_mode(handle);
          }
          else {
	    fprintf(stderr, "Error: Found %s device (version %x.%02x) not "
		  "in boot loader\n"
		  "       mode, please install jumper switch "
		  "and replug device!\n",
		  string, dev->descriptor.bcdDevice >> 8,
		  dev->descriptor.bcdDevice & 0xff);
          }

	  if(handle) usb_close(handle);
	  handle = NULL;
	}
      }
    }
  }

  if(!handle) {
    if (print_error)
      fprintf(stderr, "Could not find any xu1541 device%s!\n", in_bootmode ? " in boot loader mode" : "");
    return NULL;
  }

  if (usb_set_configuration(handle, 1) != 0) {
    fprintf(stderr, "USB error: %s\n", usb_strerror());
    usb_close(handle);
    return NULL;
  }

  /* Get exclusive access to interface 0. */
  if (usb_claim_interface(handle, 0) != 0) {
    fprintf(stderr, "USB error: %s\n", usb_strerror());
    usb_close(handle);
    return NULL;
  }

  return handle;
}

usb_dev_handle *xu1541lib_find(void) {
  return find_internal(0, 0, 1);
}

usb_dev_handle *xu1541lib_find_in_bootmode(unsigned int *p_soft_bootloader_mode) {
  usb_dev_handle *handle;

  if(!(handle = find_internal(1,1,0))) {
    if (p_soft_bootloader_mode)
      *p_soft_bootloader_mode = 1;

    handle = find_internal(0,1,1);
  }

  return handle;
}
