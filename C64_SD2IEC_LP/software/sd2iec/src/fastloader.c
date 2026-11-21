// ===================================================================================
// Fast Loaders: Detection and Common Functions                               * v1.0 *
// ===================================================================================
//
// A fast loader is recognized by the CRC16 of all bytes it uploads with M-W. When
// the computer starts the code with M-E (or U3..U8 / B-E), the handler that belongs
// to the detected loader and the start address is called.
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <string.h>
#include <avr/pgmspace.h>
#include <util/crc16.h>
#include "system.h"
#include "fastloader.h"
#include "iec.h"
#include "led.h"

// ===================================================================================
// Global Variables
// ===================================================================================
volatile uint8_t detected_loader;               // loader detected by M-W
volatile uint8_t fl_track;                      // DreamLoad job (written by the ISR)
volatile uint8_t fl_sector;
uint8_t  fl_dirchanged;                         // Wheels: disk change flag
uint8_t  (*fast_send_byte)(uint8_t b);          // GEOS/Wheels transfer routines
uint8_t  (*fast_get_byte)(void);

static uint16_t datacrc = 0xFFFF;               // CRC of the uploaded drive code
static uint8_t  previous_loader;                // loader of the last M-E

// GEOS stage 1: part of the uploaded code is needed as decryption key
static uint16_t  capture_address, capture_remain;
static uint8_t   capture_offset;
static buffer_t* capture_buffer;

// ===================================================================================
// Detection Tables
// ===================================================================================
typedef void (*fl_handler_t)(uint8_t p);

// Transfer routines of the GEOS/Wheels variants
enum { RXTX_NONE = 0, RXTX_GEOS_1MHZ, RXTX_GEOS_2MHZ, RXTX_GEOS_1581_21,
       RXTX_WHEELS_1MHZ, RXTX_WHEELS_2MHZ, RXTX_WHEELS44_1541, RXTX_WHEELS44_1581,
       RXTX_FC3OF_PAL, RXTX_FC3OF_NTSC };

typedef struct {
  uint8_t (*rx)(void);
  uint8_t (*tx)(uint8_t b);
} fl_rxtx_t;

static const fl_rxtx_t fl_rxtx_table[] PROGMEM = {
  [RXTX_NONE]          = { 0, 0 },
  #if FL_GEOS
  [RXTX_GEOS_1MHZ]     = { geos_get_byte_1mhz,     geos_send_byte_1mhz     },
  [RXTX_GEOS_2MHZ]     = { geos_get_byte_2mhz,     geos_send_byte_2mhz     },
  [RXTX_GEOS_1581_21]  = { geos_get_byte_2mhz,     geos_send_byte_1581_21  },
  #if FL_WHEELS
  [RXTX_WHEELS_1MHZ]   = { wheels_get_byte_1mhz,   wheels_send_byte_1mhz   },
  [RXTX_WHEELS_2MHZ]   = { geos_get_byte_2mhz,     geos_send_byte_1581_21  },
  [RXTX_WHEELS44_1541] = { wheels44_get_byte_1mhz, wheels_send_byte_1mhz   },
  [RXTX_WHEELS44_1581] = { wheels44_get_byte_2mhz, wheels44_send_byte_2mhz },
  #endif
  #endif
  #if FL_FC3
  [RXTX_FC3OF_PAL]     = { 0, fc3_oldfreeze_pal_send  },
  [RXTX_FC3OF_NTSC]    = { 0, fc3_oldfreeze_ntsc_send },
  #endif
};

// CRC16 of the uploaded code -> loader
typedef struct {
  uint16_t crc;
  uint8_t  loader;
  uint8_t  rxtx;
} fl_crc_t;

static const fl_crc_t fl_crc_table[] PROGMEM = {
  #if FL_TURBODISK
  { 0x9C9F, LD_TURBODISK,        RXTX_NONE          },
  #endif
  #if FL_FC3
  { 0xDAB0, LD_FC3_LOAD,         RXTX_NONE          },  // Final Cartridge III
  { 0x973B, LD_FC3_LOAD,         RXTX_NONE          },  // Final Cartridge III variation
  { 0x7E38, LD_FC3_LOAD,         RXTX_NONE          },  // EXOS v3
  { 0x1B30, LD_FC3_SAVE,         RXTX_NONE          },  // early CRC, C64 code follows
  { 0x8B0E, LD_FC3_SAVE,         RXTX_NONE          },  // variation
  { 0x9930, LD_FC3_FREEZED,      RXTX_NONE          },
  { 0x0281, LD_FC3_OLDFREEZED,   RXTX_FC3OF_PAL     },  // older freezer loader, PAL
  { 0xC196, LD_FC3_OLDFREEZED,   RXTX_FC3OF_NTSC    },  // older freezer loader, NTSC
  #endif
  #if FL_DREAMLOAD
  { 0x2E69, LD_DREAMLOAD,        RXTX_NONE          },
  #endif
  #if FL_ULOAD3
  { 0xDD81, LD_ULOAD3,           RXTX_NONE          },
  #endif
  #if FL_ELOAD1
  { 0x393E, LD_ELOAD1,           RXTX_NONE          },
  #endif
  #if FL_EPYXCART
  { 0x5A01, LD_EPYXCART,         RXTX_NONE          },
  #endif
  #if FL_GEOS
  { 0xB979, LD_GEOS_S1_64,       RXTX_GEOS_1MHZ     },  // GEOS 64 stage 1
  { 0x2469, LD_GEOS_S1_128,      RXTX_GEOS_1MHZ     },  // GEOS 128 stage 1
  { 0x4D79, LD_GEOS_S23_1541,    RXTX_GEOS_1MHZ     },  // GEOS 64 1541 stage 2
  { 0xB2BC, LD_GEOS_S23_1541,    RXTX_GEOS_1MHZ     },  // GEOS 128 1541 stage 2
  { 0xB272, LD_GEOS_S23_1541,    RXTX_GEOS_1MHZ     },  // GEOS 1541 stage 3 (Configure)
  { 0xDAED, LD_GEOS_S23_1571,    RXTX_GEOS_2MHZ     },  // GEOS 1571 stage 3 (Configure)
  { 0x3F8D, LD_GEOS_S23_1581,    RXTX_GEOS_2MHZ     },  // GEOS 1581 Configure 2.0
  { 0xC947, LD_GEOS_S23_1581,    RXTX_GEOS_1581_21  },  // GEOS 1581 Configure 2.1
  #if FL_WHEELS
  { 0xF140, LD_WHEELS_S1_64,     RXTX_WHEELS_1MHZ   },  // Wheels 64 stage 1
  { 0x737E, LD_WHEELS_S1_128,    RXTX_WHEELS_1MHZ   },  // Wheels 128 stage 1
  { 0x755A, LD_WHEELS_S2,        RXTX_WHEELS_1MHZ   },  // Wheels 64 1541 stage 2
  { 0x2920, LD_WHEELS_S2,        RXTX_WHEELS_1MHZ   },  // Wheels 128 1541 stage 2
  { 0x18E9, LD_WHEELS_S2,        RXTX_WHEELS_2MHZ   },  // Wheels 64 1571
  { 0x9804, LD_WHEELS_S2,        RXTX_WHEELS_2MHZ   },  // Wheels 64 1581
  { 0x48F5, LD_WHEELS_S2,        RXTX_WHEELS_2MHZ   },  // Wheels 64 FD native
  { 0x1356, LD_WHEELS_S2,        RXTX_WHEELS_2MHZ   },  // Wheels 64 FD emulation
  { 0xE885, LD_WHEELS_S2,        RXTX_WHEELS_2MHZ   },  // Wheels 64 HD native
  { 0x4ECA, LD_WHEELS_S2,        RXTX_WHEELS_2MHZ   },  // Wheels 64 HD emulation
  { 0xDBF6, LD_WHEELS_S2,        RXTX_WHEELS_2MHZ   },  // Wheels 128 1571
  { 0xE4AB, LD_WHEELS_S2,        RXTX_WHEELS_2MHZ   },  // Wheels 128 1581
  { 0x6DE5, LD_WHEELS_S2,        RXTX_WHEELS_2MHZ   },  // Wheels 128 FD native
  { 0x30FF, LD_WHEELS_S2,        RXTX_WHEELS_2MHZ   },  // Wheels 128 FD emulation
  { 0x46E7, LD_WHEELS_S2,        RXTX_WHEELS_2MHZ   },  // Wheels 128 HD native
  { 0x2253, LD_WHEELS_S2,        RXTX_WHEELS_2MHZ   },  // Wheels 128 HD emulation
  { 0xC26A, LD_WHEELS44_S2,      RXTX_WHEELS44_1541 },  // Wheels 4.4 1541
  { 0x550C, LD_WHEELS44_S2,      RXTX_WHEELS44_1541 },  // Wheels 4.4 1571
  { 0x825B, LD_WHEELS44_S2_1581, RXTX_WHEELS44_1581 },  // Wheels 4.4 1581
  { 0x245B, LD_WHEELS44_S2_1581, RXTX_WHEELS44_1581 },
  { 0x7021, LD_WHEELS44_S2_1581, RXTX_WHEELS44_1581 },
  { 0xD537, LD_WHEELS44_S2_1581, RXTX_WHEELS44_1581 },
  { 0xF635, LD_WHEELS44_S2_1581, RXTX_WHEELS44_1581 },
  #endif
  #endif
  #if FL_NIPPON
  { 0x43C1, LD_NIPPON,           RXTX_NONE          },
  #endif
  #if FL_AR6
  { 0x4870, LD_AR6_1581_LOAD,    RXTX_NONE          },
  { 0x2925, LD_AR6_1581_SAVE,    RXTX_NONE          },
  #endif
  #if FL_MMZAK
  { 0x12A6, LD_MMZAK,            RXTX_NONE          },  // Maniac Mansion / Zak McKracken
  #endif
  #if FL_GIJOE
  { 0x0C92, LD_GI_JOE,           RXTX_NONE          },  // GI Joe variant (Eidolon crack)
  #endif
  #if FL_N0SDOS
  { 0x327D, LD_N0SDOS_FILEREAD,  RXTX_NONE          },  // CRC up to $065F
  #endif
  #if FL_SAMSJOURNEY
  { 0x6AF4, LD_SAMSJOURNEY,      RXTX_NONE          },  // CRC of the penultimate M-W
  #endif
  { 0, LD_NONE, 0 }                                     // end marker
};

// Loader + start address -> handler
typedef struct {
  uint16_t     address;
  uint8_t      loader;
  fl_handler_t handler;
  uint8_t      parameter;
} fl_handler_entry_t;

static const fl_handler_entry_t fl_handler_table[] PROGMEM = {
  #if FL_TURBODISK
  { 0x0303, LD_TURBODISK,        load_turbodisk,       0 },
  #endif
  #if FL_FC3
  { 0x059A, LD_FC3_LOAD,         load_fc3,             0 },  // FC3
  { 0x0400, LD_FC3_LOAD,         load_fc3,             0 },  // EXOS
  { 0x059C, LD_FC3_SAVE,         save_fc3,             0 },
  { 0x059A, LD_FC3_SAVE,         save_fc3,             0 },  // variation
  { 0x0403, LD_FC3_FREEZED,      load_fc3,             1 },
  { 0x057F, LD_FC3_OLDFREEZED,   load_fc3oldfreeze,    0 },
  #endif
  #if FL_DREAMLOAD
  { 0x0700, LD_DREAMLOAD,        load_dreamload,       0 },
  #endif
  #if FL_ULOAD3
  { 0x0336, LD_ULOAD3,           load_uload3,          0 },
  #endif
  #if FL_ELOAD1
  { 0x0300, LD_ELOAD1,           load_eload1,          0 },
  #endif
  #if FL_GIJOE
  { 0x0500, LD_GI_JOE,           load_gijoe,           0 },
  #endif
  #if FL_EPYXCART
  { 0x01A9, LD_EPYXCART,         load_epyxcart,        0 },
  #endif
  #if FL_GEOS
  { 0x0457, LD_GEOS_S1_64,       load_geos_s1,         0 },
  { 0x0470, LD_GEOS_S1_128,      load_geos_s1,         1 },
  { 0x03E2, LD_GEOS_S23_1541,    load_geos,            0 },
  { 0x03DC, LD_GEOS_S23_1541,    load_geos,            0 },
  { 0x03FF, LD_GEOS_S23_1571,    load_geos,            0 },
  { 0x040F, LD_GEOS_S23_1581,    load_geos,            0 },
  #if FL_WHEELS
  { 0x0400, LD_WHEELS_S1_64,     load_wheels_s1,       0 },
  { 0x0400, LD_WHEELS_S1_128,    load_wheels_s1,       1 },
  { 0x0300, LD_WHEELS_S2,        load_wheels_s2,       0 },
  { 0x0400, LD_WHEELS44_S2,      load_wheels_s2,       0 },
  { 0x0300, LD_WHEELS44_S2_1581, load_wheels_s2,       0 },
  { 0x0500, LD_WHEELS44_S2_1581, load_wheels_s2,       0 },
  #endif
  #endif
  #if FL_NIPPON
  { 0x0300, LD_NIPPON,           load_nippon,          0 },
  #endif
  #if FL_AR6
  { 0x0500, LD_AR6_1581_LOAD,    load_ar6_1581,        0 },
  { 0x05F4, LD_AR6_1581_SAVE,    save_ar6_1581,        0 },
  #endif
  #if FL_MMZAK
  { 0x0500, LD_MMZAK,            load_mmzak,           0 },
  #endif
  #if FL_N0SDOS
  { 0x041B, LD_N0SDOS_FILEREAD,  load_n0sdos_fileread, 0 },
  #endif
  #if FL_SAMSJOURNEY
  { 0x0400, LD_SAMSJOURNEY,      load_samsjourney,     0 },
  #endif
  { 0, LD_NONE, 0, 0 }                                       // end marker
};

// ===================================================================================
// Drive ROM Emulation for M-R
// ===================================================================================
// Address/value pairs used by some programs to detect a 1541
typedef struct {
  uint16_t address;
  uint8_t  val[2];
} fl_magic_t;

static const fl_magic_t fl_magics[] PROGMEM = {
  { 0xFEA0, { 0x0D, 0xED } },                   // used by DreamLoad and ULoad Model 3
  { 0xE5C6, { 0x34, 0xB1 } },                   // used by DreamLoad and ULoad Model 3
  { 0xFFFE, { 0x00, 0x00 } },                   // disables the AR6 fast loader
  { 0,      { 0x00, 0x00 } }                    // end marker
};

void fl_memread(uint16_t address, uint8_t* data, uint8_t len) {
  const fl_magic_t* p = fl_magics;
  uint16_t a;
  while((a = pgm_read_word(&p->address))) {
    if(a == address) {
      data[0] = pgm_read_byte(&p->val[0]);
      if(len > 1) data[1] = pgm_read_byte(&p->val[1]);
      return;
    }
    p++;
  }
}

// ===================================================================================
// Detection (M-W) and Start (M-E)
// ===================================================================================
void fl_datacrc_reset(void) {
  datacrc = 0xFFFF;
}

// Copy uploaded data into the capture buffer
static void fl_capture(uint16_t address, const uint8_t* data, uint8_t len) {
  uint8_t ofs   = capture_address - address;
  uint8_t bytes = len - ofs;
  if(bytes > capture_remain) bytes = capture_remain;
  memcpy(capture_buffer->data + capture_offset, data + ofs, bytes);
  capture_offset  += bytes;
  capture_address += bytes;
  capture_remain  -= bytes;
  if(!capture_remain) capture_buffer = 0;       // done
}

void fl_memwrite(uint16_t address, const uint8_t* data, uint8_t len) {
  uint8_t i, loader;
  const fl_crc_t* p = fl_crc_table;

  // VIA registers (IRQ timer, N0SDOS port fix) are not part of the loader code
  if(address == 0x1C06 || address == 0x1C07 || address == 0x1802) return;

  previous_loader = LD_NONE;
  for(i = 0; i < len; i++) {
    datacrc = _crc16_update(datacrc, data[i]);
    #if FL_GIJOE
    if(datacrc == 0x38A2 && data[i] == 0x60) detected_loader = LD_GI_JOE;  // many variants
    #endif
  }

  // Search the CRC table
  while((loader = pgm_read_byte(&p->loader)) != LD_NONE) {
    if(datacrc == pgm_read_word(&p->crc)) break;
    p++;
  }
  if(loader != LD_NONE) {
    uint8_t idx = pgm_read_byte(&p->rxtx);
    detected_loader = loader;
    if(idx != RXTX_NONE) {
      fast_get_byte  = (uint8_t (*)(void))   pgm_read_word(&fl_rxtx_table[idx].rx);
      fast_send_byte = (uint8_t (*)(uint8_t))pgm_read_word(&fl_rxtx_table[idx].tx);
    }
  }

  // GEOS stage 1: capture the decryption key ($042A/$044F, 256 bytes)
  if(capture_buffer) fl_capture(address, data, len);
  #if FL_GEOS
  else if(loader == LD_GEOS_S1_64 || loader == LD_GEOS_S1_128) {
    capture_buffer = buffer_alloc();
    if(capture_buffer) {
      capture_buffer->sticky    = 1;
      capture_buffer->secondary = BUFFER_SYS_CAPTURE;
      capture_address = (loader == LD_GEOS_S1_64) ? 0x042A : 0x044F;
      capture_remain  = 256;
      capture_offset  = 0;
      fl_capture(address, data, len);
    }
  }
  #endif
}

void fl_execute(uint16_t address) {
  const fl_handler_entry_t* p = fl_handler_table;
  uint8_t loader;

  if(detected_loader == LD_NONE) detected_loader = previous_loader;

  while((loader = pgm_read_byte(&p->loader)) != LD_NONE) {
    if(loader == detected_loader && address == pgm_read_word(&p->address)) {
      fl_handler_t handler = (fl_handler_t)pgm_read_word(&p->handler);
      handler(pgm_read_byte(&p->parameter));
      break;
    }
    p++;
  }
  if(loader == LD_NONE) set_error_ts(ERROR_UNKNOWN_DRIVECODE, datacrc >> 8, datacrc & 0xFF);

  datacrc = 0xFFFF;
  previous_loader = detected_loader;
  detected_loader = LD_NONE;
}

// ===================================================================================
// Helpers for the Loaders
// ===================================================================================

// Disk change buttons work while a loader is running, a long press (sleep) aborts
// the loader, returns 1 then
uint8_t fl_check_keys(void) {
  uint8_t key = KEY_get();
  if(key == KEY_SLEEP) {
    LED_busy(0);
    return 1;
  }
  if(key) swap_change(key);
  return 0;
}

// Read / write a sector of the mounted disk image, returns the error code
uint8_t fl_read_sector(buffer_t* buf, uint8_t track, uint8_t sector) {
  current_error = ERROR_OK;
  fs_read_sector(buf, track, sector);
  return current_error;
}

uint8_t fl_write_sector(buffer_t* buf, uint8_t track, uint8_t sector) {
  current_error = ERROR_OK;
  buf->dirty = 1;
  LED_dirty(1);
  fs_write_sector(buf, track, sector);
  buf->dirty = 0;
  LED_dirty(0);
  return current_error;
}

// Open a file like OPEN 2,8,sa,"name" (name from flash), returns its buffer or 0
buffer_t* fl_open(uint8_t secondary, const char* name) {
  strcpy_P((char*)command_buffer, name);
  command_length = strlen((char*)command_buffer);
  file_open(secondary);
  return buffer_find(secondary);
}

#if FL_GIJOE || FL_EPYXCART
// Receive a byte with the GI Joe/Epyx protocol: 2 bits per CLOCK change on DATA,
// returns the byte or -1 if aborted
int16_t gijoe_read_byte(void) {
  uint8_t i, value = 0;
  for(i = 0; i < 4; i++) {
    while(IEC_CLOCK) if(fl_check_keys()) return -1;
    value >>= 1;
    DLY_us(3);
    if(!IEC_DATA) value |= 0x80;
    while(!IEC_CLOCK) if(fl_check_keys()) return -1;
    value >>= 1;
    DLY_us(3);
    if(!IEC_DATA) value |= 0x80;
  }
  return value;
}
#endif
