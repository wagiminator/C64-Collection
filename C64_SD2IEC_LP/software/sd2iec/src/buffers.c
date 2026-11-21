// ===================================================================================
// Disk Operating System: Channel Buffers and Status Channel                  * v1.0 *
// ===================================================================================
//
// Every open channel (secondary address 0..14) uses a buffer of 256 bytes, like the
// buffers in the RAM of a 1541. The status channel (15, read) uses an extra buffer
// that holds the current error message.
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <string.h>
#include <avr/pgmspace.h>
#include "dos.h"
#include "led.h"

// ===================================================================================
// Buffers
// ===================================================================================
#define ERROR_BUFFER_SIZE 48

buffer_t buffers[BUFFER_COUNT + 1];
uint8_t  active_buffers;
static uint8_t buffer_data[BUFFER_COUNT * 256];
static uint8_t error_buffer[ERROR_BUFFER_SIZE];

// Callback that does nothing
uint8_t buffer_dummy(buffer_t* buf) {
  return 0;
}

// Status channel read completely: the next read gives "00, OK,00,00"
static uint8_t status_refill(buffer_t* buf) {
  set_error(ERROR_OK);
  return 0;
}

// Initialize the buffers
void buffers_init(void) {
  uint8_t i;
  memset(buffers, 0, sizeof(buffers));
  for(i = 0; i < BUFFER_COUNT; i++) buffers[i].data = buffer_data + 256 * i;
  buffers[BUFFER_COUNT].data      = error_buffer;
  buffers[BUFFER_COUNT].secondary = 15;
  buffers[BUFFER_COUNT].allocated = 1;
  buffers[BUFFER_COUNT].read      = 1;
  buffers[BUFFER_COUNT].sendeoi   = 1;
  buffers[BUFFER_COUNT].refill    = status_refill;
  buffers[BUFFER_COUNT].cleanup   = buffer_dummy;
  active_buffers = 0;
}

// Allocate a buffer, returns 0 (and sets an error) if none is free
buffer_t* buffer_alloc(void) {
  uint8_t i;
  for(i = 0; i < BUFFER_COUNT; i++) {
    buffer_t* b = &buffers[i];
    if(!b->allocated) {
      uint8_t* data = b->data;
      memset(b, 0, sizeof(buffer_t));
      b->data      = data;
      b->allocated = 1;
      b->secondary = BUFFER_SYSTEM;
      b->refill    = buffer_dummy;
      b->cleanup   = buffer_dummy;
      active_buffers++;
      LED_busy(1);
      return b;
    }
  }
  set_error(ERROR_NO_CHANNEL);
  return 0;
}

// Close the channel of a buffer and free it
void buffer_free(buffer_t* buf) {
  if(!buf || !buf->allocated || buf == &buffers[BUFFER_COUNT]) return;
  buf->cleanup(buf);
  buf->allocated = 0;
  active_buffers--;
  if(!active_buffers) LED_busy(0);
}

// Free the buffers that are not sticky (all: every buffer), returns 1 on errors
uint8_t buffers_free(uint8_t all) {
  uint8_t i, err = 0;
  for(i = 0; i < BUFFER_COUNT; i++) {
    buffer_t* b = &buffers[i];
    if(b->allocated && (all || !b->sticky)) {
      if(b->cleanup(b)) err = 1;
      b->allocated = 0;
      active_buffers--;
    }
  }
  if(!active_buffers) LED_busy(0);
  return err;
}

// Buffer of a channel, 0 if the channel is not open
buffer_t* buffer_find(uint8_t secondary) {
  uint8_t i;
  if(secondary == 15) return &buffers[BUFFER_COUNT];
  for(i = 0; i < BUFFER_COUNT; i++) {
    if(buffers[i].allocated && buffers[i].secondary == secondary) return &buffers[i];
  }
  return 0;
}

// ===================================================================================
// Error Messages
// ===================================================================================
uint8_t current_error;

// Messages: error code, text (codes are characters >= 0x80 + code / 2)
#define EC(x)  (0x80 + (x))
static const uint8_t messages[] PROGMEM =
  "\x80" " OK"
  "\x81" "FILES SCRATCHED"
  "\x94\x95\x96\x97\x98\x9b" "READ ERROR"
  "\x99\x9c" "WRITE ERROR"
  "\x9a" "WRITE PROTECT ON"
  "\x9d" "DISK ID MISMATCH"
  "\x9e\x9f\xa0\xa1\xa2" "SYNTAX ERROR"
  "\xa7\xbe" "FILE NOT FOUND"
  "\xb2" "RECORD NOT PRESENT"
  "\xb4" "FILE TOO LARGE"
  "\xbc" "WRITE FILE OPEN"
  "\xbd" "FILE NOT OPEN"
  "\xbf" "FILE EXISTS"
  "\xc0" "FILE TYPE MISMATCH"
  "\xc1" "NO BLOCK"
  "\xc2\xc3" "ILLEGAL TRACK OR SECTOR"
  "\xc6" "NO CHANNEL"
  "\xc7" "DIR ERROR"
  "\xc8" "DISK FULL"
  "\xca" "DRIVE NOT READY"
  "\xcf" "IMAGE INVALID"
  "\xe2" "UNKNOWN DRIVECODE"
  "\xff";

// Version message (73 and 9)
static const char version_msg[] PROGMEM = "SD2IEC V" VERSION " LP";

// Append a number (at least two digits)
static uint8_t* appendnum(uint8_t* p, uint8_t n) {
  if(n >= 100) { *p++ = '0' + n / 100; n %= 100; }
  *p++ = '0' + n / 10;
  *p++ = '0' + n % 10;
  return p;
}

// Set the error message with track and sector
void set_error_ts(uint8_t code, uint8_t track, uint8_t sector) {
  buffer_t* b = &buffers[BUFFER_COUNT];
  uint8_t* p = error_buffer;
  uint16_t i = 0;
  uint8_t c;

  current_error = code;
  p = appendnum(p, code);
  *p++ = ',';

  if(code == ERROR_DOSVERSION || code == ERROR_LONGVERSION) {
    strcpy_P((char*)p, version_msg);
    p += strlen((char*)p);
  }
  else {
    // find the code in the table
    do {
      c = pgm_read_byte(&messages[i++]);
      if(c == 0xFF) break;
    } while(c != EC(code));
    if(c == 0xFF) *p++ = '?';
    else {
      while(pgm_read_byte(&messages[i]) >= 0x80) i++;   // skip further codes
      while((c = pgm_read_byte(&messages[i++])) < 0x80 && c) *p++ = c;
    }
  }
  *p++ = ',';
  p = appendnum(p, track);
  *p++ = ',';
  p = appendnum(p, sector);
  *p = 13;

  b->position = 0;
  b->lastused = p - error_buffer;
  b->read     = 1;
  b->sendeoi  = 1;

  LED_error(code != ERROR_OK && code != ERROR_SCRATCHED && code != ERROR_DOSVERSION
            && code != ERROR_LONGVERSION);
}

// Set the error message
void set_error(uint8_t code) {
  set_error_ts(code, 0, 0);
}
