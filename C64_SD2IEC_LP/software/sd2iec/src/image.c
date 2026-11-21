// ===================================================================================
// Disk Operating System: Disk Images (D64, D71, D81)                         * v1.0 *
// ===================================================================================
//
// A disk image on the SD card can be mounted like a disk ("CD:GAME.D64"). Files in
// the image can be loaded, saved, scratched and renamed, the blocks can be accessed
// directly (U1/U2, B-R/B-W, B-A/B-F) and the image can be formatted (N:).
//
// Supported formats (recognized by the file size):
//   D64  35 or 40 tracks (40 track BAM in SpeedDOS format), with or without error info
//   D71  70 tracks (double sided 1571)
//   D81  80 tracks (1581)
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <string.h>
#include <avr/pgmspace.h>
#include "dos.h"
#include "fastloader.h"
#include "led.h"

// ===================================================================================
// Image State
// ===================================================================================
enum { IMG_NONE = 0, IMG_D64 = 41, IMG_D71 = 71, IMG_D81 = 81 };

uint8_t image_mounted;
static struct {
  uint8_t  type;                        // IMG_D64, IMG_D71, IMG_D81
  uint8_t  tracks;                      // number of tracks
  uint8_t  dirtrack;                    // directory track (18 or 40)
  uint8_t  ro;                          // image is read only
  FAT_file fh;                          // the image file
} img;

// BAM cache: one BAM sector in RAM
static uint8_t bam[256];
static uint8_t blk[256];                // directory block / scratch block
static uint8_t blk_t, blk_s;            // directory block in blk (track 0: none)
static uint8_t bam_track, bam_sector;   // sector in the cache (track 0: none)
static uint8_t bam_dirty;

// Sectors per track
static uint8_t sectors(uint8_t track) {
  if(img.type == IMG_D81) return 40;
  if(track > 35) track -= 35;                   // second side of a D71
  if(img.type == IMG_D64 && img.tracks == 40 && track > 35) track = 35;
  if(track < 18) return 21;
  if(track < 25) return 19;
  if(track < 31) return 18;
  return 17;
}

// Check track and sector, returns 1 if valid
uint8_t image_check_ts(uint8_t track, uint8_t sector) {
  if(!image_mounted || !track || track > img.tracks) return 0;
  return sector < sectors(track);
}

// Offset of a block in the image file
static uint32_t block_offset(uint8_t track, uint8_t sector) {
  uint16_t lba = 0;
  uint8_t t;
  if(img.type == IMG_D81) lba = (track - 1) * 40;
  else for(t = 1; t < track; t++) lba += sectors(t);
  return ((uint32_t)(lba + sector)) << 8;
}

// Read a block, returns 0 if successful
static uint8_t read_block(uint8_t* data, uint8_t track, uint8_t sector) {
  if(!image_check_ts(track, sector)) { set_error_ts(ERROR_ILLEGAL_TS_LINK, track, sector); return 1; }
  if(FAT_seek(&img.fh, block_offset(track, sector)) || FAT_read(&img.fh, data, 256) != 256) {
    set_error_ts(ERROR_READ_NOSYNC, track, sector);
    return 1;
  }
  return 0;
}

// Write a block, returns 0 if successful
static uint8_t write_block(const uint8_t* data, uint8_t track, uint8_t sector) {
  if(img.ro) { set_error(ERROR_WRITE_PROTECT); return 1; }
  if(!image_check_ts(track, sector)) { set_error_ts(ERROR_ILLEGAL_TS_LINK, track, sector); return 1; }
  if(data != blk && track == blk_t && sector == blk_s) blk_t = 0;
  LED_dirty(1);
  if(FAT_seek(&img.fh, block_offset(track, sector)) || FAT_write(&img.fh, data, 256)) {
    set_error_ts(ERROR_WRITE_VERIFY, track, sector);
    return 1;
  }
  return 0;
}

// ===================================================================================
// BAM (Block Availability Map)
// ===================================================================================

// Write the cached BAM sector if changed, returns 0 if successful
static uint8_t bam_flush(void) {
  if(bam_dirty) {
    if(write_block(bam, bam_track, bam_sector)) return 1;
    bam_dirty = 0;
  }
  return 0;
}

// Load a BAM sector into the cache, returns 0 if successful
static uint8_t bam_load(uint8_t track, uint8_t sector) {
  if(bam_track == track && bam_sector == sector) return 0;
  if(bam_flush()) return 1;
  bam_track = 0;
  if(read_block(bam, track, sector)) return 1;
  bam_track = track;
  bam_sector = sector;
  return 0;
}

// Location of the free counter (*count) and the bitmap of a track, returns 0 if ok.
// For D71 tracks 36..70 the counter is in 18/0 and the bitmap in 53/0: the counter
// is returned in *countptr only while 18/0 is in the cache (see the callers).
static uint8_t bam_map(uint8_t track, uint8_t** map) {
  switch(img.type) {
    case IMG_D81:
      if(bam_load(40, track > 40 ? 2 : 1)) return 1;
      *map = bam + 0x10 + 6 * ((track - 1) % 40);
      return 0;
    case IMG_D71:
      if(track > 35) {
        if(bam_load(53, 0)) return 1;
        *map = bam + 3 * (track - 36) - 1;      // bitmap starts at map[1]
        return 0;
      }
      // fall through
    default:
      if(bam_load(18, 0)) return 1;
      if(track > 35) *map = bam + 0xC0 + 4 * (track - 36);   // SpeedDOS 40 tracks
      else *map = bam + 4 * track;
      return 0;
  }
}

// Change the free counter of a D71 track on the second side
static uint8_t d71_count(uint8_t track, int8_t delta, uint8_t* value) {
  if(bam_load(18, 0)) return 1;
  bam[0xDD + track - 36] += delta;
  bam_dirty |= (delta != 0);
  *value = bam[0xDD + track - 36];
  return 0;
}

// Number of free blocks on a track
static uint8_t track_free(uint8_t track) {
  uint8_t* map, n;
  if(img.type == IMG_D71 && track > 35) {
    if(d71_count(track, 0, &n)) return 0;
    return n;
  }
  if(bam_map(track, &map)) return 0;
  return map[0];
}

// Check if a block is free: 1 free, 0 used, -1 error
static int8_t is_free(uint8_t track, uint8_t sector) {
  uint8_t* map;
  if(bam_map(track, &map)) return -1;
  return (map[1 + (sector >> 3)] >> (sector & 7)) & 1;
}

// Mark a block as used (alloc = 1) or free (alloc = 0), returns 0 if successful
static uint8_t bam_set(uint8_t track, uint8_t sector, uint8_t alloc) {
  uint8_t* map, mask = 1 << (sector & 7), n;
  if(bam_map(track, &map)) return 1;
  map += 1 + (sector >> 3);
  if(alloc == !(*map & mask)) return 0;         // already in this state
  if(alloc) *map &= ~mask; else *map |= mask;
  bam_dirty = 1;
  if(img.type == IMG_D71 && track > 35) return d71_count(track, alloc ? -1 : 1, &n);
  if(bam_map(track, &map)) return 1;
  map[0] += alloc ? -1 : 1;
  bam_dirty = 1;
  return 0;
}

// Write the BAM (and the image file entry) to the card
void image_sync(void) {
  if(image_mounted) {
    bam_flush();
    FAT_close(&img.fh);
    LED_dirty(0);
  }
}

// File interleave
static uint8_t interleave(uint8_t track) {
  if(img.type == IMG_D81) return 1;
  if(track == img.dirtrack) return 3;
  return img.type == IMG_D71 ? 6 : 10;
}

// First block of a new file: free track closest to the directory track
static uint8_t first_block(uint8_t* track, uint8_t* sector) {
  int8_t distance = 1;
  while(distance < img.tracks) {
    int16_t t = img.dirtrack - distance;
    if(t >= 1 && t <= img.tracks && !(img.type == IMG_D71 && t == 53) && track_free(t)) {
      *track = t;
      for(*sector = 0; *sector < sectors(t); (*sector)++) {
        if(is_free(t, *sector) > 0) return 0;
      }
    }
    distance = -distance;                       // alternate below/above
    if(distance > 0) distance++;
  }
  set_error(ERROR_DISK_FULL);
  return 1;
}

// Next block of a file (1541 algorithm with interleave), returns 0 if successful
static uint8_t next_block(uint8_t* track, uint8_t* sector) {
  uint8_t tries = 0, n;

  if(*track == img.dirtrack && !track_free(*track)) { set_error(ERROR_DISK_FULL); return 1; }
  n = interleave(*track);

  // find a track with free blocks
  while(tries < 3 && !track_free(*track)) {
    if(*track < img.dirtrack) (*track)--;
    else {
      (*track)++;
      if(img.type == IMG_D71 && *track == 53) (*track)++;
    }
    if(*track < 1) { *track = img.dirtrack + 1; *sector = 0; tries++; }
    if(*track > img.tracks) { *track = img.dirtrack - 1; *sector = 0; tries++; }
  }
  if(tries == 3) { set_error(ERROR_DISK_FULL); return 1; }

  // block at interleave distance, then the next free one
  *sector += n;
  if(*sector >= sectors(*track)) {
    *sector -= sectors(*track);
    if(*sector) (*sector)--;
  }
  for(tries = 0; tries < 50; tries++) {
    if(is_free(*track, *sector) > 0) return 0;
    if(++*sector >= sectors(*track)) *sector = 0;
  }
  set_error(ERROR_DISK_FULL);
  return 1;
}

// Free blocks of the image (without the directory track)
uint16_t image_freeblocks(void) {
  uint16_t n = 0;
  uint8_t t;
  for(t = 1; t <= img.tracks; t++) {
    if(t == img.dirtrack || (img.type == IMG_D71 && t == 53)) continue;
    n += track_free(t);
  }
  return n;
}

// ===================================================================================
// Mount
// ===================================================================================

// Mount a disk image from a FAT directory entry, returns 0 if successful
uint8_t image_mount(uint32_t dir, dirent_t* dent) {
  uint32_t size = dent->pvt.fat.size;
  image_unmount();
  img.tracks = 35;
  img.dirtrack = 18;
  switch(size) {
    case 174848UL: case 175531UL: img.type = IMG_D64; break;
    case 196608UL: case 197376UL: img.type = IMG_D64; img.tracks = 40; break;
    case 349696UL: case 351062UL: img.type = IMG_D71; img.tracks = 70; break;
    case 819200UL: case 822400UL: img.type = IMG_D81; img.tracks = 80; img.dirtrack = 40; break;
    default: set_error(ERROR_IMAGE_INVALID); return 1;
  }
  FAT_open(&img.fh, &dent->pvt.fat);
  img.ro = (dent->pvt.fat.attr & FAT_ATTR_RO) || disk_wp;   // read only file or card
  bam_track = 0;
  bam_dirty = 0;
  blk_t = 0;
  image_mounted = 1;
  fl_dirchanged = 1;                            // Wheels: report the disk change
  return 0;
}

// Unmount the image (writes the BAM)
void image_unmount(void) {
  if(image_mounted) image_sync();
  image_mounted = 0;
  img.type = IMG_NONE;
  bam_track = 0;
}

// Drive type of the image (for drive detection)
uint8_t image_drive_type(void) {
  return image_mounted ? img.type : IMG_NONE;
}

// ===================================================================================
// Directory
// ===================================================================================

// First directory block
void image_dir_start(uint8_t* track, uint8_t* sector) {
  *track = img.dirtrack;
  *sector = (img.type == IMG_D81) ? 3 : 1;
}

// Read the next directory entry into dent, returns 0 if found, -1 at the end
int8_t image_readdir(dirhandle_t* dh, dirent_t* dent) {
  uint8_t* e;

  if(!dh->track) {                              // start of the directory
    image_dir_start(&dh->track, &dh->sector);
    dh->entry = 0;
    blk_t = 0;
  }
  while(1) {
    if(blk_t != dh->track || blk_s != dh->sector) {
      if(read_block(blk, dh->track, dh->sector)) { blk_t = 0; return 1; }
      blk_t = dh->track; blk_s = dh->sector;
    }
    if(dh->entry == 8) {                        // next directory block
      if(!blk[0]) return -1;
      dh->track = blk[0]; dh->sector = blk[1]; dh->entry = 0;
      continue;
    }
    e = blk + 32 * dh->entry++;
    if(!e[2]) continue;                         // empty or scratched entry
    memset(dent, 0, sizeof(dirent_t));
    memcpy(dent->name, e + 5, CBM_NAME_LENGTH);
    {
      uint8_t i;
      for(i = 0; i < CBM_NAME_LENGTH; i++) if(dent->name[i] == 0xA0) { dent->name[i] = 0; break; }
    }
    dent->ops = OPS_IMAGE;
    dent->type = e[2] & 0x07;
    if(dent->type > TYPE_REL) dent->type = TYPE_PRG;
    if(!(e[2] & 0x80)) dent->type |= FLAG_SPLAT;
    if(e[2] & 0x40) dent->type |= FLAG_RO;
    dent->blocks = e[30] | (e[31] << 8);
    dent->remainder = 0xFF;
    dent->pvt.img.track   = e[3];
    dent->pvt.img.sector  = e[4];
    dent->pvt.img.dtrack  = dh->track;
    dent->pvt.img.dsector = dh->sector;
    dent->pvt.img.dentry  = dh->entry - 1;
    return 0;
  }
}

// Disk name (16 bytes, 0xA0 padded)
void image_label(uint8_t* label) {
  if(img.type == IMG_D81) { if(!bam_load(40, 0)) memcpy(label, bam + 4, 16); }
  else if(!bam_load(18, 0)) memcpy(label, bam + 0x90, 16);
}

// Disk ID (5 bytes)
void image_id(uint8_t* id) {
  if(img.type == IMG_D81) { if(!bam_load(40, 0)) memcpy(id, bam + 0x16, 5); }
  else if(!bam_load(18, 0)) memcpy(id, bam + 0xA2, 5);
}

// Change a directory entry (field at offset, n bytes), returns 0 if successful
static uint8_t update_entry(const dirent_t* dent, uint8_t offset, const uint8_t* data, uint8_t n) {
  blk_t = 0;
  if(bam_flush() || read_block(blk, dent->pvt.img.dtrack, dent->pvt.img.dsector)) return 1;
  memcpy(blk + 32 * dent->pvt.img.dentry + offset, data, n);
  return write_block(blk, dent->pvt.img.dtrack, dent->pvt.img.dsector);
}

// ===================================================================================
// Files
// ===================================================================================

// Refill: read the next block of the file
static uint8_t image_file_read(buffer_t* buf) {
  uint8_t t = buf->data[0], s = buf->data[1];
  if(!t) { buf->sendeoi = 1; return 0; }        // no more blocks
  buf->fptr += 254;
  if(read_block(buf->data, t, s)) { buffer_free(buf); return 1; }
  buf->pvt.img.track = t;
  buf->pvt.img.sector = s;
  buf->position = 2;
  if(buf->data[0]) { buf->lastused = 255; buf->sendeoi = 0; }
  else {
    buf->lastused = buf->data[1];
    if(buf->lastused < 2) buf->lastused = 2;
    buf->sendeoi = 1;
  }
  return 0;
}

// Open a file in the image for reading
void image_open_read(dirent_t* dent, buffer_t* buf) {
  buf->data[0] = dent->pvt.img.track;
  buf->data[1] = dent->pvt.img.sector;
  buf->fptr = (uint32_t)-254;
  buf->read = 1;
  buf->sticky = 1;
  buf->refill = image_file_read;
  if(image_file_read(buf)) return;
  if(!dent->pvt.img.track) { buf->data[2] = 13; buf->lastused = 2; buf->sendeoi = 1; }
}

// Refill for writing: the buffer is full, write it with a link to the next block
static uint8_t image_file_write(buffer_t* buf) {
  uint8_t t = buf->pvt.img.track, s = buf->pvt.img.sector;
  if(!buf->mustflush) return 0;                 // only full buffers are written here
  if(next_block(&t, &s) || bam_set(t, s, 1)) { buffer_free(buf); return 1; }
  buf->data[0] = t;
  buf->data[1] = s;
  if(write_block(buf->data, buf->pvt.img.track, buf->pvt.img.sector)) { buffer_free(buf); return 1; }
  buf->pvt.img.track = t;
  buf->pvt.img.sector = s;
  buf->pvt.img.blocks++;
  buf->mustflush = 0;
  buf->position = 2;
  buf->lastused = 2;
  buf->fptr += 254;
  return 0;
}

// Cleanup: write the last block and close the directory entry
static uint8_t image_file_close(buffer_t* buf) {
  dirent_t dent;
  uint8_t data[3];
  if(buf->mustflush && image_file_write(buf)) return 1;
  buf->data[0] = 0;
  buf->data[1] = buf->position > 2 ? buf->position - 1 : 2;
  if(buf->position <= 2) buf->data[2] = 13;
  if(write_block(buf->data, buf->pvt.img.track, buf->pvt.img.sector)) return 1;
  buf->pvt.img.blocks++;

  // close the entry: type with the "closed" flag, block count
  dent.pvt.img.dtrack  = buf->pvt.img.dtrack;
  dent.pvt.img.dsector = buf->pvt.img.dsector;
  dent.pvt.img.dentry  = buf->pvt.img.dentry;
  data[0] = buf->pvt.img.type | 0x80;
  if(update_entry(&dent, 2, data, 1)) return 1;
  data[0] = buf->pvt.img.blocks; data[1] = buf->pvt.img.blocks >> 8;
  if(update_entry(&dent, 30, data, 2)) return 1;
  buf->cleanup = buffer_dummy;
  image_sync();
  return 0;
}

// Find a free directory entry (adds a directory block if needed), returns 0 if ok
static uint8_t find_free_entry(uint8_t* block, uint8_t* t, uint8_t* s, uint8_t* entry) {
  uint8_t i;
  image_dir_start(t, s);
  while(1) {
    if(read_block(block, *t, *s)) return 1;
    for(i = 0; i < 8; i++) {
      if(!block[32 * i + 2]) { *entry = i; return 0; }
    }
    if(!block[0]) {                             // last block: append a new one
      uint8_t nt = *t, ns = *s;
      if(next_block(&nt, &ns) || nt != img.dirtrack || bam_set(nt, ns, 1)) {
        set_error(ERROR_DISK_FULL); return 1;
      }
      block[0] = nt; block[1] = ns;
      if(write_block(block, *t, *s)) return 1;
      memset(block, 0, 256);
      block[1] = 0xFF;
      if(write_block(block, nt, ns)) return 1;
      *t = nt; *s = ns; *entry = 0;
      return 0;
    }
    *t = block[0]; *s = block[1];
  }
}

// Open a file in the image for writing (new file or append)
void image_open_write(dirent_t* dent, uint8_t type, buffer_t* buf, uint8_t append) {
  uint8_t t, s, e, i;
  if(img.ro) { set_error(ERROR_WRITE_PROTECT); return; }

  if(append) {
    // read up to the last block of the file
    uint16_t blocks = 0;
    t = dent->pvt.img.track; s = dent->pvt.img.sector;
    while(1) {
      if(read_block(buf->data, t, s)) return;
      blocks++;
      if(!buf->data[0]) break;
      t = buf->data[0]; s = buf->data[1];
    }
    buf->pvt.img.track   = t;
    buf->pvt.img.sector  = s;
    buf->pvt.img.dtrack  = dent->pvt.img.dtrack;
    buf->pvt.img.dsector = dent->pvt.img.dsector;
    buf->pvt.img.dentry  = dent->pvt.img.dentry;
    buf->pvt.img.blocks  = blocks - 1;
    buf->pvt.img.type    = dent->type & TYPE_MASK;
    buf->position = buf->data[1] + 1;
    if(!buf->position) buf->mustflush = 1;
  }
  else {
    // new directory entry and first block
    if(find_free_entry(buf->data, &t, &s, &e)) return;
    buf->pvt.img.dtrack = t; buf->pvt.img.dsector = s; buf->pvt.img.dentry = e;
    if(first_block(&t, &s) || bam_set(t, s, 1)) return;
    if(read_block(buf->data, buf->pvt.img.dtrack, buf->pvt.img.dsector)) return;
    {
      uint8_t* p = buf->data + 32 * e;
      memset(p + 2, 0, 30);
      p[2] = type;                              // not closed yet
      p[3] = t; p[4] = s;
      memset(p + 5, 0xA0, 16);
      for(i = 0; i < CBM_NAME_LENGTH && dent->name[i]; i++) p[5 + i] = dent->name[i];
    }
    if(write_block(buf->data, buf->pvt.img.dtrack, buf->pvt.img.dsector)) return;
    if(bam_flush()) return;
    buf->pvt.img.track  = t;
    buf->pvt.img.sector = s;
    buf->pvt.img.blocks = 0;
    buf->pvt.img.type   = type;
    buf->position = 2;
    buf->fptr = 0;
  }
  buf->lastused = 2;
  buf->write    = 1;
  buf->sticky   = 1;
  buf->refill   = image_file_write;
  buf->cleanup  = image_file_close;
  LED_dirty(1);
}

// Scratch a file, returns 1 if deleted, 255 on errors
uint8_t image_delete(dirent_t* dent) {
  uint8_t link[2], t = dent->pvt.img.track, s = dent->pvt.img.sector;
  uint16_t n = 0;
  if(img.ro) { set_error(ERROR_WRITE_PROTECT); return 255; }
  while(t && n++ < 3200) {                      // free the blocks of the file
    if(!image_check_ts(t, s) || FAT_seek(&img.fh, block_offset(t, s))
       || FAT_read(&img.fh, link, 2) != 2) break;
    bam_set(t, s, 0);
    t = link[0]; s = link[1];
  }
  link[0] = 0;
  if(update_entry(dent, 2, link, 1)) return 255;
  image_sync();
  return 1;
}

// Rename a file
void image_rename(dirent_t* dent, uint8_t* newname) {
  uint8_t name[16], i;
  memset(name, 0xA0, 16);
  for(i = 0; i < 16 && newname[i]; i++) name[i] = newname[i];
  update_entry(dent, 5, name, 16);
  image_sync();
}

// Direct access: read and write blocks
void image_read_sector(uint8_t* data, uint8_t track, uint8_t sector) {
  if(!image_check_ts(track, sector)) { set_error_ts(ERROR_ILLEGAL_TS_COMMAND, track, sector); return; }
  bam_flush();
  if(bam_track == track && bam_sector == sector) bam_track = 0;
  read_block(data, track, sector);
}

void image_write_sector(uint8_t* data, uint8_t track, uint8_t sector) {
  if(!image_check_ts(track, sector)) { set_error_ts(ERROR_ILLEGAL_TS_COMMAND, track, sector); return; }
  if(bam_track == track && bam_sector == sector) bam_track = 0;   // BAM overwritten
  if(!write_block(data, track, sector)) image_sync();
}

// Direct access: allocate (B-A) and free (B-F) blocks
void image_block_alloc(uint8_t track, uint8_t sector, uint8_t alloc) {
  if(!image_check_ts(track, sector)) { set_error_ts(ERROR_ILLEGAL_TS_COMMAND, track, sector); return; }
  if(alloc && is_free(track, sector) == 0) {
    // B-A of a used block: report the next free one
    uint8_t t = track, s = sector;
    if(next_block(&t, &s)) set_error(ERROR_DISK_FULL);
    else set_error_ts(ERROR_NO_BLOCK, t, s);
    return;
  }
  bam_set(track, sector, alloc);
  image_sync();
}

// ===================================================================================
// Format (N:name,id)
// ===================================================================================
void image_format(uint8_t* name, uint8_t* id) {
  uint8_t t, s, i, old[5], *block = blk;
  if(img.ro) { set_error(ERROR_WRITE_PROTECT); return; }
  bam_flush();
  blk_t = 0;
  LED_dirty(1);

  // Full format with ID: clear all blocks, otherwise keep the old ID
  if(id) {
    memset(block, 0, 256);
    for(t = 1; t <= img.tracks; t++)
      for(s = 0; s < sectors(t); s++)
        if(write_block(block, t, s)) return;
  }
  else {
    image_id(old);
    id = old;
  }
  bam_track = 0;

  if(img.type == IMG_D81) {
    // header 40/0
    memset(bam, 0, 256);
    bam[0] = 40; bam[1] = 3; bam[2] = 'D';
    memset(bam + 4, 0xA0, 25);
    for(i = 0; i < 16 && name[i]; i++) bam[4 + i] = name[i];
    bam[0x16] = id[0]; bam[0x17] = id[1]; bam[0x19] = '3'; bam[0x1A] = 'D';
    if(write_block(bam, 40, 0)) return;
    // BAM 40/1 and 40/2
    for(s = 1; s <= 2; s++) {
      memset(bam, 0, 256);
      bam[0] = (s == 1) ? 40 : 0; bam[1] = (s == 1) ? 2 : 0xFF;
      bam[2] = 'D'; bam[3] = 0xBB; bam[4] = id[0]; bam[5] = id[1]; bam[6] = 0xC0;
      for(t = 0; t < 40; t++) {
        uint8_t* m = bam + 0x10 + 6 * t;
        m[0] = 40; memset(m + 1, 0xFF, 5);
      }
      if(s == 1) { bam[0x10 + 6 * 39] = 36; bam[0x10 + 6 * 39 + 1] = 0xF0; }  // track 40: 0-3 used
      if(write_block(bam, 40, s)) return;
    }
    memset(block, 0, 256); block[1] = 0xFF;
    if(write_block(block, 40, 3)) return;
  }
  else {
    // BAM 18/0
    memset(bam, 0, 256);
    bam[0] = 18; bam[1] = 1; bam[2] = 'A';
    if(img.type == IMG_D71) bam[3] = 0x80;
    for(t = 1; t <= 35; t++) {
      uint8_t n = sectors(t), *m = bam + 4 * t;
      m[0] = n;
      for(s = 0; s < n; s++) m[1 + (s >> 3)] |= 1 << (s & 7);
    }
    memset(bam + 0x90, 0xA0, 27);
    for(i = 0; i < 16 && name[i]; i++) bam[0x90 + i] = name[i];
    bam[0xA2] = id[0]; bam[0xA3] = id[1]; bam[0xA5] = '2'; bam[0xA6] = 'A';
    if(img.tracks == 40) {
      for(t = 36; t <= 40; t++) { uint8_t* m = bam + 0xC0 + 4 * (t - 36); m[0] = 17; m[1] = 0xFF; m[2] = 0xFF; m[3] = 0x01; }
    }
    if(img.type == IMG_D71) for(t = 36; t <= 70; t++) bam[0xDD + t - 36] = (t == 53) ? 0 : sectors(t);
    bam[4 * 18] -= 2; bam[4 * 18 + 1] &= ~3;   // 18/0 and 18/1 used
    if(write_block(bam, 18, 0)) return;
    if(img.type == IMG_D71) {
      memset(bam, 0, 256);
      for(t = 36; t <= 70; t++) {
        uint8_t n = (t == 53) ? 0 : sectors(t), *m = bam + 3 * (t - 36);
        for(s = 0; s < n; s++) m[s >> 3] |= 1 << (s & 7);
      }
      if(write_block(bam, 53, 0)) return;
    }
    memset(block, 0, 256); block[1] = 0xFF;
    if(write_block(block, 18, 1)) return;
  }
  bam_track = 0;
  image_sync();
}
