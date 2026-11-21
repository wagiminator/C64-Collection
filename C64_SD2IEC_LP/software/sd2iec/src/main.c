// ===================================================================================
// Project:   SD2IEC - Commodore Floppy Disk Drive Emulator
// Version:   v1.0
// Year:      2026
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   GNU General Public License v2 (only)
// ===================================================================================
//
// Description:
// ------------
// The SD2IEC connects to the serial bus (IEC) of a Commodore C64, C128, C16, Plus/4
// or VIC-20 and acts like a 1541 floppy drive with an SD card instead of a disk.
// Programs are loaded and saved as files on the FAT formatted card, disk images
// (D64/D71/D81) can be mounted like disks. JiffyDOS and many fast loaders are
// supported. The buttons change the disk image of multi-disk games.
//
// This firmware is a complete rewrite of sd2iec by Ingo Korb and contributors
// (https://www.sd2iec.de), whose work it is based on. It is licensed under the
// GNU General Public License version 2 like the original.
//
// Compilation Instructions:
// -------------------------
// - Adjust the settings in config.h if necessary.
// - Make sure GCC toolchain (gcc-avr, avr-libc) and avrdude are installed.
// - Connect an ISP programmer (e.g. USBasp) to the ICSP header of the SD2IEC.
// - Run 'make fuses' once, then 'make flash' (or 'make bin' for a firmware update
//   file that the newboot bootloader flashes from the SD card).


// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================
#include "system.h"             // system functions
#include "gpio.h"               // GPIO functions
#include "dos.h"                // disk operating system
#include "iec.h"                // IEC bus
#include "led.h"                // LEDs and buttons
#include "settings.h"           // settings in the EEPROM
#include "../config.h"          // user configuration

// ===================================================================================
// System Tick and Idle Loop
// ===================================================================================

// Called every millisecond by the system tick
void MIL_callback(void) {
  LED_tick();
}

// Device address set by the DIP switches
uint8_t device_dip_address(void) {
  return DEVICE_ADDR_BASE + (PIN_read(PIN_DIP_1) ? 0 : 1) + (PIN_read(PIN_DIP_2) ? 0 : 2);
}

// Sleep mode: the device ignores the bus until a button is held again for 2s
// (allows fast loaders of other drives that need the bus for themselves)
static void main_sleep(void) {
  set_atn_irq(0);                               // release the bus
  set_data(1);
  set_clock(1);
  LED_busy(0);
  LED_dirty(1);
  while(KEY_get() != KEY_SLEEP);
  LED_dirty(0);
  set_error(ERROR_OK);
  set_atn_irq(1);
}

// Called repeatedly while the bus is idle: buttons and card changes
void main_idle(void) {
  static uint8_t card = 2;                      // card state seen last (2: unknown)
  static uint16_t since;
  uint8_t key, present = !PIN_read(PIN_SD_DETECT);

  if(card == 2) card = present;                 // state at power-up (mounted by main)

  // card inserted or removed (debounced 100ms)
  if(present != card) {
    if((uint16_t)(MIL_read() - since) >= 100) {
      card = present;
      buffers_free(1);
      swap_reset();
      if(present) fs_init();
      else {
        image_mounted = 0;
        FAT_unmount();
        disk_ready = 0;
        LED_dirty(0);
      }
      disk_changed = 0;
      set_error(present && !disk_ready ? ERROR_DRIVE_NOT_READY : ERROR_OK);
    }
  }
  else since = MIL_read();

  // buttons: change the disk image, sleep mode
  key = KEY_get();
  if(key == KEY_SLEEP) main_sleep();
  else if(key) swap_change(key);
}

// ===================================================================================
// Main Function
// ===================================================================================
int main(void) {
  // Setup
  WDT_stop();                                   // stop watchdog after a reset
  MCUCR = (1<<JTD);                             // disable JTAG (frees PC2..PC5),
  MCUCR = (1<<JTD);                             // must be written twice
  LED_init();                                   // LEDs and buttons
  PIN_input(PIN_DIP_1); PIN_pullup(PIN_DIP_1);  // device address switches
  PIN_input(PIN_DIP_2); PIN_pullup(PIN_DIP_2);
  PIN_input(PIN_SD_DETECT); PIN_pullup(PIN_SD_DETECT);
#ifdef PIN_SD_WP
  PIN_input(PIN_SD_WP); PIN_pullup(PIN_SD_WP);
#endif
  iec_init();                                   // bus lines and ATN interrupt
  MIL_init();                                   // system tick
  TMR_init();                                   // microsecond timeouts
  INT_enable();
  DLY_ms(1);
  device_address = device_dip_address();
  settings_load();                              // stored settings (XW)
  buffers_init();
  // Card detect test: PREV held at power-up, the ERROR LED shows the switch
  while(!PIN_read(PIN_KEY_PREV)) LED_dirty(!PIN_read(PIN_SD_DETECT));
  LED_dirty(0);
  KEY_get();                                    // ignore that button press

  LED_busy(1);
  if(!PIN_read(PIN_SD_DETECT)) fs_init();       // mount the card
  LED_busy(0);

  // Loop
  iec_mainloop();
}
