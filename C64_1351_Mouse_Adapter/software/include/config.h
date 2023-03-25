// ===================================================================================
// User Configurations for 1351 Mouse to USB Adapter
// ===================================================================================

#pragma once

// Pin definitions
#define PIN_POTX            P14       // pin connected to POTX
#define PIN_POTY            P15       // pin connected to POTY
#define PIN_LEFT            P16       // pin connected to left mouse button
#define PIN_RIGHT           P17       // pin connected to right mouse button

// Mouse acceleration (uncomment to activate)
//#define ACCELERATE

// USB device descriptor
#define USB_VENDOR_ID       0x16C0    // VID (shared www.voti.nl)
#define USB_PRODUCT_ID      0x27DA    // PID (shared HID-mice)
#define USB_DEVICE_VERSION  0x0100    // v1.0 (BCD-format)

// USB configuration descriptor
#define USB_MAX_POWER_mA    50        // max power in mA 

// USB descriptor strings
#define MANUFACTURER_STR    'w','a','g','i','m','i','n','a','t','o','r'
#define PRODUCT_STR         '1','3','5','1',' ','M','o','u','s','e'
#define SERIAL_STR          'C','H','5','5','x','1','3','5','1'
#define INTERFACE_STR       'H','I','D','-','M','o','u','s','e'
