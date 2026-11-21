// ===================================================================================
// SSD1306/SH1106 128x64 I2C OLED Graphics with Page Rendering                * v1.0 *
// ===================================================================================
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#include <string.h>
#include "oled.h"
#include "i2c.h"
#include "../config.h"

// ===================================================================================
// Standard ASCII 5x8 Font (chars 32 - 126) and Symbols (chars 0x80 - 0x87)
// ===================================================================================
static const uint8_t OLED_FONT[] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x07, 0x00, 0x07, 0x00,
  0x14, 0x7F, 0x14, 0x7F, 0x14, 0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x23, 0x13, 0x08, 0x64, 0x62,
  0x36, 0x49, 0x55, 0x22, 0x50, 0x00, 0x04, 0x03, 0x00, 0x00, 0x00, 0x1C, 0x22, 0x41, 0x00,
  0x00, 0x41, 0x22, 0x1C, 0x00, 0x14, 0x08, 0x3E, 0x08, 0x14, 0x08, 0x08, 0x3E, 0x08, 0x08,
  0x00, 0x80, 0x60, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x60, 0x60, 0x00, 0x00,
  0x20, 0x10, 0x08, 0x04, 0x02, 0x3E, 0x51, 0x49, 0x45, 0x3E, 0x44, 0x42, 0x7F, 0x40, 0x40,
  0x42, 0x61, 0x51, 0x49, 0x46, 0x22, 0x41, 0x49, 0x49, 0x36, 0x18, 0x14, 0x12, 0x7F, 0x10,
  0x2F, 0x49, 0x49, 0x49, 0x31, 0x3E, 0x49, 0x49, 0x49, 0x32, 0x03, 0x01, 0x71, 0x09, 0x07,
  0x36, 0x49, 0x49, 0x49, 0x36, 0x26, 0x49, 0x49, 0x49, 0x3E, 0x00, 0x36, 0x36, 0x00, 0x00,
  0x00, 0x80, 0x68, 0x00, 0x00, 0x00, 0x08, 0x14, 0x22, 0x00, 0x14, 0x14, 0x14, 0x14, 0x14,
  0x00, 0x22, 0x14, 0x08, 0x00, 0x02, 0x01, 0x51, 0x09, 0x06, 0x3E, 0x41, 0x5D, 0x55, 0x5E,
  0x7C, 0x12, 0x11, 0x12, 0x7C, 0x7F, 0x49, 0x49, 0x49, 0x36, 0x3E, 0x41, 0x41, 0x41, 0x22,
  0x7F, 0x41, 0x41, 0x22, 0x1C, 0x7F, 0x49, 0x49, 0x49, 0x41, 0x7F, 0x09, 0x09, 0x09, 0x01,
  0x3E, 0x41, 0x49, 0x49, 0x3A, 0x7F, 0x08, 0x08, 0x08, 0x7F, 0x41, 0x41, 0x7F, 0x41, 0x41,
  0x20, 0x40, 0x41, 0x3F, 0x01, 0x7F, 0x08, 0x14, 0x22, 0x41, 0x7F, 0x40, 0x40, 0x40, 0x40,
  0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x7F, 0x04, 0x08, 0x10, 0x7F, 0x3E, 0x41, 0x41, 0x41, 0x3E,
  0x7F, 0x09, 0x09, 0x09, 0x06, 0x3E, 0x41, 0x41, 0xC1, 0xBE, 0x7F, 0x09, 0x19, 0x29, 0x46,
  0x26, 0x49, 0x49, 0x49, 0x32, 0x01, 0x01, 0x7F, 0x01, 0x01, 0x3F, 0x40, 0x40, 0x40, 0x3F,
  0x1F, 0x20, 0x40, 0x20, 0x1F, 0x3F, 0x40, 0x38, 0x40, 0x3F, 0x63, 0x14, 0x08, 0x14, 0x63,
  0x07, 0x08, 0x70, 0x08, 0x07, 0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x7F, 0x41, 0x41, 0x00,
  0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x41, 0x41, 0x7F, 0x00, 0x08, 0x04, 0x02, 0x04, 0x08,
  0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x06, 0x09, 0x09, 0x06, 0x20, 0x54, 0x54, 0x54, 0x78,
  0x7F, 0x44, 0x44, 0x44, 0x38, 0x38, 0x44, 0x44, 0x44, 0x28, 0x38, 0x44, 0x44, 0x44, 0x7F,
  0x38, 0x54, 0x54, 0x54, 0x18, 0x08, 0xFE, 0x09, 0x01, 0x02, 0x18, 0xA4, 0xA4, 0xA4, 0x78,
  0x7F, 0x04, 0x04, 0x04, 0x78, 0x00, 0x44, 0x7D, 0x40, 0x00, 0x00, 0x80, 0x84, 0x7D, 0x00,
  0x41, 0x7F, 0x10, 0x28, 0x44, 0x00, 0x41, 0x7F, 0x40, 0x00, 0x7C, 0x04, 0x7C, 0x04, 0x78,
  0x7C, 0x04, 0x04, 0x04, 0x78, 0x38, 0x44, 0x44, 0x44, 0x38, 0xFC, 0x24, 0x24, 0x24, 0x18,
  0x18, 0x24, 0x24, 0x24, 0xFC, 0x7C, 0x08, 0x04, 0x04, 0x08, 0x08, 0x54, 0x54, 0x54, 0x20,
  0x04, 0x3F, 0x44, 0x40, 0x20, 0x3C, 0x40, 0x40, 0x40, 0x3C, 0x1C, 0x20, 0x40, 0x20, 0x1C,
  0x3C, 0x40, 0x30, 0x40, 0x3C, 0x44, 0x28, 0x10, 0x28, 0x44, 0x1C, 0xA0, 0xA0, 0xA0, 0x7C,
  0x44, 0x64, 0x54, 0x4C, 0x44, 0x08, 0x08, 0x36, 0x41, 0x41, 0x00, 0x00, 0xFF, 0x00, 0x00,
  0x41, 0x41, 0x36, 0x08, 0x08, 0x08, 0x04, 0x08, 0x10, 0x08,

  // Symbols: play, record, pause, folder, left, right, stop, gear
  0x7F, 0x3E, 0x1C, 0x08, 0x00, 0x1C, 0x3E, 0x3E, 0x3E, 0x1C, 0x7F, 0x7F, 0x00, 0x7F, 0x7F,
  0x7F, 0x43, 0x42, 0x42, 0x7E, 0x08, 0x1C, 0x3E, 0x7F, 0x00, 0x00, 0x7F, 0x3E, 0x1C, 0x08,
  0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x2A, 0x1C, 0x77, 0x1C, 0x2A
};

// ===================================================================================
// OLED Commands
// ===================================================================================
#define OLED_CMD_MODE     0x00          // command stream
#define OLED_DAT_MODE     0x40          // data stream
#define OLED_COL_OFFSET   (OLED_SH1106 ? 2 : 0)   // SH1106 has 132 columns

static const uint8_t OLED_INIT_CMD[] PROGMEM = {
  0xAE,                                 // display off
  0xD5, 0x80,                           // clock divider and frequency
  0xA8, 0x3F,                           // multiplex ratio: 64 rows
  0xD3, 0x00,                           // display offset: 0
  0x40,                                 // start line: 0
  0x8D, 0x14,                           // charge pump on
  0x20, 0x02,                           // page addressing mode
  #if OLED_FLIP > 0
  0xA0, 0xC0,                           // segment and COM scan direction (flipped)
  #else
  0xA1, 0xC8,                           // segment and COM scan direction (normal)
  #endif
  0xDA, 0x12,                           // COM pins configuration
  0xD9, 0xF1,                           // pre-charge period
  0xDB, 0x40,                           // VCOMH level
  0xA4,                                 // display RAM content
  0xA6,                                 // not inverted
  0xAF                                  // display on
};

// ===================================================================================
// Display Functions
// ===================================================================================
static uint8_t OLED_buf[OLED_WIDTH];    // pixels of the current page
static uint8_t OLED_pg;                 // current page (0..7)
static int16_t OLED_py;                 // first row of the current page
static uint8_t OLED_mask;               // rows of the page inside the clip rectangle
static int16_t OLED_cx0, OLED_cy0, OLED_cx1, OLED_cy1;   // clip rectangle
static uint8_t OLED_step;               // next page for OLED_renderStep()

// Send command bytes
static void OLED_commands(const uint8_t* cmd, uint8_t len, uint8_t flash) {
  I2C_start(OLED_ADDR << 1);
  I2C_write(OLED_CMD_MODE);
  while(len--) I2C_write(flash ? pgm_read_byte(cmd++) : *cmd++);
  I2C_stop();
}

// Init OLED
void OLED_init(void) {
  I2C_init();
  DLY_ms(50);                                   // boot time of the display
  OLED_commands(OLED_INIT_CMD, sizeof(OLED_INIT_CMD), 1);
}

// Set contrast (0-255)
void OLED_contrast(uint8_t v) {
  uint8_t cmd[2] = { 0x81, v };
  OLED_commands(cmd, 2, 0);
}

// Rows of the page that lie inside y .. y+h-1 (and inside the clip rectangle)
static uint8_t OLED_rows(int16_t y, int16_t h) {
  int16_t top = y - OLED_py, bottom = top + h;
  if(top < 0) top = 0;
  if(bottom > 8) bottom = 8;
  if(top >= bottom) return 0;
  return ((uint8_t)(0xFF >> (8 - (bottom - top))) << top) & OLED_mask;
}

// Restrict drawing to a rectangle
void OLED_clip(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
  OLED_cx0 = x0 < 0 ? 0 : x0;
  OLED_cx1 = x1 > OLED_WIDTH - 1 ? OLED_WIDTH - 1 : x1;
  OLED_cy0 = y0;
  OLED_cy1 = y1;
  OLED_mask = 0xFF;
  OLED_mask = OLED_rows(y0, y1 - y0 + 1);
}

// Remove the restriction
void OLED_noclip(void) {
  OLED_clip(0, 0, OLED_WIDTH - 1, OLED_HEIGHT - 1);
}

// Draw the current page and send it to the display
static void OLED_page(void (*draw)(void)) {
  uint8_t i;
  OLED_py = OLED_pg << 3;
  memset(OLED_buf, 0, OLED_WIDTH);
  OLED_noclip();
  draw();

  I2C_start(OLED_ADDR << 1);
  I2C_write(OLED_CMD_MODE);
  I2C_write(0xB0 | OLED_pg);                    // page
  I2C_write(OLED_COL_OFFSET & 0x0F);            // column low nibble
  I2C_write(0x10 | (OLED_COL_OFFSET >> 4));     // column high nibble
  I2C_stop();
  I2C_start(OLED_ADDR << 1);
  I2C_write(OLED_DAT_MODE);
  for(i = 0; i < OLED_WIDTH; i++) I2C_write(OLED_buf[i]);
  I2C_stop();
}

// Draw the whole screen
void OLED_render(void (*draw)(void)) {
  for(OLED_pg = 0; OLED_pg < 8; OLED_pg++) OLED_page(draw);
  OLED_step = 0;
}

// Draw one page, returns 1 when the screen is complete
uint8_t OLED_renderStep(void (*draw)(void)) {
  OLED_pg = OLED_step;
  OLED_page(draw);
  OLED_step = (OLED_step + 1) & 7;
  return !OLED_step;
}

// ===================================================================================
// Drawing Functions
// ===================================================================================

// Change bits of one column of the page
static void OLED_bits(int16_t x, uint8_t bits, uint8_t c) {
  if(x < OLED_cx0 || x > OLED_cx1 || !bits) return;
  if(c == 1)      OLED_buf[x] |=  bits;
  else if(c == 0) OLED_buf[x] &= ~bits;
  else            OLED_buf[x] ^=  bits;
}

// Set pixel
void OLED_pixel(int16_t x, int16_t y, uint8_t c) {
  if(y < OLED_py || y >= OLED_py + 8) return;
  OLED_bits(x, (1 << (y - OLED_py)) & OLED_mask, c);
}

// Filled rectangle
void OLED_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c) {
  uint8_t m = OLED_rows(y, h);
  int16_t x1 = x + w;
  if(!m) return;
  if(x < OLED_cx0) x = OLED_cx0;
  if(x1 > OLED_cx1 + 1) x1 = OLED_cx1 + 1;
  for(; x < x1; x++) OLED_bits(x, m, c);
}

// Horizontal and vertical lines
void OLED_hline(int16_t x, int16_t y, int16_t w, uint8_t c) { OLED_fillRect(x, y, w, 1, c); }
void OLED_vline(int16_t x, int16_t y, int16_t h, uint8_t c) { OLED_fillRect(x, y, 1, h, c); }

// Rectangle
void OLED_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c) {
  OLED_hline(x, y, w, c);
  OLED_hline(x, y + h - 1, w, c);
  OLED_vline(x, y + 1, h - 2, c);
  OLED_vline(x + w - 1, y + 1, h - 2, c);
}

// Line (Bresenham)
void OLED_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t c) {
  int16_t dx, dy, sx, sy, err, e2;
  if((y0 < OLED_py && y1 < OLED_py) || (y0 >= OLED_py + 8 && y1 >= OLED_py + 8)) return;
  dx = x1 > x0 ? x1 - x0 : x0 - x1;
  dy = y1 > y0 ? y0 - y1 : y1 - y0;
  sx = x0 < x1 ? 1 : -1;
  sy = y0 < y1 ? 1 : -1;
  err = dx + dy;
  while(1) {
    OLED_pixel(x0, y0, c);
    if(x0 == x1 && y0 == y1) break;
    e2 = 2 * err;
    if(e2 >= dy) { err += dy; x0 += sx; }
    if(e2 <= dx) { err += dx; y0 += sy; }
  }
}

// Quarter circles for circles and rounded rectangles
// corners: 1 = top left, 2 = top right, 4 = bottom right, 8 = bottom left
// The quarters of a corner are centered at (x0,y0) and (x0+dx,y0+dy).
static void OLED_corners(int16_t x0, int16_t y0, int16_t dx, int16_t dy, uint8_t r,
                         uint8_t corners, uint8_t fill, uint8_t c) {
  int16_t f = 1 - r, ddx = 1, ddy = -2 * r, x = 0, y = r;
  if(y0 - r >= OLED_py + 8 || y0 + dy + r < OLED_py) return;
  while(x <= y) {
    if(fill) {
      // vertical spans between the upper and the lower quarters
      OLED_vline(x0 + dx + x, y0 - y, 2 * y + 1 + dy, c);
      OLED_vline(x0 + dx + y, y0 - x, 2 * x + 1 + dy, c);
      OLED_vline(x0 - x, y0 - y, 2 * y + 1 + dy, c);
      OLED_vline(x0 - y, y0 - x, 2 * x + 1 + dy, c);
    }
    else {
      if(corners & 1) { OLED_pixel(x0 - y, y0 - x, c); OLED_pixel(x0 - x, y0 - y, c); }
      if(corners & 2) { OLED_pixel(x0 + dx + x, y0 - y, c); OLED_pixel(x0 + dx + y, y0 - x, c); }
      if(corners & 4) { OLED_pixel(x0 + dx + x, y0 + dy + y, c); OLED_pixel(x0 + dx + y, y0 + dy + x, c); }
      if(corners & 8) { OLED_pixel(x0 - y, y0 + dy + x, c); OLED_pixel(x0 - x, y0 + dy + y, c); }
    }
    if(f >= 0) { y--; ddy += 2; f += ddy; }
    x++; ddx += 2; f += ddx;
  }
}

// Circles
void OLED_circle(int16_t x, int16_t y, uint8_t r, uint8_t c) {
  OLED_corners(x, y, 0, 0, r, 15, 0, c);
}
void OLED_fillCircle(int16_t x, int16_t y, uint8_t r, uint8_t c) {
  OLED_corners(x, y, 0, 0, r, 15, 1, c);
}

// Rectangles with rounded corners
void OLED_roundRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r, uint8_t c) {
  OLED_hline(x + r, y, w - 2 * r, c);
  OLED_hline(x + r, y + h - 1, w - 2 * r, c);
  OLED_vline(x, y + r, h - 2 * r, c);
  OLED_vline(x + w - 1, y + r, h - 2 * r, c);
  OLED_corners(x + r, y + r, w - 2 * r - 1, h - 2 * r - 1, r, 15, 0, c);
}
void OLED_fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r, uint8_t c) {
  OLED_fillRect(x + r + 1, y, w - 2 * r - 2, h, c);
  OLED_corners(x + r, y + r, w - 2 * r - 1, h - 2 * r - 1, r, 15, 1, c);
}

// Character (5x8 in a 6x8 cell)
void OLED_char(int16_t x, int16_t y, char ch, uint8_t c) {
  int8_t  off = y - OLED_py;
  uint8_t i, b;
  uint16_t ptr;
  if(off <= -8 || off >= 8) return;
  if((uint8_t)ch >= 0x80) ptr = ((uint8_t)ch - 0x80 + 95) * 5;
  else if(ch >= 32 && ch < 127) ptr = (ch - 32) * 5;
  else ptr = ('?' - 32) * 5;
  for(i = 0; i < 5; i++) {
    b = pgm_read_byte(&OLED_FONT[ptr + i]);
    b = (off >= 0) ? (b << off) : (b >> -off);
    OLED_bits(x + i, b & OLED_mask, c);
  }
}

// String from RAM, returns x after the text
int16_t OLED_text(int16_t x, int16_t y, const char* s, uint8_t c) {
  while(*s) { OLED_char(x, y, *s++, c); x += 6; }
  return x;
}

// String from flash, returns x after the text
int16_t OLED_text_P(int16_t x, int16_t y, const char* s, uint8_t c) {
  char ch;
  while((ch = pgm_read_byte(s++))) { OLED_char(x, y, ch, c); x += 6; }
  return x;
}

// Width of a string in pixels (without the space after the last character)
uint8_t OLED_textWidth(const char* s) {
  uint8_t n = strlen(s);
  return n ? n * 6 - 1 : 0;
}
