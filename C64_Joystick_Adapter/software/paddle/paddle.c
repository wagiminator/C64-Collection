// ===================================================================================
// Project:   C64 Paddle to USB Adapter for CH551, CH552 and CH554
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
// With this adapter, a Commodore paddle can be used on a modern PC as a USB HID
// game controller.
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
// - Connect the paddle via the 9-pin connector to the board.
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

// Disable unnecessary warnings
#pragma disable_warning 84


// ===================================================================================
// Main Function
// ===================================================================================
void main(void) {
  // Variables
  __idata uint8_t potx, poty;             // POT line measurement variables

  // Setup
  PIN_output_OD(PIN_POTX);                // POT pins to open-drain output
  PIN_output_OD(PIN_POTY);                // POT pins to open-drain output
  CLK_config();                           // configure system clock
  DLY_ms(10);                             // wait for clock to settle
  JOY_init();                             // init USB HID joystick
  DLY_ms(500);                            // wait for Windows...
  T2MOD = bTMR_CLK | bT0_CLK;             // set timer clock to Fsys
  WDT_start();                            // start watchdog

  // Loop
  while(1) {
    // Discharge capacitors
    PIN_low(PIN_POTX);                    // discharge capacitor
    PIN_low(PIN_POTY);                    // discharge capacitor
    DLY_us(5);                            // a little time to discharge

    // Prepare POT measurement
    EA = 0;                               // disable interrupts
    potx = 0; poty = 0;                   // clear measurement variables
    TH0 = 0; TL0 = 0; TF0 = 0; TR0 = 1;   // clear and start timer
    PIN_high(PIN_POTX);                   // release POTX line
    PIN_high(PIN_POTY);                   // release POTY line
    
    // Measuring POT lines
    while(!TF0) {                         // for a complete timer period...
      if(!potx && PIN_read(PIN_POTX))     // POTX rising LOW to HIGH?
        potx = TH0;                       // save timer value
      if(!poty && PIN_read(PIN_POTY))     // POTY rising LOW to HIGH?
        poty = TH0;                       // save timer value
    }
    EA = 1;                               // enable interrupts
    TR0 = 0;                              // stop timer

    // Prepare and send HID report
    potx -= 1; poty -= 1;                 // adjust overflow
    JOY_report->xmove = potx;             // set paddle 1 as x-axis
    JOY_report->ymove = poty;             // set paddle 2 as y-axis
    JOY_report->buttons = 0;              // clear button flags
    if(!PIN_read(PIN_LEFT))  JOY_report->buttons |= 1;  // paddle 1 button
    if(!PIN_read(PIN_RIGHT)) JOY_report->buttons |= 2;  // paddle 2 button
    JOY_sendReport();

    //DLY_ms(10);                           // a little delay
    WDT_reset();                          // reset watchdog
  }
}
