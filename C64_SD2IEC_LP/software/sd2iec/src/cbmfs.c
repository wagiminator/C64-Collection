// ===================================================================================
// Disk Operating System: CBM View of the File Systems                        * v1.0 *
// ===================================================================================
//
// This part converts between the world of the computer (PETSCII names, file types,
// blocks of 254 bytes) and the FAT file system on the SD card. Disk images are
// handled by image.c, the fs_* functions select the right one by the path.
//
// FAT file names:
// - Long names up to 16 characters (20 with a type extension) are shown as they
//   are, longer names are shown by their 8.3 name.
// - Files with the extension .PRG/.SEQ/.USR/.REL (if extension hiding is on) or
//   P00/S00/U00/R00 (PC64 format with header) get the according file type, all
//   other files are shown as PRG.
// - New files get the CBM name as FAT name. Names that are not valid on FAT and,
//   depending on the extension mode (XE), non-PRG files are saved in P00 format.
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <string.h>
#include <avr/pgmspace.h>
#include "gpio.h"
#include "dos.h"
#include "led.h"
#include "sd.h"

path_t      current_path;
dirhandle_t matchdh;
uint8_t     ext_mode;
uint8_t     ext_hiding;
uint8_t     disk_ready;
uint8_t     disk_changed;
uint8_t     disk_wp;

#define P00_HEADER_SIZE   26
static const char p00marker[] PROGMEM = "C64File";
static const char filetypes[] PROGMEM = "DELSEQPRGUSRRELCBMDIR";
static uint8_t scratch[36];                     // name conversions, P00 headers

// ===================================================================================
// Names
// ===================================================================================

// PETSCII to ASCII (in place)
void pet2asc(uint8_t* s) {
  uint8_t c;
  while((c = *s)) {
    if(c > 192 && c < 219) c -= 128;            // shifted letters -> upper case
    else if(c > 64 && c < 91) c += 32;          // letters -> lower case
    else if(c == 255) c = '~';
    *s++ = c;
  }
}

// ASCII to PETSCII (in place)
void asc2pet(uint8_t* s) {
  uint8_t c;
  while((c = *s)) {
    if(c > 64 && c < 91) c += 128;
    else if(c > 96 && c < 123) c -= 32;
    else if(c == '~') c = 255;
    *s++ = c;
  }
}

// Lower case of a PETSCII character (for case-insensitive matching)
static uint8_t petlower(uint8_t c) {
  if(c > 192 && c < 219) return c - 128;
  return c;
}

// Match a name against a pattern with the wildcards ? and *, returns 1 if it matches
uint8_t match_name(const uint8_t* pattern, const uint8_t* name, uint8_t ignorecase) {
  uint8_t n = CBM_NAME_LENGTH, m, f;
  while(n && *name) {
    m = *pattern; f = *name;
    if(ignorecase) { m = petlower(m); f = petlower(f); }
    if(m == '*') return 1;
    if(m != '?' && m != f) return 0;
    pattern++; name++; n--;
  }
  return !*pattern || *pattern == '*' || !n;
}

// Check for the extension of a disk image (D64, D71, D81), returns 1 if found
uint8_t check_imageext(const uint8_t* name) {
  const uint8_t* ext = (const uint8_t*)strrchr((const char*)name, '.');
  uint8_t a, b, c;
  if(!ext || strlen((const char*)ext) != 4) return 0;
  a = ext[1] | 0x20; b = ext[2]; c = ext[3];
  if(a != 'd' && a != ('d' | 0x80)) return 0;
  return (b == '6' && c == '4') || ((b == '7' || b == '8') && c == '1');
}

// Check the extension of a FAT name: returns the type character for x00 (P00, S01..)
// in *x00, for .PRG/.SEQ/.USR/.REL returns the type character, 0 otherwise
static uint8_t check_extension(const char* name, uint8_t* x00) {
  const char* ext = strrchr(name, '.');
  char f, s, t;
  *x00 = 0;
  if(!ext || strlen(ext) != 4) return 0;
  f = ext[1] & 0xDF; s = ext[2]; t = ext[3];
  if((f == 'P' || f == 'S' || f == 'U' || f == 'R') && s >= '0' && s <= '9' && t >= '0' && t <= '9') {
    *x00 = f;
    return 0;
  }
  s &= 0xDF; t &= 0xDF;
  if((f == 'P' && s == 'R' && t == 'G') || (f == 'S' && s == 'E' && t == 'Q')
  || (f == 'U' && s == 'S' && t == 'R') || (f == 'R' && s == 'E' && t == 'L')) return f;
  return 0;
}

// File type from a type character
static uint8_t typechar2type(uint8_t c) {
  switch(c) {
    case 'S': return TYPE_SEQ;
    case 'U': return TYPE_USR;
    case 'R': return TYPE_REL;
    default:  return TYPE_PRG;
  }
}

// ===================================================================================
// Paths
// ===================================================================================

// Parse a CMD style path ("//DIR/SUB/:NAME", "/SUB/:NAME", "0:NAME"), sets path and
// points name to the file name. for_cd: no colon needed, an image file may end the
// path. Returns 0 if successful.
uint8_t parse_path(uint8_t* in, path_t* path, uint8_t** name, uint8_t for_cd) {
  dirent_t dent;
  uint8_t *end, saved;

  *path = current_path;
  if(!for_cd && !strchr((char*)in, ':')) { *name = in; return 0; }

  while(*in >= '0' && *in <= '9') in++;         // skip the drive number

  if(*in != '/') {
    uint8_t* colon = (uint8_t*)strchr((char*)in, ':');
    *name = colon ? colon + 1 : in;
    return 0;
  }

  while(*in) {
    switch(*in++) {
      case '/':
        if(*in == '/') {                        // double slash: root
          path->image = 0;
          path->dir = 0;
        }
        else if(!*in || *in == ':') {           // end of the path
          *name = *in ? in + 1 : in;
          return 0;
        }
        else {                                  // directory name
          end = in;
          while(*end && *end != '/' && *end != ':') end++;
          saved = *end; *end = 0;
          if(fs_first_match(path, in, FLAG_HIDDEN, &dent)) {
            if(current_error == ERROR_FILE_NOT_FOUND) set_error(ERROR_FILE_NOT_FOUND_39);
            *end = saved;
            return 1;
          }
          if((dent.type & TYPE_MASK) != TYPE_DIR) {
            if(for_cd && !saved && !path->image && check_imageext(in)) {
              *name = in;                       // image file at the end
              return 0;
            }
            *end = saved;
            set_error(ERROR_FILE_NOT_FOUND_39);
            return 1;
          }
          fs_chdir(path, &dent);
          *end = saved;
          in = end;
        }
        break;
      case ':':
        *name = in;
        return 0;
    }
  }
  *name = in;
  return 0;
}

// ===================================================================================
// FAT Directories
// ===================================================================================

// Read the next FAT entry as CBM entry, returns 0 if found, -1 at the end
static int8_t fat_readdir(dirhandle_t* dh, dirent_t* dent) {
  FAT_entry* e = &dent->pvt.fat;
  uint32_t size;                                // data size (without P00 header)
  uint8_t c, x00;

  do {
    if(!FAT_dirRead(&dh->fat, e)) return -1;
  } while(e->sfn[0] == '.' && (e->sfn[1] == ' ' || (e->sfn[1] == '.' && e->sfn[2] == ' ')));

  dent->ops = OPS_FAT;
  dent->type = 0;
  dent->name[CBM_NAME_LENGTH] = 0;
  size = e->size;

  // Name: long names are converted to PETSCII, 8.3 names stay as they are
  memset(scratch, 0, CBM_NAME_LENGTH + 5);
  strncpy((char*)scratch, e->name, CBM_NAME_LENGTH + 4);
  if(e->lfn) asc2pet(scratch);

  if(e->attr & FAT_ATTR_DIR) {
    dent->type = TYPE_DIR;
    if(e->name[0] == '.') dent->type |= FLAG_HIDDEN;
  }
  else {
    c = check_extension(e->name, &x00);
    if(x00) {
      // PC64 file: read the CBM name from the header
      FAT_file f;
      FAT_open(&f, e);
      if(FAT_read(&f, scratch, P00_HEADER_SIZE) == P00_HEADER_SIZE
         && !memcmp_P(scratch, p00marker, 8)) {
        uint8_t i;
        memmove(scratch, scratch + 8, CBM_NAME_LENGTH);
        scratch[CBM_NAME_LENGTH] = 0;
        for(i = 0; i < CBM_NAME_LENGTH; i++) if(scratch[i] == 0xA0) scratch[i] = 0;
        dent->ops = OPS_P00;
        c = x00;
        size -= P00_HEADER_SIZE;
      }
      else {
        // not a valid P00 file: show it as PRG with the FAT name
        memset(scratch, 0, CBM_NAME_LENGTH + 5);
        strncpy((char*)scratch, e->name, CBM_NAME_LENGTH + 4);
        if(e->lfn) asc2pet(scratch);
        c = 'P';
      }
    }
    else if(c && ext_hiding) scratch[strlen((char*)scratch) - 4] = 0;
    else c = 'P';
    dent->type = typechar2type(c);
  }

  // Use the 8.3 name if the name is too long
  if(strlen((char*)scratch) > CBM_NAME_LENGTH) {
    strncpy((char*)scratch, e->name, CBM_NAME_LENGTH + 1);
    if(strlen(e->name) > CBM_NAME_LENGTH) {    // long name that does not fit: 8.3
      uint8_t i, *n = scratch;
      for(i = 0; i < 8 && e->sfn[i] != ' '; i++) *n++ = e->sfn[i];
      if(e->sfn[8] != ' ') {
        *n++ = '.';
        for(i = 8; i < 11 && e->sfn[i] != ' '; i++) *n++ = e->sfn[i];
      }
      *n = 0;
    }
  }
  scratch[CBM_NAME_LENGTH] = 0;
  for(c = 0; scratch[c]; c++) if(scratch[c] == '~') scratch[c] = 0xFF;
  memcpy(dent->name, scratch, CBM_NAME_LENGTH + 1);

  // Size
  if(size > 16255746UL) dent->blocks = 63999;
  else dent->blocks = (size + 253) / 254;
  dent->remainder = size % 254;

  if(e->attr & FAT_ATTR_RO) dent->type |= FLAG_RO;
  if(e->attr & (FAT_ATTR_HIDDEN | FAT_ATTR_SYSTEM)) dent->type |= FLAG_HIDDEN;
  return 0;
}

// Open a directory, returns 0 if successful
uint8_t fs_opendir(dirhandle_t* dh, const path_t* path) {
  dh->image = path->image;
  if(!disk_ready) { set_error(ERROR_DRIVE_NOT_READY); return 1; }
  if(path->image) {
    dh->track = 0;                              // start of the image directory
    return 0;
  }
  FAT_dirOpen(&dh->fat, path->dir);
  return 0;
}

// Read the next entry of a directory
int8_t fs_readdir(dirhandle_t* dh, dirent_t* dent) {
  if(dh->image) return image_readdir(dh, dent);
  return fat_readdir(dh, dent);
}

// Next entry that matches pattern and type (0: any, FLAG_HIDDEN: include hidden)
int8_t fs_next_match(dirhandle_t* dh, const uint8_t* pattern, uint8_t type, dirent_t* dent) {
  int8_t res;
  while(1) {
    res = fs_readdir(dh, dent);
    if(res) return res;
    if((type & TYPE_MASK) && (dent->type & TYPE_MASK) != (type & TYPE_MASK)) continue;
    if((dent->type & FLAG_HIDDEN) && !(type & FLAG_HIDDEN)) continue;
    if(pattern && !match_name(pattern, dent->name, !dh->image)) continue;
    return 0;
  }
}

// First matching entry (sets FILE NOT FOUND if none)
int8_t fs_first_match(const path_t* path, const uint8_t* pattern, uint8_t type, dirent_t* dent) {
  int8_t res;
  if(fs_opendir(&matchdh, path)) return 1;
  res = fs_next_match(&matchdh, pattern, type, dent);
  if(res < 0) set_error(ERROR_FILE_NOT_FOUND);
  return res;
}

// ===================================================================================
// FAT Files
// ===================================================================================

// Refill: read the next 254 bytes into the buffer
static uint8_t fat_file_read(buffer_t* buf) {
  uint16_t n;
  buf->fptr = buf->pvt.fat.fh.pos - buf->pvt.fat.header;
  n = FAT_read(&buf->pvt.fat.fh, buf->data + 2, 254);
  if(!n) {                                      // empty file: a single CR
    n = 1;
    buf->data[2] = 13;
  }
  buf->position = 2;
  buf->lastused = n + 1;
  buf->sendeoi  = (n < 254 || buf->pvt.fat.fh.pos >= buf->pvt.fat.fh.size);
  return 0;
}

// Refill for writing: write the buffer to the file
static uint8_t fat_file_write(buffer_t* buf) {
  uint8_t n = buf->mustflush ? 254 : buf->position - 2;
  LED_dirty(1);
  if(n && FAT_write(&buf->pvt.fat.fh, buf->data + 2, n)) {
    set_error(ERROR_DISK_FULL);
    FAT_close(&buf->pvt.fat.fh);
    buf->cleanup = buffer_dummy;
    buffer_free(buf);
    return 1;
  }
  buf->mustflush = 0;
  buf->dirty     = 0;
  buf->position  = 2;
  buf->lastused  = 2;
  buf->fptr      = buf->pvt.fat.fh.pos - buf->pvt.fat.header;
  return 0;
}

// Cleanup: write the rest and close the file
static uint8_t fat_file_close(buffer_t* buf) {
  uint8_t err = 0;
  if(buf->write) {
    if(buf->dirty || buf->mustflush) err = fat_file_write(buf);
    if(!err && FAT_close(&buf->pvt.fat.fh)) { set_error(ERROR_WRITE_VERIFY); err = 1; }
    LED_dirty(0);
  }
  buf->cleanup = buffer_dummy;
  return err;
}

// Open a FAT file for reading
static void fat_open_read(dirent_t* dent, buffer_t* buf) {
  FAT_open(&buf->pvt.fat.fh, &dent->pvt.fat);
  buf->pvt.fat.header = 0;
  if(dent->ops == OPS_P00) {                    // skip the header
    FAT_seek(&buf->pvt.fat.fh, P00_HEADER_SIZE);
    buf->pvt.fat.header = P00_HEADER_SIZE;
  }
  buf->read    = 1;
  buf->sticky  = 1;
  buf->refill  = fat_file_read;
  buf->cleanup = fat_file_close;
  fat_file_read(buf);
}

// Build the FAT name of a new file in scratch, returns 1 if it is a P00 file
static uint8_t build_name(const uint8_t* name, uint8_t type) {
  uint8_t *p = scratch, valid = 1, dots = 0, c;

  strcpy((char*)scratch, (const char*)name);
  pet2asc(scratch);

  // Disk images and cartridges are always saved without header
  if(type == TYPE_PRG && check_imageext(scratch)) return 0;

  // Check the characters
  for(p = scratch; (c = *p); p++) {
    if(c == '.') dots++;
    if(!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
       || c == '!' || c == ' ' || (c >= '#' && c <= ')') || c == '-' || c == '.')) valid = 0;
  }
  if(dots > 1 || p == scratch || p[-1] == ' ' || p[-1] == '.') valid = 0;

  // PC64 format (P00) for invalid names and depending on the extension mode
  if(!valid || ext_mode == 2 || (ext_mode == 1 && type != TYPE_PRG)) {
    for(p = scratch; (c = *p); p++) {
      if(!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')
         || c == '!' || c == ' ' || (c >= '#' && c <= ')') || c == '-' || c == '.')) *p = '_';
    }
    *p++ = '.';
    *p++ = pgm_read_byte(&filetypes[3 * type]);
    *p++ = '0';
    *p++ = '0';
    *p   = 0;
    return 1;
  }

  // Type extension depending on the extension mode
  if((ext_mode == 3 && type != TYPE_PRG) || ext_mode == 4) {
    while(*p) p++;
    *p++ = '.';
    memcpy_P(p, &filetypes[3 * type], 3);
    p[3] = 0;
  }
  return 0;
}

// Open a FAT file for writing (new file or append)
static void fat_open_write(const path_t* path, dirent_t* dent, uint8_t type, buffer_t* buf, uint8_t append) {
  FAT_file* f = &buf->pvt.fat.fh;
  buf->pvt.fat.header = 0;

  if(append) {
    FAT_open(f, &dent->pvt.fat);
    if(dent->ops == OPS_P00) buf->pvt.fat.header = P00_HEADER_SIZE;
    FAT_seek(f, f->size);
  }
  else {
    uint8_t p00 = build_name(dent->name, type), err;
    uint8_t* num = scratch + strlen((char*)scratch) - 1;
    while((err = FAT_create(f, path->dir, (char*)scratch)) == FAT_ERR_EXISTS && p00) {
      // P00 exists: try P01, P02, ...
      FAT_entry x;
      if(!FAT_find(path->dir, (char*)scratch, &x)) break;
      if(++*num > '9') { *num = '0'; if(++num[-1] > '9') break; }
    }
    if(!err && p00) {
      FAT_find(path->dir, (char*)scratch, &dent->pvt.fat);
    }
    if(err == FAT_ERR_EXISTS && !p00) {
      FAT_entry x;
      if(FAT_find(path->dir, (char*)scratch, &x)) { set_error(ERROR_FILE_EXISTS); return; }
    }
    if(err) {
      set_error(err == FAT_ERR_FULL || err == FAT_ERR_DIRFULL ? ERROR_DISK_FULL : ERROR_WRITE_VERIFY);
      return;
    }
    if(p00) {                                   // write the P00 header
      memset(scratch, 0, P00_HEADER_SIZE);
      strcpy_P((char*)scratch, p00marker);
      memcpy(scratch + 8, dent->name, strlen((char*)dent->name));
      FAT_write(f, scratch, P00_HEADER_SIZE);
      buf->pvt.fat.header = P00_HEADER_SIZE;
    }
  }

  LED_dirty(1);
  buf->write    = 1;
  buf->sticky   = 1;
  buf->position = 2;
  buf->lastused = 2;
  buf->data[2]  = 13;                           // empty file: a single CR
  buf->refill   = fat_file_write;
  buf->cleanup  = fat_file_close;
  buf->fptr     = f->pos - buf->pvt.fat.header;
}

// ===================================================================================
// File System Interface (FAT or image)
// ===================================================================================

void fs_open_read(const path_t* path, dirent_t* dent, buffer_t* buf) {
  if(path->image) image_open_read(dent, buf);
  else fat_open_read(dent, buf);
}

// Card write protected? Sets the error (images check their own flag, see image.c)
static uint8_t fs_protected(void) {
  if(disk_wp) set_error(ERROR_WRITE_PROTECT);
  return disk_wp;
}

void fs_open_write(const path_t* path, dirent_t* dent, uint8_t type, buffer_t* buf, uint8_t append) {
  if(!path->image && fs_protected()) return;
  if(path->image) image_open_write(dent, type, buf, append);
  else fat_open_write(path, dent, type, buf, append);
}

// Delete a file, returns 1 if deleted, 0 if not, 255 on errors
uint8_t fs_delete(const path_t* path, dirent_t* dent) {
  uint8_t err;
  if(path->image) return image_delete(dent);
  if(fs_protected()) return 255;
  LED_dirty(1);
  err = FAT_delete(path->dir, &dent->pvt.fat);
  LED_dirty(0);
  if(err == FAT_ERR_NOTEMPTY) return 0;
  if(err) { set_error(ERROR_WRITE_VERIFY); return 255; }
  return 1;
}

// Rename a file
void fs_rename(const path_t* path, dirent_t* dent, uint8_t* newname) {
  uint8_t err;
  if(path->image) { image_rename(dent, newname); return; }
  if(fs_protected()) return;
  if(dent->ops == OPS_P00) {
    // change the name in the header only
    FAT_file f;
    FAT_open(&f, &dent->pvt.fat);
    memset(scratch, 0, CBM_NAME_LENGTH);
    memcpy(scratch, newname, strlen((char*)newname));
    FAT_seek(&f, 8);
    FAT_write(&f, scratch, CBM_NAME_LENGTH);
    FAT_close(&f);
    return;
  }
  strcpy((char*)scratch, (char*)newname);
  pet2asc(scratch);
  err = FAT_rename(path->dir, &dent->pvt.fat, (char*)scratch);
  if(err == FAT_ERR_EXISTS) set_error(ERROR_FILE_EXISTS);
  else if(err) set_error(ERROR_WRITE_VERIFY);
}

// Create a directory
void fs_mkdir(const path_t* path, uint8_t* name) {
  uint8_t err;
  if(path->image) { set_error(ERROR_SYNTAX_UNABLE); return; }
  if(fs_protected()) return;
  strcpy((char*)scratch, (char*)name);
  pet2asc(scratch);
  err = FAT_mkdir(path->dir, (char*)scratch);
  if(err == FAT_ERR_EXISTS) set_error(ERROR_FILE_EXISTS);
  else if(err) set_error(ERROR_WRITE_VERIFY);
}

// Change into a directory or mount an image, returns 0 if successful.
// An empty name means root, "_" means parent directory.
uint8_t fs_chdir(path_t* path, dirent_t* dent) {
  if(!dent->name[0]) {                          // root
    path->image = 0;
    path->dir = 0;
    return 0;
  }
  if(dent->name[0] == '_' && !dent->name[1]) {  // parent
    FAT_entry e;
    if(path->image) { path->image = 0; return 0; }
    if(!path->dir) return 0;
    if(!FAT_find(path->dir, "..", &e)) { set_error(ERROR_DIR_ERROR); return 1; }
    path->dir = (e.cluster == FAT_root()) ? 0 : e.cluster;
    return 0;
  }
  if(path->image) { set_error(ERROR_FILE_NOT_FOUND_39); return 1; }
  if((dent->type & TYPE_MASK) == TYPE_DIR) {
    path->dir = dent->pvt.fat.cluster;
    if(path->dir == FAT_root()) path->dir = 0;
    return 0;
  }
  if(check_imageext((uint8_t*)dent->pvt.fat.name) || check_imageext(dent->name)) {
    buffers_free(1);
    if(image_mount(path->dir, dent)) return 1;
    path->image = 1;
    return 0;
  }
  set_error(ERROR_FILE_TYPE_MISMATCH);
  return 1;
}

// Position in a file that is open for reading (P command), returns 0 if successful
uint8_t fs_seek(buffer_t* buf, uint32_t pos) {
  if(!buf->read || buf->refill != fat_file_read) return 1;
  if(FAT_seek(&buf->pvt.fat.fh, pos + buf->pvt.fat.header)) return 1;
  return fat_file_read(buf);
}

// Free blocks (254 bytes)
uint16_t fs_freeblocks(void) {
  uint32_t blocks;
  if(current_path.image && image_mounted) return image_freeblocks();
  blocks = FAT_free() * (FAT_clusterSize() / 254);
  return blocks > 65535 ? 65535 : blocks;
}

// Disk name for the directory header (16 bytes, padded with 0xA0)
void fs_label(const path_t* path, uint8_t* label) {
  memset(label, 0xA0, 16);
  if(path->image) { image_label(label); return; }
  if(path->dir) {                               // name of the directory
    FAT_dir d;
    FAT_entry e;
    uint32_t parent = 0;
    if(FAT_find(path->dir, "..", &e)) parent = e.cluster;
    FAT_dirOpen(&d, parent);
    while(FAT_dirRead(&d, &e)) {
      if((e.attr & FAT_ATTR_DIR) && (e.cluster == path->dir) && e.sfn[0] != '.') {
        uint8_t n = strlen(e.name);
        if(n > 16) { n = 0; while(n < 8 && e.sfn[n] != ' ') n++; memcpy(e.name, e.sfn, n); }
        memcpy(scratch, e.name, n); scratch[n] = 0;
        if(e.lfn) asc2pet(scratch);
        memcpy(label, scratch, n);
        return;
      }
    }
  }
  else if(FAT_label((char*)scratch)) {         // volume label of the root
    uint8_t n = 11;
    while(n && scratch[n - 1] == ' ') n--;
    memcpy(label, scratch, n);
    return;
  }
  memcpy_P(label, PSTR(DIR_LABEL), sizeof(DIR_LABEL) - 1);
}

// Disk ID (5 bytes: ID, shifted space, DOS type)
void fs_id(const path_t* path, uint8_t* id) {
  if(path->image) { image_id(id); return; }
  id[0] = DIR_ID[0]; id[1] = DIR_ID[1]; id[2] = 0xA0; id[3] = '2'; id[4] = 'A';
}

// Direct sector access (only in images)
void fs_read_sector(buffer_t* buf, uint8_t track, uint8_t sector) {
  if(current_path.image) image_read_sector(buf->data, track, sector);
  else set_error_ts(ERROR_ILLEGAL_TS_COMMAND, track, sector);
}

void fs_write_sector(buffer_t* buf, uint8_t track, uint8_t sector) {
  if(current_path.image) image_write_sector(buf->data, track, sector);
  else set_error_ts(ERROR_ILLEGAL_TS_COMMAND, track, sector);
}

// Format (only images)
void fs_format(uint8_t* name, uint8_t* id) {
  if(current_path.image) image_format(name, id);
  else set_error(ERROR_SYNTAX_UNABLE);
}

// Write cached data to the card
void fs_sync(void) {
  if(image_mounted) image_sync();
  FAT_sync();
}

// Mount the card and reset the paths
void fs_init(void) {
  image_unmount();
  current_path.image = 0;
  current_path.dir   = 0;
  FAT_unmount();
  disk_ready = 0;
#ifdef PIN_SD_WP
  disk_wp = PIN_read(PIN_SD_WP) ? 1 : 0;       // write protect switch of the card
#endif
  if(SD_init() || FAT_mount()) return;
  disk_ready = 1;
}
