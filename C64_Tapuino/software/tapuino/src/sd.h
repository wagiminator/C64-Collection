// ===================================================================================
// SD Card Functions (SPI mode)                                               * v1.0 *
// ===================================================================================
//
// Functions available:
// --------------------
// SD_init()                Init SD card, returns 0 if successful (error code otherwise)
// SD_read(lba, buf)        Read 512-byte sector (lba) into buffer, returns 0 if successful
// SD_write(lba, buf)       Write 512-byte sector (lba) from buffer, returns 0 if successful
//
// SD_writeStart(lba)       Start writing a sector in parts, returns 0 if successful
// SD_writeBytes(buf, len)  Send the next len bytes of the sector
// SD_writeStop()           Finish the sector (all 512 bytes sent), returns 0 if successful
//
// SD_buffer[512]           Common sector buffer
//
// Notes:
// ------
// - Supports SD cards version 1 (SDSC, byte addressed) and version 2 (SDSC, SDHC and
//   SDXC, block addressed). MMC cards are not supported.
// - The sector address (lba) is always counted in 512-byte sectors.
// - Needs the SPI functions and the system tick (MIL_read) for timeouts.
// - The chip select pin is set in config.h (PIN_SD_CS).
//
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "system.h"

extern uint8_t SD_buffer[512];          // common sector buffer

uint8_t SD_init(void);                  // init SD card
uint8_t SD_read(uint32_t lba, uint8_t* buf);        // read sector
uint8_t SD_write(uint32_t lba, const uint8_t* buf); // write sector

uint8_t SD_writeStart(uint32_t lba);    // start writing a sector in parts
void SD_writeBytes(const uint8_t* buf, uint16_t len); // send part of the sector
uint8_t SD_writeStop(void);             // finish writing the sector

#ifdef __cplusplus
};
#endif
