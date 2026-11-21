// ===================================================================================
// User Configuration
// ===================================================================================
//
// All settings that depend on the hardware or that change the behaviour of the
// firmware are collected here. The defaults match the SD2IEC_LP board:
// ATmega644PA, 8MHz crystal, IEC bus on PC0..PC2 (direct, open collector emulation).

#pragma once

// ===================================================================================
// IEC Bus (serial bus to the computer)
// ===================================================================================
// ATN, CLOCK and DATA must be on the same port (the bus is read with one access).
// The lines are driven open collector style: input = released, output low = pulled.
#define PIN_IEC_ATN       PC0       // ATN   (input, pin change interrupt)
#define PIN_IEC_DATA      PC1       // DATA
#define PIN_IEC_CLOCK     PC2       // CLOCK

// ===================================================================================
// SD Card
// ===================================================================================
// The SD card uses the hardware SPI (MOSI PB5, MISO PB6, SCK PB7).
#define PIN_SD_CS         PB4       // chip select
#define PIN_SD_DETECT     PD2       // card detect switch (low = card inserted)
#define PIN_SD_WP         PD6       // write protect (high = protected), comment out if n/a
#define SD_FAST_CLOCK     0         // 0: SPI F_CPU/4 (2MHz, safe with the resistor
                                    //    level shifters), 1: F_CPU/2 (4MHz)

// ===================================================================================
// LEDs, Buttons and Device Address Switches
// ===================================================================================
#define PIN_LED_BUSY      PA0       // BUSY  LED (green): drive is working
#define PIN_LED_ERROR     PA1       // ERROR LED (red): error / unsaved data
#define LED_ACTIVE_LOW    1         // 1: LEDs light up when the pin is low

#define PIN_KEY_NEXT      PA5       // NEXT button: next disk image of the swap list
#define PIN_KEY_PREV      PA4       // PREV button: previous disk image
                                    // (both together: first disk image)
#define PIN_DIP_1         PA2       // DIP switch 1: device address + 1
#define PIN_DIP_2         PA3       // DIP switch 2: device address + 2
#define DEVICE_ADDR_BASE  8         // device address with both switches off

// ===================================================================================
// Disk Operating System
// ===================================================================================
#define BUFFER_COUNT      6         // number of 256 byte data buffers (channels)
#define COMMAND_LENGTH    120       // max. length of DOS commands and file names
#define DIR_LABEL         "SD2IEC"  // name of the disk in FAT directories (no label)
#define DIR_ID            "SD"      // ID shown in FAT directories
#define SWAP_LIST_NAME    "AUTOSWAP.LST" // swap list used if none is set
#define DEF_EXT_MODE      1         // extension mode (XE): 0..4, see README
#define DEF_EXT_HIDING    0         // 1: hide .PRG/.SEQ/.USR/.REL extensions (XE+)

// ===================================================================================
// Fast Loaders (1 = enabled)
// ===================================================================================
#define FL_JIFFYDOS       1         // JiffyDOS (incl. JiffyDOS LOAD protocol)
#define FL_TURBODISK      1         // Turbodisk
#define FL_FC3            1         // Final Cartridge III (LOAD, freezer, EXOS)
#define FL_DREAMLOAD      1         // DreamLoad (games by Dreamload / OCEAN)
#define FL_ULOAD3         1         // ULoad Model 3 (Ninja / Covert Bitops games)
#define FL_GIJOE          1         // GI Joe loader
#define FL_EPYXCART       1         // Epyx FastLoad cartridge
#define FL_GEOS           1         // GEOS disk drivers (1541, 1571, 1581)
#define FL_WHEELS         1         // Wheels disk drivers (needs FL_GEOS)
#define FL_AR6            1         // Action Replay 6
#define FL_ELOAD1         1         // ELoad version 1 (EasyProg)
#define FL_NIPPON         1         // Nippon loader
#define FL_MMZAK          1         // Maniac Mansion / Zak McKracken
#define FL_N0SDOS         1         // N0stalgia fastloader
#define FL_SAMSJOURNEY    1         // Sam's Journey

// ===================================================================================
// Firmware Update (newboot bootloader)
// ===================================================================================
// A firmware file (*.bin) in the root of the SD card is flashed by the bootloader
// at power-up if its device ID matches and its version differs.
#define BOOT_DEVID        0x5053524C // "LRSP": device ID of the LarsP hardware
