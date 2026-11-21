// ===================================================================================
// Fast Loaders: Sector Based Loaders                                         * v1.0 *
// ===================================================================================
//
// Loaders that read (and write) single sectors of the mounted disk image, like a job
// loop in the drive: DreamLoad, ULoad Model 3, Nippon and Maniac Mansion / Zak
// McKracken. They only work inside D64/D71/D81 images.
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
//   DreamLoad: Thomas Giesel, Nippon: Ingo Korb and contributors.
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <util/atomic.h>
#include "system.h"
#include "fastloader.h"
#include "iec.h"
#include "led.h"

// ===================================================================================
// DreamLoad
// ===================================================================================
// The commands (track/sector) are received by the bus interrupt (fastloader_ll.S),
// on CLOCK low (new protocol) or ATN low (old protocol). Track 0 is a command:
// sector 0 ends the loader, sector 1 reads the first directory sector.
#if FL_DREAMLOAD
static void dreamload_send_block(const uint8_t* p) {
  uint8_t checksum = 0;
  uint16_t n;
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    for(n = 0; n < 256; n++) checksum ^= p[n];  // checksum is EOR of all bytes
    dreamload_send_byte(0);                     // status
    for(n = 0; n < 256; n++) dreamload_send_byte(*p++);
    dreamload_send_byte(checksum);
    set_clock(1);
    set_data(1);
  }
}

void load_dreamload(uint8_t p) {
  uint16_t n;
  uint8_t type;
  buffer_t* buf;
  uint8_t dt, ds;

  // Receive the final drive code without interrupts, so no job codes are read
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    set_clock_irq(0);
    set_atn_irq(0);
    set_clock(1);
    set_data(1);
    while(!IEC_CLOCK);                          // wait until the C64 released CLOCK

    type = 0;                                   // 4 blocks of drive code
    for(n = 4 * 256; n; n--) type ^= dreamload_get_byte();

    if(type == 0xAC || type == 0xDC) {          // old protocol: commands on ATN
      set_atn_irq(1);
      detected_loader = LD_DREAMLOAD_OLD;
    }
    else set_clock_irq(1);                      // new protocol: commands on CLOCK

    fl_track = 0xFF;                            // no job waiting
  }

  buf = buffer_alloc();
  if(!buf) goto error;
  image_dir_start(&dt, &ds);                    // start of the directory

  while(1) {
    while(fl_track == 0xFF) {
      if(fl_check_keys()) { fl_track = 0; fl_sector = 0; break; }
    }
    LED_busy(1);

    if(!fl_track) {
      if(!fl_sector) { LED_busy(0); break; }    // end of the loader
      if(fl_sector == 1) {                      // first directory sector
        DLY_ms(1000);                           // slow, gives time to swap disks
        fl_read_sector(buf, dt, ds);
        dreamload_send_block(buf->data);
      }
      else LED_busy(0);
    }
    else {
      fl_read_sector(buf, fl_track, fl_sector);
      dreamload_send_block(buf->data);
    }
    fl_track = 0xFF;
  }

error:
  if(buf) buffer_free(buf);
  set_clock_irq(0);
  set_atn_irq(0);
}
#endif

// ===================================================================================
// ULoad Model 3
// ===================================================================================
#if FL_ULOAD3
// Transfer a sector chain, returns 1 if aborted by ATN
static uint8_t uload3_transferchain(uint8_t track, uint8_t sector, uint8_t saving) {
  buffer_t* buf = buffer_alloc();
  uint8_t i, bytecount, first = 1;
  int16_t tmp;

  if(!buf) { uload3_send_byte(0xFF); return 0; }
  do {
    if(fl_read_sector(buf, track, sector)) goto error;

    bytecount = buf->data[0] ? 254 : buf->data[1] - 1;  // bytes in this sector
    uload3_send_byte(bytecount);

    if(saving) {
      i = 0;
      if(first) {                               // send the load address
        first = 0;
        uload3_send_byte(buf->data[2]);
        uload3_send_byte(buf->data[3]);
        i = 2;
      }
      for(; i < bytecount; i++) {               // receive the new contents
        if((tmp = uload3_get_byte()) < 0) { buffer_free(buf); return 1; }
        buf->data[i + 2] = tmp;
      }
      if(fl_write_sector(buf, track, sector)) goto error;
    }
    else for(i = 0; i < bytecount; i++) uload3_send_byte(buf->data[i + 2]);

    track  = buf->data[0];
    sector = buf->data[1];
  } while(track);

  uload3_send_byte(0);                          // end marker
  buffer_free(buf);
  return 0;

error:
  uload3_send_byte(0xFF);
  buffer_free(buf);
  return 0;
}

void load_uload3(uint8_t p) {
  int16_t cmd, t, s;
  uint8_t dt, ds;

  image_dir_start(&dt, &ds);                    // start of the directory
  while((cmd = uload3_get_byte()) >= 0) {       // -1: ATN
    switch(cmd) {
      case 1:                                   // load a file
      case 2:                                   // save (replace) a file
        if((t = uload3_get_byte()) < 0) return;
        if((s = uload3_get_byte()) < 0) return;
        if(uload3_transferchain(t, s, cmd == 2)) return;
        break;
      case '$':                                 // read the directory
        uload3_transferchain(dt, ds, 0);
        break;
      default:                                  // unknown command
        uload3_send_byte(0xFF);
        break;
    }
  }
}
#endif

// ===================================================================================
// Nippon Loader
// ===================================================================================
#if FL_NIPPON
// Sync: wait for CLOCK low; if ATN is low instead, the loader resyncs (returns 0)
static uint8_t nippon_atn_clock_handshake(void) {
  if(IEC_ATN) {
    while(IEC_CLOCK);
    return 1;
  }
  set_clock(0);
  while(IEC_ATN);
  return 0;
}

// Receive a byte, returns 0 if out of sync
static uint8_t nippon_read_byte(uint8_t* b) {
  uint8_t i, tmp = 0;
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    set_clock(1);
    set_data(1);
    DLY_us(3);                                  // slow rise times
    for(i = 8; i; i--) {
      if(!nippon_atn_clock_handshake()) return 0;
      tmp = (tmp >> 1) | (IEC_DATA ? 0 : 0x80);
      while(!IEC_CLOCK);
    }
    set_clock(0);
    set_data(1);
  }
  *b = tmp;
  return 1;
}

// Send a byte, returns 0 if out of sync
static uint8_t nippon_send_byte(uint8_t b) {
  uint8_t i;
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    set_clock(1);
    DLY_us(3);
    for(i = 8; i; i--) {
      if(!nippon_atn_clock_handshake()) return 0;
      set_data(b & 1);
      b >>= 1;
      while(!IEC_CLOCK);
    }
    set_clock(0);
    set_data(1);
  }
  return 1;
}

void load_nippon(uint8_t p) {
  uint8_t t, s, i = 0, j = 1;
  buffer_t* buf;

  set_atn_irq(0);
  buf = buffer_alloc();
  if(!buf) return;

  while(1) {                                    // until a track >= $80 (exit code)
    set_data(1);                                // release quickly (C64 waits at $0BF0)
    set_clock(1);
    LED_busy(0);
    while(IEC_ATN && !(i = fl_check_keys()));   // wait for a command
    if(i) break;
    set_clock(0);
    LED_busy(1);

    while(!IEC_ATN);
    if(!nippon_read_byte(&t)) continue;         // track (>= $80: exit)
    if(t & 0x80) break;
    if(!nippon_read_byte(&s)) continue;         // sector (bit 7: read, else write)
    if(s & 0x80) {
      fl_read_sector(buf, t, s & 0x7F);
      i = 0;
      do { if(!nippon_send_byte(buf->data[i])) break; } while(++i);
    }
    else {
      i = 0;
      do { if(!(j = nippon_read_byte(&buf->data[i]))) break; } while(++i);
      if(!j) break;
      fl_write_sector(buf, t, s);
    }
  }
  buffer_free(buf);
}
#endif

// ===================================================================================
// Maniac Mansion / Zak McKracken
// ===================================================================================
#if FL_MMZAK
// Send a byte (MSB first, 2 bits per CLOCK cycle), returns 1 on abort
static uint8_t mmzak_send_byte(uint8_t byte) {
  uint8_t i;
  for(i = 0; i < 4; i++) {
    while(!IEC_CLOCK) if(fl_check_keys()) return 1;
    set_data(byte & 0x80);
    byte <<= 1;
    while(IEC_CLOCK) if(fl_check_keys()) return 1;
    set_data(byte & 0x80);
    byte <<= 1;
  }
  return 0;
}

// Receive a byte, returns -1 on abort
static int16_t mmzak_read_byte(void) {
  uint8_t i, value = 0;
  for(i = 0; i < 4; i++) {
    while(IEC_CLOCK) if(fl_check_keys()) return -1;
    value <<= 1;
    DLY_us(3);
    if(!IEC_DATA) value |= 1;
    while(!IEC_CLOCK) if(fl_check_keys()) return -1;
    value <<= 1;
    DLY_us(3);
    if(!IEC_DATA) value |= 1;
  }
  return value;
}

static uint8_t mmzak_send_error(void) {
  set_clock(1);
  set_data(1);
  return mmzak_send_byte(0x01) || mmzak_send_byte(0x11);
}

// Read a sector and send it ($01 is escaped), returns 1 on abort
static uint8_t mmzak_read_sector(uint8_t track, uint8_t sector, buffer_t* buf) {
  uint8_t* ptr = buf->data;
  uint16_t i;
  if(fl_read_sector(buf, track, sector)) return mmzak_send_error();
  set_clock(1);
  set_data(1);
  DLY_us(3);
  for(i = 0; i < 256; i++) {
    if(*ptr == 0x01 && mmzak_send_byte(0x01)) return 1;
    if(mmzak_send_byte(*ptr++)) return 1;
  }
  if(mmzak_send_byte(0x01) || mmzak_send_byte(0x81)) return 1;   // status: ok
  set_clock(0);
  set_data(1);
  return 0;
}

// Receive a sector and write it, returns 1 on abort
static uint8_t mmzak_write_sector(uint8_t track, uint8_t sector, buffer_t* buf) {
  uint8_t* ptr = buf->data;
  uint16_t i;
  int16_t v;
  set_clock(1);
  set_data(1);
  DLY_us(3);
  for(i = 0; i < 256; i++) {
    if((v = mmzak_read_byte()) < 0) return 1;
    *ptr++ = v;
  }
  set_clock(0);
  if(fl_write_sector(buf, track, sector)) return mmzak_send_error();
  return 0;
}

void load_mmzak(uint8_t p) {
  buffer_t* buf = buffer_alloc();
  uint8_t i, track, sector, command, done = 0;
  int16_t v;

  if(!buf) return;
  set_atn_irq(0);

  for(i = 0; i < 8; i++) {                      // initial handshake
    set_clock(1);
    set_data(0);
    DLY_us(1285);
    set_data(1);
    DLY_us(1290);
  }
  while((IEC_bus() & (IEC_BIT_CLOCK | IEC_BIT_DATA)) != (IEC_BIT_CLOCK | IEC_BIT_DATA))
    if(fl_check_keys()) goto exit;

  while(!done) {
    set_clock(0);                               // ready
    set_data(1);
    LED_busy(0);
    DLY_us(3);
    while(IEC_DATA) if(fl_check_keys()) goto exit;
    set_clock(1);

    if((v = mmzak_read_byte()) < 0) break;      // command block: track, sector, cmd
    track = v;
    if((v = mmzak_read_byte()) < 0) break;
    sector = v;
    if((v = mmzak_read_byte()) < 0) break;
    command = v;
    LED_busy(1);
    set_clock(0);

    switch(command) {
      case 0x20: done = 1; break;               // exit
      case 0x30: done = mmzak_read_sector(track, sector, buf); break;
      case 0x40: done = mmzak_write_sector(track, sector, buf); break;
      default:   done = mmzak_send_error(); break;
    }
  }
exit:
  buffer_free(buf);
  LED_busy(0);
}
#endif
