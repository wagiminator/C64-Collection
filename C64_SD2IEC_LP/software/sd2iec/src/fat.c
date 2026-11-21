// ===================================================================================
// FAT16/FAT32 File System Functions                                          * v2.0 *
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
  uint8_t  infoDirty;                   // FSInfo must be written
  uint32_t fatStart;                    // first sector of the first FAT
  uint32_t fatSize;                     // sectors per FAT
  uint32_t rootStart;                   // FAT16: first sector of the root directory
  uint16_t rootSectors;                 // FAT16: sectors of the root directory
  uint32_t rootCluster;                 // FAT32: first cluster of the root directory
  uint32_t dataStart;                   // first sector of cluster 2
  uint32_t maxCluster;                  // highest cluster number
  uint32_t fsInfo;                      // FAT32: sector of the FSInfo structure (0: none)
  uint32_t freeCount;                   // free clusters (0xFFFFFFFF: unknown)
  uint32_t freeHint;                    // start of the search for free clusters
} FAT;

// Date and time of new files: 2026-01-01 12:00 (no real time clock)
#define FAT_DATE        (((2026 - 1980) << 9) | (1 << 5) | 1)
#define FAT_TIME        (12 << 11)
#define FAT_UNKNOWN     0xFFFFFFFF

// Little endian access
#define rd16(p)         ((uint16_t)(p)[0] | ((uint16_t)(p)[1] << 8))
#define rd32(p)         ((uint32_t)rd16(p) | ((uint32_t)rd16((p) + 2) << 16))
static void wr16(uint8_t* p, uint16_t v) { p[0] = v; p[1] = v >> 8; }
static void wr32(uint8_t* p, uint32_t v) { wr16(p, v); wr16(p + 2, v >> 16); }

// First sector of a cluster
static uint32_t FAT_lba(uint32_t cl) {
  return FAT.dataStart + (cl - 2) * FAT.spc;
}

// Bytes per cluster
uint32_t FAT_clusterSize(void) {
  return (uint32_t)FAT.spc << 9;
}

// First cluster of the root directory
uint32_t FAT_root(void) {
  return FAT.rootCluster;
}

// ===================================================================================
// Sector Cache (SD_buffer, write-back)
// ===================================================================================
static uint32_t FAT_cacheLBA = FAT_UNKNOWN;     // sector in the cache
static uint8_t  FAT_cacheDirty;                 // cache must be written

// Write the cached sector if it was changed, returns 0 if successful
static uint8_t FAT_flush(void) {
  uint8_t i;
  uint32_t lba = FAT_cacheLBA;
  if(!FAT_cacheDirty) return 0;
  for(i = 0; i < FAT.fats; i++) {
    if(SD_write(lba, SD_buffer)) return FAT_ERR_DISK;
    if(lba < FAT.fatStart || lba >= FAT.fatStart + FAT.fatSize) break;  // not a FAT sector
    lba += FAT.fatSize;                         // mirror to the other FATs
  }
  FAT_cacheDirty = 0;
  return 0;
}

// Get a sector into the cache, returns pointer to the data or 0 on errors
static uint8_t* FAT_cache(uint32_t lba) {
  if(lba == FAT_cacheLBA) return SD_buffer;
  if(FAT_flush()) return 0;
  FAT_cacheLBA = FAT_UNKNOWN;
  if(SD_read(lba, SD_buffer)) return 0;
  FAT_cacheLBA = lba;
  return SD_buffer;
}

// Get a cleared sector into the cache (its old content is not needed)
static uint8_t* FAT_cacheNew(uint32_t lba) {
  if(lba != FAT_cacheLBA) {
    if(FAT_flush()) return 0;
    FAT_cacheLBA = lba;
  }
  memset(SD_buffer, 0, 512);
  FAT_cacheDirty = 1;
  return SD_buffer;
}

// Write everything to the card, returns 0 if successful
uint8_t FAT_sync(void) {
  if(!FAT.type) return 0;
  if(FAT_flush()) return FAT_ERR_DISK;
  if(FAT.infoDirty && FAT.fsInfo) {
    uint8_t* p = FAT_cache(FAT.fsInfo);
    if(!p) return FAT_ERR_DISK;
    if(rd32(p) == 0x41615252 && rd32(p + 484) == 0x61417272) {
      wr32(p + 488, FAT.freeCount);
      wr32(p + 492, FAT.freeHint);
      FAT_cacheDirty = 1;
      if(FAT_flush()) return FAT_ERR_DISK;
    }
  }
  FAT.infoDirty = 0;
  return 0;
}

// ===================================================================================
// Mount
// ===================================================================================

// Check if the sector buffer holds a FAT boot sector (volume boot record)
static uint8_t FAT_isVBR(const uint8_t* p) {
  return (p[0] == 0xEB || p[0] == 0xE9) && rd16(p + 11) == 512 && p[13]
      && (p[16] == 1 || p[16] == 2);
}

// Mount the first FAT16/FAT32 partition, returns 0 if successful
uint8_t FAT_mount(void) {
  uint32_t vbr = 0, total, clusters;
  uint16_t rootEntries, fatSize16;
  uint8_t* p;

  FAT.type = 0;
  FAT_cacheLBA = FAT_UNKNOWN;
  FAT_cacheDirty = 0;
  if(!(p = FAT_cache(0))) return FAT_ERR_DISK;
  if(p[510] != 0x55 || p[511] != 0xAA) return FAT_ERR_NOFS;

  // Sector 0 is either the boot sector itself or a master boot record
  if(!FAT_isVBR(p)) {
    uint8_t i;
    for(i = 0; i < 4; i++) {
      uint8_t* e = p + 446 + i * 16;
      if(e[4] && rd32(e + 8)) { vbr = rd32(e + 8); break; }
    }
    if(i == 4) return FAT_ERR_NOFS;
    if(!(p = FAT_cache(vbr))) return FAT_ERR_DISK;
    if(!FAT_isVBR(p)) return FAT_ERR_NOFS;
  }

  // Read the BIOS parameter block
  FAT.spc       = p[13];
  FAT.fats      = p[16];
  FAT.fatStart  = vbr + rd16(p + 14);
  rootEntries   = rd16(p + 17);
  fatSize16     = rd16(p + 22);
  FAT.fatSize   = fatSize16 ? fatSize16 : rd32(p + 36);
  total         = rd16(p + 19);
  if(!total) total = rd32(p + 32);

  FAT.rootStart   = FAT.fatStart + FAT.fats * FAT.fatSize;
  FAT.rootSectors = (rootEntries + 15) >> 4;
  FAT.dataStart   = FAT.rootStart + FAT.rootSectors;
  clusters        = (total - (FAT.dataStart - vbr)) / FAT.spc;
  FAT.maxCluster  = clusters + 1;
  FAT.freeHint    = 2;
  FAT.freeCount   = FAT_UNKNOWN;
  FAT.infoDirty   = 0;
  FAT.fsInfo      = 0;
  FAT.rootCluster = 0;

  // FAT32 has no 16 bit FAT size (like Linux; small FAT32 volumes exist), FAT12 is
  // not supported
  if(fatSize16) {
    if(clusters < 4085) return FAT_ERR_NOFS;
    FAT.type = 16;
  }
  else {
    FAT.type = 32;
    FAT.rootCluster = rd32(p + 44);
    if(rd16(p + 48)) {
      FAT.fsInfo = vbr + rd16(p + 48);
      if(!(p = FAT_cache(FAT.fsInfo))) return FAT_ERR_DISK;
      if(rd32(p) == 0x41615252 && rd32(p + 484) == 0x61417272) {
        FAT.freeCount = rd32(p + 488);
        if(FAT.freeCount > clusters) FAT.freeCount = FAT_UNKNOWN;
        FAT.freeHint = rd32(p + 492);
        if(FAT.freeHint < 2 || FAT.freeHint > FAT.maxCluster) FAT.freeHint = 2;
      }
    }
  }
  return 0;
}

// Forget the mounted volume
void FAT_unmount(void) {
  FAT.type = 0;
  FAT_cacheLBA = FAT_UNKNOWN;
  FAT_cacheDirty = 0;
}

// ===================================================================================
// File Allocation Table
// ===================================================================================

// Read the FAT entry of a cluster (0xFFFFFFFF on errors)
static uint32_t FAT_get(uint32_t cl) {
  uint8_t* p;
  if(FAT.type == 16) {
    if(!(p = FAT_cache(FAT.fatStart + (cl >> 8)))) return FAT_UNKNOWN;
    return rd16(p + ((cl & 255) << 1));
  }
  if(!(p = FAT_cache(FAT.fatStart + (cl >> 7)))) return FAT_UNKNOWN;
  return rd32(p + ((cl & 127) << 2)) & 0x0FFFFFFF;
}

// Write the FAT entry of a cluster, returns 0 if successful
static uint8_t FAT_set(uint32_t cl, uint32_t val) {
  uint8_t* p;
  if(FAT.type == 16) {
    if(!(p = FAT_cache(FAT.fatStart + (cl >> 8)))) return FAT_ERR_DISK;
    wr16(p + ((cl & 255) << 1), val);
  }
  else {
    if(!(p = FAT_cache(FAT.fatStart + (cl >> 7)))) return FAT_ERR_DISK;
    p += (cl & 127) << 2;
    wr32(p, (rd32(p) & 0xF0000000) | (val & 0x0FFFFFFF));
  }
  FAT_cacheDirty = 1;
  return 0;
}

// Next cluster of a chain, 0 at the end of the chain or on errors
static uint32_t FAT_next(uint32_t cl) {
  cl = FAT_get(cl);
  if(cl < 2 || cl > FAT.maxCluster) return 0;
  return cl;
}

// Allocate a free cluster and append it to prev (0: new chain), returns 0 if full
static uint32_t FAT_alloc(uint32_t prev) {
  uint32_t cl = FAT.freeHint, n = FAT.maxCluster - 1, v;
  while(n--) {
    if(cl < 2 || cl > FAT.maxCluster) cl = 2;
    v = FAT_get(cl);
    if(v == FAT_UNKNOWN) return 0;
    if(!v) {
      if(FAT_set(cl, 0x0FFFFFFF)) return 0;
      if(prev && FAT_set(prev, cl)) return 0;
      FAT.freeHint = cl + 1;
      if(FAT.freeCount != FAT_UNKNOWN) FAT.freeCount--;
      FAT.infoDirty = 1;
      return cl;
    }
    cl++;
  }
  return 0;
}

// Free a cluster chain, returns 0 if successful
static uint8_t FAT_freeChain(uint32_t cl) {
  while(cl >= 2 && cl <= FAT.maxCluster) {
    uint32_t next = FAT_get(cl);
    if(next == FAT_UNKNOWN || FAT_set(cl, 0)) return FAT_ERR_DISK;
    if(FAT.freeCount != FAT_UNKNOWN) FAT.freeCount++;
    FAT.infoDirty = 1;
    cl = next;
  }
  return 0;
}

// Number of free clusters (counted once if the FSInfo sector does not know it)
uint32_t FAT_free(void) {
  if(!FAT.type) return 0;
  if(FAT.freeCount == FAT_UNKNOWN) {             // count the zero entries sector by sector
                                                // (entries 0 and 1 are reserved, never 0)
    uint32_t cl = 0, n = 0, sec = FAT.fatStart;
    uint8_t* p;
    uint16_t i;
    while(cl <= FAT.maxCluster) {
      if(!(p = FAT_cache(sec++))) return 0;
      for(i = 0; i < 512 && cl <= FAT.maxCluster; cl++) {
        if(FAT.type == 16) { if(!(p[i] | p[i + 1])) n++; i += 2; }
        else { if(!(p[i] | p[i + 1] | p[i + 2] | (p[i + 3] & 0x0F))) n++; i += 4; }
      }
    }
    FAT.freeCount = n;
    FAT.infoDirty = 1;
  }
  return FAT.freeCount;
}

// ===================================================================================
// Directories
// ===================================================================================

// Sector of the entry d->index (follows the cluster chain), 0 at the end
static uint32_t FAT_dirSector(FAT_dir* d) {
  uint16_t sec = d->index >> 4;
  uint8_t inCluster;
  if(!d->first) return sec < FAT.rootSectors ? FAT.rootStart + sec : 0;
  inCluster = sec & (FAT.spc - 1);
  if(d->index && !(d->index & 15) && !inCluster) d->cluster = FAT_next(d->cluster);
  if(!d->cluster) return 0;
  return FAT_lba(d->cluster) + inCluster;
}

// Start reading a directory (0 = root)
void FAT_dirOpen(FAT_dir* d, uint32_t cluster) {
  if(!cluster) cluster = FAT.rootCluster;
  d->first = d->cluster = cluster;
  d->index = 0;
}

// Pointer to the directory entry with the index, 0 at the end or on errors
static uint8_t* FAT_dirEntry(uint32_t dir, uint16_t index, uint32_t* lba) {
  FAT_dir d;
  uint8_t* p;
  FAT_dirOpen(&d, dir);
  if(d.first) {                                 // skip whole clusters
    uint16_t perCluster = (uint16_t)FAT.spc << 4;
    while(index >= perCluster) {
      d.cluster = FAT_next(d.cluster);
      if(!d.cluster) return 0;
      index -= perCluster;
    }
  }
  d.index = index;
  if(!(*lba = FAT_dirSector(&d))) return 0;
  if(!(p = FAT_cache(*lba))) return 0;
  return p + ((index & 15) << 5);
}

// Checksum of an 8.3 name for its long name entries
static uint8_t FAT_checksum(const uint8_t* sfn) {
  uint8_t i, sum = 0;
  for(i = 0; i < 11; i++) sum = ((sum & 1) << 7) + (sum >> 1) + sfn[i];
  return sum;
}

// Positions of the 13 characters in a long name entry
static const uint8_t FAT_lfnPos[13] PROGMEM = { 1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30 };

static const char* FAT_cmpName;                 // name compared by FAT_dirRead (or 0)
static uint16_t FAT_cmpLen;                     // its length
static uint8_t FAT_cmpMatch;                    // long name equals FAT_cmpName

// Upper case of an ASCII character
static char FAT_upper(char c) {
  return (c >= 'a' && c <= 'z') ? c - 32 : c;
}

// Read the next entry, returns 1 if found
uint8_t FAT_dirRead(FAT_dir* d, FAT_entry* e) {
  uint8_t* p;
  uint32_t lba;
  uint8_t ord = 0, sum = 0, nameOK = 0, i;
  uint16_t j, end = 0;

  if(!FAT.type) return 0;
  e->lfn = 0;
  while(1) {
    if(!(lba = FAT_dirSector(d))) return 0;
    if(!(p = FAT_cache(lba))) return 0;
    p += (d->index & 15) << 5;
    if(!p[0]) return 0;                         // end of directory (index stays)
    d->index++;

    // Deleted entry
    if(p[0] == 0xE5) { e->lfn = 0; ord = 0; continue; }

    // Long name entry
    if(p[11] == 0x0F) {
      if(p[0] & 0x40) {                         // last part comes first
        ord = p[0] & 0x1F;
        sum = p[13];
        nameOK = 1;
        end = ord * 13;                         // length if there is no terminator
        FAT_cmpMatch = FAT_cmpName ? 1 : 0;
        e->lfn = 0;
        memset(e->name, 0, sizeof(e->name));
      }
      else if(!ord || (p[0] & 0x1F) != ord - 1 || p[13] != sum) { ord = 0; e->lfn = 0; continue; }
      else ord--;
      e->lfn++;
      for(i = 0; i < 13; i++) {
        uint16_t c = rd16(p + pgm_read_byte(&FAT_lfnPos[i]));
        j = (ord - 1) * 13 + i;
        if(c == 0xFFFF) break;
        if(!c) { end = j; break; }
        if(FAT_cmpMatch && (j >= FAT_cmpLen || FAT_upper(FAT_cmpName[j]) != FAT_upper(c))) FAT_cmpMatch = 0;
        if(c < 0x20 || c > 0x7E || j >= FAT_LFN_MAX) nameOK = 0;
        else e->name[j] = c;
      }
      continue;
    }

    // Volume label
    if(p[11] & FAT_ATTR_VOLUME) { e->lfn = 0; ord = 0; continue; }

    // 8.3 entry
    memcpy(e->sfn, p, 11);
    e->attr    = p[11];
    e->cluster = rd16(p + 26);
    if(FAT.type == 32) e->cluster |= (uint32_t)rd16(p + 20) << 16;
    e->size    = rd32(p + 28);
    e->time    = rd16(p + 22);
    e->date    = rd16(p + 24);
    e->index   = d->index - 1;
    e->lba     = lba;
    if(ord != 1 || FAT_checksum(e->sfn) != sum) e->lfn = 0;
    if(!e->lfn || end != FAT_cmpLen) FAT_cmpMatch = 0;
    if(!e->lfn || !nameOK || !e->name[0]) {
      // Build "NAME.EXT" from the 8.3 name
      char* n = e->name;
      for(i = 0; i < 8 && e->sfn[i] != ' '; i++) *n++ = e->sfn[i];
      if(e->sfn[8] != ' ') {
        *n++ = '.';
        for(i = 8; i < 11 && e->sfn[i] != ' '; i++) *n++ = e->sfn[i];
      }
      *n = 0;
      if(e->name[0] == 0x05) e->name[0] = 0xE5;
    }
    return 1;
  }
}

// Find an entry by name (long name or 8.3 name), returns 1 if found
uint8_t FAT_find(uint32_t dir, const char* name, FAT_entry* e) {
  FAT_dir d;
  uint8_t found = 0;
  FAT_dirOpen(&d, dir);
  FAT_cmpName = name;
  FAT_cmpLen = strlen(name);
  while(FAT_dirRead(&d, e)) {
    uint8_t i = 0, j;
    if(e->lfn && FAT_cmpMatch) { found = 1; break; }
    // compare with "NAME.EXT"
    for(j = 0; j < 11; j++) {
      char c = e->sfn[j];
      if(j == 8 && c != ' ' && name[i++] != '.') break;
      if(c == ' ') { if(j < 8) { j = 7; continue; } else break; }
      if(FAT_upper(name[i++]) != c) break;
    }
    if(j >= 8 && !name[i] && (j == 11 || e->sfn[j] == ' ')) { found = 1; break; }
  }
  FAT_cmpName = 0;
  return found;
}

// Volume label of the root directory, returns 1 if found
uint8_t FAT_label(char* buf) {
  FAT_dir d;
  uint32_t lba;
  uint8_t* p;
  if(!FAT.type) return 0;
  FAT_dirOpen(&d, 0);
  while((lba = FAT_dirSector(&d)) && (p = FAT_cache(lba))) {
    p += (d.index & 15) << 5;
    if(!p[0]) break;
    d.index++;
    if(p[0] != 0xE5 && p[11] != 0x0F && (p[11] & FAT_ATTR_VOLUME)) {
      memcpy(buf, p, 11);
      return 1;
    }
  }
  return 0;
}

// ===================================================================================
// Creating Directory Entries
// ===================================================================================

// Characters allowed in 8.3 names (besides letters and digits)
static uint8_t FAT_validChar(char c) {
  return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (uint8_t)c >= 0x80
      || strchr_P(PSTR("!#$%&'()-@^_`{}~"), c);
}

// Build the 8.3 name, returns case flags (0x08 base, 0x10 extension in lower case)
// or 0xFF if the name needs a long name
static uint8_t FAT_makeSFN(const char* name, uint8_t* sfn) {
  uint8_t i = 0, part = 0, flags = 0, upper = 0, len = 8;
  char c;
  memset(sfn, ' ', 11);
  if(*name == '.') return 0xFF;
  while((c = *name++)) {
    if(c == '.') {
      if(part || !i) return 0xFF;               // second dot or empty base
      part = 1; i = 8; len = 11;
      continue;
    }
    if(c >= 'a' && c <= 'z') { c -= 32; flags |= part ? 0x10 : 0x08; }
    else if(c >= 'A' && c <= 'Z') upper |= part ? 0x10 : 0x08;
    if(!FAT_validChar(c) || i >= len) return 0xFF;
    sfn[i++] = c;
  }
  if(!i || (part && i == 8)) return 0xFF;
  if(flags & upper) return 0xFF;                // mixed case in a part
  if(sfn[0] == 0xE5) sfn[0] = 0x05;
  return flags;
}

// Build a unique short alias "BASE~N.EXT" for a long name, returns 0 if successful
static uint8_t FAT_makeAlias(uint32_t dir, const char* name, uint8_t* sfn) {
  const char* ext = strrchr(name, '.');
  uint8_t i, n, base = 0, used[13];
  FAT_dir d;
  FAT_entry e;
  char c;

  memset(sfn, ' ', 11);
  while((c = *name) && name != ext && base < 6) {
    name++;
    if(c == ' ' || c == '.') continue;
    if(c >= 'a' && c <= 'z') c -= 32;
    sfn[base++] = FAT_validChar(c) ? c : '_';
  }
  if(!base) sfn[base++] = '_';
  if(ext) {
    for(i = 8, ext++; *ext && i < 11; ext++) {
      c = *ext;
      if(c == ' ') continue;
      if(c >= 'a' && c <= 'z') c -= 32;
      sfn[i++] = FAT_validChar(c) ? c : '_';
    }
  }

  // Find the numbers already used for this base and extension
  memset(used, 0, sizeof(used));
  FAT_dirOpen(&d, dir);
  while(FAT_dirRead(&d, &e)) {
    if(memcmp(e.sfn, sfn, base) || memcmp(e.sfn + 8, sfn + 8, 3) || e.sfn[base] != '~') continue;
    n = 0;
    for(i = base + 1; i < 8 && e.sfn[i] >= '0' && e.sfn[i] <= '9'; i++) n = n * 10 + e.sfn[i] - '0';
    if(n < 100) used[n >> 3] |= 1 << (n & 7);
  }
  for(n = 1; n < 100 && (used[n >> 3] & (1 << (n & 7))); n++);
  if(n == 100) return FAT_ERR_EXISTS;
  if(n >= 10 && base > 5) base = 5;
  sfn[base++] = '~';
  if(n >= 10) sfn[base++] = '0' + n / 10;
  sfn[base] = '0' + n % 10;
  return 0;
}

// Fill an 8.3 directory entry
static void FAT_fillEntry(uint8_t* p, const uint8_t* sfn, uint8_t nt, uint8_t attr, uint32_t cl, uint32_t size) {
  memset(p, 0, 32);
  memcpy(p, sfn, 11);
  p[11] = attr;
  p[12] = nt;
  wr16(p + 14, FAT_TIME);
  wr16(p + 16, FAT_DATE);
  wr16(p + 18, FAT_DATE);
  wr16(p + 20, cl >> 16);
  wr16(p + 22, FAT_TIME);
  wr16(p + 24, FAT_DATE);
  wr16(p + 26, cl);
  wr32(p + 28, size);
}

// Add a directory entry (with long name if needed), returns 0 if successful.
// lba/slot receive the position of the 8.3 entry.
static uint8_t FAT_addEntry(uint32_t dir, const char* name, uint8_t attr, uint32_t cl,
                            uint32_t size, uint32_t* lba, uint8_t* slot) {
  uint8_t sfn[11], nt, need, run = 0, i, ord, sum;
  uint16_t index = 0, start = 0, len = strlen(name);
  uint8_t* p;

  if(!len || len > 255) return FAT_ERR_NAME;
  nt = FAT_makeSFN(name, sfn);
  if(nt == 0xFF) {
    if(FAT_makeAlias(dir, name, sfn)) return FAT_ERR_EXISTS;
    need = (len + 12) / 13 + 1;
    nt = 0;
  }
  else need = 1;

  // Find enough consecutive free entries (extend the directory if necessary)
  while(run < need) {
    p = FAT_dirEntry(dir, index, lba);
    if(!p) {
      uint32_t last, next, newCl;
      uint8_t s;
      if(!dir && FAT.type == 16) return FAT_ERR_DIRFULL;
      last = dir ? dir : FAT.rootCluster;       // append a cluster to the directory
      while((next = FAT_next(last))) last = next;
      if(!(newCl = FAT_alloc(last))) return FAT_ERR_FULL;
      for(s = 0; s < FAT.spc; s++) if(!FAT_cacheNew(FAT_lba(newCl) + s)) return FAT_ERR_DISK;
      continue;
    }
    if(p[0] == 0 || p[0] == 0xE5) {
      if(!run) start = index;
      run++;
    }
    else run = 0;
    index++;
  }

  // Long name entries (last part first)
  sum = FAT_checksum(sfn);
  for(ord = need - 1, index = start; ord; ord--, index++) {
    if(!(p = FAT_dirEntry(dir, index, lba))) return FAT_ERR_DISK;
    memset(p, 0xFF, 32);
    p[0] = ord | (ord == need - 1 ? 0x40 : 0);
    p[11] = 0x0F; p[12] = 0; p[13] = sum; p[26] = 0; p[27] = 0;
    for(i = 0; i < 13; i++) {
      uint16_t c, pos = (ord - 1) * 13 + i;
      if(pos < len) c = (uint8_t)name[pos];
      else if(pos == len) c = 0;
      else break;
      wr16(p + pgm_read_byte(&FAT_lfnPos[i]), c);
    }
    FAT_cacheDirty = 1;
  }

  // 8.3 entry
  if(!(p = FAT_dirEntry(dir, index, lba))) return FAT_ERR_DISK;
  FAT_fillEntry(p, sfn, nt, attr, cl, size);
  FAT_cacheDirty = 1;
  *slot = index & 15;
  return 0;
}

// Mark the entries of a directory entry (8.3 + long name) as deleted
static uint8_t FAT_removeEntry(uint32_t dir, const FAT_entry* e) {
  uint16_t index = e->index - e->lfn;
  uint32_t lba;
  uint8_t* p;
  for(; index <= e->index; index++) {
    if(!(p = FAT_dirEntry(dir, index, &lba))) return FAT_ERR_DISK;
    p[0] = 0xE5;
    FAT_cacheDirty = 1;
  }
  return 0;
}

// Create a directory, returns 0 if successful
uint8_t FAT_mkdir(uint32_t dir, const char* name) {
  FAT_entry e;
  uint32_t cl, lba;
  uint8_t s, slot, err, *p = 0;
  static const uint8_t dots[11] PROGMEM = { '.', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' };
  uint8_t sfn[11];

  if(!FAT.type) return FAT_ERR_NOFS;
  if(FAT_find(dir, name, &e)) return FAT_ERR_EXISTS;
  if(!(cl = FAT_alloc(0))) return FAT_ERR_FULL;
  for(s = FAT.spc; s--; ) {
    if(!(p = FAT_cacheNew(FAT_lba(cl) + s))) return FAT_ERR_DISK;
  }
  memcpy_P(sfn, dots, 11);
  FAT_fillEntry(p, sfn, 0, FAT_ATTR_DIR, cl, 0);
  sfn[1] = '.';
  FAT_fillEntry(p + 32, sfn, 0, FAT_ATTR_DIR, (dir == FAT.rootCluster) ? 0 : dir, 0);
  err = FAT_addEntry(dir, name, FAT_ATTR_DIR, cl, 0, &lba, &slot);
  if(err) { FAT_freeChain(cl); return err; }
  return FAT_sync();
}

// Delete a file or an empty directory, returns 0 if successful
uint8_t FAT_delete(uint32_t dir, FAT_entry* e) {
  if(e->attr & FAT_ATTR_DIR) {
    FAT_dir d;
    FAT_entry x;
    FAT_dirOpen(&d, e->cluster);
    while(FAT_dirRead(&d, &x)) {
      if(x.sfn[0] != '.') return FAT_ERR_NOTEMPTY;
    }
  }
  if(FAT_removeEntry(dir, e) || FAT_freeChain(e->cluster)) return FAT_ERR_DISK;
  return FAT_sync();
}

// Rename an entry, returns 0 if successful
uint8_t FAT_rename(uint32_t dir, FAT_entry* e, const char* name) {
  FAT_entry x;
  uint32_t lba;
  uint8_t slot, err;
  if(FAT_find(dir, name, &x) && !(x.index == e->index)) return FAT_ERR_EXISTS;
  err = FAT_addEntry(dir, name, e->attr, e->cluster, e->size, &lba, &slot);
  if(err) return err;
  if(FAT_removeEntry(dir, e)) return FAT_ERR_DISK;
  return FAT_sync();
}

// Change the attributes of an entry, returns 0 if successful
uint8_t FAT_setAttr(FAT_entry* e, uint8_t attr) {
  uint8_t* p = FAT_cache(e->lba);
  if(!p) return FAT_ERR_DISK;
  p[((e->index & 15) << 5) + 11] = attr;
  FAT_cacheDirty = 1;
  e->attr = attr;
  return FAT_sync();
}

// ===================================================================================
// Files
// ===================================================================================

// Open the file of a directory entry
void FAT_open(FAT_file* f, const FAT_entry* e) {
  f->first   = e->cluster;
  f->cluster = 0;
  f->size    = e->size;
  f->pos     = 0;
  f->lba     = e->lba;
  f->slot    = e->index & 15;
  f->dirty   = 0;
}

// Create a new empty file, returns 0 if successful
uint8_t FAT_create(FAT_file* f, uint32_t dir, const char* name) {
  uint8_t err;
  if(!FAT.type) return FAT_ERR_NOFS;
  err = FAT_addEntry(dir, name, FAT_ATTR_ARCH, 0, 0, &f->lba, &f->slot);
  if(err) return err;
  f->first = f->cluster = f->size = f->pos = 0;
  f->dirty = 0;
  return FAT_flush();
}

// Cluster for the current position (crossing into the next cluster if needed),
// alloc: append a new cluster at the end of the file. Returns 0 if successful.
static uint8_t FAT_step(FAT_file* f, uint8_t alloc) {
  uint32_t cl;
  if(f->pos & (FAT_clusterSize() - 1)) return 0; // still in the same cluster
  cl = f->pos ? FAT_next(f->cluster) : f->first;
  if(!cl) {
    if(!alloc) return FAT_ERR_EOF;
    if(!(cl = FAT_alloc(f->pos ? f->cluster : 0))) return FAT_ERR_FULL;
    if(!f->first) f->first = cl;
    f->dirty = 1;
  }
  f->cluster = cl;
  return 0;
}

// Sector of the current position
static uint32_t FAT_fileLBA(FAT_file* f) {
  return FAT_lba(f->cluster) + ((f->pos & (FAT_clusterSize() - 1)) >> 9);
}

// Read up to n bytes, returns the number of bytes read
uint16_t FAT_read(FAT_file* f, uint8_t* buf, uint16_t n) {
  uint16_t done = 0, off, chunk;
  uint8_t* p;
  if(f->pos + n > f->size) n = f->size - f->pos;
  while(n) {
    if(FAT_step(f, 0)) break;
    off = f->pos & 511;
    chunk = 512 - off;
    if(chunk > n) chunk = n;
    if(!(p = FAT_cache(FAT_fileLBA(f)))) break;
    memcpy(buf, p + off, chunk);
    buf += chunk; f->pos += chunk; done += chunk; n -= chunk;
  }
  return done;
}

// Write n bytes, returns 0 if successful
uint8_t FAT_write(FAT_file* f, const uint8_t* buf, uint16_t n) {
  uint16_t off, chunk;
  uint8_t* p, err;
  while(n) {
    if((err = FAT_step(f, 1))) return err;
    off = f->pos & 511;
    chunk = 512 - off;
    if(chunk > n) chunk = n;
    if(!off && f->pos >= f->size) p = FAT_cacheNew(FAT_fileLBA(f));   // new sector
    else p = FAT_cache(FAT_fileLBA(f));
    if(!p) return FAT_ERR_DISK;
    memcpy(p + off, buf, chunk);
    FAT_cacheDirty = 1;
    buf += chunk; f->pos += chunk; n -= chunk;
    if(f->pos > f->size) { f->size = f->pos; f->dirty = 1; }
  }
  return 0;
}

// Set the position (not beyond the end of the file), returns 0 if successful
uint8_t FAT_seek(FAT_file* f, uint32_t pos) {
  uint32_t cs = FAT_clusterSize(), now, target;
  if(pos > f->size) return FAT_ERR_EOF;
  if(!pos) { f->pos = 0; f->cluster = 0; return 0; }
  target = (pos - 1) / cs;
  if(f->pos && (now = (f->pos - 1) / cs) <= target) target -= now;   // go forward
  else { f->cluster = f->first; }                                     // from the start
  while(target--) {
    if(!(f->cluster = FAT_next(f->cluster))) return FAT_ERR_DISK;
  }
  f->pos = pos;
  return 0;
}

// Finish a file: write size and first cluster to its entry, returns 0 if successful
uint8_t FAT_close(FAT_file* f) {
  if(f->dirty) {
    uint8_t* p = FAT_cache(f->lba);
    if(!p) return FAT_ERR_DISK;
    p += f->slot << 5;
    wr16(p + 20, f->first >> 16);
    wr16(p + 26, f->first);
    wr32(p + 28, f->size);
    wr16(p + 22, FAT_TIME);
    wr16(p + 24, FAT_DATE);
    p[11] |= FAT_ATTR_ARCH;
    FAT_cacheDirty = 1;
    f->dirty = 0;
  }
  return FAT_sync();
}
