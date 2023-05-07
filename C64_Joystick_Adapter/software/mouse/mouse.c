// ===================================================================================
// Project:   1350/1351 Mouse to USB Adapter for CH551, CH552 and CH554
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
// With this adapter, a Commodore mouse (or compatible) can be used on a modern PC 
// as a USB HID mouse.
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
// - Connect the mouse via the 9-pin connector to the board.
// - Connect the board via USB to your PC. It should be detected as a HID mouse.


// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================

// Libraries
#include "src/config.h"                   // user configurations
#include "src/gpio.h"                     // GPIO functions
#include "src/system.h"                   // system functions
#include "src/delay.h"                    // delay functions
#include "src/usb_mouse.h"                // USB HID mouse functions

// Prototypes for used interrupts
void USB_interrupt(void);
void USB_ISR(void) __interrupt(INT_NO_USB) {
  USB_interrupt();
}

// Macros
#define abs(x)  ((x)>0?(x):-(x))

// Disable unnecessary warnings
#pragma disable_warning 84

// ===================================================================================
// Timer0 Functions
// ===================================================================================

// Setup and start Timer0
inline void TIM_init(void) {
  TMOD = bT0_M1;                          // timer0 in overload mode
  TH0  = 0;                               // full 256 cycles = 256 microseconds
  TR0  = 1;                               // start timer
}

// Wait for next timer0 overflow
inline void TIM_wait(void) {
  TF0 = 0;                                // clear timer0 overflow flag
  while(!TF0);                            // wait for next overflow
}

// ===================================================================================
// Main Function
// ===================================================================================
void main(void) {
  // Variables
  __bit buttonleft;                       // left mouse button flag
  __bit buttonright;                      // right mouse button flag
  __bit lastleft  = 1;                    // last left mouse button flag
  __bit lastright = 1;                    // last right mouse button flag
  __idata uint8_t potx, poty;             // POT line measurement variables
  uint8_t potx_last, poty_last;           // save last POT line values
  int8_t  movx, movy;                     // relative mouse movement variables
  int8_t  joyx=0, joyy=0;                 // joystick mouse acceleration

  // Setup
  PIN_output_OD(PIN_POTY);                // POTY pin to open-drain output
  CLK_config();                           // configure system clock
  DLY_ms(10);                             // wait for clock to settle
  MOUSE_init();                           // init USB HID mouse
  DLY_ms(500);                            // wait for Windows...
  TIM_init();                             // init timer0
  WDT_start();                            // start watchdog

  // Loop
  while(1) {
    // Prepare POT measurement
    potx = 0;                             // clear measurement variables
    poty = 0;
    movx = 0;
    movy = 0;

    // Discharge capacitors and SYNC with mouse
    EA = 0;                               // disable interrupts
    TIM_wait();                           // wait for next overflow
    PIN_low(PIN_POTX);                    // discharge capacitor and SYNC
    PIN_low(PIN_POTY);                    // discharge capacitor
    PIN_output_OD(PIN_POTX);              // POTX pin to open-drain output
    TIM_wait();                           // wait 256 microseconds
    PIN_high(PIN_POTX);                   // release POTX line
    PIN_high(PIN_POTY);                   // release POTY line

    // Measuring POT lines
    TF0 = 0;                              // clear timer0 overflow flag
    while(!TF0) {                         // wait 256 microseconds
      if(!potx && PIN_read(PIN_POTX))     // POTX rising LOW to HIGH?
        potx = TL0;                       // save timer value
      if(!poty && PIN_read(PIN_POTY))     // POTY rising LOW to HIGH?
        poty = TL0;                       // save timer value
    }
    PIN_input_PU(PIN_POTX);               // pullup on POTX pin
    EA = 1;                               // enable interrupts

    // Check if proportional or joystick mouse is present
    if(potx && poty) {                    // meaningful POT measurement? -> proportional

      // Calculate mouse movement
      potx &= 127;                        // get the correct bits
      poty &= 127;                        // get the correct bits
      movx = (potx - potx_last) & 127;    // calculate relative mouse movement
      if(movx >= 64) movx -= 128;
      movy = (poty_last - poty) & 127;
      if(movy >= 64) movy -= 128;
      potx_last = potx;                   // save current POT line values
      poty_last = poty;

      // Mouse acceleration
      #ifdef ACCELERATE
      if((abs(movx) > 5) || (abs(movy) > 5)) {
        movx *= 2;
        movy *= 2;
      }
      #endif

      // Move mouse pointer
      if(movx || movy) MOUSE_move(movx, movy);

      // Get left mouse button and send report if changed
      buttonleft = !PIN_read(PIN_FIRE);
      if(buttonleft != lastleft) {
        if(buttonleft) MOUSE_press(MOUSE_BUTTON_LEFT);
        else MOUSE_release(MOUSE_BUTTON_LEFT);
        lastleft = buttonleft;
      }

      // Get right mouse button and send report if changed
      buttonright = !PIN_read(PIN_UP);
      if(buttonright != lastright) {
        if(buttonright) MOUSE_press(MOUSE_BUTTON_RIGHT);
        else MOUSE_release(MOUSE_BUTTON_RIGHT);
        lastright = buttonright;
      }
    }

    else {                                // joystick mouse

      // Calculate mouse movement and send report
      if(!PIN_read(PIN_LEFT))  movx = -1;
      if(!PIN_read(PIN_RIGHT)) movx =  1;
      if(!PIN_read(PIN_UP))    movy = -1;
      if(!PIN_read(PIN_DOWN))  movy =  1;
      if(movx) joyx += movx;
      else if(joyx < 0) joyx++;
      else if(joyx > 0) joyx--;
      if(movy) joyy += movy;
      else if(joyy < 0) joyy++;
      else if(joyy > 0) joyy--;
      if(joyx > 4) joyx = 4; if (joyx < -4) joyx = -4;
      if(joyy > 4) joyy = 4; if (joyy < -4) joyy = -4;
      if(joyx || joyy) MOUSE_move(joyx, joyy);

      // Get left mouse button and send report if changed
      buttonleft = !PIN_read(PIN_FIRE);
      if(buttonleft != lastleft) {
        if(buttonleft) MOUSE_press(MOUSE_BUTTON_LEFT);
        else MOUSE_release(MOUSE_BUTTON_LEFT);
        lastleft = buttonleft;
      }

      // Get right mouse button and send report if changed
      buttonright = !PIN_read(PIN_POTX);
      if(buttonright != lastright) {
        if(buttonright) MOUSE_press(MOUSE_BUTTON_RIGHT);
        else MOUSE_release(MOUSE_BUTTON_RIGHT);
        lastright = buttonright;
      }
    }

    DLY_ms(5);                            // a little delay
    WDT_reset();                          // reset watchdog
  }
}
