/*
  flash.c
*/

#include <string.h>

#include "ihex.h"
#include "flash.h"

int flash_get_pages(ihex_file_t *ifile, int psize, int * pstart) {
  int start = ihex_file_get_start_address(ifile);
  int end = ihex_file_get_end_address(ifile);

  /* make sure start and end sum up to full pages */
  start &= ~(psize-1);
  end = (end & ~(psize-1)) + (psize - 1);

  if (pstart)
    *pstart = start;

  return((end-start+psize-1)/psize);
}

int flash_get_page(ihex_file_t *ifile, int page, char *data, int psize) {
  int start = ihex_file_get_start_address(ifile) & ~(psize-1);

  /* erase page */
  memset(data, FLASH_FILL, psize);

  /* and fill it with data */
  ihex_file_get_mem(ifile, start + page * psize, psize, data);

  return 0;
}
