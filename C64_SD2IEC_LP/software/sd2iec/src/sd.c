// ===================================================================================
// SD Card Functions (SPI mode)                                               * v1.0 *
// ===================================================================================
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include "sd.h"
#include "spi.h"
#include "gpio.h"
#include "../config.h"

// SD card commands
#define SD_CMD0         0               // GO_IDLE_STATE
#define SD_CMD8         8               // SEND_IF_COND
#define SD_CMD16        16              // SET_BLOCKLEN
#define SD_CMD17        17              // READ_SINGLE_BLOCK
#define SD_CMD24        24              // WRITE_BLOCK
#define SD_CMD55        55              // APP_CMD
#define SD_CMD58        58              // READ_OCR
#define SD_ACMD41       41              // SD_SEND_OP_COND (after CMD55)

// Card types
#define SD_TYPE_NONE    0               // no card or not initialized
#define SD_TYPE_SDSC    1               // byte addressed (SD version 1 or 2)
#define SD_TYPE_SDHC    2               // block addressed (SDHC/SDXC)

uint8_t SD_buffer[512];                 // common sector buffer
static uint8_t SD_type = SD_TYPE_NONE;  // type of the inserted card

// Select and deselect the card
#define SD_select()     PIN_low(PIN_SD_CS)
#define SD_deselect()   do { PIN_high(PIN_SD_CS); SPI_read(); } while(0)

// Wait until the card is ready (MISO high), returns 1 if ready
static uint8_t SD_waitReady(uint16_t ms) {
  uint16_t start = MIL_read();
  do {
    if(SPI_read() == 0xFF) return 1;
  } while((uint16_t)(MIL_read() - start) < ms);
  return 0;
}

// Send a command, returns the R1 response (bit 7 set: no response)
static uint8_t SD_command(uint8_t cmd, uint32_t arg) {
  uint8_t r1, n;

  // Application specific command: send CMD55 first
  if(cmd & 0x80) {
    cmd &= 0x7F;
    r1 = SD_command(SD_CMD55, 0);
    if(r1 > 1) return r1;
  }

  // Select the card and wait until it is ready
  SD_deselect();
  SD_select();
  if(cmd != SD_CMD0 && !SD_waitReady(500)) return 0xFF;

  // Send command frame
  SPI_write(0x40 | cmd);
  SPI_write(arg >> 24);
  SPI_write(arg >> 16);
  SPI_write(arg >>  8);
  SPI_write(arg);
  SPI_write(cmd == SD_CMD0 ? 0x95 : cmd == SD_CMD8 ? 0x87 : 0x01);   // CRC (needed for CMD0/8)

  // Wait for the response (up to 10 bytes)
  n = 10;
  do r1 = SPI_read(); while((r1 & 0x80) && --n);
  return r1;
}

// Init SD card, returns 0 if successful
uint8_t SD_init(void) {
  uint8_t i, r1, ocr[4];
  uint16_t start;

  SD_type = SD_TYPE_NONE;
  PIN_high(PIN_SD_CS);
  PIN_output(PIN_SD_CS);
  SPI_init();                                   // slow clock for init

  // At least 74 clock cycles with CS high
  for(i = 10; i; i--) SPI_read();

  // Reset the card into SPI mode (idle state)
  for(i = 10; i; i--) {
    if(SD_command(SD_CMD0, 0) == 0x01) break;
  }
  if(!i) { SD_deselect(); return 1; }           // no card

  // Check voltage range (only SD version 2 knows CMD8)
  if(SD_command(SD_CMD8, 0x1AA) == 0x01) {
    for(i = 0; i < 4; i++) ocr[i] = SPI_read();
    if(ocr[2] != 0x01 || ocr[3] != 0xAA) { SD_deselect(); return 2; }

    // Initialize with support for high capacity cards (HCS)
    start = MIL_read();
    do {
      r1 = SD_command(0x80 | SD_ACMD41, 1UL << 30);
      if((uint16_t)(MIL_read() - start) > 1000) { SD_deselect(); return 3; }
    } while(r1);

    // Read OCR: CCS bit tells if the card is block addressed
    if(SD_command(SD_CMD58, 0)) { SD_deselect(); return 4; }
    for(i = 0; i < 4; i++) ocr[i] = SPI_read();
    SD_type = (ocr[0] & 0x40) ? SD_TYPE_SDHC : SD_TYPE_SDSC;
  }
  else {
    // SD version 1
    start = MIL_read();
    do {
      r1 = SD_command(0x80 | SD_ACMD41, 0);
      if((uint16_t)(MIL_read() - start) > 1000) { SD_deselect(); return 5; }
    } while(r1);
    SD_type = SD_TYPE_SDSC;
  }

  // Byte addressed cards: set block length to 512 bytes
  if(SD_type == SD_TYPE_SDSC && SD_command(SD_CMD16, 512)) {
    SD_type = SD_TYPE_NONE;
    SD_deselect();
    return 6;
  }

  SD_deselect();
  SPI_fast();
  return 0;
}

// Read 512-byte sector into buffer, returns 0 if successful
uint8_t SD_read(uint32_t lba, uint8_t* buf) {
  uint16_t start, n;
  uint8_t token;

  if(SD_type == SD_TYPE_NONE) return 1;
  if(SD_type == SD_TYPE_SDSC) lba <<= 9;
  if(SD_command(SD_CMD17, lba)) { SD_deselect(); return 2; }

  // Wait for the data token
  start = MIL_read();
  do {
    token = SPI_read();
    if((uint16_t)(MIL_read() - start) > 200) break;
  } while(token == 0xFF);
  if(token != 0xFE) { SD_deselect(); return 3; }

  // Read data and skip CRC
  n = 512;
  do *buf++ = SPI_read(); while(--n);
  SPI_read();
  SPI_read();
  SD_deselect();
  return 0;
}

// Start writing a sector in parts, returns 0 if successful
// (the card stays selected until SD_writeStop(); the SPI bus must not be used otherwise)
uint8_t SD_writeStart(uint32_t lba) {
  if(SD_type == SD_TYPE_NONE) return 1;
  if(SD_type == SD_TYPE_SDSC) lba <<= 9;
  if(SD_command(SD_CMD24, lba)) { SD_deselect(); return 2; }
  SPI_read();
  SPI_write(0xFE);                              // data token
  return 0;
}

// Send the next part of the sector
void SD_writeBytes(const uint8_t* buf, uint16_t len) {
  while(len--) SPI_write(*buf++);
}

// Finish writing the sector (after exactly 512 bytes), returns 0 if successful
uint8_t SD_writeStop(void) {
  uint8_t response;

  SPI_read();                                   // dummy CRC
  SPI_read();

  // Data response: xxx00101 = accepted
  response = SPI_read();
  if((response & 0x1F) != 0x05) { SD_deselect(); return 3; }

  // Wait until the card has written the data
  if(!SD_waitReady(500)) { SD_deselect(); return 4; }
  SD_deselect();
  return 0;
}

// Write 512-byte sector from buffer, returns 0 if successful
uint8_t SD_write(uint32_t lba, const uint8_t* buf) {
  uint8_t err = SD_writeStart(lba);
  if(err) return err;
  SD_writeBytes(buf, 512);
  return SD_writeStop();
}
