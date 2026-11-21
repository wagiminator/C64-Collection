// ===================================================================================
// Disk Operating System: File Operations (OPEN)                              * v1.0 *
// ===================================================================================
//
// OPEN on a data channel (secondary address 0..14) with the name in command_buffer:
//
//   "$[0][:pattern][=type]"    directory listing (sa 0: BASIC listing, else raw)
//   "#[n]"                     direct access buffer (for U1/U2, B-R/B-W, B-P)
//   "[@][path:]name[,type][,mode]"   file: type P/S/U/L, mode R/W/A/M
//   "*"                        (LOAD) the file loaded last, or the first file
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <string.h>
#include <avr/pgmspace.h>
#include "dos.h"

// Header line of the directory listing
static const uint8_t dirheader[] PROGMEM = {
  1, 4,                                 // BASIC start address $0401
  1, 1,                                 // link (any non-zero value)
  0, 0,                                 // line number 0 (drive)
  0x12, 0x22,                           // reverse on, quote
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',   // disk name
  0x22, 0x20,                           // quote, space
  'S','D',0xA0,'2','A',                 // ID and DOS type
  0                                     // end of line
};
#define HEADER_NAME   8
#define HEADER_ID     26

// Footer line
static const uint8_t dirfooter[] PROGMEM = {
  1, 1, 0, 0,                           // link, free blocks
  'B','L','O','C','K','S',' ','F','R','E','E','.',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ', 0, 0, 0
};

static const char filetypes[] PROGMEM = "DELSEQPRGUSRRELCBMDIR";

// Last file opened for reading ("*")
static dirent_t previous_dent;
static path_t   previous_path;

// ===================================================================================
// Directory Listing
// ===================================================================================

// Create a BASIC line for a directory entry in the buffer
static void createentry(const dirent_t* dent, buffer_t* buf) {
  uint8_t* d = buf->data;
  uint8_t i;

  memset(d, ' ', 31);
  d[31] = 0;
  buf->lastused = 31;
  *d++ = (dent->remainder != 0xFF) ? dent->remainder + 2 : 1;   // link (1571 style)
  *d++ = 1;
  *d++ = dent->blocks;
  *d++ = dent->blocks >> 8;

  // name position depends on the number of digits
  d++;
  if(dent->blocks < 100) d++;
  if(dent->blocks < 10) d++;
  *d++ = '"';
  for(i = 0; i < CBM_NAME_LENGTH && dent->name[i] && dent->name[i] != '"'; i++) d[i] = dent->name[i];
  d[i] = '"';
  d += CBM_NAME_LENGTH + 1;
  if(dent->type & FLAG_SPLAT) *d = '*';
  memcpy_P(d + 1, &filetypes[3 * (dent->type & TYPE_MASK)], 3);
  if(dent->type & FLAG_RO) d[4] = '<';
}

// Refill: next entry of the directory listing, then the footer
static uint8_t dir_refill(buffer_t* buf) {
  dirent_t dent;
  buf->position = 0;
  switch(fs_next_match(&buf->pvt.dir.dh, buf->pvt.dir.match, buf->pvt.dir.type, &dent)) {
    case 0:
      createentry(&dent, buf);
      return 0;
    case -1: {
      uint16_t blocks = fs_freeblocks();
      memcpy_P(buf->data, dirfooter, sizeof(dirfooter));
      buf->data[2] = blocks;
      buf->data[3] = blocks >> 8;
      buf->lastused = 31;
      buf->sendeoi = 1;
      return 0;
    }
    default:
      buffer_free(buf);
      return 1;
  }
}

// Refill: raw directory (1541 directory blocks, 8 entries each; the counter of the
// entry in the block is kept in pvt.dir.type, bit 7 marks the end of the directory)
static uint8_t rawdir_refill(buffer_t* buf) {
  dirent_t dent;
  uint8_t n = buf->pvt.dir.type;
  memset(buf->data, 0, 32);
  if(!(n & 0x80) && fs_readdir(&buf->pvt.dir.dh, &dent) == 0) {
    buf->data[2] = (dent.type & TYPE_MASK) | ((dent.type & FLAG_SPLAT) ? 0 : 0x80)
                 | ((dent.type & FLAG_RO) ? 0x40 : 0);
    buf->data[3] = 1;
    memset(buf->data + 5, 0xA0, CBM_NAME_LENGTH);
    memcpy(buf->data + 5, dent.name, strlen((char*)dent.name));
    buf->data[30] = dent.blocks;
    buf->data[31] = dent.blocks >> 8;
  }
  else {                                        // end: fill the block with empty entries
    n |= 0x80;
    if((n & 7) == 7) buf->sendeoi = 1;
  }
  buf->position = (n & 7) ? 0 : 2;              // link bytes only once per block
  buf->lastused = 31;
  buf->pvt.dir.type = (n & 0x80) | ((n + 1) & 7);
  return 0;
}

// Prepare the directory listing on a channel
static void load_directory(uint8_t secondary) {
  buffer_t* buf = buffer_alloc();
  path_t path = current_path;
  uint8_t *name = 0, *p;

  if(!buf) return;
  buf->secondary = secondary;
  buf->read = 1;
  buf->sticky = 1;

  if(command_length > 2) {
    if(parse_path(command_buffer + 1, &path, &name, 0)) { buffer_free(buf); return; }
    if(!*name) name = 0;
    p = name ? (uint8_t*)strchr((char*)name, '=') : 0;
    if(p) {
      *p++ = 0;
      switch(*p) {
        case 'S': buf->pvt.dir.type = TYPE_SEQ; break;
        case 'P': buf->pvt.dir.type = TYPE_PRG; break;
        case 'U': buf->pvt.dir.type = TYPE_USR; break;
        case 'R': buf->pvt.dir.type = TYPE_REL; break;
        case 'C': buf->pvt.dir.type = TYPE_CBM; break;
        case 'D': case 'B': buf->pvt.dir.type = TYPE_DIR; break;
        case 'H': buf->pvt.dir.type = FLAG_HIDDEN; break;
      }
      if(!*name) name = 0;
    }
  }
  else if(command_length == 2 && command_buffer[1] != '0') name = command_buffer + 1;

  buf->pvt.dir.match = name;
  if(fs_opendir(&buf->pvt.dir.dh, &path)) { buffer_free(buf); return; }

  if(secondary) {
    // raw directory: a BAM-like first block, then the entries
    memset(buf->data, 0, 256);
    buf->data[0] = 'A';
    fs_label(&path, buf->data + 0x90 - 2);
    memset(buf->data + 0xA0 - 2, 0xA0, 2);
    fs_id(&path, buf->data + 0xA2 - 2);
    memset(buf->data + 0xA7 - 2, 0xA0, 4);
    buf->position = 0;
    buf->lastused = 253;
    buf->pvt.dir.type = 0;
    buf->refill = rawdir_refill;
    return;
  }

  memcpy_P(buf->data, dirheader, sizeof(dirheader));
  fs_label(&path, buf->data + HEADER_NAME);
  for(p = buf->data + HEADER_NAME; p < buf->data + HEADER_NAME + 16; p++) if(*p == 0xA0) *p = ' ';
  fs_id(&path, buf->data + HEADER_ID);
  buf->data[HEADER_ID + 2] = ' ';
  buf->position = 0;
  buf->lastused = 31;
  buf->refill = dir_refill;
}

// ===================================================================================
// Direct Access Buffers
// ===================================================================================

// Refill of "#" buffers: start again at the beginning
uint8_t directbuffer_refill(buffer_t* buf) {
  buf->position = 0;
  buf->mustflush = 0;
  return 0;
}

static void open_buffer(uint8_t secondary) {
  buffer_t* buf = buffer_alloc();
  if(!buf) return;
  buf->secondary = secondary;
  buf->read      = 1;
  buf->write     = 1;
  buf->sticky    = 1;
  buf->position  = 1;
  buf->lastused  = 255;
  buf->sendeoi   = 1;
  buf->refill    = directbuffer_refill;
}

// ===================================================================================
// OPEN
// ===================================================================================
enum { OPEN_READ, OPEN_WRITE, OPEN_APPEND, OPEN_MODIFY };

void file_open(uint8_t secondary) {
  buffer_t* buf;
  dirent_t dent;
  path_t path;
  uint8_t *ptr = command_buffer, *fname, mode = OPEN_READ, type = TYPE_DEL, i = 0;
  int8_t res;

  // Close the channel if it is still open
  buf = buffer_find(secondary);
  if(buf) buffer_free(buf);

  set_error(ERROR_OK);
  if(!disk_ready) { set_error(ERROR_DRIVE_NOT_READY); return; }

  // Strip CR at the end and terminate the name
  if(command_length && command_buffer[command_length - 1] == 13) command_length--;
  memset(command_buffer + command_length, 0, sizeof(command_buffer) - command_length);

  if(command_buffer[0] == '#') { open_buffer(secondary); return; }

  // "*": the last file loaded
  if(secondary == 0 && command_length == 1 && command_buffer[0] == '*') {
    if(previous_dent.name[0]) {
      dent = previous_dent;
      if(!(buf = buffer_alloc())) return;
      buf->secondary = 0;
      fs_open_read(&previous_path, &dent, buf);
      return;
    }
    type = TYPE_PRG;
  }

  // Type and mode after the name
  while(i++ < 2 && *ptr && (ptr = (uint8_t*)strchr((char*)ptr, ','))) {
    *ptr++ = 0;
    switch(*ptr) {
      case 'R': mode = OPEN_READ;   break;
      case 'W': mode = OPEN_WRITE;  break;
      case 'A': mode = OPEN_APPEND; break;
      case 'M': mode = OPEN_MODIFY; break;
      case 'S': type = TYPE_SEQ;    break;
      case 'P': type = TYPE_PRG;    break;
      case 'U': type = TYPE_USR;    break;
      case 'D': type = TYPE_DEL;    break;
      case 'L': set_error(ERROR_SYNTAX_UNABLE); return;   // REL files: not supported
    }
  }

  if(command_buffer[0] == '$') { load_directory(secondary); return; }

  if(parse_path(command_buffer, &path, &fname, 0)) return;
  if(!*fname) { set_error(ERROR_SYNTAX_NONAME); return; }

  // Secondary addresses 0 and 1: LOAD and SAVE
  if(secondary == 0) { mode = OPEN_READ; if(type == TYPE_DEL) type = TYPE_PRG; }
  else if(secondary == 1) { mode = OPEN_WRITE; if(type == TYPE_DEL) type = TYPE_PRG; }
  else if(type == TYPE_DEL) type = TYPE_SEQ;

  if(mode == OPEN_WRITE && (strchr((char*)fname, '*') || strchr((char*)fname, '?'))) {
    set_error(ERROR_SYNTAX_JOKER);
    return;
  }

  // Find the file (directories do not count, except for writing)
  if(fs_opendir(&matchdh, &path)) return;
  do {
    res = fs_next_match(&matchdh, fname, FLAG_HIDDEN, &dent);
    if(res > 0) return;
  } while(!res && mode != OPEN_WRITE &&
          ((dent.type & TYPE_MASK) == TYPE_DIR || (dent.type & TYPE_MASK) == TYPE_DEL));

  if(!res && (dent.type & TYPE_MASK) == TYPE_REL) { set_error(ERROR_FILE_TYPE_MISMATCH); return; }

  if(mode == OPEN_WRITE) {
    if(!res) {
      if(command_buffer[0] != '@') { set_error(ERROR_FILE_EXISTS); return; }
      if((dent.type & TYPE_MASK) == TYPE_DIR) { set_error(ERROR_FILE_TYPE_MISMATCH); return; }
      if(fs_delete(&path, &dent) == 255) return;  // replace: delete the old file
    }
    memset(&dent, 0, sizeof(dent));
    strncpy((char*)dent.name, (char*)fname, CBM_NAME_LENGTH);
    set_error(ERROR_OK);
  }
  else if(res) { set_error(ERROR_FILE_NOT_FOUND); return; }

  if(!(buf = buffer_alloc())) return;
  buf->secondary = secondary;

  if(mode == OPEN_READ || mode == OPEN_MODIFY) {
    previous_dent = dent;
    previous_path = path;
    fs_open_read(&path, &dent, buf);
  }
  else fs_open_write(&path, &dent, type, buf, mode == OPEN_APPEND);
}
