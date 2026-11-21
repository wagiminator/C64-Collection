// ===================================================================================
// Project:   Tapuino - Commodore Datasette Emulator
// Version:   v1.0
// Year:      2026
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// The Tapuino emulates a Commodore Datasette (C2N/1530/1531). It plays TAP files from
// an SD card to a C64, VIC-20, C16 or Plus/4 and records programs saved by the
// computer as new TAP files. The OLED shows a cassette: the label carries the name of
// the selected file, the tape packs on the reels and the bar in the tape window show
// the position on the tape, and the status in the head opening tells what is going on.
//
// This firmware is a complete rewrite based on the ideas of the original Tapuino by
// Peter Edwards (sweetlilmre): https://github.com/sweetlilmre/tapuino
//
// Operating Instructions:
// -----------------------
// Main menu:     NEXT/PREV select, SELECT starts the item.
// Load TAP:      NEXT/PREV browse files and folders, SELECT opens a folder or plays a
//                file, BACK goes to the parent folder, hold BACK for the main menu.
// Playback:      Type LOAD on the computer. SELECT pauses/continues, BACK stops.
// Record TAP:    Enter a name (preset with the next free number), hold SELECT to
//                start. Type SAVE on the computer. BACK or SELECT ends the recording,
//                it also ends automatically when the motor has stopped for a while.
// Settings:      NEXT/PREV select, SELECT changes the value, BACK saves and returns.
//
// Compilation Instructions:
// -------------------------
// - Adjust the settings in config.h if necessary.
// - Make sure GCC toolchain (gcc-avr, avr-libc) and avrdude are installed.
// - Connect an ISP programmer (e.g. USBasp) to the ICSP header of the Tapuino.
// - Run 'make fuses' once, then 'make flash'.


// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================
#include <string.h>             // for string functions
#include <stdlib.h>             // for utoa()
#include "../config.h"          // user configuration
#include "system.h"             // for system functions
#include "gpio.h"               // for GPIO functions
#include "sd.h"                 // for SD card functions
#include "fat.h"                // for FAT file system functions
#include "oled.h"               // for OLED functions
#include "tape.h"               // for Datasette emulation
#include "ui.h"                 // for user interface screens

#define VERSION   "v1.0"        // firmware version shown on the start screen
#define ANIM_MS   80            // time between animation steps (reels, scrolling)

// ===================================================================================
// Settings (stored in EEPROM)
// ===================================================================================
#define SET_MAGIC 0x7A          // marks valid settings in the EEPROM

typedef struct {
  uint8_t magic;                // SET_MAGIC
  uint8_t machine;              // machine for recordings (TAPE_C64, ...)
  uint8_t video;                // video standard for recordings (TAPE_PAL, ...)
  uint8_t invert;               // invert signals
  uint8_t autostop;             // seconds until a recording stops (0: off)
  uint8_t contrast;             // display contrast 0..3
} SET_t;

SET_t SET;

static const uint8_t SET_contrast[4] PROGMEM = { 0x08, 0x30, 0x80, 0xFF };
static const uint8_t SET_autostop[]  PROGMEM = { 0, 2, 3, 5, 10, 20 };

// Load settings from the EEPROM (or the defaults)
void SET_load(void) {
  uint8_t i;
  for(i = 0; i < sizeof(SET_t); i++) ((uint8_t*)&SET)[i] = EEPROM_read(i);
  if(SET.magic != SET_MAGIC || SET.machine > TAPE_C16 || SET.video > TAPE_NTSC
     || SET.invert > 1 || SET.autostop > 60 || SET.contrast > 3) {
    SET.magic    = SET_MAGIC;
    SET.machine  = DEF_MACHINE;
    SET.video    = DEF_VIDEO;
    SET.invert   = DEF_INVERT;
    SET.autostop = DEF_AUTOSTOP;
    SET.contrast = DEF_CONTRAST;
  }
}

// Store settings in the EEPROM (only changed bytes are written)
void SET_save(void) {
  uint8_t i;
  for(i = 0; i < sizeof(SET_t); i++) EEPROM_write(i, ((uint8_t*)&SET)[i]);
}

// Apply display contrast
void SET_applyContrast(void) {
  OLED_contrast(pgm_read_byte(&SET_contrast[SET.contrast]));
}

// ===================================================================================
// Buttons
// ===================================================================================
// The buttons are sampled every 10ms by the system tick. NEXT and PREV give an event
// when pressed and repeat while held. SELECT and BACK give a short event when
// released, or a long event as soon as they are held for KEY_LONG_PRESS.

enum { KEY_NONE, KEY_NEXT, KEY_PREV, KEY_BACK, KEY_SELECT, KEY_BACK_LONG, KEY_SELECT_LONG };

#define KEY_TICK  10            // sampling period in ms

volatile uint8_t KEY_event;     // last event (KEY_NONE: none)

// Read the buttons, returns the pressed one (only one at a time)
static uint8_t KEY_read(void) {
  if(!PIN_read(PIN_KEY_NEXT))   return KEY_NEXT;
  if(!PIN_read(PIN_KEY_PREV))   return KEY_PREV;
  if(!PIN_read(PIN_KEY_BACK))   return KEY_BACK;
  if(!PIN_read(PIN_KEY_SELECT)) return KEY_SELECT;
  return KEY_NONE;
}

// Called by the system tick every millisecond
void MIL_callback(void) {
  static uint8_t  div, raw, key, done;
  static uint16_t held;
  uint8_t k;

  if(++div < KEY_TICK) return;
  div = 0;

  // Debounce: accept a state when it was read twice in a row
  k = KEY_read();
  if(k != raw) { raw = k; return; }

  if(k != key) {                                // state has changed
    if((key == KEY_BACK || key == KEY_SELECT) && !done) KEY_event = key;
    if(k == KEY_NEXT || k == KEY_PREV) KEY_event = k;
    key  = k;
    held = 0;
    done = 0;
  }
  else if(key) {                                // button is held
    held++;
    if(key == KEY_NEXT || key == KEY_PREV) {
      if(held >= KEY_REPEAT_DELAY / KEY_TICK) {
        KEY_event = key;
        held -= KEY_REPEAT_RATE / KEY_TICK;
      }
    }
    else if(!done && held >= KEY_LONG_PRESS / KEY_TICK) {
      KEY_event = key + 2;                      // KEY_BACK_LONG, KEY_SELECT_LONG
      done = 1;
    }
  }
}

// Init the button pins
void KEY_init(void) {
  PIN_input(PIN_KEY_NEXT);   PIN_pullup(PIN_KEY_NEXT);
  PIN_input(PIN_KEY_PREV);   PIN_pullup(PIN_KEY_PREV);
  PIN_input(PIN_KEY_BACK);   PIN_pullup(PIN_KEY_BACK);
  PIN_input(PIN_KEY_SELECT); PIN_pullup(PIN_KEY_SELECT);
}

// Get the last event and clear it
uint8_t KEY_get(void) {
  uint8_t k;
  INT_ATOMIC_BLOCK { k = KEY_event; KEY_event = KEY_NONE; }
  return k;
}

// ===================================================================================
// Screen Helpers
// ===================================================================================
static const char TXT_error[]   PROGMEM = "Error";
static const char TXT_sdCard[]  PROGMEM = "SD card";
static const char TXT_noCard[]  PROGMEM = "No SD card found";
static const char TXT_noFAT[]   PROGMEM = "No FAT16/32 found";
static const char TXT_empty[]   PROGMEM = "No TAP files here";
static const char TXT_read[]    PROGMEM = "Card read error";
static const char TXT_format[]  PROGMEM = "Not a TAP file";
static const char TXT_write[]   PROGMEM = "Card write error";
static const char TXT_speed[]   PROGMEM = "Card too slow";
static const char TXT_exists[]  PROGMEM = "File exists";
static const char TXT_folder[]  PROGMEM = "Cannot create folder";
static const char TXT_record[]  PROGMEM = "Recording";
static const char TXT_saved[]   PROGMEM = "Saved as";

static PGM_P const TXT_tapeErr[] PROGMEM = { 0, 0, TXT_read, TXT_format, TXT_write, TXT_speed };

static uint16_t anim;           // time of the last animation step

// Wait for a key, keep scrolling the label of the cassette screen
uint8_t waitKey(void (*draw)(void)) {
  uint8_t k;
  while(!(k = KEY_get())) {
    if(draw == UI_drawCassette && (uint16_t)(MIL_read() - anim) >= ANIM_MS) {
      anim = MIL_read();
      if(UI_scrollLabel()) OLED_render(draw);
    }
  }
  return k;
}

// Show a message and wait for a key
void message(PGM_P title, PGM_P line1, const char* line2) {
  UI_msg.title = title;
  UI_msg.line1 = line1;
  UI_msg.line2 = line2;
  OLED_render(UI_drawMessage);
  KEY_get();
  waitKey(UI_drawMessage);
}

// Show the message for a TAPE_ERR_... code
void tapeError(uint8_t err) {
  message(TXT_error, (PGM_P)pgm_read_word(&TXT_tapeErr[err]), 0);
}

// Write status text: symbol, text from flash, optional number with unit
void status(char sym, PGM_P text, int16_t value, char unit) {
  char* p = UI_tape.status;
  *p++ = sym;
  *p++ = ' ';
  strcpy_P(p, text);
  if(value >= 0) {
    p += strlen(p);
    *p++ = ' ';
    utoa(value, p, 10);
    p += strlen(p);
    *p++ = unit;
    *p = 0;
  }
}

// Mount the SD card, returns 0 if successful (shows a message otherwise)
uint8_t mount(void) {
  if(SD_init())   { message(TXT_sdCard, TXT_noCard, 0); return 1; }
  if(FAT_mount()) { message(TXT_sdCard, TXT_noFAT,  0); return 1; }
  return 0;
}

// ===================================================================================
// Playback
// ===================================================================================
static const char TXT_play[]  PROGMEM = "PLAY";
static const char TXT_pause[] PROGMEM = "PAUSE";
static const char TXT_wait[]  PROGMEM = "WAIT";
static const char TXT_done[]  PROGMEM = "DONE";

// Play a TAP file
void play(FAT_entry* e) {
  uint8_t r, k, paused = 0, motor;

  r = TAPE_playStart(e, SET.machine, SET.video, SET.invert);
  if(r) { TAPE_playStop(); tapeError(r); return; }
  UI_tape.bar = 1;

  while(1) {
    // Keep the buffer filled
    r = TAPE_playService();
    if(r > TAPE_END) { TAPE_playStop(); tapeError(r); break; }

    // Buttons
    k = KEY_get();
    if(k == KEY_BACK || k == KEY_BACK_LONG || (k && r == TAPE_END)) break;
    if(k == KEY_SELECT) TAPE_pause(paused = !paused);

    // Draw one page of the cassette, update the picture when the frame is complete
    if(OLED_renderStep(UI_drawCassette)) {
      motor = TAPE_motor();
      UI_tape.progress = TAPE_progress();
      if(r == TAPE_END)
        status(OLED_SYM_STOP[0], TXT_done, 100, '%');
      else if(paused)
        status(OLED_SYM_PAUSE[0], TXT_pause, -1, 0);
      else
        status(motor ? OLED_SYM_PLAY[0] : OLED_SYM_STOP[0], motor ? TXT_play : TXT_wait,
               (uint16_t)UI_tape.progress * 100 / 255, '%');
      if((uint16_t)(MIL_read() - anim) >= ANIM_MS) {
        anim = MIL_read();
        UI_scrollLabel();
        if(motor && !paused && r != TAPE_END) UI_tape.angle = (UI_tape.angle + 35) % 36;
      }
    }
  }

  TAPE_playStop();
  UI_tape.bar = 0;
  UI_tape.progress = 0;
}

// ===================================================================================
// File Browser
// ===================================================================================
static FAT_entry entry;         // selected entry
static uint32_t  folder;        // current folder (0: root)
static uint16_t  position;      // position of the entry in the folder
static uint16_t  total;         // number of entries in the folder

// Show the selected entry on the cassette
void browseShow(void) {
  char* p;
  UI_setLabel(entry.name, entry.attr & FAT_ATTR_DIR);
  UI_tape.progress = 0;
  UI_tape.bar = 0;
  p = UI_tape.status;
  utoa(position + 1, p, 10);
  p += strlen(p);
  *p++ = '/';
  utoa(total, p, 10);
  OLED_render(UI_drawCassette);
}

// Open a folder and select its first entry, returns 0 if not empty
uint8_t browseOpen(uint32_t dir) {
  folder = dir;
  position = 0;
  total  = FAT_count(dir);
  return !total || !FAT_first(dir, &entry);
}

// Go to the parent folder and select the folder we came from, returns 0 if successful
uint8_t browseUp(void) {
  FAT_entry* e = &entry;
  uint32_t from = folder;
  if(!folder || !FAT_find(folder, "..", e)) return 1;
  if(browseOpen(e->cluster)) return 1;
  if(FAT_findCluster(folder, from, &entry)) position = FAT_index(folder, &entry);
  return 0;
}

// Browse the card and play files
void browse(void) {
  uint8_t k;

  if(mount()) return;
  if(browseOpen(0)) { message(TXT_sdCard, TXT_empty, 0); return; }

  while(1) {
    browseShow();
    k = waitKey(UI_drawCassette);
    switch(k) {
      case KEY_NEXT:
        if(FAT_step(folder, &entry, 1)) position = (position + 1 < total) ? position + 1 : 0;
        break;
      case KEY_PREV:
        if(FAT_step(folder, &entry, -1)) position = position ? position - 1 : total - 1;
        break;
      case KEY_SELECT:
        if(entry.attr & FAT_ATTR_DIR) {
          uint32_t from = folder;
          if(browseOpen(entry.cluster)) {         // empty folder: stay
            message(TXT_sdCard, TXT_empty, 0);
            browseOpen(from);
          }
        }
        else play(&entry);
        break;
      case KEY_BACK:
        if(browseUp()) return;
        break;
      case KEY_BACK_LONG:
        return;
    }
  }
}

// ===================================================================================
// Recording
// ===================================================================================
static const char TXT_ready[] PROGMEM = "READY";
static const char TXT_rec[]   PROGMEM = "REC";
static const char NAME_chars[] PROGMEM = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-";

// Edit the name in UI_name, returns 1 when done, 0 when cancelled
uint8_t nameEdit(void) {
  uint8_t k, n = sizeof(NAME_chars) - 1;
  char* c;

  UI_name.pos = strlen(UI_name.name);
  if(UI_name.pos) UI_name.pos--;
  while(1) {
    OLED_render(UI_drawName);
    k = waitKey(UI_drawName);
    c = &UI_name.name[UI_name.pos];
    switch(k) {
      case KEY_NEXT:
      case KEY_PREV: {
        const char* p = *c ? strchr_P(NAME_chars, *c) : 0;
        uint8_t i = p ? p - NAME_chars : (k == KEY_NEXT ? n - 1 : 0);
        i = (k == KEY_NEXT) ? (i + 1) % n : (i + n - 1) % n;
        *c = pgm_read_byte(&NAME_chars[i]);
        break;
      }
      case KEY_SELECT:
        if(*c && UI_name.pos < 7) UI_name.pos++;
        break;
      case KEY_BACK:
        if(*c) *c = 0;
        else if(UI_name.pos) UI_name.name[--UI_name.pos] = 0;
        break;
      case KEY_SELECT_LONG:
        if(UI_name.name[0]) return 1;
        break;
      case KEY_BACK_LONG:
        return 0;
    }
  }
}

// Record a new TAP file
void record(void) {
  static FAT_file file;
  static char fname[13];
  FAT_entry* e = &entry;
  uint32_t dir;
  uint16_t off = 0, n;
  uint8_t  k, r, motor, started = 0;

  // Folder for recordings
  if(mount()) return;
  if(FAT_find(0, REC_FOLDER, e) && (e->attr & FAT_ATTR_DIR)) dir = e->cluster;
  else if(!(dir = FAT_mkdir(0, REC_FOLDER))) { message(TXT_error, TXT_folder, 0); return; }

  // Name: preset with the next free number
  n = FAT_lastNumber(dir, REC_PREFIX) + 1;
  strcpy(UI_name.name, REC_PREFIX);
  k = strlen(UI_name.name);
  UI_name.name[k++] = '0' + (n / 1000) % 10;
  UI_name.name[k++] = '0' + (n / 100) % 10;
  UI_name.name[k++] = '0' + (n / 10) % 10;
  UI_name.name[k++] = '0' + n % 10;
  UI_name.name[k] = 0;
  while(1) {
    if(!nameEdit()) return;
    strcpy(fname, UI_name.name);
    strcat_P(fname, PSTR(".TAP"));
    if(!FAT_find(dir, fname, 0)) break;
    message(TXT_error, TXT_exists, fname);
  }

  // Create the file and start
  if(FAT_create(&file, dir, fname)) { message(TXT_error, TXT_write, 0); return; }
  UI_setLabel(fname, 0);
  UI_tape.progress = 0;
  UI_tape.bar = 0;
  status(OLED_SYM_REC[0], TXT_ready, -1, 0);
  OLED_render(UI_drawCassette);
  KEY_get();
  TAPE_recStart(&file, SET.machine, SET.video, SET.invert);

  while(1) {
    // Write the buffer to the card
    r = TAPE_recService();
    if(r) break;

    // Buttons
    k = KEY_get();
    if(k == KEY_BACK || k == KEY_SELECT || k == KEY_BACK_LONG) break;

    // Motor and automatic stop
    motor = TAPE_motor();
    if(motor) { started = 1; off = MIL_read(); }
    else if(started && SET.autostop &&
            (uint16_t)(MIL_read() - off) >= (uint16_t)SET.autostop * 1000) break;

    // Draw one page, update the picture when the frame is complete
    if(OLED_renderStep(UI_drawCassette)) {
      uint32_t size = TAPE_recSize();
      UI_tape.progress = (size >> 11) > 255 ? 255 : size >> 11;
      if(!started) status(OLED_SYM_REC[0], TXT_ready, -1, 0);
      else status(motor ? OLED_SYM_REC[0] : OLED_SYM_PAUSE[0], TXT_rec,
                  (size + 1023) >> 10, 'K');
      if((uint16_t)(MIL_read() - anim) >= ANIM_MS) {
        anim = MIL_read();
        UI_scrollLabel();
        if(motor) UI_tape.angle = (UI_tape.angle + 35) % 36;
      }
    }
  }

  // Finish the file
  k = TAPE_recStop();
  if(r) tapeError(r);
  else if(k) tapeError(k);
  else message(TXT_record, TXT_saved, fname);
}

// ===================================================================================
// Settings Menu
// ===================================================================================
static const char TXT_settings[] PROGMEM = "Settings";
static const char SET_names[]    PROGMEM = "Machine\0Video\0Signal\0Auto stop\0Contrast";
static const char SET_machines[] PROGMEM = "C64\0VIC-20\0C16/+4";
static const char SET_videos[]   PROGMEM = "PAL\0NTSC";
static const char SET_signals[]  PROGMEM = "normal\0invert";

// Copy the n-th string of a list of zero-terminated strings
static char* strList(char* buf, PGM_P list, uint8_t n) {
  while(n--) list += strlen_P(list) + 1;
  strcpy_P(buf, list);
  return buf + strlen(buf);
}

// Text of a settings item: name left, value right
void SET_item(uint8_t i, char* buf) {
  char v[8];
  uint8_t n;
  switch(i) {
    case 0: strList(v, SET_machines, SET.machine); break;
    case 1: strList(v, SET_videos, SET.video);     break;
    case 2: strList(v, SET_signals, SET.invert);   break;
    case 3:
      if(!SET.autostop) strcpy_P(v, PSTR("off"));
      else { utoa(SET.autostop, v, 10); strcat_P(v, PSTR(" s")); }
      break;
    default: v[0] = '1' + SET.contrast; v[1] = 0;  break;
  }
  n = strList(buf, SET_names, i) - buf;
  while(n < 18 - strlen(v)) buf[n++] = ' ';
  strcpy(buf + n, v);
}

// Settings menu
void settings(void) {
  uint8_t k, i;

  UI_menu.title = TXT_settings;
  UI_menu.count = 5;
  UI_menu.sel   = 0;
  UI_menu.first = 0;
  UI_menu.item  = SET_item;

  while(1) {
    OLED_render(UI_drawMenu);
    k = waitKey(UI_drawMenu);
    if(k == KEY_NEXT) UI_menu.sel = (UI_menu.sel + 1) % UI_menu.count;
    if(k == KEY_PREV) UI_menu.sel = (UI_menu.sel + UI_menu.count - 1) % UI_menu.count;
    if(k == KEY_BACK || k == KEY_BACK_LONG) break;
    if(k == KEY_SELECT) {
      switch(UI_menu.sel) {
        case 0: SET.machine = (SET.machine + 1) % 3; break;
        case 1: SET.video  ^= 1; break;
        case 2: SET.invert ^= 1; break;
        case 3:
          for(i = 0; i < sizeof(SET_autostop) - 1; i++)
            if(pgm_read_byte(&SET_autostop[i]) == SET.autostop) break;
          i = (i + 1) % sizeof(SET_autostop);
          SET.autostop = pgm_read_byte(&SET_autostop[i]);
          break;
        default:
          SET.contrast = (SET.contrast + 1) & 3;
          SET_applyContrast();
          break;
      }
    }
  }
  SET_save();
}

// ===================================================================================
// Main Menu
// ===================================================================================
static const char TXT_title[] PROGMEM = "TAPUINO " VERSION;
static const char MENU_items[] PROGMEM = OLED_SYM_PLAY " Load TAP\0"
                                         OLED_SYM_REC " Record TAP\0"
                                         OLED_SYM_GEAR " Settings";

// Text of a main menu item
void MENU_item(uint8_t i, char* buf) {
  strList(buf, MENU_items, i);
}

// ===================================================================================
// Main Function
// ===================================================================================
int main(void) {
  uint8_t k, sel = 0;

  // Setup
  WDT_stop();                                   // stop watchdog after a reset
  KEY_init();                                   // buttons
  TAPE_init();                                  // cassette port
  MIL_init();                                   // system tick (buttons, timeouts)
  INT_enable();                                 // enable interrupts
  SET_load();                                   // settings from EEPROM
  OLED_init();                                  // display
  SET_applyContrast();

  // Start screen: cassette with the name of the device
  UI_setLabel("TAPUINO", 0);
  status(OLED_SYM_PLAY[0], PSTR(VERSION), -1, 0);
  OLED_render(UI_drawCassette);
  anim = MIL_read();
  while((uint16_t)(MIL_read() - anim) < 1500 && !KEY_get());

  // Loop
  while(1) {
    UI_menu.title = TXT_title;
    UI_menu.count = 3;
    UI_menu.sel   = sel;
    UI_menu.first = 0;
    UI_menu.item  = MENU_item;
    OLED_render(UI_drawMenu);
    k = waitKey(UI_drawMenu);
    sel = UI_menu.sel;
    if(k == KEY_NEXT) sel = (sel + 1) % 3;
    if(k == KEY_PREV) sel = (sel + 2) % 3;
    if(k == KEY_SELECT) {
      if(sel == 0) browse();
      if(sel == 1) record();
      if(sel == 2) settings();
      KEY_get();
    }
  }
}
