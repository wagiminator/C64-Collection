// ===================================================================================
// SSD1306/SH1106 128x64 I2C OLED Graphics with Page Rendering                * v1.0 *
// ===================================================================================
//
// The ATmega328P has not enough RAM for a frame buffer (1024 bytes). Instead, the
// picture is drawn page by page: a drawing function is called once for each of the
// 8 pages (8 pixel rows each), all drawing operations only change the pixels of the
// current page in a 128-byte buffer, then the page is sent to the display.
//
// Display functions available:
// ----------------------------
// OLED_init()                    Init OLED
// OLED_contrast(v)               Set contrast (0-255)
// OLED_render(draw)              Draw the whole screen with the function draw()
// OLED_renderStep(draw)          Draw one page; returns 1 when the screen is complete
//                                (spreads the work over several calls)
//
// Drawing functions available (only inside the draw function):
// ------------------------------------------------------------
// OLED_clip(x0,y0,x1,y1)         Restrict drawing to the rectangle (inclusive)
// OLED_noclip()                  Remove the restriction
// OLED_pixel(x,y,c)              Set pixel at (x,y) with color c
// OLED_hline(x,y,w,c)            Horizontal line from (x,y) with width w
// OLED_vline(x,y,h,c)            Vertical line from (x,y) with height h
// OLED_line(x0,y0,x1,y1,c)       Line from (x0,y0) to (x1,y1)
// OLED_rect(x,y,w,h,c)           Rectangle
// OLED_fillRect(x,y,w,h,c)       Filled rectangle
// OLED_roundRect(x,y,w,h,r,c)    Rectangle with rounded corners (radius r)
// OLED_fillRoundRect(x,y,w,h,r,c) Filled rectangle with rounded corners
// OLED_circle(x,y,r,c)           Circle with center (x,y) and radius r
// OLED_fillCircle(x,y,r,c)       Filled circle
// OLED_char(x,y,ch,c)            Character (5x8 font in a 6x8 cell)
// OLED_text(x,y,s,c)             String from RAM, returns x after the text
// OLED_text_P(x,y,s,c)           String from flash, returns x after the text
// OLED_textWidth(s)              Width of a string in pixels
//
// Notes:
// ------
// - Color c: 0 = clear pixel (black), 1 = set pixel (white), 2 = invert pixel
// - Coordinates may lie outside the screen, everything is clipped.
// - Characters 0x80.. are symbols (see OLED_SYM_...).
//
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "system.h"

#define OLED_WIDTH      128
#define OLED_HEIGHT     64

// Symbols in the font
#define OLED_SYM_PLAY   "\x80"          // play triangle
#define OLED_SYM_REC    "\x81"          // record dot
#define OLED_SYM_PAUSE  "\x82"          // pause bars
#define OLED_SYM_FOLDER "\x83"          // folder
#define OLED_SYM_LEFT   "\x84"          // triangle left
#define OLED_SYM_RIGHT  "\x85"          // triangle right
#define OLED_SYM_STOP   "\x86"          // stop square
#define OLED_SYM_GEAR   "\x87"          // settings

// Display functions
void OLED_init(void);
void OLED_contrast(uint8_t v);
void OLED_render(void (*draw)(void));
uint8_t OLED_renderStep(void (*draw)(void));

// Drawing functions
void OLED_clip(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
void OLED_noclip(void);
void OLED_pixel(int16_t x, int16_t y, uint8_t c);
void OLED_hline(int16_t x, int16_t y, int16_t w, uint8_t c);
void OLED_vline(int16_t x, int16_t y, int16_t h, uint8_t c);
void OLED_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t c);
void OLED_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c);
void OLED_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c);
void OLED_roundRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r, uint8_t c);
void OLED_fillRoundRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t r, uint8_t c);
void OLED_circle(int16_t x, int16_t y, uint8_t r, uint8_t c);
void OLED_fillCircle(int16_t x, int16_t y, uint8_t r, uint8_t c);
void OLED_char(int16_t x, int16_t y, char ch, uint8_t c);
int16_t OLED_text(int16_t x, int16_t y, const char* s, uint8_t c);
int16_t OLED_text_P(int16_t x, int16_t y, const char* s, uint8_t c);
uint8_t OLED_textWidth(const char* s);

#ifdef __cplusplus
};
#endif
