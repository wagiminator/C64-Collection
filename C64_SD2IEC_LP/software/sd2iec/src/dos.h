// ===================================================================================
// Disk Operating System: Common Definitions                                  * v1.0 *
// ===================================================================================
//
// This header collects the types and global variables that are shared by the parts
// of the disk operating system (DOS):
//
//   buffers.c   channel buffers and the status (error) channel
//   cbmfs.c     CBM view of the file systems: names, matching, paths, FAT files, P00
//   image.c     disk images D64/D71/D81
//   fileops.c   OPEN on data channels: files, directory listing, direct access
//   doscmd.c    commands on the command channel 15
//   swap.c      disk swap lists
//
// Notes:
// ------
// - File names inside the DOS are PETSCII strings. FAT names are converted from and
//   to ASCII when a FAT directory is accessed (see cbmfs.c).
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "fat.h"
#include "led.h"
#include "../config.h"

#define VERSION           "1.0"         // firmware version (status message 73)

// ===================================================================================
// File Types and Directory Entries
// ===================================================================================
#define CBM_NAME_LENGTH   16            // max. length of a CBM file name

enum { TYPE_DEL = 0, TYPE_SEQ, TYPE_PRG, TYPE_USR, TYPE_REL, TYPE_CBM, TYPE_DIR };
#define TYPE_MASK         0x07          // file type bits
#define FLAG_HIDDEN       0x20          // hidden file (FAT attribute)
#define FLAG_RO           0x40          // locked / read only
#define FLAG_SPLAT        0x80          // not closed ("splat" file)

// Storage of a directory entry
enum { OPS_FAT = 0, OPS_P00, OPS_IMAGE };

// Directory entry in CBM terms
typedef struct {
  uint8_t  name[CBM_NAME_LENGTH + 1];   // PETSCII name, zero terminated
  uint8_t  type;                        // file type and flags
  uint8_t  ops;                         // OPS_FAT, OPS_P00, OPS_IMAGE
  uint8_t  remainder;                   // file size % 254 (0xFF: unknown)
  uint16_t blocks;                      // size in blocks
  union {
    FAT_entry fat;                      // FAT: the entry
    struct {
      uint8_t track, sector;            // image: first data block
      uint8_t dtrack, dsector, dentry;  // image: location of the directory entry
    } img;
  } pvt;
} dirent_t;

// Path: where a file or directory is
typedef struct {
  uint8_t  image;                       // 1: inside the mounted disk image
  uint32_t dir;                         // FAT: first cluster of the directory
} path_t;

// Directory reader
typedef struct {
  uint8_t  image;                       // reading a disk image directory
  FAT_dir  fat;                         // FAT directory reader
  uint8_t  track, sector, entry;        // image: next directory entry
} dirhandle_t;

// ===================================================================================
// Channel Buffers
// ===================================================================================
#define BUFFER_SYSTEM     100           // secondary address of internal buffers

typedef struct buffer_s {
  uint8_t* data;                        // 256 bytes of data
  uint8_t  secondary;                   // secondary address (channel)
  uint8_t  position;                    // read/write position
  uint8_t  lastused;                    // last valid byte
  uint8_t  allocated : 1;               // buffer in use
  uint8_t  read      : 1;               // channel can be read
  uint8_t  write     : 1;               // channel can be written
  uint8_t  dirty     : 1;               // data must be written
  uint8_t  sendeoi   : 1;               // last byte of the buffer ends the file
  uint8_t  mustflush : 1;               // buffer is full, refill before next byte
  uint8_t  sticky    : 1;               // keep after the bus transaction
  uint32_t fptr;                        // file position of data[0] (P command)
  uint8_t  (*refill)(struct buffer_s* buf);   // next data block / write the block
  uint8_t  (*cleanup)(struct buffer_s* buf);  // close the channel
  union {
    struct {                            // FAT file
      FAT_file fh;
      uint8_t  header;                  // size of the P00 header
    } fat;
    struct {                            // file in a disk image
      uint8_t  track, sector;           // current block
      uint8_t  dtrack, dsector, dentry; // directory entry (for writing)
      uint16_t blocks;                  // blocks written
      uint8_t  type;                    // file type for writing
    } img;
    struct {                            // directory listing
      dirhandle_t dh;
      uint8_t  type;                    // file type filter (0: all)
      uint8_t* match;                   // name pattern (or 0)
    } dir;
  } pvt;
} buffer_t;

extern buffer_t buffers[BUFFER_COUNT + 1];      // last one is the status channel
extern uint8_t  active_buffers;                  // number of allocated buffers

void      buffers_init(void);
buffer_t* buffer_alloc(void);                    // allocate a buffer (0 if none free)
void      buffer_free(buffer_t* buf);            // call cleanup and free
uint8_t   buffers_free(uint8_t all);             // free unsticky (all: all) buffers
buffer_t* buffer_find(uint8_t secondary);        // buffer of a channel (or 0)
uint8_t   buffer_dummy(buffer_t* buf);           // refill/cleanup that does nothing

// ===================================================================================
// Status Channel (Error Messages)
// ===================================================================================
enum {
  ERROR_OK = 0, ERROR_SCRATCHED = 1, ERROR_LONGVERSION = 9,
  ERROR_READ_NOHEADER = 20, ERROR_READ_NOSYNC = 21, ERROR_READ_NODATA = 22,
  ERROR_READ_CHECKSUM = 23, ERROR_WRITE_VERIFY = 25, ERROR_WRITE_PROTECT = 26,
  ERROR_DISK_ID_MISMATCH = 29, ERROR_SYNTAX_UNKNOWN = 30, ERROR_SYNTAX_UNABLE = 31,
  ERROR_SYNTAX_TOOLONG = 32, ERROR_SYNTAX_JOKER = 33, ERROR_SYNTAX_NONAME = 34,
  ERROR_FILE_NOT_FOUND_39 = 39, ERROR_RECORD_MISSING = 50, ERROR_FILE_TOO_LARGE = 52,
  ERROR_WRITE_FILE_OPEN = 60, ERROR_FILE_NOT_OPEN = 61, ERROR_FILE_NOT_FOUND = 62,
  ERROR_FILE_EXISTS = 63, ERROR_FILE_TYPE_MISMATCH = 64, ERROR_NO_BLOCK = 65,
  ERROR_ILLEGAL_TS_COMMAND = 66, ERROR_ILLEGAL_TS_LINK = 67, ERROR_NO_CHANNEL = 70,
  ERROR_DIR_ERROR = 71, ERROR_DISK_FULL = 72, ERROR_DOSVERSION = 73,
  ERROR_DRIVE_NOT_READY = 74, ERROR_IMAGE_INVALID = 79, ERROR_UNKNOWN_DRIVECODE = 98
};

extern uint8_t current_error;                    // last error code
void set_error(uint8_t code);                    // set message with track/sector 0
void set_error_ts(uint8_t code, uint8_t track, uint8_t sector);

// ===================================================================================
// Global State
// ===================================================================================
extern uint8_t  device_address;                  // current device address (8..30)
extern uint8_t  command_buffer[COMMAND_LENGTH + 2];  // command / file name
extern uint8_t  command_length;                  // bytes in command_buffer
extern path_t   current_path;                    // current directory
extern uint8_t  ext_mode;                        // extension mode (XE)
extern uint8_t  ext_hiding;                      // hide type extensions (XE+)
extern uint8_t  disk_ready;                      // card mounted
extern uint8_t  disk_changed;                    // card was changed
extern uint8_t  disk_wp;                         // card is write protected

// ===================================================================================
// File System Interface (cbmfs.c / image.c)
// ===================================================================================
// Names and paths
void     pet2asc(uint8_t* s);                    // PETSCII -> ASCII (in place)
void     asc2pet(uint8_t* s);                    // ASCII -> PETSCII (in place)
uint8_t  match_name(const uint8_t* pattern, const uint8_t* name, uint8_t ignorecase);
uint8_t  parse_path(uint8_t* in, path_t* path, uint8_t** name, uint8_t for_cd);
uint8_t  check_imageext(const uint8_t* name);    // 1: D64/D71/D81 extension

// Directory access (FAT or image, depending on the path)
uint8_t  fs_opendir(dirhandle_t* dh, const path_t* path);
int8_t   fs_readdir(dirhandle_t* dh, dirent_t* dent);       // 0 ok, -1 end, 1 error
int8_t   fs_first_match(const path_t* path, const uint8_t* pattern, uint8_t type, dirent_t* dent);
int8_t   fs_next_match(dirhandle_t* dh, const uint8_t* pattern, uint8_t type, dirent_t* dent);
extern dirhandle_t matchdh;                      // reader used by fs_first_match

// Files
void     fs_open_read(const path_t* path, dirent_t* dent, buffer_t* buf);
void     fs_open_write(const path_t* path, dirent_t* dent, uint8_t type, buffer_t* buf, uint8_t append);
uint8_t  fs_delete(const path_t* path, dirent_t* dent);     // 1: deleted, 0 not, 255 error
void     fs_rename(const path_t* path, dirent_t* dent, uint8_t* newname);
void     fs_mkdir(const path_t* path, uint8_t* name);
uint8_t  fs_chdir(path_t* path, dirent_t* dent);            // 0 if successful
uint8_t  fs_seek(buffer_t* buf, uint32_t pos);              // 0 if successful
uint16_t fs_freeblocks(void);
void     fs_label(const path_t* path, uint8_t* label);     // 16 bytes, 0xA0 padded
void     fs_id(const path_t* path, uint8_t* id);           // 5 bytes ("ID 2A")
void     fs_read_sector(buffer_t* buf, uint8_t track, uint8_t sector);
void     fs_write_sector(buffer_t* buf, uint8_t track, uint8_t sector);
void     fs_format(uint8_t* name, uint8_t* id);
void     fs_init(void);                          // mount card, reset paths
void     fs_sync(void);                          // write cached data

// Disk images (image.c)
extern uint8_t image_mounted;                    // 1: an image is mounted
uint8_t  image_mount(uint32_t dir, dirent_t* dent);         // 0 if successful
void     image_unmount(void);
uint8_t  image_check_ts(uint8_t track, uint8_t sector);    // 1 if valid
uint8_t  image_drive_type(void);                 // 0 (none), 41, 71, 81
void     image_dir_start(uint8_t* track, uint8_t* sector);   // first directory block
int8_t   image_readdir(dirhandle_t* dh, dirent_t* dent);
void     image_open_read(dirent_t* dent, buffer_t* buf);
void     image_open_write(dirent_t* dent, uint8_t type, buffer_t* buf, uint8_t append);
uint8_t  image_delete(dirent_t* dent);
void     image_rename(dirent_t* dent, uint8_t* newname);
uint16_t image_freeblocks(void);
void     image_label(uint8_t* label);
void     image_id(uint8_t* id);
void     image_read_sector(uint8_t* data, uint8_t track, uint8_t sector);
void     image_write_sector(uint8_t* data, uint8_t track, uint8_t sector);
void     image_block_alloc(uint8_t track, uint8_t sector, uint8_t alloc);
void     image_format(uint8_t* name, uint8_t* id);
void     image_sync(void);

// File operations (fileops.c)
void     file_open(uint8_t secondary);           // OPEN with the name in command_buffer
uint8_t  directbuffer_refill(buffer_t* buf);     // "#" buffers

// Commands (doscmd.c)
void     parse_doscommand(void);
void     do_chdir(uint8_t* str);                // CD with the parameter string

// Main (main.c)
void     main_idle(void);                       // called while the bus is idle
uint8_t  device_dip_address(void);              // address set by the DIP switches

// Swap lists (swap.c)
void     swap_set(uint8_t* name);                // XS command: set the list
void     swap_change(uint8_t key);               // NEXT/PREV/HOME pressed
void     swap_reset(void);

#ifdef __cplusplus
};
#endif
