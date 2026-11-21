// ===================================================================================
// IEC Bus (CBM Serial Bus) Device Functions                                  * v1.0 *
// ===================================================================================
//
// Bus protocol of the 1541 drive (the comments name the corresponding addresses in
// the 1541/1571 ROM), with JiffyDOS support and hooks for the fast loaders.
//
// Based on sd2iec by Ingo Korb (https://www.sd2iec.de), GPL v2 only.
// JiffyDOS protocol documentation by Jochen Adler (http://www.nlq.de).
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <string.h>
#include "system.h"
#include "iec.h"
#include "dos.h"
#include "led.h"
#include "fastloader.h"

iec_data_t iec_data;
uint8_t    device_address;
uint8_t    jiffy_enabled = FL_JIFFYDOS;
uint8_t    vc20mode;

// Bus states of the main loop
enum { BUS_IDLE = 0, BUS_ATNACTIVE, BUS_FOUNDATN, BUS_FORME, BUS_NOTFORME,
       BUS_ATNFINISH, BUS_ATNPROCESS, BUS_CLEANUP };
enum { DEVICE_IDLE = 0, DEVICE_LISTEN, DEVICE_TALK };

// ===================================================================================
// Low Level Bus Access
// ===================================================================================

// Read the bus with debouncing (E9C0)
static uint8_t iec_debounced(void) {
  uint8_t a;
  do {
    a = IEC_bus();
    DLY_us(2);
  } while(a != IEC_bus());
  return a;
}

// Check if ATN changed and change the state accordingly (EA59), returns 1 if so
uint8_t iec_check_atn(void) {
  if(iec_data.bus_state == BUS_ATNACTIVE) {
    if(IEC_ATN) { iec_data.bus_state = BUS_ATNPROCESS; return 1; }   // A9AC
  }
  else if(!IEC_ATN) { iec_data.bus_state = BUS_FOUNDATN; return 1; } // A7B3
  return 0;
}

// The ATN interrupt (acknowledge ATN by pulling DATA, like the ATNA logic of the
// 1541) is in fastloader_ll.S, as it also receives the DreamLoad commands.

// ===================================================================================
// Byte Transfer
// ===================================================================================

// Receive a byte (E9C9), returns -1 if the bus state changed
static int16_t iec_getc_raw(void) {
  uint8_t i, val = 0, tmp;

  do {                                          // E9CD: wait for CLOCK released
    if(iec_check_atn()) return -1;
  } while(!(iec_debounced() & IEC_BIT_CLOCK));

  set_data(1);                                  // E9D7: ready for data
  while(!IEC_DATA);                             // wait for other devices

  TMR_start(256);                               // EOI timeout
  do {
    if(iec_check_atn()) return -1;
    tmp = TMR_expired();
  } while((iec_debounced() & IEC_BIT_CLOCK) && !tmp);

  if(tmp) {                                     // EOI: acknowledge it
    set_data(0);
    DLY_us(73);
    set_data(1);
    do {
      if(iec_check_atn()) return -1;
    } while(iec_debounced() & IEC_BIT_CLOCK);
    iec_data.flags |= IEC_EOI_RECVD;
  }

  for(i = 0; i < 8; i++) {
    // JiffyDOS: a delay before the last bit of an ATN byte identifies the computer
    if(iec_data.bus_state == BUS_ATNACTIVE && i == 7 && jiffy_enabled) {
      TMR_start(218);
      do {
        tmp = IEC_bus();
        if(!(iec_data.flags & IEC_JIFFY_ACTIVE) && TMR_expired()) {
          if((val >> 1) < 0x60 && ((val >> 1) & 0x1F) == device_address) {
            set_data(0);                        // answer: we know JiffyDOS too
            DLY_us(101);
            set_data(1);
            iec_data.flags |= IEC_JIFFY_ACTIVE;
          }
        }
      } while(!(tmp & IEC_BIT_CLOCK));
    }
    else {
      do tmp = IEC_bus(); while(!(tmp & IEC_BIT_CLOCK));   // EA0B: data valid
    }
    val = (val >> 1) | ((tmp & IEC_BIT_DATA) ? 0x80 : 0);
    do {                                        // EA1A
      if(iec_check_atn()) return -1;
    } while(iec_debounced() & IEC_BIT_CLOCK);
  }

  DLY_us(5);
  set_data(0);                                  // EA28: frame handshake
  DLY_us(50);
  return val;
}

static int16_t iec_getc(void) {
  int16_t val;
  INT_disable();
  val = iec_getc_raw();
  INT_enable();
  return val;
}

// Send a byte (E916), returns non-zero if the bus state changed
static uint8_t iec_putc(uint8_t data, uint8_t with_eoi) {
  uint8_t i;

  if(iec_check_atn()) return 1;

#if FL_JIFFYDOS
  if(iec_data.flags & IEC_JIFFY_ACTIVE) {       // JiffyDOS byte
    if(jiffy_send(data, with_eoi, 0)) { iec_check_atn(); return 1; }
    return 0;
  }
#endif

  i = iec_debounced();
  DLY_us(60);
  set_clock(1);                                 // ready to send

  do {                                          // E925: wait for listener ready
    if(iec_check_atn()) return 1;
  } while(!(iec_debounced() & IEC_BIT_DATA));

  if(with_eoi || (i & IEC_BIT_DATA)) {          // EOI: wait for the acknowledge
    do {
      if(iec_check_atn()) return 1;
    } while(!(iec_debounced() & IEC_BIT_DATA));
    do {
      if(iec_check_atn()) return 1;
    } while(iec_debounced() & IEC_BIT_DATA);
  }

  set_clock(0);                                 // E94B
  DLY_us(40);
  do {
    if(iec_check_atn()) return 1;
  } while(!(iec_debounced() & IEC_BIT_DATA));
  DLY_us(21);

  for(i = 0; i < 8; i++) {
    if(!(iec_debounced() & IEC_BIT_DATA)) {     // E95C: listener gone
      iec_data.bus_state = BUS_CLEANUP;
      return 1;
    }
    DLY_us(45);
    set_data(data & 1);
    data >>= 1;
    DLY_us(22);
    set_clock(1);                               // bit valid
    if(vc20mode) DLY_us(34); else DLY_us(75);
    set_clock(0);
    DLY_us(22);
    set_data(1);
    DLY_us(14);
  }

  do {                                          // wait for the frame handshake
    if(iec_check_atn()) return 1;
  } while(iec_debounced() & IEC_BIT_DATA);

  // Wait up to 250us for DATA released or ATN: a real 1541 needs about 350us
  // between two bytes, some programs rely on this (Castle Wolfenstein)
  TMR_start(250);
  while(!IEC_DATA && IEC_ATN && !TMR_expired());
  return 0;
}

// ===================================================================================
// Listen and Talk
// ===================================================================================

// Handle LISTEN (EA2E): receive data or a command / file name
static uint8_t iec_listen_handler(uint8_t cmd) {
  buffer_t* buf = buffer_find(cmd & 0x0F);
  int16_t c;

  // only the command channel, open channels for writing, or OPEN
  if((cmd & 0x0F) != 0x0F && (cmd & 0xF0) != 0xF0 && (!buf || !buf->write)) {
    iec_data.bus_state = BUS_CLEANUP;
    return 1;
  }

  while(1) {
#if FL_JIFFYDOS
    if(iec_data.flags & IEC_JIFFY_ACTIVE) {
      uint8_t flags;
      set_atn_irq(1);
      c = jiffy_receive(&flags);
      if(!(flags & IEC_BIT_ATN)) c = iec_getc();  // ATN active at the end
      else if(flags & IEC_BIT_CLOCK) iec_data.flags |= IEC_EOI_RECVD;
      else iec_data.flags &= ~IEC_EOI_RECVD;
    }
    else
#endif
    c = iec_getc();
    if(c < 0) return 1;

    if((cmd & 0x0F) == 0x0F || (cmd & 0xF0) == 0xF0) {
      // command or file name
      if(command_length < COMMAND_LENGTH) command_buffer[command_length++] = c;
      if(iec_data.flags & IEC_EOI_RECVD) iec_data.flags |= IEC_COMMAND_RECVD;
    }
    else {
      // data
      if(buf->mustflush) {
        if(buf->refill(buf)) return 1;
        buf = buffer_find(cmd & 0x0F);
        if(!buf) return 1;
      }
      buf->data[buf->position] = c;
      buf->dirty = 1;
      if(buf->lastused < buf->position) buf->lastused = buf->position;
      if(!++buf->position) buf->mustflush = 1;
    }
  }
}

// Handle TALK (E909): send data
static uint8_t iec_talk_handler(uint8_t cmd) {
  buffer_t* buf = buffer_find(cmd & 0x0F);
  if(!buf) return 0;

#if FL_JIFFYDOS
  if(iec_data.flags & IEC_JIFFY_ACTIVE) DLY_us(360);    // C64 must reach fbb7/fb0c

  if(iec_data.flags & IEC_JIFFY_LOAD) {
    do {                                        // wait for DATA released
      if(iec_check_atn()) return 1;
    } while(!IEC_DATA);
    buf->position = 4;                          // 2 bytes already read by the C64
    set_data(0);                                // ready for the first block
    set_clock(1);
    DLY_us(50);
  }
#endif

  while(buf->read) {
    do {
      uint8_t last = (buf->position == buf->lastused);
#if FL_JIFFYDOS
      if(iec_data.flags & IEC_JIFFY_LOAD) {
        // the last byte of the buffer is sent with CLOCK low: next block takes time
        if(jiffy_send(buf->data[buf->position], 0, 128 | !last)) { iec_check_atn(); return 1; }
        if(last && buf->sendeoi) {              // EOI marker
          DLY_us(100); set_clock(1); DLY_us(100); set_clock(0); DLY_us(100); set_clock(1);
        }
      }
      else
#endif
      if(last && buf->sendeoi) {
        if(iec_putc(buf->data[buf->position], 1)) return 1;
        if(iec_data.flags & IEC_JIFFY_ACTIVE) { // JiffyDOS resets EOI after 30-40us
          DLY_us(50);
          set_data(1);
          set_clock(0);
        }
      }
      else if(iec_putc(buf->data[buf->position], 0)) return 1;
    } while(buf->position++ < buf->lastused);

    if(buf->sendeoi && (cmd & 0x0F) != 0x0F && buf->refill != directbuffer_refill) {
      buf->read = 0;
      break;
    }
    if(buf->refill(buf)) { iec_data.bus_state = BUS_CLEANUP; return 1; }
    buf = buffer_find(cmd & 0x0F);
    if(!buf) return 1;

#if FL_JIFFYDOS
    if(iec_data.flags & IEC_JIFFY_LOAD) {
      TMR_start(120);                           // wait until the C64 is at FB06
      while(!IEC_DATA && !TMR_expired());
      if(iec_check_atn()) return 1;
      set_data(0);                              // ready for the next block
      set_clock(1);
      DLY_us(50);
    }
#endif
  }
  return 0;
}

// ===================================================================================
// Init and Main Loop
// ===================================================================================

void iec_init(void) {
  IEC_PORT_REG &= ~(IEC_BIT_ATN | IEC_BIT_DATA | IEC_BIT_CLOCK);   // no pull-ups
  IEC_DDR_REG  &= ~(IEC_BIT_ATN | IEC_BIT_DATA | IEC_BIT_CLOCK);   // released
  if(!IEC_ATN) set_data(0);                     // request already on the bus
  PCICR |= IEC_PCIE;
  PCIFR |= IEC_PCIE;
}

void iec_mainloop(void) {
  int16_t cmd = 0;

  set_error(ERROR_DOSVERSION);
  iec_data.bus_state = BUS_IDLE;

  while(1) {
    switch(iec_data.bus_state) {
      case BUS_IDLE:                            // EBFF: wait for ATN
        set_atn_irq(1);
        while(IEC_ATN) main_idle();
        iec_data.bus_state = BUS_FOUNDATN;
        break;

      case BUS_FOUNDATN:                        // E85B: we are here
        set_clock(1);
        set_data(0);
        set_atn_irq(0);
        iec_data.device_state = DEVICE_IDLE;
        iec_data.bus_state = BUS_ATNACTIVE;
        iec_data.flags &= ~(IEC_EOI_RECVD | IEC_JIFFY_ACTIVE | IEC_JIFFY_LOAD);

        // wait until CLOCK is pulled (or 250us): the C64 does not always pull
        // CLOCK before ATN
        TMR_start(250);
        while(IEC_CLOCK && !TMR_expired()) if(IEC_ATN) iec_data.bus_state = BUS_ATNPROCESS;
        while(!IEC_CLOCK) if(IEC_ATN) iec_data.bus_state = BUS_ATNPROCESS;
        break;

      case BUS_ATNACTIVE:                       // E884: command byte under ATN
        cmd = iec_getc();
        if(cmd < 0) break;                      // state changed
        if(cmd == 0x3F) {                       // UNLISTEN
          if(iec_data.device_state == DEVICE_LISTEN) iec_data.device_state = DEVICE_IDLE;
          iec_data.bus_state = BUS_ATNFINISH;
        }
        else if(cmd == 0x5F) {                  // UNTALK
          if(iec_data.device_state == DEVICE_TALK) iec_data.device_state = DEVICE_IDLE;
          iec_data.bus_state = BUS_ATNFINISH;
        }
        else if(cmd == 0x40 + device_address) { // TALK
          iec_data.device_state = DEVICE_TALK;
          iec_data.bus_state = BUS_FORME;
        }
        else if(cmd == 0x20 + device_address) { // LISTEN
          iec_data.device_state = DEVICE_LISTEN;
          iec_data.bus_state = BUS_FORME;
        }
        else if((cmd & 0x60) == 0x60) {         // OPEN, CLOSE, DATA
          // JiffyDOS LOAD uses 0x61 instead of 0x60 when talking
          if(cmd == 0x61 && iec_data.device_state == DEVICE_TALK && (iec_data.flags & IEC_JIFFY_ACTIVE)) {
            cmd = 0x60;
            iec_data.flags |= IEC_JIFFY_LOAD;
          }
          iec_data.secondary = cmd & 0x0F;
          if((cmd & 0xF0) == 0xE0) {            // CLOSE (handled here like the 1571)
            if(cmd == 0xEF) {
              if(buffers_free(1)) { iec_data.bus_state = BUS_CLEANUP; break; }
            }
            else {
              buffer_t* buf = buffer_find(iec_data.secondary);
              if(buf) {
                uint8_t err = buf->cleanup(buf);
                buf->cleanup = buffer_dummy;
                buffer_free(buf);
                if(err) { iec_data.bus_state = BUS_CLEANUP; break; }
              }
            }
            iec_data.bus_state = BUS_FORME;
          }
          else iec_data.bus_state = BUS_ATNFINISH;
        }
        else iec_data.bus_state = BUS_NOTFORME;
        break;

      case BUS_FORME:                           // E8D2
        iec_data.bus_state = IEC_ATN ? BUS_ATNPROCESS : BUS_ATNACTIVE;
        break;

      case BUS_NOTFORME:                        // E8FD: not for us, release the bus
        set_atn_irq(0);
        set_clock(1);
        set_data(1);
        iec_data.bus_state = BUS_ATNFINISH;
        break;

      case BUS_ATNFINISH:                       // E902: wait until ATN is released
        while(!IEC_ATN);
        iec_data.bus_state = BUS_ATNPROCESS;
        break;

      case BUS_ATNPROCESS:                      // E8D7
        set_atn_irq(1);
        if(iec_data.device_state == DEVICE_LISTEN) {
          if(iec_listen_handler(cmd)) break;
        }
        else if(iec_data.device_state == DEVICE_TALK) {
          set_data(1);                          // turnaround: we become talker
          DLY_us(50);
          set_clock(0);
          DLY_us(70);
          if(iec_talk_handler(cmd)) break;
        }
        iec_data.bus_state = BUS_CLEANUP;
        break;

      case BUS_CLEANUP:
        // release the bus (DATA stays pulled while ATN is active, like the
        // ATN acknowledge logic of a real 1541)
        INT_disable();
        set_atn_irq(1);
        set_clock(1);
        if(IEC_ATN) set_data(1); else set_data(0);
        INT_enable();

        // execute commands and file names
        if(iec_data.flags & IEC_COMMAND_RECVD) {
          if(disk_changed) fs_init();           // card changed meanwhile
          disk_changed = 0;
          if(iec_data.secondary == 0x0F) parse_doscommand();
          else {
            fl_datacrc_reset();
            file_open(iec_data.secondary);
          }
          command_length = 0;
          iec_data.flags &= ~IEC_COMMAND_RECVD;
        }
        buffers_free(0);                        // free buffers that are not needed
        iec_data.bus_state = BUS_IDLE;
        break;
    }
  }
}
