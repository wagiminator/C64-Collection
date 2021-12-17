/*
  ihex.c

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ihex.h"

#define RECORD_DATA  0
#define RECORD_END   1
#define RECORD_START_SEGMENT_ADDRESS 3

static int ihex_iswhite(unsigned char c) {
  return( (c == ' ')||(c == '\n')||(c == '\r')||(c == '\t'));
}

static int ihex_line_is_white(char *line) {
  int is_white = 1;

  while(*line)
    if(!(ihex_iswhite(*line++)))
      is_white = 0;

  return is_white;
}

static int hex2bin(unsigned char c) {
  if((c >= '0') && (c <= '9')) return(c - '0');
  if((c >= 'a') && (c <= 'f')) return(c - 'a' + 10);
  if((c >= 'A') && (c <= 'F')) return(c - 'A' + 10);
  return -1;
}

/* convert two digits hex byte into binary, return NULL on error */
static char *ihex_parse_byte(char *line, int *crc, int *value) {
  int b0 = hex2bin(line[0]);
  int b1 = hex2bin(line[1]);

  if((b0 < 0) || (b1 < 0))
    return NULL;

  if(value) *value = 16*b0 + b1;
  *crc = (*crc + (16*b0 + b1))&0xff;

  return line+2;
}

/* convert four digits hex word into binary, return NULL on error */
static char *ihex_parse_word(char *line, int *crc, int *value) {
  int b0 = hex2bin(line[0]);
  int b1 = hex2bin(line[1]);
  int b2 = hex2bin(line[2]);
  int b3 = hex2bin(line[3]);

  if((b0 < 0) || (b1 < 0) || (b2 < 0) || (b3 < 0))
    return NULL;

  if(value) *value = 4096 * b0 + 256 * b1 + 16*b2 + b3;
  *crc = (*crc + (16 * b0 + b1 + 16*b2 + b3))&0xff;

  return line+4;
}

static void ihex_free_line(ihex_line_t *iline) {
  if(iline) {
    if(iline->data)
      free(iline->data);

    free(iline);
  }
}

/* parse a single line from the intel hex file */
static ihex_line_t *ihex_parse_line(char *line, int line_num) {
  ihex_line_t *iline = NULL;
  int tmp;

  if(!(iline = malloc(sizeof(ihex_line_t)))) {
    fprintf(stderr, "ERROR: Out of memory parsing line %d\n", line_num);
    return NULL;
  }

  memset(iline, 0, sizeof(ihex_line_t));

  /* skip white space up to start delimiter ':' */
  while(*line != ':') {
    if(!ihex_iswhite(*line))
      line++;
    else {
      fprintf(stderr, "ERROR: Missing start delimiter in line %d\n", line_num);
      ihex_free_line(iline);
      return NULL;
    }
  }

  /* skip ':' */
  line++;

  /* parse line "header" which needs at least 10 more bytes */
  if(strlen(line) < 10) {
    fprintf(stderr, "ERROR: Insufficient header data in line %d\n", line_num);
    ihex_free_line(iline);
    return NULL;
  }

  /* parse header fields */
  if(!(line = ihex_parse_byte(line, &iline->crc, &iline->length))) {
    fprintf(stderr, "ERROR: Illegal length in line %d\n", line_num);
    ihex_free_line(iline);
    return NULL;
  }

  if(!(line = ihex_parse_word(line, &iline->crc, &iline->address))) {
    fprintf(stderr, "ERROR: Illegal address in line %d\n", line_num);
    ihex_free_line(iline);
    return NULL;
  }

  if(!(line = ihex_parse_byte(line, &iline->crc, &iline->type))) {
    fprintf(stderr, "ERROR: Illegal type in line %d\n", line_num);
    ihex_free_line(iline);
    return NULL;
  }

  /* (always) process the data bytes */
  {
    int i;

    /* check if remaining line is long enough for data + crc */
    if(strlen(line) < 2 * (1+iline->length)) {
      fprintf(stderr, "ERROR: Line to short in line %d\n", line_num);
      ihex_free_line(iline);
      return NULL;
    }

    /* allocate space for data */
    if(!(iline->data = malloc(iline->length))) {
      fprintf(stderr, "ERROR: Out of memory allocating data for line %d\n",
	      line_num);
      ihex_free_line(iline);
      return NULL;
    }

    for(i=0;i<iline->length;i++) {
      if(!(line = ihex_parse_byte(line, &iline->crc, &tmp))) {
	fprintf(stderr, "ERROR: Illegal data in line %d\n", line_num);
	ihex_free_line(iline);
	return NULL;
      }

      /* store byte */
      iline->data[i] = tmp;
    }
  }

  /* special handling for RECORD_START_SEGMENT_ADDRESS */
  if(iline->type == RECORD_START_SEGMENT_ADDRESS) {
    if(iline->length != 4) {
      fprintf(stderr, "wrong length of start address: %d.\n", iline->length);
    }
    else {
      fprintf(stderr, "start address from file: 0x%02x%02x:0x%02x%02x.\n", iline->data[0], iline->data[1], iline->data[2], iline->data[3]);
    }
  }

  /* read crc ... */
  if(!(line = ihex_parse_byte(line, &iline->crc, NULL))) {
    fprintf(stderr, "ERROR: Illegal crc in line %d\n", line_num);
    ihex_free_line(iline);
    return NULL;
  }

  /* ... and verify it */
  if(iline->crc) {
    fprintf(stderr, "ERROR: CRC (%x) mismatch in line %d\n",
	    iline->crc, line_num);
    ihex_free_line(iline);
    return NULL;
  }

  /* the remaining data must only be whitespace */
  while(*line) {
    if(!ihex_iswhite(*line)) {
      fprintf(stderr, "ERROR: garbage after line in line %d\n", line_num);
      ihex_free_line(iline);
      return NULL;
    }
    line++;
  }

  return(iline);
}

static ihex_chunk_t *ihex_new_chunk(ihex_line_t *iline) {
  ihex_chunk_t *ichunk = NULL;

  if(!(ichunk = malloc(sizeof(ihex_chunk_t)))) {
    fprintf(stderr, "ERROR: Out of memory allocating %zu bytes chunk\n",
	    sizeof(ihex_chunk_t));
    return NULL;
  }

  memset(ichunk, 0, sizeof(ihex_chunk_t));

  /* also allocate buffer for data if required */
  if(iline->length) {
    if(!(ichunk->data = malloc(iline->length))) {
      fprintf(stderr, "ERROR: Out of memory allocating %d bytes chunk data\n",
	      iline->length);
      return NULL;
    }

    /* copy data if successful */
    memcpy(ichunk->data, iline->data, iline->length);
  }

  /* store chunk address */
  ichunk->length = iline->length;
  ichunk->address = iline->address;

  return ichunk;
}

/* insert data from line into file structure */
static int ihex_insert(ihex_file_t *ifile, ihex_line_t *iline) {
  ihex_chunk_t **ichunk = &ifile->first;

  /* walk chunk chain to find insertion point */
  while(*ichunk) {
    ihex_chunk_t *next = (*ichunk)->next;

    /* check if new chunk begins within this one */
    if((iline->address >= (*ichunk)->address) &&
       (iline->address  < (*ichunk)->address + (*ichunk)->length)) {
      fprintf(stderr, "ERROR: data overlap at address 0x%04x!\n",
	      iline->address);
      return -1;
    }

    /* check if we can append the chunk directly */
    if(iline->address  == (*ichunk)->address + (*ichunk)->length) {
      unsigned char *old_data = (*ichunk)->data;

      (*ichunk)->data = malloc((*ichunk)->length + iline->length);
      if(!((*ichunk)->data)) {
	fprintf(stderr, "ERROR: Out of memory increasing chunk to %d bytes!\n",
		(*ichunk)->length + iline->length);
	return -1;
      }

      /* copy old data */
      memcpy((*ichunk)->data, old_data, (*ichunk)->length);
      free(old_data);

      /* and new data */
      memcpy((*ichunk)->data+(*ichunk)->length, iline->data, iline->length);

      /* and adjust new size */
      (*ichunk)->length += iline->length;

      /* check if the following chunk can be attached as well */
      if(next) {
	if(next->address  == (*ichunk)->address + (*ichunk)->length) {
	  unsigned char *old_data = (*ichunk)->data;

	  (*ichunk)->data = malloc((*ichunk)->length + next->length);
	  if(!((*ichunk)->data)) {
	    fprintf(stderr, "ERROR: Out of memory increasing chunk to "
		    "%d bytes!\n", (*ichunk)->length + next->length);
	    return -1;
	  }

	  /* copy old data */
	  memcpy((*ichunk)->data, old_data, (*ichunk)->length);
	  free(old_data);

	  /* and new data */
	  memcpy((*ichunk)->data+(*ichunk)->length, next->data, next->length);

	  /* and adjust new size */
	  (*ichunk)->length += next->length;

	  /* adjust next pointer */
	  (*ichunk)->next = next->next;

	  /* free old chunk */
	  free(next->data);
	  free(next);
	}
      }
      return 0;
    }

    /* check if this line has to be inserted before the next chunk */
    if(next && (iline->address <= next->address)) {

      /* check if chunk overlaps next one */
      if(iline->address+iline->length-1  >= next->address) {
	fprintf(stderr, "ERROR: data overlap at address 0x%04x!\n",
		iline->address);
	return -1;
      }

      ichunk = &(*ichunk)->next;

      if(!((*ichunk) = ihex_new_chunk(iline))) {
	fprintf(stderr, "ERROR: Error appending new chunk\n");
	return -1;
      }

      /* close chain again */
      (*ichunk)->next = next;

      /* check if the following chunk can be attached as well */
      if(next->address  == (*ichunk)->address + (*ichunk)->length) {
	unsigned char *old_data = (*ichunk)->data;

	(*ichunk)->data = malloc((*ichunk)->length + next->length);
	if(!((*ichunk)->data)) {
	  fprintf(stderr, "ERROR: Out of memory increasing chunk to "
		  "%d bytes!\n", (*ichunk)->length + next->length);
	  return -1;
	}

	/* copy old data */
	memcpy((*ichunk)->data, old_data, (*ichunk)->length);
	free(old_data);

	/* and new data */
	memcpy((*ichunk)->data+(*ichunk)->length, next->data, next->length);

	/* and adjust new size */
	(*ichunk)->length += next->length;

	/* adjust next pointer */
	(*ichunk)->next = next->next;

	/* free old chunk */
	free(next->data);
	free(next);
      }

      return 0;
    }

    /* search next chunk */
    ichunk = &((*ichunk)->next);
  }

  /* we walked all chunks without finding an entry point -> attach to end */
  if(!(*ichunk = ihex_new_chunk(iline))) {
    fprintf(stderr, "ERROR: Error appending new chunk\n");
    return -1;
  }

  return 0;
}

/* walk chunk chain and free it */
static void ihex_free_chunk(ihex_chunk_t *ichunk) {
  if(ichunk) {
    ihex_free_chunk((ihex_chunk_t*)ichunk->next);
    if(ichunk->data) free(ichunk->data);
    free(ichunk);
  }
}

void ihex_free_file(ihex_file_t *ifile) {
  if(ifile) {
    /* free chunk chain */
    ihex_free_chunk(ifile->first);
    free(ifile);
  }
}

ihex_file_t *ihex_parse_file(char *filename) {
  ihex_file_t *ifile = NULL;
  FILE *file = NULL;
  char line[128];

  ifile = malloc(sizeof(ihex_file_t));
  if(!ifile) {
    fprintf(stderr, "ERROR: Out of memory allocating file structure\n");
    return NULL;
  }

  memset(ifile, 0, sizeof(ihex_file_t));

  file = fopen(filename, "r");
  if(!file) {
    fprintf(stderr, "ERROR: Unable to open file %s\n", filename);
    ihex_free_file(ifile);
    return(NULL);
  }

  while(fgets(line, sizeof(line), file) != NULL) {
    ihex_line_t *iline;

    ifile->lines++;

    /* force line termination */
    line[sizeof(line)-1] = 0;

    /* if line is completely white, just skip it */
    if(!ihex_line_is_white(line)) {

      /* parse line */
      if(!(iline = ihex_parse_line(line, ifile->lines))) {
	fprintf(stderr, "ERROR: Hex parsing failed\n");
	ihex_free_file(ifile);
	fclose(file);
	return(NULL);
      }

      /* data must not occur after and marker */
      if(ifile->ended) {
	fprintf(stderr, "ERROR: Data after end marker\n");
	ihex_free_file(ifile);
	fclose(file);
	return(NULL);
      }

      /* remember that the end marker has been found */
      if(iline->type == RECORD_END)
	ifile->ended = 1;

      /* insert data chunks into structure */
      if(iline->type == RECORD_DATA) {
	/* integrate line into file structure */
	if(ihex_insert(ifile, iline) != 0) {
	  fprintf(stderr, "ERROR: Insertion failed\n");
	  ihex_free_file(ifile);
	  fclose(file);
	  return(NULL);
	}
      }

      ihex_free_line(iline);
    }
  }

  if(!ifile->ended) {
    fprintf(stderr, "ERROR: No end marker found\n");
    ihex_free_file(ifile);
    fclose(file);
    return(NULL);
  }

  fclose(file);
  return ifile;
}

int ihex_file_get_chunks(ihex_file_t *ifile) {
  ihex_chunk_t **ichunk = &ifile->first;
  int chunks = 0;

  /* walk chunk chain to count chunks */
  while(*ichunk) {
    chunks++;

    /* search next chunk */
    ichunk = &((*ichunk)->next);
  }

  return chunks;
}

int ihex_file_get_start_address(ihex_file_t *ifile) {
  if(!ifile->first)
    return -1;

  return ifile->first->address;
}

int ihex_file_get_end_address(ihex_file_t *ifile) {
  ihex_chunk_t **ichunk = &ifile->first;
  int address = -1;

  /* walk chunk chain to count chunks */
  while(*ichunk) {
    address = (*ichunk)->address + (*ichunk)->length - 1;

    /* search next chunk */
    ichunk = &((*ichunk)->next);
  }

  return address;
}

int ihex_file_get_mem(ihex_file_t *ifile, int start, int len, char *data) {
  ihex_chunk_t **ichunk = &ifile->first;

  /* walk chunk chain to count chunks */
  while(*ichunk) {
    int soffset = 0, doffset = 0;
    int bytes2copy = (*ichunk)->length;

    /* is anything of this chunk to be copied? */
    /* (is start of chunk before end  of requested area and */
    /* end of chunk after start of requested area?) */
    if(((*ichunk)->address <= (start + len - 1)) &&
       ((*ichunk)->address + (*ichunk)->length >= start )) {

      /* calculate source and destination offset */
      if(start > (*ichunk)->address) {
	/* shift beginning of area to be copied */
	soffset = start - (*ichunk)->address;
	bytes2copy -= soffset;
      }

      if((*ichunk)->address > start)
	doffset = (*ichunk)->address - start;

      /* calculate number of bytes to be copied */
      if(bytes2copy > len-doffset)
	bytes2copy = len-doffset;

      /* and finally copy ... */
      memcpy(data + doffset, (*ichunk)->data + soffset, bytes2copy);
    }

    /* search next chunk */
    ichunk = &((*ichunk)->next);
  }

  return 0;
}
