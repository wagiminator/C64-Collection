// ===================================================================================
// User Configuration for the Tapuino Firmware
// ===================================================================================
//
// All settings that depend on the hardware or on personal preferences are collected
// here. The settings menu of the device (machine, video standard, signal polarity,
// automatic stop of recordings, display contrast) is stored in the EEPROM and starts
// with the default values given below.
//
// Pins are named PB0..PB7, PC0..PC7, PD0..PD7.

#pragma once

// ===================================================================================
// Pin Assignments
// ===================================================================================

// Buttons (to GND, internal pull-ups are used)
#define PIN_KEY_NEXT      PC0           // NEXT     (next file / value up)
#define PIN_KEY_PREV      PC1           // PREV     (previous file / value down)
#define PIN_KEY_BACK      PC2           // BACK     (back / abort)
#define PIN_KEY_SELECT    PC3           // SELECT   (select / start / pause)

// Cassette port of the Commodore
#define PIN_TAPE_READ     PD3           // READ  to the computer (output)
#define PIN_TAPE_WRITE    PB0           // WRITE from the computer (input, must be ICP1 = PB0)
#define PIN_TAPE_MOTOR    PD4           // MOTOR from the computer via optocoupler (low = on)
#define PIN_TAPE_SENSE    PD5           // SENSE to the computer (open collector, low = key pressed)

// SD card (SPI: MOSI = PB3, MISO = PB4, SCK = PB5)
#define PIN_SD_CS         PB2           // chip select of the SD card

// Optional recording LED (comment out if not present)
//#define PIN_LED_REC     PD2           // LED is on while recording
#define LED_REC_ACTIVE    0             // level that switches the LED on (0: low, 1: high)

// OLED: SDA = PC4, SCL = PC5 (hardware I2C)

// ===================================================================================
// Display
// ===================================================================================
#define OLED_ADDR         0x3C          // I2C address of the OLED (0x3C or 0x3D)
#define OLED_SH1106       0             // 0: SSD1306 0.96", 1: SH1106 1.3" (column offset)
#define OLED_FLIP         0             // 1: rotate the picture by 180 degrees
#define I2C_CLOCK         400000        // I2C clock rate in Hz

// ===================================================================================
// SD Card and Files
// ===================================================================================
#define SD_FAST_CLOCK     1             // SPI clock after init: 1: 8MHz, 0: 4MHz (for long wires)
#define REC_FOLDER        "RECORD"      // folder for recordings (max. 8 characters, A-Z 0-9 _ -)
#define REC_PREFIX        "REC_"        // name of automatic recordings: REC_0001.TAP ...

// ===================================================================================
// Default Settings (until changed in the settings menu)
// ===================================================================================
#define DEF_MACHINE       0             // machine for recordings: 0: C64, 1: VIC-20, 2: C16/Plus4
#define DEF_VIDEO         0             // video standard for recordings: 0: PAL, 1: NTSC
#define DEF_INVERT        0             // signal polarity: 0: normal, 1: inverted (real Datasette)
#define DEF_AUTOSTOP      3             // stop recording n seconds after the motor stops (0: off)
#define DEF_CONTRAST      2             // display contrast: 0 (dim) .. 3 (bright)

// ===================================================================================
// Buttons
// ===================================================================================
#define KEY_REPEAT_DELAY  450           // NEXT/PREV: time until auto-repeat starts (ms)
#define KEY_REPEAT_RATE   110           // NEXT/PREV: time between repeats (ms)
#define KEY_LONG_PRESS    700           // SELECT/BACK: time for a long press (ms)
