// ===================================================================================
// Project:   C64 Joystick to USB Adapter for CH551, CH552 and CH554
// Version:   v1.2
// Year:      2023
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// With this adapter, a Commodore C64 Joystick can be used on a modern PC as a USB 
// HID game controller.
//
// References:
// -----------
// - Blinkinlabs: https://github.com/Blinkinlabs/ch554_sdcc
// - Deqing Sun: https://github.com/DeqingSun/ch55xduino
// - Ralph Doncaster: https://github.com/nerdralph/ch554_sdcc
// - WCH Nanjing Qinheng Microelectronics: http://wch.cn
//
// Compilation Instructions:
// -------------------------
// - Chip:  CH551, CH552 or CH554
// - Clock: 12 MHz internal
// - Adjust the firmware parameters in src/config.h if necessary.
// - Make sure SDCC toolchain and Python3 with PyUSB is installed.
// - Press BOOT button on the board and keep it pressed while connecting it via USB
//   with your PC.
// - Run 'make flash' immediatly afterwards.
// - To compile the firmware using the Arduino IDE, follow the instructions in the 
//   .ino file.
//
// Operating Instructions:
// -----------------------
// - Connect the joystick via the 9-pin connector to the board.
// - Connect the board via USB to your PC. It should be detected as a HID joystick.


// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================

// Libraries
#include "src/config.h"                   // user configurations
#include "src/gpio.h"                     // GPIO functions
#include "src/system.h"                   // system functions
#include "src/delay.h"                    // delay functions
#include "src/usb_joystick.h"             // USB HID joystick functions

// Prototypes for used interrupts
void USB_interrupt(void);
void USB_ISR(void) __interrupt(INT_NO_USB) {
  USB_interrupt();
}

// ===================================================================================
// Main Function
// ===================================================================================
void main(void) {
  // Setup
  CLK_config();                           // configure system clock
  DLY_ms(10);                             // wait for clock to settle
  JOY_init();                             // init USB HID joystick
  DLY_ms(500);                            // wait for Windows...
  WDT_start();                            // start watchdog

  // Loop
  while(1) {
    JOY_report->buttons = PIN_read(PIN_FIRE) ? 0 : 1;
    JOY_report->xmove = 127;
    JOY_report->ymove = 127;
    if(!PIN_read(PIN_LEFT))  JOY_report->xmove = 0;
    if(!PIN_read(PIN_RIGHT)) JOY_report->xmove = 255;
    if(!PIN_read(PIN_UP))    JOY_report->ymove = 0;
    if(!PIN_read(PIN_DOWN))  JOY_report->ymove = 255;
    JOY_sendReport();

    //DLY_ms(10);
    WDT_reset();                          // reset watchdog
  }
}
