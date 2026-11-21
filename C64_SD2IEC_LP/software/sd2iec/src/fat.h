// ===================================================================================
// FAT16/FAT32 File System Functions                                          * v2.0 *
// ===================================================================================
//
// Volume functions available:
// ---------------------------
// FAT_mount()              Mount the first FAT16/FAT32 partition, returns 0 if successful
// FAT_unmount()            Forget the mounted volume (card removed)
// FAT_sync()               Write all cached data to the card, returns 0 if successful
// FAT_free()               Number of free clusters
// FAT_clusterSize()        Bytes per cluster
// FAT_root()               First cluster of the root directory (0 for FAT16)
// FAT_label(buf)           Copy the volume label (11 chars) into buf, returns 1 if found
//
// Directory functions available:
// ------------------------------
// FAT_dirOpen(d, cl)       Start reading the directory with first cluster cl (0 = root)
// FAT_dirRead(d, e)        Read the next entry (in disk order), returns 1 if found
// FAT_find(dir, name, e)   Find an entry by name (LFN or 8.3, case-insensitive)
// FAT_mkdir(dir, name)     Create a directory, returns 0 if successful
// FAT_delete(dir, e)       Delete a file or an empty directory, returns 0 if successful
// FAT_rename(dir, e, name) Rename an entry, returns 0 if successful
// FAT_setAttr(e, attr)     Change the attributes of an entry, returns 0 if successful
//
// File functions available:
// -------------------------
// FAT_open(f, e)           Open the file of a directory entry
// FAT_create(f, dir, name) Create a new empty file, returns 0 if successful
// FAT_read(f, buf, n)      Read up to n bytes, returns the number of bytes read
// FAT_write(f, buf, n)     Write n bytes, returns 0 if successful
// FAT_seek(f, pos)         Set the position (pos <= size), returns 0 if successful
// FAT_close(f)             Finish a file (writes size and date), returns 0 if successful
//
// Notes:
// ------
// - Directories are given by their first cluster, 0 is the root directory.
// - All functions share one 512 byte sector cache (SD_buffer) with write-back. Both
//   FAT copies are updated, the free cluster count in the FSInfo sector is kept up
//   to date. FAT_sync() (or FAT_close()) must be called before the card is removed.
// - Names are ASCII strings. Long file names are read (up to FAT_LFN_MAX characters,
//   longer or non-ASCII names are reported by their 8.3 name) and created when a name
//   is not a valid 8.3 name. 8.3 names that are completely in lower case are stored
//   with the case flags used by Windows.
// - Error codes: FAT_ERR_* (0 = OK)
//
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "sd.h"

#define FAT_LFN_MAX     20              // max. length of long names that are used
#define FAT_ATTR_RO     0x01            // read only
#define FAT_ATTR_HIDDEN 0x02            // hidden
#define FAT_ATTR_SYSTEM 0x04            // system
#define FAT_ATTR_VOLUME 0x08            // volume label
#define FAT_ATTR_DIR    0x10            // directory
#define FAT_ATTR_ARCH   0x20            // archive

// Error codes
enum { FAT_OK = 0, FAT_ERR_DISK, FAT_ERR_NOFS, FAT_ERR_FULL, FAT_ERR_DIRFULL,
       FAT_ERR_EXISTS, FAT_ERR_NOTEMPTY, FAT_ERR_NAME, FAT_ERR_EOF };

// Directory entry
typedef struct {
  char     name[FAT_LFN_MAX + 1];       // long name, or 8.3 name ("NAME.EXT")
  uint8_t  sfn[11];                     // 8.3 name as stored
  uint8_t  attr;                        // attributes
  uint8_t  lfn;                         // number of LFN entries before the 8.3 entry
  uint32_t cluster;                     // first cluster
  uint32_t size;                        // file size in bytes
  uint16_t date;                        // FAT date
  uint16_t time;                        // FAT time
  uint16_t index;                       // position of the 8.3 entry in the directory
  uint32_t lba;                         // sector of the 8.3 entry
} FAT_entry;

// Directory reader
typedef struct {
  uint32_t first;                       // first cluster (0: FAT16 root)
  uint32_t cluster;                     // current cluster
  uint16_t index;                       // index of the next entry
} FAT_dir;

// Open file
typedef struct {
  uint32_t first;                       // first cluster (0: no data yet)
  uint32_t cluster;                     // cluster of the byte before pos (0 at pos 0)
  uint32_t size;                        // file size in bytes
  uint32_t pos;                         // position
  uint32_t lba;                         // sector of the directory entry
  uint8_t  slot;                        // entry within that sector (0..15)
  uint8_t  dirty;                       // size or first cluster changed
} FAT_file;

// Volume functions
uint8_t  FAT_mount(void);
void     FAT_unmount(void);
uint8_t  FAT_sync(void);
uint32_t FAT_free(void);
uint32_t FAT_clusterSize(void);
uint32_t FAT_root(void);
uint8_t  FAT_label(char* buf);

// Directory functions
void     FAT_dirOpen(FAT_dir* d, uint32_t cluster);
uint8_t  FAT_dirRead(FAT_dir* d, FAT_entry* e);
uint8_t  FAT_find(uint32_t dir, const char* name, FAT_entry* e);
uint8_t  FAT_mkdir(uint32_t dir, const char* name);
uint8_t  FAT_delete(uint32_t dir, FAT_entry* e);
uint8_t  FAT_rename(uint32_t dir, FAT_entry* e, const char* name);
uint8_t  FAT_setAttr(FAT_entry* e, uint8_t attr);

// File functions
void     FAT_open(FAT_file* f, const FAT_entry* e);
uint8_t  FAT_create(FAT_file* f, uint32_t dir, const char* name);
uint16_t FAT_read(FAT_file* f, uint8_t* buf, uint16_t n);
uint8_t  FAT_write(FAT_file* f, const uint8_t* buf, uint16_t n);
uint8_t  FAT_seek(FAT_file* f, uint32_t pos);
uint8_t  FAT_close(FAT_file* f);

#ifdef __cplusplus
};
#endif
