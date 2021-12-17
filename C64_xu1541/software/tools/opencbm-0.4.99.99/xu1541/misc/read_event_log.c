/*
 * read_event_log.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <usb.h>

#include "xu1541lib.h"

#include "xu1541_types.h"
#include "xu1541_event_log.h"

usb_dev_handle      *handle = NULL;

#ifdef WIN
#define QUIT_KEY  { printf("Press return to quit\n"); getchar(); }
#else
#define QUIT_KEY
#endif

void dump_event_log(void) {

  int nBytes, i, log_len;
  unsigned char ret[2];

  nBytes = usb_control_msg(handle,
	   USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
	   XU1541_GET_EVENT, 0, 0, (char*)ret, sizeof(ret), 1000);

  if(nBytes < 0) {
    fprintf(stderr, "USB request failed: %s!\n", usb_strerror());
    return;
  } else if(nBytes != sizeof(ret)) {
    fprintf(stderr, "Unexpected number of bytes (%d) returned\n", nBytes);
    return;
  }

  log_len = ret[0];

  printf("Event log buffer size: %d\n", log_len);
  printf("Event log:\n");

  for(i=0;i<log_len;i++) {
    nBytes = usb_control_msg(handle,
		  USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
		  XU1541_GET_EVENT, i, 0, (char*)ret, sizeof(ret), 1000);

    if(nBytes < 0) {
      fprintf(stderr, "USB request failed: %s!\n", usb_strerror());
      return;
    } else if(nBytes != sizeof(ret)) {
      fprintf(stderr, "Unexpected number of bytes (%d) returned\n", nBytes);
      return;
    }

    switch(ret[1]) {
    case EVENT_NONE:      /* ignore unused entries */
      break;

    case EVENT_START:
      printf("  system started\n");
      break;

    case EVENT_RESET_EXT:
      printf("  booted by external reset\n");
      break;

    case EVENT_RESET_WATCHDOG:
      printf("  booted by watchdog reset\n");
      break;

    case EVENT_RESET_BROWNOUT:
      printf("  booted by brownout reset\n");
      break;

    case EVENT_TIMEOUT_FREE_BUS:
      printf("  timeout while waiting for free bus\n");
      break;

    case EVENT_BYTE_NAK:
      printf("  byte not acknowledged\n");
      break;

    case EVENT_TIMEOUT_LISTENER:
      printf("  timeout waiting for listener\n");
      break;

    case EVENT_WRITE_NO_DEV:
      printf("  write: no device\n");
      break;

    case EVENT_WRITE_FAILED:
      printf("  write failed\n");
      break;

    case EVENT_WRITE_DEV_NOT_PRESENT:
      printf("  write: device not present\n");
      break;

    case EVENT_READ_TIMEOUT:
      printf("  read timeout\n");
      break;

    case EVENT_READ_ERROR:
      printf("  read error\n");
      break;

    case EVENT_TIMEOUT_IEC_WAIT:
      printf("  timeout in iec wait\n");
      break;

    default:
      printf("  Unknown event!\n");
      break;
    }
  }
}

int main(int argc, char *argv[]) {
  printf("--       XU1541 event log dumper       --\n");
  printf("--      (c) 2007 by Till Harbaum       --\n");
  printf("-- http://www.harbaum.org/till/xu1541  --\n");

  usb_init();

  if (!(handle = xu1541lib_find())) {
    return 1;
  }

  xu1541lib_display_device_info(handle);

  dump_event_log();

  xu1541lib_close(handle);

  return 0;
}
