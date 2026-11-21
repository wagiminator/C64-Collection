// ===================================================================================
// Datasette Emulation: TAP Playback and Recording                            * v1.0 *
// ===================================================================================
//
// Functions available:
// --------------------
// TAPE_init()              Init the pins of the cassette port
// TAPE_motor()             1 if the computer has switched the motor on
//
// TAPE_playStart(e,m,v,i)  Open TAP file of entry e and start playback, m/v: machine
//                          and video standard if the file does not tell them,
//                          i: invert signal; returns 0 if successful (TAPE_ERR_...)
// TAPE_playService()       Keep the buffer filled, call often; returns TAPE_RUN while
//                          playing, TAPE_END when done, or an error code
// TAPE_playStop()          Stop playback (also after TAPE_END)
// TAPE_progress()          Playback position 0..255 (part of the file sent)
// TAPE_pause(p)            Pause (1) or continue (0) playback
//
// TAPE_recStart(f,m,v,i)   Start recording into the created file f, m: machine,
//                          v: video standard, i: invert signal; returns 0 if successful
// TAPE_recService()        Write the buffer to the card, call often; returns TAPE_RUN
//                          or an error code
// TAPE_recStop()           Finish the recording and close the file, returns 0 if OK
// TAPE_recSize()           Bytes recorded so far (TAP data without header)
//
// TAPE_machine             Machine of the file being played (TAPE_C64, ...)
// TAPE_video               Video standard of the file being played (TAPE_PAL, ...)
//
// Notes:
// ------
// - Timer 1 is used: CTC mode at 2MHz for playback (READ is set in the compare match
//   interrupt), input capture at 16MHz on ICP1 (PB0) for recording.
// - TAP format versions 0, 1 (full waves) and 2 (half waves, C16) are played.
//   Recordings are written as version 1.
// - Pulse lengths are converted with fixed-point factors for the clock of the
//   machine (C64, VIC-20, C16/Plus4, each PAL or NTSC).
// - Both directions use the 512-byte ring buffer TAPE_buffer, filled and emptied in
//   halves of 256 bytes; playback also uses SD_buffer for the file sectors.
//
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "fat.h"

// Machines and video standards
enum { TAPE_C64, TAPE_VIC, TAPE_C16 };
enum { TAPE_PAL, TAPE_NTSC };

// Return values
#define TAPE_RUN        0               // running
#define TAPE_END        1               // playback finished
#define TAPE_ERR_READ   2               // card read error
#define TAPE_ERR_FORMAT 3               // not a TAP file
#define TAPE_ERR_WRITE  4               // card write error (or card full)
#define TAPE_ERR_SPEED  5               // recording buffer overrun (card too slow)

extern uint8_t TAPE_machine;            // machine of the file being played
extern uint8_t TAPE_video;              // video standard of the file being played

// Functions
void     TAPE_init(void);
uint8_t  TAPE_motor(void);

uint8_t  TAPE_playStart(const FAT_entry* e, uint8_t machine, uint8_t video, uint8_t invert);
uint8_t  TAPE_playService(void);
void     TAPE_playStop(void);
uint8_t  TAPE_progress(void);
void     TAPE_pause(uint8_t p);

uint8_t  TAPE_recStart(FAT_file* f, uint8_t machine, uint8_t video, uint8_t invert);
uint8_t  TAPE_recService(void);
uint8_t  TAPE_recStop(void);
uint32_t TAPE_recSize(void);

#ifdef __cplusplus
};
#endif
