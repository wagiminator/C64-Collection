/*
  xu1541_update.c

  xz1541 USB update tool, based on the fischl boot loader
*/

#include <stdio.h>
#include <string.h>

#include <usb.h>

#include "ihex.h"
#include "flash.h"

#include "xu1541_types.h"

#include "xu1541lib.h"

#ifdef WIN32
#include <windows.h>
#include <winbase.h>
#define MSLEEP(a) Sleep(a)
#else
#define MSLEEP(a) usleep(a*1000)
#endif

#ifdef WIN
#define WINKEY { printf("Press return to quit\n"); getchar(); }
#else
#define WINKEY
#endif

int xu1541_write_page(usb_dev_handle *handle,
		      char *data, int address, int len);

static int usb_was_reset = 0;

static unsigned char *page_for_address_0 = NULL;
static unsigned int   page_for_address_0_is_valid = 0;

void xu1541_close(usb_dev_handle *handle) {
  if( ! usb_was_reset) {
    xu1541lib_close(handle);
  }
  else {
    /* close usb device */
    usb_close(handle);
  }
}

int xu1541_start_application(usb_dev_handle *handle, int page_size) {
  if (page_for_address_0_is_valid) {
    /* page 0 was flashed 'invalidly', correct it now as last step */
    /* before rebooting. */
    xu1541_write_page(handle, (char*)page_for_address_0, 0, page_size);
  }

  usb_control_msg(handle,
	   USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN,
	   USBBOOT_FUNC_LEAVE_BOOT, 0, 0,
	   NULL, 0, 1000);

/*
 * This will always be an error, as the xu1541 just reboots, and does
 * not answer!

  if (nBytes != 0) {
    fprintf(stderr, "Error starting application: %s\n", usb_strerror());
    return -1;
  }
*/
  usb_reset( handle ); /* re-enumerate that device */
  usb_was_reset = 1;

  return 0;
}

int xu1541_write_page(usb_dev_handle *handle,
		      char *data, int address, int len) {
  int nBytes;

  nBytes = usb_control_msg(handle,
	   USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT,
	   USBBOOT_FUNC_WRITE_PAGE, address, 0,
	   data, len, 1000);

  if(nBytes != len) {
    fprintf(stderr, "Error uploading flash: %s\n", usb_strerror());
    return -1;
  }
  return 0;
}


int main(int argc, char **argv) {
  ihex_file_t *ifile = NULL;
  usb_dev_handle *handle = NULL;
  int page_size, i;
  int start;
  char *page = NULL;
  unsigned int soft_bootloader_mode = 0;

  printf("--        XU1541 flash updater        --\n");
  printf("--    (c) 2007 by the opencbm team    --\n");
  printf("-- http://www.harbaum.org/till/xu1541 --\n");

  if(argc < 2) {
    fprintf(stderr, "Usage: xu1541_update [-o OFFSET] <ihex_file> [[-o OFFSET] <ihex_file2> ...]\n");
    WINKEY;
    exit(-1);
  }

  usb_init();

  /* find required usb device */
  if(!(handle = xu1541lib_find_in_bootmode(&soft_bootloader_mode))) {
      WINKEY;
      exit(1);
  }

  /* check page size */
  if((page_size = xu1541lib_get_pagesize(handle)) != FLASH_PAGE_SIZE) {
    fprintf(stderr, "Error: unexpected page size %d\n", page_size);
    xu1541_close(handle);
    exit(-1);
  }

  if(!(page = malloc(page_size))) {
    fprintf(stderr, "Error: Out of memory allocating page buffer\n");
    xu1541_close(handle);
    exit(-1);
  }

  if(!(page_for_address_0 = malloc(page_size))) {
    fprintf(stderr, "Error: Out of memory allocating page buffer\n");
    xu1541_close(handle);
    exit(-1);
  }

  do {
          unsigned int flash_offset = 0;

          /* process -o (offset) parameter */
          if (strncmp(argv[1], "-o", 2) == 0) {
            char *pos = argv[1]+2;

            if (*pos == '=')
              ++pos;

            flash_offset = strtol(pos, &pos, 0);
            printf("An offset of 0x%04x is specified!\n", flash_offset);

            if (*pos != 0) {
              fprintf(stderr, "ERROR: extra input '%s' after offset of 0x%04x.\n", pos, flash_offset);
              xu1541_close(handle);
              return -1;
            }

            /* proceed to next argument */
            ++argv;
            --argc;
          }

          /* process -R (reboot) parameter */
          if (strcmp(argv[1], "-R") == 0) {
            printf("Starting application...\n");
            xu1541_start_application(handle, page_size);
            printf("Waiting for reboot...\n");
            xu1541lib_wait(handle);
            xu1541lib_wait(handle); /* TODO: remove that and replace the xu1541_wait with a better approach */

            /* find required usb device */
            printf("Find xu1541 again...\n");
            if(!(handle = xu1541lib_find_in_bootmode(NULL))) {
              WINKEY;
              exit(1);
            }

            /* proceed to next argument */
            ++argv;
            --argc;
          }

          /* load the file into memory */

          ifile = ihex_parse_file(argv[1]);

          if(ifile) {
            /* flash the file */
            printf("Uploading %d pages ", flash_get_pages(ifile, page_size, &start));
            printf("starting from 0x%04x", start);

            if (flash_offset != 0) {
                    start -= flash_offset;
                    printf(", moved to 0x%04x", start);
            }
            printf("\n");

            for(i=0;i<flash_get_pages(ifile, page_size, NULL);i++) {

              /* fill page from ihex image */
              flash_get_page(ifile, i, page, page_size);

              /* special handling of page 0: */

              if (i * page_size + start == 0) {
                /* remember the data to write */
                memcpy(page_for_address_0, page, page_size);
                page_for_address_0_is_valid = 1;

                /* mark the page address is invalid application */
                page[0] = -1;
                page[1] = -1;
              }

              /* and flash it */
              xu1541_write_page(handle, page, i*page_size + start, page_size);

              printf(".");
              fflush(stdout);
            }

            ihex_free_file(ifile);
          } else {
            fprintf(stderr, "ERROR: Failed to load hex image\n");
            free(page);
            xu1541_close(handle);
            return -1;
          }

          /* proceed to next file */
          if (argc > 2)
             printf(" done\n");
          --argc;
          ++argv;

  } while (argc >= 2);

  xu1541_start_application(handle, page_size);

  printf(" done\n"
	 "%s\n",
         (soft_bootloader_mode
          ? "Rebooting the xu1541."
          : "If you had installed a jumper switch, please remove it and replug the USB cable\n"
	    "to return to normal operation!\n"
            "If not, the xu1541 will reboot itself automatically.\n"));

  free(page);
  xu1541_close(handle);
  return 0;
}
