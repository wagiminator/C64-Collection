// ===================================================================================
// Fast Loaders: File Based Loaders                                           * v1.0 *
// ===================================================================================
//
// Loaders that transfer whole files that were opened by name: Turbodisk, Final
// Cartridge III / EXOS, ELoad, GI Joe, Epyx FastLoad cartridge, Action Replay 6
// (1581 mode), N0stalgia and Sam's Journey.
//
// The file is read through the normal channel buffer (254 data bytes per block in
// data[2..lastused], refill() reads the next block, sendeoi marks the last one).
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
//   Final Cartridge III / DreamLoad: Thomas Giesel, ELoad: Thomas Giesel,
//   Nippon/N0SDOS/Sam's Journey/MM-Zak: Ingo Korb and contributors.
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <string.h>
#include <util/atomic.h>
#include <avr/boot.h>
#include <avr/pgmspace.h>
#include "system.h"
#include "fastloader.h"
#include "iec.h"
#include "led.h"

// Append a received byte to a write buffer (flushes full blocks)
static void fl_put(buffer_t* buf, uint8_t b) {
  if(buf->mustflush) buf->refill(buf);         // errors are ignored, like the C64 does
  buf->data[buf->position] = b;
  if(buf->lastused < buf->position) buf->lastused = buf->position;
  if(!++buf->position) buf->mustflush = 1;
}

// ===================================================================================
// Turbodisk
// ===================================================================================
#if FL_TURBODISK
void load_turbodisk(uint8_t p) {
  uint8_t i, len, first = 1;
  buffer_t* buf;

  // The protocol needs a crystal, refuse to work with the internal RC oscillator
  if((boot_lock_fuse_bits_get(GET_LOW_FUSE_BITS) & 0x0F) == 2) {
    set_error(ERROR_UNKNOWN_DRIVECODE);
    return;
  }

  set_clock(0);

  // File name follows the drive code in the M-E command
  len = command_buffer[9];
  memmove(command_buffer, command_buffer + 10, len);
  command_buffer[len] = 0;
  command_length = len;
  file_open(0);
  buf = buffer_find(0);

  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    if(!buf) turbodisk_byte(0xFF);              // file not found
    else while(1) {
      turbodisk_byte(buf->sendeoi ? 0 : 1);     // status: last block or more
      if(first) {                               // load address is sent separately
        turbodisk_byte(buf->data[buf->position++]);
        turbodisk_byte(buf->data[buf->position++]);
        first = 0;
      }
      if(buf->sendeoi) {                        // last block: length + bytes
        turbodisk_byte(buf->lastused - buf->position + 2);
        i = buf->position;
        do turbodisk_byte(buf->data[i]); while(i++ < buf->lastused);
        break;
      }
      turbodisk_buffer(buf->data + buf->position, 254);
      if(buf->refill(buf)) { turbodisk_byte(0xFF); break; }
    }
  }
  if(buf) buffer_free(buf);
  set_clock(1);
  set_data(1);
}
#endif

// ===================================================================================
// Final Cartridge III / EXOS
// ===================================================================================
#if FL_FC3
void load_fc3(uint8_t freezed) {
  buffer_t* buf = buffer_find(0);
  uint8_t step, pos, counter = 0;
  uint8_t block[4];

  if(!buf) { set_data(0); set_clock(0); return; }   // error: both lines low

  DLY_ms(20);                                   // make sure the VIC DMA is off
  while(1) {
    clk_data_handshake();

    // First 4-byte block: (unused), sector counter, length (0 = full), first byte
    pos = 2;
    block[1] = counter++;
    block[2] = buf->sendeoi ? buf->lastused : 0;
    block[3] = buf->data[pos++];
    if(!freezed) DLY_us(190);
    fastloader_fc3_send_block(block);

    // 64 more blocks; the last 3 bytes are read behind the data (harmless)
    for(step = 0; step < 64; step++) {
      if(!IEC_ATN) goto cleanup;
      if(freezed) clk_data_handshake();
      else DLY_us(190);
      fastloader_fc3_send_block(buf->data + pos);
      pos += 4;
    }

    if(buf->sendeoi) { set_data(0); break; }   // DATA low: last block
    if(buf->refill(buf)) { set_data(0); set_clock(0); break; }
  }
cleanup:
  buffer_free(buf);
}

void save_fc3(uint8_t p) {
  buffer_t* buf = buffer_find(1);
  uint8_t n, size, eof = 0;

  if(!buf || !buf->write) return;
  DLY_ms(5);                                    // the host pulls DATA and gets ready
  do {
    set_data(0);
    size = fc3_get_byte();
    if(!size) size = 254;                       // full block follows
    else { size--; eof = 1; }                   // last block
    for(n = 0; n < size; n++) fl_put(buf, fc3_get_byte());
  } while(!eof);
  buffer_free(buf);
}

void load_fc3oldfreeze(uint8_t p) {
  buffer_t* buf;
  uint16_t i;

  set_clock(1);                                 // busy
  set_data(0);
  DLY_us(1);
  TMR_start(100);                               // wait until UNLISTEN is done
  while(!IEC_CLOCK && !TMR_expired());

  buf = buffer_find(0);
  if(!buf) return;
  while(1) {
    for(i = 2; i <= buf->lastused; i++)
      if(fast_send_byte(buf->data[i])) goto done;   // ATN: abort
    if(buf->sendeoi || buf->refill(buf)) break;
  }
done:
  buffer_free(buf);
}
#endif

// ===================================================================================
// ELoad (version 1)
// ===================================================================================
#if FL_ELOAD1
void load_eload1(uint8_t p) {
  buffer_t* buf;
  int16_t cmd;
  uint8_t count, pos;

  while((cmd = uload3_get_byte()) >= 0) {       // -1: ATN
    if(cmd != 1) { uload3_send_byte(0xFF); continue; }  // only "load file"

    buf = buffer_find(0);
    if(!buf) {
      if(IEC_ATN) uload3_send_byte(0xFF);
      return;
    }
    while(1) {
      count = buf->lastused - 1;
      if(!IEC_ATN) return;
      uload3_send_byte(count);
      pos = 2;
      while(count--) {
        if(!IEC_ATN) return;
        uload3_send_byte(buf->data[pos++]);
      }
      if(buf->sendeoi) { uload3_send_byte(0); break; }     // EOF
      if(buf->refill(buf)) { uload3_send_byte(0xFF); break; }
    }
  }
}
#endif

// ===================================================================================
// GI Joe
// ===================================================================================
#if FL_GIJOE
static void gijoe_send_byte(uint8_t value) {
  uint8_t i;
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    for(i = 0; i < 4; i++) {
      while(!IEC_CLOCK);
      set_data(value & 1); value >>= 1;
      while(IEC_CLOCK);
      set_data(value & 1); value >>= 1;
    }
  }
}

static void gijoe_send_error(void) {
  gijoe_send_byte(0xFE); gijoe_send_byte(0xFE);
  gijoe_send_byte(0xAC); gijoe_send_byte(0xF7);
}

void load_gijoe(uint8_t p) {
  buffer_t* buf;
  uint8_t i;

  set_data(1);
  set_clock(1);
  set_atn_irq(0);
  DLY_ms(10);                                   // wait until the bus has settled
  while(!IEC_DATA || !IEC_CLOCK);

  while(1) {
    set_clock(0);                               // handshake
    while(IEC_DATA) if(fl_check_keys()) return;
    set_clock(1);

    if(gijoe_read_byte() < 0) return;           // first byte is ignored
    command_buffer[0] = gijoe_read_byte();      // two characters of the name
    command_buffer[1] = gijoe_read_byte();
    set_clock(0);
    command_buffer[2] = '*';
    command_buffer[3] = 0;
    command_length = 3;
    file_open(0);
    buf = buffer_find(0);
    if(!buf) { set_clock(1); gijoe_send_error(); continue; }

    while(1) {
      i = buf->position;
      set_clock(1);
      DLY_us(2);
      do {
        if(buf->data[i] == 0xAC) gijoe_send_byte(0xAC);   // escape the marker
        gijoe_send_byte(buf->data[i]);
      } while(i++ < buf->lastused);

      if(buf->sendeoi) {                        // end marker
        gijoe_send_byte(0xAC); gijoe_send_byte(0xFF);
        buffer_free(buf);
        break;
      }
      gijoe_send_byte(0xAC); gijoe_send_byte(0xC3);       // another block follows
      DLY_us(50);
      set_clock(0);
      if(buf->refill(buf)) { gijoe_send_error(); buffer_free(buf); break; }
    }
  }
}
#endif

// ===================================================================================
// Epyx FastLoad Cartridge
// ===================================================================================
#if FL_EPYXCART
void load_epyxcart(uint8_t p) {
  uint8_t checksum = 0;
  int16_t b, i;
  buffer_t* buf;

  set_data(1);                                  // initial handshake
  set_clock(0);
  set_atn_irq(0);
  while(IEC_DATA) if(!IEC_ATN) return;
  set_clock(1);

  // Receive stage 2 (256 bytes, the last 19 are junk) and check it
  for(i = 0; i < 256; i++) {
    if((b = gijoe_read_byte()) < 0) return;
    if(i < 237) checksum ^= b;
  }
  if(checksum != 0x91 && checksum != 0x5B) return;

  // Receive the file name (backwards)
  if((i = gijoe_read_byte()) < 0 || i > COMMAND_LENGTH) return;
  command_length = i;
  command_buffer[i] = 0;
  while(i > 0) {
    if((b = gijoe_read_byte()) < 0) return;
    command_buffer[--i] = b;
  }
  set_clock(0);

  file_open(0);
  buf = buffer_find(0);
  if(!buf) { set_clock(1); return; }

  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    while(1) {
      set_clock(1);
      set_data(1);
      if(epyxcart_send_byte(buf->lastused - 1)) break;      // number of bytes
      for(i = 2; i <= buf->lastused; i++)
        if(epyxcart_send_byte(buf->data[i])) break;
      if(!IEC_ATN || buf->sendeoi) break;
      set_clock(0);
      if(buf->refill(buf)) break;
    }
  }
  set_clock(1);
  set_data(1);
  buffer_free(buf);
}
#endif

// ===================================================================================
// Action Replay 6 (1581 mode)
// ===================================================================================
#if FL_AR6
void load_ar6_1581(uint8_t p) {
  buffer_t* buf = buffer_find(0);
  uint16_t i;

  if(!buf) return;
  set_clock(0);
  set_data(1);
  DLY_ms(1);
  while(1) {
    ar6_1581_send_byte(buf->lastused - 1);      // bytes in this block
    for(i = 2; i <= buf->lastused; i++) ar6_1581_send_byte(buf->data[i]);
    if(buf->sendeoi || buf->refill(buf)) break;
  }
  ar6_1581_send_byte(0);                        // end marker
  DLY_ms(1);
  set_clock(1);
  set_data(1);
}

void save_ar6_1581(uint8_t p) {
  buffer_t* buf = buffer_find(1);
  uint8_t i;

  if(!buf) return;
  set_clock(0);
  set_data(1);
  DLY_ms(1);
  do {
    buf->dirty = 1;
    LED_dirty(1);
    i = 0;                                      // receive a whole block with link
    do buf->data[i] = ar6_1581p_get_byte(); while(++i);
    buf->position = buf->data[0] ? 0 : buf->data[1];  // end of the data
    if(buf->refill(buf)) break;
  } while(buf->data[0]);
  buffer_free(buf);
}
#endif

// ===================================================================================
// N0stalgia Loaders and Sam's Journey: Common Byte Receiver
// ===================================================================================
#if FL_N0SDOS || FL_SAMSJOURNEY
// Receive a byte: the C64 pulls DATA (1) or CLOCK (0), the drive acknowledges with
// the other line. Returns -1 if ATN goes low.
static int16_t n0s_getbyte(uint8_t release_first) {
  uint8_t i, bus, byte = 0;
  for(i = 0; i < 8; i++) {
    if(release_first) { set_clock(1); set_data(1); DLY_us(2); }
    do {                                        // wait for a bit
      fl_check_keys();
      bus = IEC_bus();
      if(!(bus & IEC_BIT_ATN)) return -1;
    } while((bus & (IEC_BIT_CLOCK | IEC_BIT_DATA)) == (IEC_BIT_CLOCK | IEC_BIT_DATA));
    byte >>= 1;
    if(!(bus & IEC_BIT_DATA)) byte |= 0x80;
    if(bus & IEC_BIT_DATA) set_data(0);         // acknowledge
    else set_clock(0);
    DLY_us(2);
    do {                                        // wait for the C64's acknowledge
      bus = IEC_bus();
      if(!(bus & IEC_BIT_ATN)) return -1;
    } while(!(bus & (IEC_BIT_CLOCK | IEC_BIT_DATA)));
    if(!release_first) { set_clock(1); set_data(1); DLY_us(2); }
  }
  return byte;
}
#endif

// ===================================================================================
// N0stalgia Loaders (file read)
// ===================================================================================
#if FL_N0SDOS
void load_n0sdos_fileread(uint8_t p) {
  buffer_t* buf;
  uint8_t i, *ptr;
  int16_t val;

  set_clock(1);
  set_data(0);
  DLY_ms(10);

  while(IEC_ATN) {
    set_clock(1);                               // receive a file name (max. 7 chars)
    set_data(1);
    DLY_us(2);
    memset(command_buffer, 0, sizeof(command_buffer));
    for(i = 0; i < 7; i++) {
      if((val = n0s_getbyte(0)) < 0) return;
      if(!val) break;
      command_buffer[i] = val;
    }
    command_length = i;
    if(i == 7) command_buffer[command_length++] = '*';   // partial match at max length
    set_clock(0);
    set_data(0);

    file_open(0);
    buf = buffer_find(0);
    if(!buf) { n0sdos_send_byte(0xFF); continue; }

    n0sdos_send_byte(0x00);                     // transfer until CLOCK goes high
    while(1) {
      ptr = buf->data + buf->position;
      for(i = 0; i < 254; i++) {
        if(IEC_CLOCK || !IEC_ATN) goto leave;
        n0sdos_send_byte(*ptr++);
      }
      if(!buf->sendeoi) buf->refill(buf);
    }
  leave:
    buffer_free(buf);
  }
}
#endif

// ===================================================================================
// Sam's Journey
// ===================================================================================
#if FL_SAMSJOURNEY
static const char hexchars[16] PROGMEM = "0123456789ABCDEF";

static uint8_t hexnibble(uint8_t c) {
  if(c >= '0' && c <= '9') return c - '0';
  if(c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0xFF;
}

static uint8_t hex2bin(const uint8_t* s) {
  uint8_t h = hexnibble(s[0]), l = hexnibble(s[1]);
  return (h > 15 || l > 15) ? 0xFF : (h << 4) | l;
}

// Send a byte as four bit pairs, clocked by ATN
static void sj_transmit_byte(uint8_t byte) {
  byte = ~byte;
  while(!IEC_ATN);
  set_clock(byte & 0x80); set_data(byte & 0x20);
  while(IEC_ATN);
  set_clock(byte & 0x40); set_data(byte & 0x10);
  while(!IEC_ATN);
  set_clock(byte & 0x08); set_data(byte & 0x02);
  while(IEC_ATN);
  set_clock(byte & 0x04); set_data(byte & 0x01);
}

// Send a block: length + 2, marker (0: more, 1: last, 0xFF: error), data
static void sj_transmit_block(uint8_t marker, uint8_t length, const uint8_t* data) {
  uint8_t i;
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    set_clock(1); set_data(1);
    while(IEC_ATN);
    set_clock(0); set_data(0);
    sj_transmit_byte(length + 2);
    sj_transmit_byte(marker);
    for(i = 0; i < length; i++) sj_transmit_byte(data[i]);
    while(!IEC_ATN);
    set_clock(0); set_data(0);
  }
}

static void sj_send_error(void) {
  sj_transmit_block(0xFF, 0, 0);
}

// Directory: the files are named by two hex digits, sent as (name, track, sector)
static void sj_scan_directory(void) {
  dirhandle_t dh;
  dirent_t dent;
  int8_t res;
  uint8_t entry[3] = { 0xFF, 0, 0 };

  if(fs_opendir(&dh, &current_path)) { sj_send_error(); return; }
  while(1) {
    res = fs_readdir(&dh, &dent);
    if(res > 0) { sj_send_error(); return; }
    if(res < 0) { sj_transmit_block(1, 3, entry); return; }   // last entry
    if((dent.type & TYPE_MASK) != TYPE_PRG) continue;
    if(entry[1]) sj_transmit_block(0, 3, entry); // delayed: no dummy entry at the end
    entry[0] = hex2bin(dent.name);
    entry[1] = (entry[0] >> 4) + 1;
    entry[2] = entry[0] & 0x0F;
  }
}

static void sj_hexname(uint8_t* s, uint8_t name) {
  s[0] = pgm_read_byte(&hexchars[name >> 4]);
  s[1] = pgm_read_byte(&hexchars[name & 0x0F]);
}

static void sj_read_file(uint8_t name) {
  buffer_t* buf;
  sj_hexname(command_buffer, name);
  command_buffer[2] = 0;
  command_length = 2;
  file_open(0);
  buf = buffer_find(0);
  if(!buf) { sj_send_error(); return; }
  while(1) {
    sj_transmit_block(buf->sendeoi ? 1 : 0, buf->lastused - 1, buf->data + 2);
    if(buf->sendeoi) break;
    if(buf->refill(buf)) { sj_send_error(); return; }
  }
  buffer_free(buf);
}

// Returns 1 on abort (ATN)
static uint8_t sj_write_file(uint8_t name) {
  buffer_t* buf;
  int16_t res, length;
  uint8_t retval = 1;

  command_buffer[0] = '@';                      // replace
  command_buffer[1] = ':';
  sj_hexname(command_buffer + 2, name);
  command_buffer[4] = 0;
  command_length = 4;
  file_open(1);
  buf = buffer_find(1);
  if(!buf) { sj_send_error(); return 0; }

  sj_transmit_block(0, 0, 0);                   // success
  while(1) {
    if((length = n0s_getbyte(1)) < 0) break;
    if(!length) { retval = 0; break; }
    while(length-- > 0) {
      if((res = n0s_getbyte(1)) < 0) goto fail;
      fl_put(buf, res);
    }
  }
fail:
  buffer_free(buf);
  return retval;
}

static uint8_t sj_ts_to_name(uint8_t track, uint8_t sector) {
  if(!track || track > 16 || sector > 15) return 0xFF;
  return ((track - 1) << 4) | sector;
}

void load_samsjourney(uint8_t p) {
  int16_t tmp;
  uint8_t i, command, len, args[4] = { 0, 0, 0, 0 };

  DLY_ms(1);                                    // let the previous transaction end
  while(1) {
    if((tmp = n0s_getbyte(1)) < 0) break;       // command
    command = tmp;
    if((tmp = n0s_getbyte(1)) < 0) break;       // length of the arguments
    len = tmp;
    for(i = 0; i < len; i++) {
      if((tmp = n0s_getbyte(1)) < 0) goto abort;
      if(i < sizeof(args)) args[i] = tmp;
    }
    switch(command) {
      case 0x01: sj_scan_directory(); break;
      case 0x02: sj_read_file(args[0]); break;
      case 0x03: if(sj_write_file(args[0])) goto abort; break;
      case 0x82: sj_read_file(sj_ts_to_name(args[0], args[1])); break;
      case 0x83: if(sj_write_file(sj_ts_to_name(args[0], args[1]))) goto abort; break;
      default:   sj_send_error(); break;
    }
  }
abort:
  set_data(1);
  set_clock(1);
}
#endif
