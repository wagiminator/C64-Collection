// ===================================================================================
// USB HID Standard Joystick Functions for CH551, CH552 and CH554
// ===================================================================================

#pragma once
#include <stdint.h>
#include "usb_hid.h"

// HID report typedef
typedef struct _HID_JOY_REPORT_TYPE {
  uint8_t xmove;                      // movement on the x-axis
  uint8_t ymove;                      // movement on the y-axis
  uint8_t buttons;                    // button states
} HID_JOY_REPORT_TYPE, *PHID_JOY_REPORT_TYPE;

// HID report
__xdata HID_JOY_REPORT_TYPE HID_report;
#define JOY_report        ((PHID_JOY_REPORT_TYPE)HID_report)

// Functions
#define JOY_init()        HID_init()
#define JOY_sendReport()  HID_sendReport((uint8_t*)&HID_report, sizeof(HID_report))
