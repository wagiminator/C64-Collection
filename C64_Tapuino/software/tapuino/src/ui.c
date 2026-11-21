// ===================================================================================
// User Interface: Screens of the Tapuino                                     * v1.0 *
// ===================================================================================
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <string.h>
#include "ui.h"

UI_tape_t UI_tape;
UI_menu_t UI_menu;
UI_name_t UI_name;
UI_msg_t  UI_msg;

// ===================================================================================
// Helpers
// ===================================================================================

// Draw n characters of a string
static int16_t UI_chars(int16_t x, int16_t y, const char* s, uint8_t n, uint8_t c) {
  while(n-- && *s) { OLED_char(x, y, *s++, c); x += 6; }
  return x;
}

// Draw a string centered between x0 and x1
static void UI_center(int16_t x0, int16_t x1, int16_t y, const char* s, uint8_t c) {
  OLED_text(x0 + ((x1 - x0 + 1) - OLED_textWidth(s)) / 2, y, s, c);
}

// Same for a string in flash
static void UI_center_P(int16_t x0, int16_t x1, int16_t y, const char* s, uint8_t c) {
  uint8_t w = strlen_P(s) * 6 - 1;
  OLED_text_P(x0 + ((x1 - x0 + 1) - w) / 2, y, s, c);
}

// Title bar
static void UI_title(const char* title) {
  OLED_fillRect(0, 0, OLED_WIDTH, 11, 1);
  UI_center_P(0, OLED_WIDTH - 1, 2, title, 0);
}

// Integer square root (for the radius of the tape packs)
static uint8_t UI_sqrt(uint16_t a) {
  uint8_t r = 0;
  while((uint16_t)(r + 1) * (r + 1) <= a) r++;
  return r;
}

// ===================================================================================
// Cassette Label
// ===================================================================================
#define LABEL_X0        12              // text area of the label
#define LABEL_X1        115
#define LABEL_CHARS     17              // characters per line
#define LABEL_GAP       24              // gap between the end and the start of scrolling text
#define LABEL_HOLD      12              // steps to wait before the text starts to scroll

static struct {
  uint8_t len1;                         // characters of line 1
  uint8_t start2;                       // first character of line 2
  uint8_t len2;                         // characters of line 2
  uint16_t scroll;                      // scroll position of line 2 in pixels
  uint16_t period;                      // length of a scroll period (0: no scrolling)
  uint8_t hold;                         // steps to wait at the start position
} UI_lab;

// Set the text on the label: split into two lines, long second lines scroll
void UI_setLabel(const char* name, uint8_t folder) {
  uint8_t len = strlen(name), max1, i, cut;

  UI_tape.name = name;
  UI_tape.folder = folder;
  memset(&UI_lab, 0, sizeof(UI_lab));

  // Files: hide the extension .TAP
  if(!folder && len > 4 && name[len - 4] == '.' &&
     (name[len - 3] | 32) == 't' && (name[len - 2] | 32) == 'a' && (name[len - 1] | 32) == 'p') len -= 4;

  max1 = folder ? LABEL_CHARS - 2 : LABEL_CHARS;   // the folder symbol takes two places
  if(len <= max1) { UI_lab.len1 = len; return; }

  // Break the line at a space if possible
  cut = max1;
  for(i = max1; i > max1 / 2; i--) {
    if(name[i] == ' ') { cut = i; break; }
  }
  UI_lab.len1 = cut;
  while(name[cut] == ' ') cut++;
  UI_lab.start2 = cut;
  UI_lab.len2 = len - cut;
  if(UI_lab.len2 > LABEL_CHARS) UI_lab.period = UI_lab.len2 * 6 + LABEL_GAP;
  UI_lab.hold = LABEL_HOLD;
}

// Move a long label text by one step, returns 1 if it moved
uint8_t UI_scrollLabel(void) {
  if(!UI_lab.period) return 0;
  if(UI_lab.hold) { UI_lab.hold--; return 0; }
  UI_lab.scroll += 2;
  if(UI_lab.scroll >= UI_lab.period) {
    UI_lab.scroll = 0;
    UI_lab.hold = LABEL_HOLD;
  }
  return 1;
}

// Draw the label with its text
static void UI_drawLabel(void) {
  const char* s = UI_tape.name;
  int16_t x, w;

  OLED_roundRect(9, 2, 110, 22, 3, 1);
  if(!s) return;

  // Line 1 (centered, alone in the middle if there is no line 2)
  w = UI_lab.len1 * 6 - 1 + (UI_tape.folder ? 12 : 0);
  x = (OLED_WIDTH - w) / 2;
  if(UI_tape.folder) { OLED_char(x, UI_lab.len2 ? 5 : 9, 0x83, 1); x += 12; }
  UI_chars(x, UI_lab.len2 ? 5 : 9, s, UI_lab.len1, 1);
  if(!UI_lab.len2) return;

  // Line 2 (centered, or scrolling inside the label)
  s += UI_lab.start2;
  if(!UI_lab.period) {
    w = UI_lab.len2 * 6 - 1;
    UI_chars((OLED_WIDTH - w) / 2, 14, s, UI_lab.len2, 1);
  }
  else {
    OLED_clip(LABEL_X0, 0, LABEL_X1, OLED_HEIGHT - 1);
    x = LABEL_X0 - UI_lab.scroll;
    UI_chars(x, 14, s, UI_lab.len2, 1);
    UI_chars(x + UI_lab.period, 14, s, UI_lab.len2, 1);
    OLED_noclip();
  }
}

// ===================================================================================
// Cassette Screen
// ===================================================================================
#define REEL_L          32              // x of the left reel
#define REEL_R          95              // x of the right reel
#define REEL_Y          37              // y of both reels
#define HUB_R           6               // radius of the hubs
#define PACK_MIN        7               // radius of an empty tape pack
#define PACK_MAX        16              // radius of a full tape pack

// cos(i * 10 degrees) * 16
static const int8_t UI_cos[36] PROGMEM = {
  16, 16, 15, 14, 12, 10,  8,  5,  3,  0, -3, -5, -8,-10,-12,-14,-15,-16,
 -16,-16,-15,-14,-12,-10, -8, -5, -3,  0,  3,  5,  8, 10, 12, 14, 15, 16
};
#define UI_COS(i)       ((int8_t)pgm_read_byte(&UI_cos[(i) % 36]))
#define UI_SIN(i)       UI_COS((i) + 27)

// Draw a reel: tape pack, hub with six teeth
static void UI_drawReel(int16_t cx, uint8_t pack, uint8_t angle) {
  uint8_t k, a;
  OLED_clip(18, 26, 109, 48);                   // packs are seen through the window
  OLED_fillCircle(cx, REEL_Y, pack, 1);
  OLED_noclip();
  OLED_fillCircle(cx, REEL_Y, HUB_R, 0);
  OLED_circle(cx, REEL_Y, HUB_R, 1);
  for(k = 0; k < 6; k++) {
    a = angle + k * 6;
    OLED_line(cx + ((UI_COS(a) * 2 + 8) >> 4), REEL_Y + ((UI_SIN(a) * 2 + 8) >> 4),
              cx + ((UI_COS(a) * 5 + 8) >> 4), REEL_Y + ((UI_SIN(a) * 5 + 8) >> 4), 1);
  }
}

// Draw the cassette
void UI_drawCassette(void) {
  uint16_t amin = PACK_MIN * PACK_MIN, amax = PACK_MAX * PACK_MAX, move;
  uint8_t i, p = UI_tape.progress;

  // Shell with screws
  OLED_roundRect(0, 0, OLED_WIDTH, OLED_HEIGHT, 4, 1);
  OLED_circle(4, 4, 2, 1);
  OLED_circle(123, 4, 2, 1);
  OLED_circle(4, 59, 2, 1);
  OLED_circle(123, 59, 2, 1);

  // Label
  UI_drawLabel();

  // Window with the reels: the tape moves from the left to the right pack
  OLED_roundRect(17, 25, 94, 25, 5, 1);
  move = (uint32_t)(amax - amin) * p / 255;
  UI_drawReel(REEL_L, UI_sqrt(amax - move), UI_tape.angle);
  UI_drawReel(REEL_R, UI_sqrt(amin + move), UI_tape.angle);

  // Progress window between the reels with a scale
  OLED_fillRect(49, 30, 30, 14, 0);
  OLED_rect(49, 30, 30, 14, 1);
  if(UI_tape.bar) OLED_fillRect(51, 32, ((uint16_t)26 * p + 127) / 255, 10, 1);
  for(i = 0; i < 6; i++) OLED_vline(51 + i * 5, 39, 3 - (i & 1), 2);

  // Head opening at the bottom with the status text
  OLED_line(22, 63, 28, 51, 1);
  OLED_hline(28, 51, 72, 1);
  OLED_line(99, 51, 105, 63, 1);
  UI_center(29, 98, 54, UI_tape.status, 1);
}

// ===================================================================================
// Menu Screen
// ===================================================================================
#define MENU_ROWS       4               // visible items
#define MENU_Y          14              // y of the first item
#define MENU_H          12              // height of an item

void UI_drawMenu(void) {
  char buf[21];
  uint8_t i, n, y;

  UI_title(UI_menu.title);

  // Keep the selected item visible
  if(UI_menu.sel < UI_menu.first) UI_menu.first = UI_menu.sel;
  if(UI_menu.sel >= UI_menu.first + MENU_ROWS) UI_menu.first = UI_menu.sel - MENU_ROWS + 1;

  n = UI_menu.count - UI_menu.first;
  if(n > MENU_ROWS) n = MENU_ROWS;
  y = MENU_Y + (MENU_ROWS - n) * MENU_H / 2;    // center short lists
  for(i = 0; i < n; i++, y += MENU_H) {
    uint8_t item = UI_menu.first + i;
    UI_menu.item(item, buf);
    if(item == UI_menu.sel) {
      OLED_fillRoundRect(2, y - 2, OLED_WIDTH - 4, MENU_H - 1, 3, 1);
      OLED_text(8, y, buf, 0);
    }
    else OLED_text(8, y, buf, 1);
  }

  // Scroll marks
  if(UI_menu.first) OLED_char(OLED_WIDTH - 7, MENU_Y - 1, '^', 1);
  if(UI_menu.first + MENU_ROWS < UI_menu.count) OLED_char(OLED_WIDTH - 7, OLED_HEIGHT - 8, 'v', 1);
}

// ===================================================================================
// Name Entry Screen
// ===================================================================================
static const char UI_txtName[]  PROGMEM = "Recording name";
static const char UI_txtHelp1[] PROGMEM = "NEXT/PREV: letter";
static const char UI_txtHelp2[] PROGMEM = "SEL: next  BACK: del";
static const char UI_txtHelp3[] PROGMEM = "hold SEL: done";

void UI_drawName(void) {
  uint8_t i;
  int16_t x = 64 - (8 * 9) / 2 - 12;

  UI_title(UI_txtName);
  OLED_roundRect(8, 15, OLED_WIDTH - 16, 15, 3, 1);
  for(i = 0; i < 8; i++) {
    char c = UI_name.name[i];
    int16_t cx = x + i * 9 + 2;
    if(i == UI_name.pos) {
      OLED_fillRect(cx - 2, 18, 9, 9, 1);
      OLED_char(cx, 19, c ? c : ' ', 0);
    }
    else if(c) OLED_char(cx, 19, c, 1);
    else OLED_hline(cx, 26, 5, 1);
  }
  OLED_text_P(x + 8 * 9 + 2, 19, PSTR(".TAP"), 1);
  UI_center_P(0, OLED_WIDTH - 1, 34, UI_txtHelp1, 1);
  UI_center_P(0, OLED_WIDTH - 1, 44, UI_txtHelp2, 1);
  UI_center_P(0, OLED_WIDTH - 1, 54, UI_txtHelp3, 1);
}

// ===================================================================================
// Message Screen
// ===================================================================================
void UI_drawMessage(void) {
  UI_title(UI_msg.title);
  OLED_roundRect(4, 17, OLED_WIDTH - 8, 41, 4, 1);
  if(UI_msg.line1) UI_center_P(0, OLED_WIDTH - 1, UI_msg.line2 ? 27 : 33, UI_msg.line1, 1);
  if(UI_msg.line2) UI_center(0, OLED_WIDTH - 1, 39, UI_msg.line2, 1);
}
