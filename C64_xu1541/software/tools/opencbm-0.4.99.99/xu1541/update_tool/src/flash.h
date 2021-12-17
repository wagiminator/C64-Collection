/*
  flash.h
*/

#ifndef FLASH_H
#define FLASH_H

#define FLASH_PAGE_SIZE   64      /* page size of mega8 */
#define FLASH_FILL        0xff    /* used to fill empty areas */

extern int flash_get_pages(ihex_file_t *ifile, int psize, int * pstart);
extern int flash_get_page(ihex_file_t *ifile, int page, char *data, int psize);

#endif /* FLASH_H */
