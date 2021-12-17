/*
  ihex.h
*/

#ifndef IHEX_H
#define IHEX_H

/* data extracted from a single line read from the hex file */
typedef struct {
  int length;
  int address;
  int type;
  int crc;

  unsigned char *data;
} ihex_line_t;

/* a continous chunk of data */
typedef struct ihex_chunk {
  int length;
  int address;
  unsigned char *data;
  struct ihex_chunk *next;
} ihex_chunk_t;

/* the entire files contents */
typedef struct {
  int lines;
  int ended;

  ihex_chunk_t *first;
} ihex_file_t;

/* exported functions */
extern void ihex_free_file(ihex_file_t *ifile);
extern ihex_file_t *ihex_parse_file(char *filename);
extern int ihex_file_get_chunks(ihex_file_t *ifile);
extern int ihex_file_get_start_address(ihex_file_t *ifile);
extern int ihex_file_get_end_address(ihex_file_t *ifile);
extern int ihex_file_get_mem(ihex_file_t *ifile, int start, int len, char *data);

#endif /* IHEX_H */

