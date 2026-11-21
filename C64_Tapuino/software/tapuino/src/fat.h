// ===================================================================================
// FAT16/FAT32 File System Functions                                          * v1.0 *
// ===================================================================================
//
// Volume and directory functions available:
// -----------------------------------------
// FAT_mount()              Mount the first FAT16/FAT32 partition, returns 0 if successful
// FAT_first(dir, e)        Get the first entry of a directory (sorted), returns 1 if found
// FAT_step(dir, e, d)      Get the next (d=1) or previous (d=-1) entry after e (sorted,
//                          wraps around), returns 1 if found
// FAT_count(dir)           Number of entries shown in a directory
// FAT_index(dir, e)        Position of entry e in the sorted list (0 = first)
// FAT_find(dir, name, e)   Find an entry by name (any type, case-insensitive), returns 1
// FAT_findCluster(d,c,e)  Find a listed entry by its first cluster c, returns 1 if found
// FAT_mkdir(dir, name)     Create a directory (name: 8.3 name), returns its cluster or 0
// FAT_lastNumber(dir, p)   Highest number n of the files named <p>nnnn.TAP in a directory
//
// File functions available:
// -------------------------
// FAT_open(f, e)           Open the file of entry e for reading
// FAT_read(f)              Read the next sector of a file into SD_buffer, returns the
//                          number of valid bytes (0: end of file or error)
// FAT_create(f, dir, name) Create a new empty file (name: 8.3 name), returns 0 if successful
// FAT_writeStart(f)        Start writing the next sector of a file, returns 0 if successful
//                          (then send 512 bytes with SD_writeBytes())
// FAT_writeStop(f)         Finish writing the sector, returns 0 if successful
// FAT_close(f, size)       Write the final size of a created file, returns 0 if successful
// FAT_patch(f, o, p, n)    Overwrite n bytes at offset o (< 512) of a written file
//
// Notes:
// ------
// - Directories are given by their first cluster, 0 is the root directory.
// - Entries are listed sorted: directories first, then files, each in alphabetical
//   order (case-insensitive). Only directories and files with the extension .TAP are
//   shown; hidden and system entries and names starting with "." are skipped.
// - Long file names are read (ASCII characters, others are shown as '?'). New files
//   and directories get 8.3 names only.
// - All functions use the common sector buffer SD_buffer.
//
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "sd.h"

#define FAT_NAME_LEN    56              // max. length of names incl. terminating zero
#define FAT_ATTR_DIR    0x10            // directory attribute

// Directory entry
typedef struct {
  char     name[FAT_NAME_LEN];          // long name, or 8.3 name
  uint8_t  attr;                        // attributes (FAT_ATTR_DIR for directories)
  uint32_t cluster;                     // first cluster
  uint32_t size;                        // file size in bytes
} FAT_entry;

// Open file
typedef struct {
  uint32_t first;                       // first cluster (0: no data yet)
  uint32_t cluster;                     // current cluster
  uint32_t size;                        // file size in bytes
  uint32_t pos;                         // position (bytes read or written)
  uint8_t  sector;                      // next sector within the current cluster
  uint32_t entrySector;                 // sector of the directory entry
  uint16_t entryOffset;                 // offset of the directory entry in that sector
} FAT_file;

// Volume and directory functions
uint8_t  FAT_mount(void);
uint8_t  FAT_first(uint32_t dir, FAT_entry* e);
uint8_t  FAT_step(uint32_t dir, FAT_entry* e, int8_t d);
uint16_t FAT_count(uint32_t dir);
uint16_t FAT_index(uint32_t dir, const FAT_entry* e);
uint8_t  FAT_find(uint32_t dir, const char* name, FAT_entry* e);
uint8_t  FAT_findCluster(uint32_t dir, uint32_t cluster, FAT_entry* e);
uint32_t FAT_mkdir(uint32_t dir, const char* name);
uint16_t FAT_lastNumber(uint32_t dir, const char* prefix);

// File functions
void     FAT_open(FAT_file* f, const FAT_entry* e);
uint16_t FAT_read(FAT_file* f);
uint8_t  FAT_create(FAT_file* f, uint32_t dir, const char* name);
uint8_t  FAT_writeStart(FAT_file* f);
uint8_t  FAT_writeStop(FAT_file* f);
uint8_t  FAT_close(FAT_file* f, uint32_t size);
uint8_t  FAT_patch(FAT_file* f, uint16_t offset, const uint8_t* data, uint8_t len);

#ifdef __cplusplus
};
#endif
