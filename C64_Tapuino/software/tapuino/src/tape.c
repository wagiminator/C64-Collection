// ===================================================================================
// Datasette Emulation: TAP Playback and Recording                            * v1.0 *
// ===================================================================================
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <string.h>
#include "tape.h"
#include "gpio.h"
#include "../config.h"

#if PIN_TAPE_WRITE != PB0
  #error PIN_TAPE_WRITE must be PB0 (input capture pin ICP1 of timer 1)
#endif

// ===================================================================================
// Machine Clocks and Conversion Factors
// ===================================================================================
// CPU clock of the machines in Hz: [machine][video]
#define CLK_C64_PAL     985248UL
#define CLK_C64_NTSC    1022272UL
#define CLK_VIC_PAL     1108404UL
#define CLK_VIC_NTSC    1022727UL
#define CLK_C16_PAL     886724UL
#define CLK_C16_NTSC    894886UL

// Playback: half-wave in timer ticks (0.5us) = cycles * 1e6 / clock
//           = cycles + cycles * frac / 65536
#define PLAY_FRAC(c)    ((int16_t)((1000000ULL * 65536 + (c) / 2) / (c) - 65536))
static const int16_t TAPE_playFrac[3][2] = {
  { PLAY_FRAC(CLK_C64_PAL), PLAY_FRAC(CLK_C64_NTSC) },
  { PLAY_FRAC(CLK_VIC_PAL), PLAY_FRAC(CLK_VIC_NTSC) },
  { PLAY_FRAC(CLK_C16_PAL), PLAY_FRAC(CLK_C16_NTSC) }
};

// Recording: cycles = timer ticks (1/16us) * clock / 16e6 = ticks * k / 65536
#define REC_K(c)        ((uint16_t)(((c) * 65536ULL + 8000000) / 16000000))
static const uint16_t TAPE_recK[3][2] = {
  { REC_K(CLK_C64_PAL), REC_K(CLK_C64_NTSC) },
  { REC_K(CLK_VIC_PAL), REC_K(CLK_VIC_NTSC) },
  { REC_K(CLK_C16_PAL), REC_K(CLK_C16_NTSC) }
};

// ===================================================================================
// Common Variables and Pin Functions
// ===================================================================================
#define TAPE_BUF_SIZE   512             // ring buffer (two halves of 256 bytes)
#define TAPE_BUF_MASK   (TAPE_BUF_SIZE - 1)
#define TAPE_HALF       256

static uint8_t TAPE_buffer[TAPE_BUF_SIZE];      // ring buffer
static volatile uint16_t TAPE_rd;               // read counter (free-running)
static volatile uint16_t TAPE_wr;               // write counter (free-running)
static volatile uint8_t  TAPE_flags;            // status flags (see below)
static uint8_t  TAPE_invert;                    // invert signal
uint8_t TAPE_machine;                           // machine of the file being played
uint8_t TAPE_video;                             // video standard of the file

#define FLAG_EOF        0x01            // playback: all file data is in the buffer
#define FLAG_DONE       0x02            // playback: last pulse sent
#define FLAG_PAUSE      0x04            // playback: paused
#define FLAG_OVERRUN    0x08            // recording: buffer overrun
#define FLAG_STARTED    0x10            // recording: first edge seen

// Sense line: open collector, low = key pressed on the Datasette
#define SENSE_on()      PIN_output(PIN_TAPE_SENSE)
#define SENSE_off()     PIN_input(PIN_TAPE_SENSE)

// Optional recording LED
#ifdef PIN_LED_REC
  #define LED_on()      PIN_write(PIN_LED_REC, LED_REC_ACTIVE)
  #define LED_off()     PIN_write(PIN_LED_REC, !LED_REC_ACTIVE)
#else
  #define LED_on()
  #define LED_off()
#endif

// Set the READ line (to the computer)
#define READ_write(v)   PIN_write(PIN_TAPE_READ, (v))

// Init the pins of the cassette port
void TAPE_init(void) {
  PIN_low(PIN_TAPE_SENSE);                      // sense: open collector, released
  SENSE_off();
  PIN_high(PIN_TAPE_READ);                      // read: output, idle high
  PIN_output(PIN_TAPE_READ);
  PIN_input(PIN_TAPE_WRITE);                    // write: input, no pull-up yet
  PIN_input(PIN_TAPE_MOTOR);                    // motor: input with pull-up
  PIN_pullup(PIN_TAPE_MOTOR);
  #ifdef PIN_LED_REC
  LED_off();
  PIN_output(PIN_LED_REC);
  #endif
}

// 1 if the computer has switched the motor on (optocoupler pulls the pin low)
uint8_t TAPE_motor(void) {
  return !PIN_read(PIN_TAPE_MOTOR);
}

// Number of bytes in the ring buffer
static uint16_t TAPE_fill(void) {
  uint16_t n;
  INT_ATOMIC_BLOCK { n = TAPE_wr - TAPE_rd; }
  return n;
}

// Stop timer 1 and its interrupts
static void TAPE_timerStop(void) {
  TIMSK1 = 0;
  TCCR1B = 0;
  TIFR1  = 0xFF;
}

// ===================================================================================
// Playback
// ===================================================================================
// Each TAP value is a full wave (versions 0/1) or a half wave (version 2). The output
// goes low for the first half and high for the second half of a wave, the computer
// counts the falling edges. Long half waves are split into timer periods of 65536
// ticks. The duration of the next half wave is computed in advance, so that the edge
// comes right at the start of the interrupt.

#define PLAY_MIN        64              // shortest half wave in ticks (32us)
#define PLAY_WAIT       1000            // timer period while waiting (0.5ms)

static uint8_t  TAPE_version;                   // TAP format version
static int16_t  TAPE_frac;                      // conversion factor (see above)
static uint32_t TAPE_next;                      // next half wave in ticks (0: none)
static uint32_t TAPE_second;                    // second half of a wave (v0/v1)
static uint32_t TAPE_remain;                    // rest of a long half wave
static uint8_t  TAPE_phase;                     // 0: next half wave is the first

static FAT_file TAPE_file;                      // file being played or recorded
static uint32_t TAPE_length;                    // TAP data bytes in the file
static uint32_t TAPE_loaded;                    // TAP data bytes put into the buffer
static uint16_t TAPE_secPos;                    // read position in SD_buffer
static uint16_t TAPE_secLen;                    // valid bytes in SD_buffer

// Convert machine cycles into timer ticks of a half wave (v0/v1)
static uint32_t TAPE_ticks(uint32_t cycles) {
  if(cycles < 0x20000)
    return cycles + (((int32_t)cycles * TAPE_frac + 0x8000) >> 16);
  return cycles + (((int32_t)(cycles >> 8) * TAPE_frac + 0x80) >> 8);
}

// Get the duration of the next half wave from the buffer (0: no data)
static uint32_t TAPE_fetch(void) {
  uint32_t cycles, ticks;
  uint16_t rd = TAPE_rd;
  uint16_t avail = TAPE_wr - rd;
  uint8_t  v;

  // Second half of a full wave
  if(TAPE_second) {
    ticks = TAPE_second;
    TAPE_second = 0;
    return ticks;
  }

  if(!avail) return 0;
  v = TAPE_buffer[rd & TAPE_BUF_MASK];
  if(v) {                                       // 8-bit value: cycles / 8
    cycles = (uint16_t)v << 3;
    rd++;
  }
  else if(TAPE_version == 0) {                  // version 0: zero means 256
    cycles = 256 << 3;
    rd++;
  }
  else {                                        // zero + 24-bit cycle count
    if(avail < 4) {
      if(TAPE_flags & FLAG_EOF) TAPE_rd = TAPE_wr;   // incomplete value at the end
      return 0;
    }
    cycles  = TAPE_buffer[(rd + 1) & TAPE_BUF_MASK];
    cycles |= (uint16_t)TAPE_buffer[(rd + 2) & TAPE_BUF_MASK] << 8;
    cycles |= (uint32_t)TAPE_buffer[(rd + 3) & TAPE_BUF_MASK] << 16;
    rd += 4;
  }
  TAPE_rd = rd;

  ticks = TAPE_ticks(cycles);
  if(TAPE_version == 2) ticks <<= 1;            // half wave given directly
  else TAPE_second = ticks;                     // same duration for the second half
  if(ticks < PLAY_MIN) ticks = PLAY_MIN;
  if(TAPE_second && TAPE_second < PLAY_MIN) TAPE_second = PLAY_MIN;
  return ticks;
}

// Start a timer period of t ticks (longer ones are split)
static inline void TAPE_period(uint32_t t) {
  if(t > 65536) {
    OCR1A = 65535;
    TAPE_remain = t - 65536;
  }
  else {
    OCR1A = t - 1;
    TAPE_remain = 0;
  }
}

// Compare match interrupt: start of the next half wave
ISR(TIMER1_COMPA_vect) {
  // Continue a long half wave
  if(TAPE_remain) {
    TAPE_period(TAPE_remain);
    return;
  }

  // Wait while the motor is off, playback is paused or the buffer is empty
  if(!TAPE_next || !TAPE_motor() || (TAPE_flags & FLAG_PAUSE)) {
    OCR1A = PLAY_WAIT - 1;
    if(!TAPE_next) {
      TAPE_next = TAPE_fetch();
      if(!TAPE_next && (TAPE_flags & FLAG_EOF)) TAPE_flags |= FLAG_DONE;
    }
    return;
  }

  // Edge: low for the first half, high for the second half of a wave
  READ_write(TAPE_phase ^ TAPE_invert);
  TAPE_phase ^= 1;
  TAPE_period(TAPE_next);

  // Prepare the next half wave
  TAPE_next = TAPE_fetch();
}

// Copy file data into the ring buffer (up to n bytes), returns 0 if successful
static uint8_t TAPE_load(uint16_t n) {
  uint16_t wr = TAPE_wr;
  while(n && TAPE_loaded < TAPE_length) {
    uint16_t chunk;
    if(TAPE_secPos >= TAPE_secLen) {
      TAPE_secLen = FAT_read(&TAPE_file);
      TAPE_secPos = 0;
      if(!TAPE_secLen) return 1;
    }
    chunk = TAPE_secLen - TAPE_secPos;
    if(chunk > n) chunk = n;
    if(chunk > TAPE_length - TAPE_loaded) chunk = TAPE_length - TAPE_loaded;
    memcpy(TAPE_buffer + (wr & TAPE_BUF_MASK), SD_buffer + TAPE_secPos, chunk);
    TAPE_secPos += chunk;
    TAPE_loaded += chunk;
    wr += chunk;
    n  -= chunk;
  }
  INT_ATOMIC_BLOCK {
    TAPE_wr = wr;
    if(TAPE_loaded >= TAPE_length) TAPE_flags |= FLAG_EOF;
  }
  return 0;
}

// Open TAP file and start playback, returns 0 if successful
uint8_t TAPE_playStart(const FAT_entry* e, uint8_t machine, uint8_t video, uint8_t invert) {
  static const char magic[] PROGMEM = "-TAPE-RAW";

  TAPE_timerStop();
  TAPE_rd = TAPE_wr = 0;
  TAPE_flags = 0;
  TAPE_invert = invert;

  // Read and check the header: "C64-TAPE-RAW" or "C16-TAPE-RAW", version, machine, video
  FAT_open(&TAPE_file, e);
  TAPE_secLen = FAT_read(&TAPE_file);
  if(!TAPE_secLen) return TAPE_ERR_READ;
  if(TAPE_secLen < 20 || SD_buffer[0] != 'C' || memcmp_P(SD_buffer + 3, magic, 9)
     || SD_buffer[12] > 2) return TAPE_ERR_FORMAT;
  if(SD_buffer[1] == '1' && SD_buffer[2] == '6') machine = TAPE_C16;
  else if(SD_buffer[1] != '6' || SD_buffer[2] != '4') return TAPE_ERR_FORMAT;
  TAPE_version = SD_buffer[12];
  if(SD_buffer[13] <= TAPE_C16) machine = SD_buffer[13];
  if(SD_buffer[14] <= TAPE_NTSC) video = SD_buffer[14];
  TAPE_machine = machine;
  TAPE_video   = video;
  TAPE_frac    = TAPE_playFrac[machine][video];
  TAPE_length  = e->size - 20;                  // the size field is not always right
  TAPE_loaded  = 0;
  TAPE_secPos  = 20;

  // Fill the buffer
  if(TAPE_load(TAPE_BUF_SIZE)) return TAPE_ERR_READ;

  // Prepare the first half wave and start the timer (CTC, prescaler 8: 2MHz)
  TAPE_phase  = 0;
  TAPE_second = 0;
  TAPE_remain = 0;
  TAPE_next   = TAPE_fetch();
  READ_write(!TAPE_invert);                     // idle level
  SENSE_on();                                   // "PLAY pressed"
  TCCR1A = 0;
  TCNT1  = 0;
  OCR1A  = PLAY_WAIT - 1;
  TIFR1  = 0xFF;
  TCCR1B = (1<<WGM12) | (1<<CS11);
  TIMSK1 = (1<<OCIE1A);
  return 0;
}

// Keep the buffer filled, returns TAPE_RUN, TAPE_END or an error code
uint8_t TAPE_playService(void) {
  if(TAPE_flags & FLAG_DONE) return TAPE_END;
  if(!(TAPE_flags & FLAG_EOF) && (TAPE_BUF_SIZE - TAPE_fill()) >= TAPE_HALF) {
    if(TAPE_load(TAPE_HALF)) return TAPE_ERR_READ;
  }
  return TAPE_RUN;
}

// Stop playback
void TAPE_playStop(void) {
  TAPE_timerStop();
  READ_write(!TAPE_invert);
  SENSE_off();
}

// Playback position 0..255
uint8_t TAPE_progress(void) {
  uint32_t sent = TAPE_loaded - TAPE_fill();
  if(!TAPE_length) return 255;
  if(TAPE_length > 0xFFFFFF) return (sent >> 8) * 255 / (TAPE_length >> 8);
  return (sent * 255) / TAPE_length;
}

// Pause or continue playback
void TAPE_pause(uint8_t p) {
  INT_ATOMIC_BLOCK {
    if(p) TAPE_flags |= FLAG_PAUSE;
    else  TAPE_flags &= ~FLAG_PAUSE;
  }
}

// ===================================================================================
// Recording
// ===================================================================================
// Timer 1 runs at 16MHz, the input capture unit takes the time of each edge of one
// direction (rising, or falling if inverted). The time between two edges is the
// length of a full wave; it is converted into machine cycles and stored as a TAP
// version 1 value. Overflows extend the timer to 32 bits. The time while the motor
// is off is not counted.

static uint16_t TAPE_k;                         // conversion factor
static uint16_t TAPE_ovf;                       // timer overflows (upper 16 bits)
static uint32_t TAPE_last;                      // time of the last edge
static uint8_t  TAPE_open;                      // number of halves sent of a sector
static uint32_t TAPE_bytes;                     // bytes written into the buffer

// Store one byte in the ring buffer (interrupt context)
static inline void TAPE_put(uint8_t v, uint16_t* wr) {
  TAPE_buffer[*wr & TAPE_BUF_MASK] = v;
  (*wr)++;
}

// Input capture interrupt: edge from the computer
ISR(TIMER1_CAPT_vect) {
  uint16_t icr = ICR1;
  uint16_t ovf = TAPE_ovf;
  uint32_t now, ticks, cycles, v;
  uint16_t wr;

  // An overflow that happened just before the capture is not counted yet
  if((TIFR1 & (1<<TOV1)) && icr < 0x8000) ovf++;
  now = ((uint32_t)ovf << 16) | icr;

  if(!TAPE_motor()) return;
  if(!(TAPE_flags & FLAG_STARTED)) {            // first edge: start of measuring
    TAPE_flags |= FLAG_STARTED;
    TAPE_last = now;
    return;
  }
  ticks = now - TAPE_last;
  TAPE_last = now;

  // Ticks (1/16us) into machine cycles
  if(ticks < (1UL << 19))      cycles = (ticks * TAPE_k) >> 16;
  else if(ticks < (1UL << 24)) cycles = ((ticks >> 8) * TAPE_k) >> 8;
  else                         cycles = (ticks >> 16) * TAPE_k;
  if(cycles > 0xFFFFFF) cycles = 0xFFFFFF;

  // Room for up to four bytes?
  wr = TAPE_wr;
  if((uint16_t)(wr - TAPE_rd) > TAPE_BUF_SIZE - 4) {
    TAPE_flags |= FLAG_OVERRUN;
    return;
  }

  // Short pulse: one byte (cycles / 8), long pulse: zero and 24-bit cycles
  v = (cycles + 4) >> 3;
  if(!v) v = 1;
  if(v < 256) TAPE_put(v, &wr);
  else {
    TAPE_put(0, &wr);
    TAPE_put(cycles, &wr);
    TAPE_put(cycles >> 8, &wr);
    TAPE_put(cycles >> 16, &wr);
  }
  TAPE_wr = wr;
}

// Timer overflow interrupt: extend timer, restart measuring while the motor is off
ISR(TIMER1_OVF_vect) {
  TAPE_ovf++;
  if(!TAPE_motor()) TAPE_flags &= ~FLAG_STARTED;
}

// Start recording into the created file, returns 0 if successful
uint8_t TAPE_recStart(FAT_file* f, uint8_t machine, uint8_t video, uint8_t invert) {
  static const char magic[] PROGMEM = "C64-TAPE-RAW";

  TAPE_timerStop();
  memcpy(&TAPE_file, f, sizeof(FAT_file));
  TAPE_k = TAPE_recK[machine][video];
  TAPE_flags = 0;
  TAPE_open = 0;
  TAPE_ovf = 0;

  // The TAP header goes into the buffer first, the length is set at the end
  memcpy_P(TAPE_buffer, magic, 12);
  if(machine == TAPE_C16) TAPE_buffer[1] = '1', TAPE_buffer[2] = '6';
  TAPE_buffer[12] = 1;                          // version 1
  TAPE_buffer[13] = machine;
  TAPE_buffer[14] = video;
  memset(TAPE_buffer + 15, 0, 5);
  TAPE_rd = 0;
  TAPE_wr = 20;
  TAPE_bytes = 0;

  // Input capture on the WRITE line: 16MHz, noise canceler, edge by polarity
  PIN_pullup(PIN_TAPE_WRITE);
  SENSE_on();
  LED_on();
  TCCR1A = 0;
  TCNT1  = 0;
  TIFR1  = 0xFF;
  TCCR1B = (1<<ICNC1) | (1<<CS10) | (invert ? 0 : (1<<ICES1));
  TIMSK1 = (1<<ICIE1) | (1<<TOIE1);
  return 0;
}

// Write full halves of the buffer to the card, returns TAPE_RUN or an error code
uint8_t TAPE_recService(void) {
  if(TAPE_flags & FLAG_OVERRUN) return TAPE_ERR_SPEED;
  while(TAPE_fill() >= TAPE_HALF) {
    if(!TAPE_open && FAT_writeStart(&TAPE_file)) return TAPE_ERR_WRITE;
    SD_writeBytes(TAPE_buffer + (TAPE_rd & TAPE_BUF_MASK), TAPE_HALF);
    INT_ATOMIC_BLOCK { TAPE_rd += TAPE_HALF; }
    TAPE_bytes += TAPE_HALF;
    if(++TAPE_open == 2) {
      TAPE_open = 0;
      if(FAT_writeStop(&TAPE_file)) return TAPE_ERR_WRITE;
    }
  }
  return TAPE_RUN;
}

// Finish the recording and close the file, returns 0 if successful
uint8_t TAPE_recStop(void) {
  uint16_t n, pad;
  uint32_t size, length;
  uint8_t  err = 0;
  static const uint8_t zero[16] = { 0 };

  TAPE_timerStop();
  PIN_pulloff(PIN_TAPE_WRITE);
  SENSE_off();
  LED_off();

  // Write the rest of the buffer, fill the sector with zeros
  if(TAPE_recService() == TAPE_ERR_WRITE) err = 1;
  n = TAPE_fill();
  if(!err && (n || TAPE_open)) {
    if(!TAPE_open && FAT_writeStart(&TAPE_file)) err = 1;
    else {
      SD_writeBytes(TAPE_buffer + (TAPE_rd & TAPE_BUF_MASK), n);
      TAPE_bytes += n;
      pad = (TAPE_open ? TAPE_HALF : 512) - n;
      while(pad) {
        uint8_t c = pad > 16 ? 16 : pad;
        SD_writeBytes(zero, c);
        pad -= c;
      }
      if(FAT_writeStop(&TAPE_file)) err = 1;
    }
    TAPE_open = 0;
  }
  if(err) return TAPE_ERR_WRITE;

  // File size and TAP length field
  size = TAPE_bytes;
  length = size - 20;
  if(FAT_close(&TAPE_file, size)) return TAPE_ERR_WRITE;
  if(FAT_patch(&TAPE_file, 16, (const uint8_t*)&length, 4)) return TAPE_ERR_WRITE;
  return 0;
}

// Bytes recorded so far (TAP data without header)
uint32_t TAPE_recSize(void) {
  uint32_t n = TAPE_bytes + TAPE_fill();
  return n > 20 ? n - 20 : 0;
}
