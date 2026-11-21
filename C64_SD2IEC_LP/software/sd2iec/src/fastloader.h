// ===================================================================================
// Fast Loaders                                                               * v1.0 *
// ===================================================================================
//
// Fast loaders upload their own code into the drive (M-W) and start it (M-E). The
// SD2IEC cannot run 6502 code; instead it recognizes known loaders by a CRC of the
// uploaded data and emulates their transfer protocol.
//
// Functions available:
// --------------------
// fl_memwrite(a, d, n)     M-W: detect the loader by the CRC of the data
// fl_memread(a, d, n)      M-R: answers for drive detection (n bytes into d)
// fl_execute(a)            M-E / U3..U8: run the handler of the detected loader
// fl_datacrc_reset()       reset the CRC (OPEN of a file)
//
// Helpers for the loaders (fastloader.c):
// ---------------------------------------
// fl_check_keys()          handle the disk change buttons while a loader runs
// fl_read_sector(b, t, s)  read a sector of the image into a buffer, returns error
// fl_write_sector(b, t, s) write a buffer to a sector of the image, returns error
// fl_open(sa, name)        open a file (PETSCII name from flash), returns its buffer
// gijoe_read_byte()        receive a byte with the GI Joe/Epyx protocol (-1: abort)
//
// Notes:
// ------
// - The protocols are documented in the upstream sd2iec sources (docs/).
// - Every loader is a function "void load_xxx(uint8_t parameter)" that takes over
//   the bus until the loader on the computer side ends.
// - Low level routines that need exact timing are in fastloader_ll.S.
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

// These two values are needed by the interrupt routine in fastloader_ll.S
#define FLCODE_DREAMLOAD      1
#define FLCODE_DREAMLOAD_OLD  2

#ifndef __ASSEMBLER__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "dos.h"
#include "../config.h"

// Loader IDs
enum {
  LD_NONE = 0,
  LD_DREAMLOAD = FLCODE_DREAMLOAD,
  LD_DREAMLOAD_OLD = FLCODE_DREAMLOAD_OLD,
  LD_TURBODISK, LD_FC3_LOAD, LD_FC3_SAVE, LD_FC3_FREEZED, LD_FC3_OLDFREEZED,
  LD_ULOAD3, LD_GI_JOE, LD_EPYXCART, LD_ELOAD1, LD_NIPPON,
  LD_GEOS_S1_64, LD_GEOS_S1_128, LD_GEOS_S23_1541, LD_GEOS_S23_1571, LD_GEOS_S23_1581,
  LD_WHEELS_S1_64, LD_WHEELS_S1_128, LD_WHEELS_S2, LD_WHEELS44_S2, LD_WHEELS44_S2_1581,
  LD_AR6_1581_LOAD, LD_AR6_1581_SAVE, LD_MMZAK, LD_N0SDOS_FILEREAD, LD_SAMSJOURNEY
};

#define BUFFER_SYS_CAPTURE  (BUFFER_SYSTEM + 1)   // GEOS: captured key data

extern volatile uint8_t detected_loader;      // loader detected by M-W (or LD_NONE)
extern volatile uint8_t fl_track;             // DreamLoad: job track (set by the ISR)
extern volatile uint8_t fl_sector;            // DreamLoad: job sector
extern uint8_t  fl_dirchanged;                // Wheels: disk has changed
extern uint8_t  (*fast_send_byte)(uint8_t b); // GEOS/Wheels: current transfer routines
extern uint8_t  (*fast_get_byte)(void);

// DOS interface
void     fl_memwrite(uint16_t address, const uint8_t* data, uint8_t len);
void     fl_memread(uint16_t address, uint8_t* data, uint8_t len);
void     fl_execute(uint16_t address);
void     fl_datacrc_reset(void);

// Helpers
uint8_t  fl_check_keys(void);
uint8_t  fl_read_sector(buffer_t* buf, uint8_t track, uint8_t sector);
uint8_t  fl_write_sector(buffer_t* buf, uint8_t track, uint8_t sector);
buffer_t* fl_open(uint8_t secondary, const char* name);
int16_t  gijoe_read_byte(void);

// Loaders (fl_file.c, fl_sector.c, fl_geos.c)
void load_turbodisk(uint8_t p);
void load_fc3(uint8_t freezed);
void load_fc3oldfreeze(uint8_t p);
void save_fc3(uint8_t p);
void load_dreamload(uint8_t p);
void load_uload3(uint8_t p);
void load_eload1(uint8_t p);
void load_gijoe(uint8_t p);
void load_epyxcart(uint8_t p);
void load_geos(uint8_t p);
void load_geos_s1(uint8_t version);
void load_wheels_s1(uint8_t version);
void load_wheels_s2(uint8_t p);
void load_nippon(uint8_t p);
void load_ar6_1581(uint8_t p);
void save_ar6_1581(uint8_t p);
void load_mmzak(uint8_t p);
void load_n0sdos_fileread(uint8_t p);
void load_samsjourney(uint8_t p);

// Low level routines (fastloader_ll.S)
uint8_t jiffy_send(uint8_t value, uint8_t eoi, uint8_t loadflags);
uint8_t jiffy_receive(uint8_t* busstate);
void    turbodisk_byte(uint8_t value);
void    turbodisk_buffer(uint8_t* data, uint8_t length);
void    clk_data_handshake(void);
void    fastloader_fc3_send_block(uint8_t* data);
uint8_t fc3_get_byte(void);
uint8_t fc3_oldfreeze_pal_send(uint8_t byte);
uint8_t fc3_oldfreeze_ntsc_send(uint8_t byte);
uint8_t dreamload_get_byte(void);
void    dreamload_send_byte(uint8_t byte);
int16_t uload3_get_byte(void);
void    uload3_send_byte(uint8_t byte);
uint8_t epyxcart_send_byte(uint8_t byte);
uint8_t geos_get_byte_1mhz(void);
uint8_t geos_get_byte_2mhz(void);
uint8_t geos_send_byte_1mhz(uint8_t byte);
uint8_t geos_send_byte_2mhz(uint8_t byte);
uint8_t geos_send_byte_1581_21(uint8_t byte);
uint8_t wheels_send_byte_1mhz(uint8_t byte);
uint8_t wheels_get_byte_1mhz(void);
uint8_t wheels44_get_byte_1mhz(void);
uint8_t wheels44_get_byte_2mhz(void);
uint8_t wheels44_send_byte_2mhz(uint8_t byte);
void    ar6_1581_send_byte(uint8_t byte);
uint8_t ar6_1581p_get_byte(void);
void    n0sdos_send_byte(uint8_t byte);

#ifdef __cplusplus
};
#endif

#endif // __ASSEMBLER__
