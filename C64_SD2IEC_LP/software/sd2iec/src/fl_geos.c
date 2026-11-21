// ===================================================================================
// Fast Loaders: GEOS and Wheels                                              * v1.0 *
// ===================================================================================
//
// GEOS and Wheels replace the drive DOS with their own disk drivers. The SD2IEC
// emulates the job interface of these drivers (read/write sector, status, ...) on
// the mounted disk image. The byte transfer routines differ between the variants;
// fastloader.c selects them by the CRC of the uploaded driver (fast_get_byte and
// fast_send_byte).
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <util/atomic.h>
#include <avr/pgmspace.h>
#include "system.h"
#include "fastloader.h"
#include "iec.h"
#include "led.h"

#if FL_GEOS

// ===================================================================================
// GEOS Transfer Functions
// ===================================================================================

// Receive a fixed-length data block (backwards)
static void geos_receive_datablock(uint8_t* data, uint16_t length) {
  data += length - 1;
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    while(!IEC_CLOCK);
    set_data(1);
    while(length--) *data-- = fast_get_byte();
    set_data(0);
  }
}

// Receive a data block with a length byte (0 = 256)
static void geos_receive_lenblock(uint8_t* data) {
  uint16_t length;
  while(!IEC_CLOCK && IEC_ATN) fl_check_keys();
  if(!IEC_ATN) { data[0] = 0; data[1] = 0; return; }   // ATN: internal QUIT
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    set_data(1);
    length = fast_get_byte();
    set_data(0);
  }
  if(!length) length = 256;
  geos_receive_datablock(data, length);
}

// Send a single byte after waiting for CLOCK high
static void geos_transmit_byte_wait(uint8_t byte) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    while(!IEC_CLOCK);
    set_data(1);
    fast_send_byte(byte);
    set_clock(1);
    set_data(0);
  }
  DLY_us(25);
}

// Send a data block (backwards)
static void geos_transmit_buffer_s3(const uint8_t* data, uint16_t len) {
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    while(!IEC_CLOCK);
    set_data(1);
    data += len - 1;
    while(len--) fast_send_byte(*data--);
    set_clock(1);
    set_data(0);
    DLY_us(15);
  }
}

// Stage 2: send a length byte first
static void geos_transmit_buffer_s2(const uint8_t* data, uint16_t len) {
  geos_transmit_byte_wait(len);
  geos_transmit_buffer_s3(data, len);
}

// Job result code: 1 = ok, 8 = write protected, 2 = some error
static uint8_t geos_job_status(void) {
  if(current_error == ERROR_OK) return 1;
  if(current_error == ERROR_WRITE_PROTECT) return 8;
  return 2;
}

static void geos_transmit_status(void) {
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    geos_transmit_byte_wait(1);                 // length
    geos_transmit_byte_wait(geos_job_status());
  }
}

// ===================================================================================
// GEOS Stage 2/3 Disk Driver
// ===================================================================================
void load_geos(uint8_t p) {
  buffer_t* cmdbuf  = buffer_alloc();
  buffer_t* databuf = buffer_alloc();
  uint8_t* cmd;
  uint16_t job;

  if(!cmdbuf || !databuf) goto exit;
  cmd = cmdbuf->data;

  DLY_ms(1);                                    // initial handshake
  set_data(0);
  while(IEC_CLOCK);

  while(1) {
    LED_busy(0);
    geos_receive_lenblock(cmd);                 // command block: address, track, sector
    LED_busy(1);
    job = cmd[0] | (cmd[1] << 8);               // address of the job in the driver
    switch(job) {
      case 0x0320:                              // 1541 stage 3: transmit
        geos_transmit_buffer_s3(databuf->data, 256);
        geos_transmit_status();
        break;
      case 0x031F:                              // 1571 / 1541 stage 2 status, 1581 transmit
        if(detected_loader == LD_GEOS_S23_1581)
          geos_transmit_buffer_s3(databuf->data, (cmd[2] & 0x80) ? 2 : 256);
        geos_transmit_status();
        break;
      case 0x0325:                              // 1541 stage 3 status
      case 0x032B:                              // 1581 status
        geos_transmit_status();
        break;
      case 0x0000:                              // internal QUIT (ATN)
      case 0x0412:                              // 1541 stage 2 quit
      case 0x0420:                              // 1541 stage 3 quit
      case 0x0457:                              // 1581 quit
      case 0x0475:                              // 1571 stage 3 quit
        while(!IEC_CLOCK);
        set_data(1);
        goto exit;
      case 0x0432:                              // 1541 stage 2 transmit
        if(current_error) geos_transmit_status();
        else geos_transmit_buffer_s2(databuf->data, 256);
        break;
      case 0x0439:                              // 1541 stage 3 set address
      case 0x04A5:                              // 1571 stage 3 set address
        device_address = cmd[2] & 0x1F;
        break;
      case 0x049B:                              // 1581 initialize
      case 0x04B9:                              // 1581 flush
      case 0x04DC:                              // 1541 stage 3 initialize
      case 0x0504:                              // 1541 stage 2 initialize (GEOS 1.3)
      case 0x057E:                              // 1571 initialize
        break;
      case 0x057C:                              // 1541 stage 2/3 write
        LED_dirty(1);
        geos_receive_lenblock(databuf->data);
        fl_write_sector(databuf, cmd[2], cmd[3]);
        break;
      case 0x058E:                              // 1541 stage 2/3 read
      case 0x04CC:                              // 1581 read
        fl_read_sector(databuf, cmd[2] & 0x7F, cmd[3]);
        break;
      case 0x04AF:                              // 1571 read and send
        fl_read_sector(databuf, cmd[2], cmd[3]);
        geos_transmit_buffer_s3(databuf->data, 256);
        geos_transmit_status();
        break;
      case 0x047C:                              // 1581 write
      case 0x05FE:                              // 1571 write
        LED_dirty(1);
        geos_receive_datablock(databuf->data, 256);
        fl_write_sector(databuf, cmd[2], cmd[3]);
        geos_transmit_status();
        break;
      default:                                  // unknown job
        goto exit;
    }
  }
exit:
  if(cmdbuf)  buffer_free(cmdbuf);
  if(databuf) buffer_free(databuf);
}

// ===================================================================================
// GEOS Stage 1 Loader
// ===================================================================================
// Sends the kernal files as sector chains; all but the first are encrypted with a
// key that is part of the uploaded loader code (captured during M-W).
static void geos_send_chain(uint8_t track, uint8_t sector, buffer_t* buf, const uint8_t* key) {
  uint8_t i, bytes;
  do {
    fl_read_sector(buf, track, sector);
    if(key) for(i = 0; i < 254; i++) buf->data[2 + i] ^= key[i];
    track  = buf->data[0];
    sector = buf->data[1];
    bytes  = track ? 254 : sector - 1;
    geos_transmit_buffer_s2(buf->data + 2, bytes);
  } while(track);
  geos_transmit_byte_wait(0);
}

static const uint8_t geos64_chains[]  PROGMEM = { 19, 13,  20, 15,  20, 17,  0 };
static const uint8_t geos128_chains[] PROGMEM = { 19, 12,  20, 15,  23, 6,  24, 4,  0 };

void load_geos_s1(uint8_t version) {
  buffer_t* keybuf  = buffer_find(BUFFER_SYS_CAPTURE);
  buffer_t* databuf = buffer_alloc();
  const uint8_t* chain = version ? geos128_chains : geos64_chains;
  const uint8_t* key = 0;
  uint8_t track;

  if(keybuf && databuf) {
    DLY_ms(1);                                  // initial handshake
    set_data(0);
    while(IEC_CLOCK);
    while((track = pgm_read_byte(chain++))) {
      geos_send_chain(track, pgm_read_byte(chain++), databuf, key);
      key = keybuf->data;                       // decrypt after the first chain
    }
    set_data(1);
  }
  if(keybuf)  buffer_free(keybuf);
  if(databuf) buffer_free(databuf);
}

// ===================================================================================
// Wheels
// ===================================================================================
#if FL_WHEELS

// Wheels 4.4 (1581): send a data block (backwards)
static void wheels44_transmit_buffer(const uint8_t* data, uint16_t len) {
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    while(!IEC_CLOCK);
    set_data(1);
    data += len - 1;
    while(len--) fast_send_byte(*data--);
    set_clock(1);
    set_data(1);
    DLY_us(5);
    while(IEC_CLOCK);
    set_data(0);
    DLY_us(15);
  }
}

// Send a single byte after waiting for CLOCK high
static void wheels_transmit_byte_wait(uint8_t byte) {
  if(detected_loader == LD_WHEELS44_S2_1581) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      while(!IEC_CLOCK);
      set_data(1);
      fast_send_byte(byte);
      set_clock(1);
      set_data(1);
      DLY_us(5);
      while(IEC_CLOCK);
      set_data(0);
    }
    DLY_us(15);
  }
  else {
    geos_transmit_byte_wait(byte);
    DLY_us(15);
    while(IEC_CLOCK);
  }
}

static void wheels_transmit_datablock(const void* data, uint16_t length) {
  if(detected_loader == LD_WHEELS44_S2_1581) wheels44_transmit_buffer(data, length);
  else {
    geos_transmit_buffer_s3(data, length);
    while(IEC_CLOCK);
  }
}

static void wheels_receive_datablock(void* dst, uint16_t length) {
  uint8_t* data = (uint8_t*)dst + length - 1;
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    while(!IEC_CLOCK);
    set_data(1);
    while(length--) *data-- = fast_get_byte();
    if(detected_loader == LD_WHEELS44_S2 || detected_loader == LD_WHEELS44_S2_1581)
      while(IEC_CLOCK);
    set_data(0);
  }
}

static void wheels_transmit_status(void) {
  ATOMIC_BLOCK(ATOMIC_FORCEON) {
    wheels_transmit_byte_wait(geos_job_status());
  }
}

// Stage 1: send the file SYSTEM1 / 128SYSTEM1 in whole blocks
static const char wheels_system1[]    PROGMEM = "SYSTEM1";
static const char wheels_system1_128[] PROGMEM = "128SYSTEM1";

void load_wheels_s1(uint8_t version) {
  buffer_t* buf;
  DLY_ms(2);
  while(IEC_CLOCK);
  set_data(0);
  buf = fl_open(0, version ? wheels_system1_128 : wheels_system1);
  if(buf) {
    while(1) {
      wheels_transmit_datablock(buf->data, 256);
      if(buf->sendeoi || buf->refill(buf)) break;
    }
  }
  while(!IEC_CLOCK);
  set_data(1);
  set_clock(1);
  if(buf) buffer_free(buf);
}

// Stage 2: disk driver
void load_wheels_s2(uint8_t p) {
  struct {
    uint16_t address;
    uint8_t  track;
    uint8_t  sector;
  } cmd;
  uint8_t data[3];
  uint16_t freeblocks;
  buffer_t* buf = buffer_alloc();

  if(!buf) return;
  DLY_ms(1);                                    // initial handshake
  while(IEC_CLOCK);
  set_data(0);
  set_clock(1);
  DLY_us(3);

  while(1) {
    while(!IEC_CLOCK && IEC_ATN) fl_check_keys();
    wheels_receive_datablock(&cmd, 4);
    LED_busy(1);
    switch(cmd.address & 0xFF) {
      case 0x03:                                // QUIT
        while(!IEC_CLOCK);
        set_data(1);
        goto exit;
      case 0x06:                                // WRITE
        LED_dirty(1);
        wheels_receive_datablock(buf->data, 256);
        fl_write_sector(buf, cmd.track, cmd.sector);
        wheels_transmit_status();
        break;
      case 0x09:                                // READ
      case 0x0C:                                // READLINK (2 bytes)
        fl_read_sector(buf, cmd.track, cmd.sector);
        wheels_transmit_datablock(buf->data, (cmd.address & 0xFF) == 0x09 ? 256 : 2);
        wheels_transmit_status();
        break;
      case 0x0F:                                // STATUS
        wheels_transmit_status();
        break;
      case 0x12:                                // NATIVE_FREE (limits are ignored)
        freeblocks = fs_freeblocks();
        wheels_transmit_datablock(&freeblocks, 2);
        wheels_transmit_status();
        break;
      case 0x15:                                // GET_CURRENT_PART_DIR
        image_dir_start(&data[0], &data[1]);
        data[2] = 1;                            // partition 1
        wheels_transmit_datablock(data, 3);
        break;
      case 0x18:                                // SET_CURRENT_PART_DIR (no subdirs)
        wheels_receive_datablock(data, 3);
        break;
      case 0x1B:                                // CHECK_CHANGE
        ATOMIC_BLOCK(ATOMIC_FORCEON) {
          if(fl_dirchanged) wheels_transmit_byte_wait(3);
          else if(detected_loader == LD_WHEELS44_S2 && image_drive_type() == 71)
            wheels_transmit_byte_wait(0x80);
          else wheels_transmit_byte_wait(0);
          fl_dirchanged = 0;
          while(IEC_CLOCK);
        }
        break;
      default:                                  // unknown job
        goto exit;
    }
    LED_busy(0);
    while(IEC_CLOCK && IEC_ATN) fl_check_keys();
  }
exit:
  buffer_free(buf);
  LED_busy(0);
}
#endif // FL_WHEELS

#endif // FL_GEOS
