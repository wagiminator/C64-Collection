// ===================================================================================
// User configurations
// ===================================================================================

#pragma once

// Pin definitions
#define PIN_LED             P14       // pin connected to LED, active low
#define PIN_ATN             P15       // pin connected to IEC ATN (attention)
#define PIN_DATA            P16       // pin connected to IEC DATA
#define PIN_CLK             P17       // pin connected to IEC CLK (clock)

// Firmware parameters
#define VERSION             "1.1"     // version number sent via serial if requested
#define IDENT      "DiskMaster64"     // identifier sent via serial if requested

// USB device descriptor
#define USB_VENDOR_ID       0x16C0    // VID (shared www.voti.nl)
#define USB_PRODUCT_ID      0x27DD    // PID (shared CDC)
#define USB_DEVICE_VERSION  0x0100    // v1.0 (BCD-format)

// USB configuration descriptor
#define USB_MAX_POWER_mA    100       // max power in mA 

// USB descriptor strings
#define MANUFACTURER_STR    'w','a','g','i','m','i','n','a','t','o','r'
#define PRODUCT_STR         'D','i','s','k','M','a','s','t','e','r','6','4'
#define SERIAL_STR          'g','i','t','h','u','b','.','c','o','m','/', \
                            'w','a','g','i','m','i','n','a','t','o','r'
#define INTERFACE_STR       'C','D','C',' ','S','e','r','i','a','l'
