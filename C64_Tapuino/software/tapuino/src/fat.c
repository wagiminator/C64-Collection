// ===================================================================================
// FAT16/FAT32 File System Functions                                          * v1.0 *
// ===================================================================================
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <string.h>
#include "fat.h"

// ===================================================================================
// Volume Parameters
// ===================================================================================
static struct {
  uint8_t  type;                        // 16 or 32 (0: not mounted)
  uint8_t  spc;                         // sectors per cluster
  uint8_t  fats;                        // number of FATs
  uint32_t fatStart;                    // first sector of the first FAT
  uint32_t fatSize;                     // sectors per FAT
  uint32_t rootStart;                   // FAT16: first sector of the root directory
  uint16_t rootSectors;                 // FAT16: sectors of the root directory
  uint32_t rootCluster;                 // FAT32: first cluster of the root directory
  uint32_t dataStart;                   // first sector of cluster 2
  uint32_t maxCluster;                  // highest cluster number
  uint32_t fsInfo;                      // FAT32: sector of the FSInfo structure (0: none)
  uint32_t freeHint;                    // start of the search for free clusters
  uint8_t  infoDone;                    // FSInfo free count already invalidated
} FAT;

// Date and time of new files: 2026-01-01 12:00
#define FAT_DATE        (((2026 - 1980) << 9) | (1 << 5) | 1)
#define FAT_TIME        (12 << 11)

// Little endian access to the sector buffer
#define rd16(p)         ((uint16_t)(p)[0] | ((uint16_t)(p)[1] << 8))
#define rd32(p)         ((uint32_t)rd16(p) | ((uint32_t)rd16((p) + 2) << 16))
static void wr16(uint8_t* p, uint16_t v) { p[0] = v; p[1] = v >> 8; }
static void wr32(uint8_t* p, uint32_t v) { wr16(p, v); wr16(p + 2, v >> 16); }

// First sector of a cluster
static uint32_t FAT_lba(uint32_t cl) {
  return FAT.dataStart + (cl - 2) * FAT.spc;
}

// ===================================================================================
// Mount
// ===================================================================================

// Check if the sector buffer holds a FAT boot sector (volume boot record)
static uint8_t FAT_isVBR(void) {
  return (SD_buffer[0] == 0xEB || SD_buffer[0] == 0xE9)
      && rd16(SD_buffer + 11) == 512 && SD_buffer[13]
      && (SD_buffer[16] == 1 || SD_buffer[16] == 2);
}

// Mount the first FAT16/FAT32 partition, returns 0 if successful
uint8_t FAT_mount(void) {
  uint32_t vbr = 0, total, data, clusters;
  uint16_t rootEntries;

  FAT.type = 0;
  if(SD_read(0, SD_buffer)) return 1;
  if(SD_buffer[510] != 0x55 || SD_buffer[511] != 0xAA) return 2;

  // Sector 0 is either the boot sector itself or a master boot record
  if(!FAT_isVBR()) {
    uint8_t i;
    for(i = 0; i < 4; i++) {
      uint8_t* p = SD_buffer + 446 + i * 16;
      if(p[4] && rd32(p + 8)) { vbr = rd32(p + 8); break; }
    }
    if(i == 4) return 3;
    if(SD_read(vbr, SD_buffer)) return 1;
    if(!FAT_isVBR()) return 3;
  }

  // Read the BIOS parameter block
  FAT.spc       = SD_buffer[13];
  FAT.fats      = SD_buffer[16];
  FAT.fatStart  = vbr + rd16(SD_buffer + 14);
  rootEntries   = rd16(SD_buffer + 17);
  FAT.fatSize   = rd16(SD_buffer + 22);
  if(!FAT.fatSize) FAT.fatSize = rd32(SD_buffer + 36);
  total         = rd16(SD_buffer + 19);
  if(!total) total = rd32(SD_buffer + 32);

  FAT.rootStart   = FAT.fatStart + FAT.fats * FAT.fatSize;
  FAT.rootSectors = (rootEntries + 15) >> 4;
  FAT.dataStart   = FAT.rootStart + FAT.rootSectors;
  data            = total - (FAT.dataStart - vbr);
  clusters        = data / FAT.spc;
  FAT.maxCluster  = clusters + 1;
  FAT.freeHint    = 2;
  FAT.infoDone    = 0;
  FAT.fsInfo      = 0;

  // The number of clusters decides the FAT type
  if(clusters < 4085) return 4;                 // FAT12 is not supported
  if(clusters < 65525) {
    FAT.type = 16;
    FAT.rootCluster = 0;
  }
  else {
    FAT.type = 32;
    FAT.rootCluster = rd32(SD_buffer + 44);
    if(rd16(SD_buffer + 48)) FAT.fsInfo = vbr + rd16(SD_buffer + 48);
  }
  return 0;
}

// ===================================================================================
// File Allocation Table
// ===================================================================================

// Sector and offset of the FAT entry of a cluster
static uint32_t FAT_entryPos(uint32_t cl, uint16_t* offset) {
  if(FAT.type == 16) {
    *offset = (cl & 255) << 1;
    return FAT.fatStart + (cl >> 8);
  }
  *offset = (cl & 127) << 2;
  return FAT.fatStart + (cl >> 7);
}

// Read the FAT entry of a cluster from the sector in the buffer
static uint32_t FAT_getEntry(uint16_t offset) {
  if(FAT.type == 16) return rd16(SD_buffer + offset);
  return rd32(SD_buffer + offset) & 0x0FFFFFFF;
}

// Next cluster of a chain, 0 at the end of the chain or on errors
static uint32_t FAT_next(uint32_t cl) {
  uint16_t offset;
  uint32_t lba = FAT_entryPos(cl, &offset);
  if(SD_read(lba, SD_buffer)) return 0;
  cl = FAT_getEntry(offset);
  if(cl < 2 || cl > FAT.maxCluster) return 0;
  return cl;
}

// Write the sector in the buffer to all FATs
static uint8_t FAT_writeFAT(uint32_t lba) {
  uint8_t i;
  for(i = 0; i < FAT.fats; i++) {
    if(SD_write(lba, SD_buffer)) return 1;
    lba += FAT.fatSize;
  }
  return 0;
}

// Set the FAT entry of a cluster, returns 0 if successful
static uint8_t FAT_setEntry(uint32_t cl, uint32_t value) {
  uint16_t offset;
  uint32_t lba = FAT_entryPos(cl, &offset);
  if(SD_read(lba, SD_buffer)) return 1;
  if(FAT.type == 16) wr16(SD_buffer + offset, value);
  else wr32(SD_buffer + offset, (rd32(SD_buffer + offset) & 0xF0000000) | value);
  return FAT_writeFAT(lba);
}

// Find a free cluster, mark it as the end of a chain and append it to the chain of
// cluster prev (0: new chain). Returns the new cluster, 0 if the card is full.
static uint32_t FAT_alloc(uint32_t prev) {
  uint32_t cl = FAT.freeHint, lba, last = 0;
  uint32_t count = FAT.maxCluster - 1;
  uint16_t offset;

  // Search the FAT for a free entry
  while(count--) {
    if(cl > FAT.maxCluster) cl = 2;
    lba = FAT_entryPos(cl, &offset);
    if(lba != last) {
      if(SD_read(lba, SD_buffer)) return 0;
      last = lba;
    }
    if(!FAT_getEntry(offset)) break;
    cl++;
  }
  if(count == 0xFFFFFFFF) return 0;             // no free cluster

  // Mark it as end of chain
  if(FAT.type == 16) wr16(SD_buffer + offset, 0xFFFF);
  else wr32(SD_buffer + offset, (rd32(SD_buffer + offset) & 0xF0000000) | 0x0FFFFFFF);
  if(FAT_writeFAT(lba)) return 0;
  FAT.freeHint = cl + 1;

  // Link it to the previous cluster
  if(prev && FAT_setEntry(prev, cl)) return 0;

  // FAT32: the free cluster count of the FSInfo is no longer valid
  if(FAT.fsInfo && !FAT.infoDone) {
    FAT.infoDone = 1;
    if(!SD_read(FAT.fsInfo, SD_buffer) && rd32(SD_buffer) == 0x41615252) {
      wr32(SD_buffer + 488, 0xFFFFFFFF);
      SD_write(FAT.fsInfo, SD_buffer);
    }
  }
  return cl;
}

// Fill all sectors of a cluster with zeros
static uint8_t FAT_clearCluster(uint32_t cl) {
  uint32_t lba = FAT_lba(cl);
  uint8_t i;
  memset(SD_buffer, 0, 512);
  for(i = 0; i < FAT.spc; i++) {
    if(SD_write(lba + i, SD_buffer)) return 1;
  }
  return 0;
}

// ===================================================================================
// Directory Scan
// ===================================================================================
static FAT_entry FAT_scratch;           // entry being read
static FAT_entry FAT_best;              // best match of a search
static const FAT_entry* FAT_ref;        // reference entry of a search
static int8_t   FAT_dirn;               // search direction
static uint8_t  FAT_found;              // search result flag
static uint16_t FAT_number;             // counter / highest number
static const char* FAT_text;            // name or prefix to look for
static uint32_t FAT_lastSector;         // location of the last entry read
static uint16_t FAT_lastOffset;

// Compare names (case-insensitive ASCII)
static int8_t FAT_strcmp(const char* a, const char* b) {
  char ca, cb;
  do {
    ca = *a++; cb = *b++;
    if(ca >= 'a' && ca <= 'z') ca -= 32;
    if(cb >= 'a' && cb <= 'z') cb -= 32;
    if(ca != cb) return (ca < cb) ? -1 : 1;
  } while(ca);
  return 0;
}

// Sort order: directories first, then by name
static int8_t FAT_compare(const FAT_entry* a, const FAT_entry* b) {
  uint8_t da = a->attr & FAT_ATTR_DIR, db = b->attr & FAT_ATTR_DIR;
  if(da != db) return da ? -1 : 1;
  return FAT_strcmp(a->name, b->name);
}

// Store one character of a long name
static void FAT_lfnChar(uint8_t pos, uint16_t c) {
  if(pos >= FAT_NAME_LEN - 1) return;
  if(c == 0xFFFF) c = 0;                        // padding after the end
  else if(c && (c < 0x20 || c > 0x7E)) c = '?'; // not displayable
  FAT_scratch.name[pos] = c;
}

// Character offsets of a long name entry
static const uint8_t FAT_lfnPos[13] = { 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 };

// Scan a directory and call cb() for every entry (all = 0: only listed entries).
// cb() returns 1 to stop the scan. Returns 1 on read errors.
static uint8_t FAT_scan(uint32_t dir, uint8_t all, uint8_t (*cb)(void)) {
  uint32_t cl, lba;
  uint16_t secs, offset;
  uint8_t  s, i, lfnSum = 0, lfnNext = 0, lfnOK = 0, sum;
  uint8_t* d;

  if(!FAT.type) return 1;
  cl = dir ? dir : FAT.rootCluster;
  while(1) {
    if(cl) { lba = FAT_lba(cl); secs = FAT.spc; }
    else   { lba = FAT.rootStart; secs = FAT.rootSectors; }

    for(s = 0; s < secs; s++, lba++) {
      if(SD_read(lba, SD_buffer)) return 1;
      for(offset = 0; offset < 512; offset += 32) {
        d = SD_buffer + offset;
        if(!d[0]) return 0;                     // end of directory
        if(d[0] == 0xE5) { lfnOK = 0; continue; }     // deleted entry

        // Long name entry: part n holds the characters (n-1)*13 .. n*13-1
        if((d[11] & 0x3F) == 0x0F) {
          uint8_t n = d[0] & 0x1F;
          if(d[0] & 0x40) {                     // last part comes first
            memset(FAT_scratch.name, 0, FAT_NAME_LEN);
            lfnSum = d[13];
            lfnOK  = 1;
          }
          else if(!lfnOK || n != lfnNext || d[13] != lfnSum) { lfnOK = 0; continue; }
          if(!n) { lfnOK = 0; continue; }
          for(i = 0; i < 13; i++) FAT_lfnChar((n - 1) * 13 + i, rd16(d + FAT_lfnPos[i]));
          lfnNext = n - 1;
          continue;
        }

        // Short entry: use the long name if it belongs to this entry
        for(sum = 0, i = 0; i < 11; i++) sum = ((sum & 1) << 7) + (sum >> 1) + d[i];
        if(!(lfnOK && lfnNext == 0 && lfnSum == sum && FAT_scratch.name[0])) {
          // Build the 8.3 name (with the lowercase flags of Windows)
          char* p = FAT_scratch.name;
          for(i = 0; i < 11; i++) {
            char c = d[i];
            if(i == 8) {
              if(d[8] == ' ') break;
              *p++ = '.';
            }
            if(c == ' ') continue;
            if(i == 0 && c == 0x05) c = 0xE5;
            if(c >= 'A' && c <= 'Z' && (d[12] & (i < 8 ? 0x08 : 0x10))) c += 32;
            if(c < 0x20 || c > 0x7E) c = '?';
            *p++ = c;
          }
          *p = 0;
        }
        lfnOK = 0;

        FAT_scratch.attr    = d[11];
        FAT_scratch.cluster = ((uint32_t)rd16(d + 20) << 16) | rd16(d + 26);
        FAT_scratch.size    = rd32(d + 28);
        FAT_lastSector      = lba;
        FAT_lastOffset      = offset;

        // Listed entries: directories and .TAP files, no volume label, hidden, system
        if(!all) {
          if(d[11] & 0x0E) continue;
          if(FAT_scratch.name[0] == '.') continue;
          if(!(d[11] & FAT_ATTR_DIR) && !(d[8] == 'T' && d[9] == 'A' && d[10] == 'P')) continue;
        }
        if(cb()) return 0;
      }
    }
    if(!cl) return 0;                           // end of the FAT16 root directory
    cl = FAT_next(cl);
    if(!cl) return 0;
  }
}

// ===================================================================================
// Directory Functions
// ===================================================================================

// Callback: find the smallest entry (FAT_dirn > 0) or largest entry (< 0) that comes
// after/before the reference entry (no reference: smallest/largest of all)
static uint8_t FAT_cbStep(void) {
  if(FAT_ref && FAT_compare(&FAT_scratch, FAT_ref) * FAT_dirn <= 0) return 0;
  if(!FAT_found || FAT_compare(&FAT_scratch, &FAT_best) * FAT_dirn < 0) {
    memcpy(&FAT_best, &FAT_scratch, sizeof(FAT_entry));
    FAT_found = 1;
  }
  return 0;
}

// Get the first entry of a directory, returns 1 if found
uint8_t FAT_first(uint32_t dir, FAT_entry* e) {
  FAT_ref = 0; FAT_dirn = 1; FAT_found = 0;
  if(FAT_scan(dir, 0, FAT_cbStep) || !FAT_found) return 0;
  memcpy(e, &FAT_best, sizeof(FAT_entry));
  return 1;
}

// Get the next (d=1) or previous (d=-1) entry, wraps around, returns 1 if found
uint8_t FAT_step(uint32_t dir, FAT_entry* e, int8_t d) {
  FAT_ref = e; FAT_dirn = d; FAT_found = 0;
  if(FAT_scan(dir, 0, FAT_cbStep)) return 0;
  if(!FAT_found) {                              // wrap around
    FAT_ref = 0;
    if(FAT_scan(dir, 0, FAT_cbStep) || !FAT_found) return 0;
  }
  memcpy(e, &FAT_best, sizeof(FAT_entry));
  return 1;
}

// Callback: count entries (and the entries before the reference)
static uint8_t FAT_cbCount(void) {
  FAT_number++;
  return 0;
}

// Number of entries shown in a directory
uint16_t FAT_count(uint32_t dir) {
  FAT_number = 0;
  FAT_scan(dir, 0, FAT_cbCount);
  return FAT_number;
}

// Callback: count entries that come before the reference entry
static uint8_t FAT_cbIndex(void) {
  if(FAT_compare(&FAT_scratch, FAT_ref) < 0) FAT_number++;
  return 0;
}

// Position of an entry in the sorted list of a directory (0 = first)
uint16_t FAT_index(uint32_t dir, const FAT_entry* e) {
  FAT_ref = e; FAT_number = 0;
  FAT_scan(dir, 0, FAT_cbIndex);
  return FAT_number;
}

// Callback: compare names
static uint8_t FAT_cbFind(void) {
  if(FAT_strcmp(FAT_scratch.name, FAT_text)) return 0;
  FAT_found = 1;
  return 1;
}

// Find an entry by name (any type), returns 1 if found
uint8_t FAT_find(uint32_t dir, const char* name, FAT_entry* e) {
  FAT_text = name; FAT_found = 0;
  if(FAT_scan(dir, 1, FAT_cbFind) || !FAT_found) return 0;
  if(e) memcpy(e, &FAT_scratch, sizeof(FAT_entry));
  return 1;
}

// Callback: compare the first cluster
static uint32_t FAT_cl;                 // cluster to look for
static uint8_t FAT_cbCluster(void) {
  if(FAT_scratch.cluster != FAT_cl) return 0;
  FAT_found = 1;
  return 1;
}

// Find a listed entry by its first cluster (e.g. a subdirectory), returns 1 if found
uint8_t FAT_findCluster(uint32_t dir, uint32_t cluster, FAT_entry* e) {
  FAT_cl = cluster; FAT_found = 0;
  if(FAT_scan(dir, 0, FAT_cbCluster) || !FAT_found) return 0;
  memcpy(e, &FAT_scratch, sizeof(FAT_entry));
  return 1;
}

// Callback: highest number of files named <prefix>nnnn.TAP
static uint8_t FAT_cbNumber(void) {
  const char* p = FAT_text;
  const char* n = FAT_scratch.name;
  uint16_t v = 0;
  uint8_t i;
  while(*p) {
    char c = *n++;
    if(c >= 'a' && c <= 'z') c -= 32;
    if(c != *p++) return 0;
  }
  for(i = 0; i < 4; i++, n++) {
    if(*n < '0' || *n > '9') return 0;
    v = v * 10 + (*n - '0');
  }
  if(v > FAT_number) FAT_number = v;
  return 0;
}

// Highest number n of the files named <prefix>nnnn.TAP
uint16_t FAT_lastNumber(uint32_t dir, const char* prefix) {
  FAT_text = prefix; FAT_number = 0;
  FAT_scan(dir, 1, FAT_cbNumber);
  return FAT_number;
}

// Convert a name like "NAME.EXT" into the 11 characters of a directory entry
static void FAT_shortName(uint8_t* d, const char* name) {
  uint8_t i = 0;
  memset(d, ' ', 11);
  while(*name && *name != '.' && i < 8) {
    char c = *name++;
    if(c >= 'a' && c <= 'z') c -= 32;
    d[i++] = c;
  }
  while(*name && *name != '.') name++;
  if(*name == '.') {
    name++;
    for(i = 8; *name && i < 11; i++) {
      char c = *name++;
      if(c >= 'a' && c <= 'z') c -= 32;
      d[i] = c;
    }
  }
}

// Find a free directory entry (extends the directory if needed). On success the
// sector of the entry is in SD_buffer, FAT_lastSector/FAT_lastOffset point to it.
static uint8_t FAT_freeEntry(uint32_t dir) {
  uint32_t cl, lba, last = 0;
  uint16_t secs, offset;
  uint8_t  s;

  cl = dir ? dir : FAT.rootCluster;
  while(1) {
    if(cl) { lba = FAT_lba(cl); secs = FAT.spc; }
    else   { lba = FAT.rootStart; secs = FAT.rootSectors; }
    for(s = 0; s < secs; s++, lba++) {
      if(SD_read(lba, SD_buffer)) return 1;
      for(offset = 0; offset < 512; offset += 32) {
        if(!SD_buffer[offset] || SD_buffer[offset] == 0xE5) {
          FAT_lastSector = lba;
          FAT_lastOffset = offset;
          return 0;
        }
      }
    }
    if(!cl) return 1;                           // FAT16 root directory is full
    last = cl;
    cl = FAT_next(cl);
    if(!cl) break;
  }

  // Directory is full: append a new cluster
  cl = FAT_alloc(last);
  if(!cl || FAT_clearCluster(cl)) return 1;
  FAT_lastSector = FAT_lba(cl);
  FAT_lastOffset = 0;
  return 0;                                     // SD_buffer holds the cleared sector
}

// Fill a directory entry
static void FAT_fillEntry(uint8_t* d, const uint8_t* name11, uint8_t attr, uint32_t cl, uint32_t size) {
  memset(d, 0, 32);
  memcpy(d, name11, 11);
  d[11] = attr;
  wr16(d + 14, FAT_TIME);                       // creation time
  wr16(d + 16, FAT_DATE);                       // creation date
  wr16(d + 18, FAT_DATE);                       // last access date
  wr16(d + 20, cl >> 16);                       // first cluster (high)
  wr16(d + 22, FAT_TIME);                       // write time
  wr16(d + 24, FAT_DATE);                       // write date
  wr16(d + 26, cl);                             // first cluster (low)
  wr32(d + 28, size);                           // size
}

// Create a directory, returns its first cluster (0 on errors)
uint32_t FAT_mkdir(uint32_t dir, const char* name) {
  uint8_t name11[11];
  uint32_t cl;

  FAT_shortName(name11, name);

  // Allocate and clear the cluster, then write "." and ".."
  cl = FAT_alloc(0);
  if(!cl || FAT_clearCluster(cl)) return 0;
  {
    uint8_t dot[11];
    memset(dot, ' ', 11);
    dot[0] = '.';
    FAT_fillEntry(SD_buffer, dot, FAT_ATTR_DIR, cl, 0);
    dot[1] = '.';
    FAT_fillEntry(SD_buffer + 32, dot, FAT_ATTR_DIR, dir, 0);
    if(SD_write(FAT_lba(cl), SD_buffer)) return 0;
  }

  // Entry in the parent directory
  if(FAT_freeEntry(dir)) return 0;
  FAT_fillEntry(SD_buffer + FAT_lastOffset, name11, FAT_ATTR_DIR, cl, 0);
  if(SD_write(FAT_lastSector, SD_buffer)) return 0;
  return cl;
}

// ===================================================================================
// File Functions
// ===================================================================================

// Open the file of an entry for reading
void FAT_open(FAT_file* f, const FAT_entry* e) {
  f->first = f->cluster = e->cluster;
  f->size  = e->size;
  f->pos   = 0;
  f->sector = 0;
}

// Read the next sector into SD_buffer, returns the number of valid bytes
uint16_t FAT_read(FAT_file* f) {
  uint32_t left;
  if(f->pos >= f->size || !f->cluster) return 0;
  if(f->sector == FAT.spc) {
    f->cluster = FAT_next(f->cluster);
    f->sector  = 0;
    if(!f->cluster) return 0;
  }
  if(SD_read(FAT_lba(f->cluster) + f->sector, SD_buffer)) return 0;
  f->sector++;
  left = f->size - f->pos;
  if(left > 512) left = 512;
  f->pos += left;
  return left;
}

// Create a new empty file, returns 0 if successful
uint8_t FAT_create(FAT_file* f, uint32_t dir, const char* name) {
  uint8_t name11[11];
  FAT_shortName(name11, name);
  if(FAT_freeEntry(dir)) return 1;
  FAT_fillEntry(SD_buffer + FAT_lastOffset, name11, 0x20, 0, 0);   // archive attribute
  if(SD_write(FAT_lastSector, SD_buffer)) return 2;
  f->first = f->cluster = 0;
  f->size = f->pos = 0;
  f->sector = 0;
  f->entrySector = FAT_lastSector;
  f->entryOffset = FAT_lastOffset;
  return 0;
}

// Start writing the next sector of a file, returns 0 if successful
uint8_t FAT_writeStart(FAT_file* f) {
  // A new cluster is needed at the start and when the current one is full
  if(!f->first || f->sector == FAT.spc) {
    uint32_t cl = FAT_alloc(f->first ? f->cluster : 0);
    if(!cl) return 1;
    if(!f->first) f->first = cl;
    f->cluster = cl;
    f->sector  = 0;
  }
  return SD_writeStart(FAT_lba(f->cluster) + f->sector);
}

// Finish writing the sector, returns 0 if successful
uint8_t FAT_writeStop(FAT_file* f) {
  f->sector++;
  f->pos += 512;
  return SD_writeStop();
}

// Write the final size and first cluster of a created file, returns 0 if successful
uint8_t FAT_close(FAT_file* f, uint32_t size) {
  uint8_t* d = SD_buffer + f->entryOffset;
  f->size = size;
  if(SD_read(f->entrySector, SD_buffer)) return 1;
  wr16(d + 20, f->first >> 16);
  wr16(d + 26, f->first);
  wr32(d + 28, size);
  return SD_write(f->entrySector, SD_buffer);
}

// Overwrite bytes in the first sector of a written file, returns 0 if successful
uint8_t FAT_patch(FAT_file* f, uint16_t offset, const uint8_t* data, uint8_t len) {
  uint32_t lba;
  if(!f->first) return 1;
  lba = FAT_lba(f->first);
  if(SD_read(lba, SD_buffer)) return 1;
  memcpy(SD_buffer + offset, data, len);
  return SD_write(lba, SD_buffer);
}
