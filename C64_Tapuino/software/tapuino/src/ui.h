// ===================================================================================
// User Interface: Screens of the Tapuino                                     * v1.0 *
// ===================================================================================
//
// Draw functions (for OLED_render() / OLED_renderStep()):
// --------------------------------------------------------
// UI_drawCassette()        Cassette with the file name on the label, the tape packs on
//                          the reels and a progress bar in the tape window, status
//                          text in the head opening at the bottom
// UI_drawMenu()            List with title bar and selection (main menu, settings)
// UI_drawName()            Entry of a file name
// UI_drawMessage()         Title bar with two centered lines of text
//
// Other functions available:
// --------------------------
// UI_setLabel(name, dir)   Set the text on the cassette label (dir: folder symbol)
// UI_scrollLabel()         Move a long label text by one step, returns 1 if it moved
//
// Notes:
// ------
// - The screens take their content from the structures UI_tape, UI_menu, UI_name and
//   UI_msg, which are filled by the application before drawing.
//
// 2026 by Stefan Wagner:   https://github.com/wagiminator

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "oled.h"

// Cassette screen
typedef struct {
  const char* name;                     // text on the label
  uint8_t folder;                       // 1: label shows a folder
  uint8_t progress;                     // tape position 0..255 (packs and window)
  uint8_t bar;                          // 1: show the progress bar in the window
  uint8_t angle;                        // reel rotation 0..35 (steps of 10 degrees)
  char    status[13];                   // text in the head opening (max. 11 chars)
} UI_tape_t;

// Menu screen
typedef struct {
  const char* title;                    // title (flash)
  uint8_t count;                        // number of items
  uint8_t sel;                          // selected item
  uint8_t first;                        // first visible item
  void (*item)(uint8_t i, char* buf);   // writes the text of item i into buf (max. 20)
} UI_menu_t;

// Name entry screen
typedef struct {
  char    name[9];                      // name (8 characters max.)
  uint8_t pos;                          // cursor position
} UI_name_t;

// Message screen
typedef struct {
  const char* title;                    // title (flash)
  const char* line1;                    // first line (flash)
  const char* line2;                    // second line (RAM, may be 0)
} UI_msg_t;

extern UI_tape_t UI_tape;
extern UI_menu_t UI_menu;
extern UI_name_t UI_name;
extern UI_msg_t  UI_msg;

void UI_drawCassette(void);
void UI_drawMenu(void);
void UI_drawName(void);
void UI_drawMessage(void);
void UI_setLabel(const char* name, uint8_t folder);
uint8_t UI_scrollLabel(void);

#ifdef __cplusplus
};
#endif
