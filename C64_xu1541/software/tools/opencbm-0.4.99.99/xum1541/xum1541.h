/*
 * xum1541 firmware defines
 * Copyright (c) 2009-2010 Nate Lawson <nate@root.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */
#ifndef _XUM1541_H
#define _XUM1541_H

#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <util/delay.h>

#include <LUFA/Version.h>
#include <LUFA/Drivers/USB/USB.h>

//#include "xum1541_types.h"      // Version and protocol definitions

// All supported models. Add new ones below.
#define USBKEY                  0
#define BUMBLEB                 1
#define ZOOMFLOPPY              2
#define OLIMEX                  3

#if MODEL == USBKEY
#include "cpu-usbkey.h"
#include "board-usbkey.h"
#elif MODEL == BUMBLEB
#include "cpu-bumbleb.h"
#include "board-zoomfloppy.h"
#elif MODEL == OLIMEX
#include "cpu-bumbleb.h"
#include "board-zoomfloppy.h"
#elif MODEL == ZOOMFLOPPY
#include "cpu-zoomfloppy.h"
#include "board-zoomfloppy.h"
#endif

#include "xum1541_types.h"      // Version and protocol definitions

#ifdef DEBUG
#include <stdio.h>
#define DBG_ERROR   0
#define DBG_INFO    1
#define DBG_ALL     DBG_ERROR
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DBG_ALL
#endif
#define DEBUGF(level, format, args...)      \
    do {                                    \
        if (DEBUG_LEVEL >= level)           \
            printf_P(PSTR(format), ##args); \
    } while (0)
#else
#define DEBUGF(level, format, args...)
#endif

// USB parameters for descriptor configuration
#define XUM_DATA_DIR_NONE       0x0f
#if defined (__AVR_AT90USB1287__)
#define XUM_ENDPOINT_BULK_SIZE  64
#else
#define XUM_ENDPOINT_BULK_SIZE  32
#endif

// Status levels to notify the user (e.g. LEDS)
#define STATUS_INIT             0
#define STATUS_CONNECTING       1
#define STATUS_READY            2
#define STATUS_ACTIVE           3
#define STATUS_ERROR            4

#ifdef TAPE_SUPPORT

// Tape State Register: Current state of tape operations
extern volatile uint8_t TSR;

// CBM 153x tape operations states for TSR (Tape State Register)
#define XUM1541_TAP_DEVICE_CONFIG_READ  0x01 // tape configured for read (1 = true)
#define XUM1541_TAP_DEVICE_CONFIG_WRITE 0x02 // tape configured for write (1 = true)
#define XUM1541_TAP_CAPTURING           0x04 // tape read in progress  (1 = capture, 0 = stop capture)
#define XUM1541_TAP_WRITING             0x08 // tape write in progress (1 = writing, 0 = stop write)
#define XUM1541_TAP_MOTOR               0x10 // last tape MOTOR CONTROL setting remains active (1 = active)
#define XUM1541_TAP_WRITE_STARTFALLEDGE 0x20 // start writing with falling edge (1 = true)
#define XUM1541_TAP_READ_STARTFALLEDGE  0x40 // start reading with falling edge (1 = true)
#define XUM1541_TAP_DISCONNECTED        0x80 // tape device was disconnected (1 = true)

// Restore options after CBM 153x tape operation finished
#define TAPE_CONFIG_OPTION_BASIC      1 // Basic configuration is restored, motor off.
#define TAPE_CONFIG_OPTION_KEEP_MOTOR 2 // Basic configuration is restored, last tape MOTOR CONTROL setting remains active

// Tape status values (must match values in OpenCBM tape applications)
#define Tape_Status_OK                              1
#define Tape_Status_OK_Tape_Device_Present          (Tape_Status_OK + 1)
#define Tape_Status_OK_Tape_Device_Not_Present      (Tape_Status_OK + 2)
#define Tape_Status_OK_Device_Configured_for_Read   (Tape_Status_OK + 3)
#define Tape_Status_OK_Device_Configured_for_Write  (Tape_Status_OK + 4)
#define Tape_Status_OK_Sense_On_Play                (Tape_Status_OK + 5)
#define Tape_Status_OK_Sense_On_Stop                (Tape_Status_OK + 6)
#define Tape_Status_OK_Motor_On                     (Tape_Status_OK + 7)
#define Tape_Status_OK_Motor_Off                    (Tape_Status_OK + 8)
#define Tape_Status_OK_Capture_Finished             (Tape_Status_OK + 9)
#define Tape_Status_OK_Write_Finished               (Tape_Status_OK + 10)
#define Tape_Status_OK_Config_Uploaded              (Tape_Status_OK + 11)
#define Tape_Status_OK_Config_Downloaded            (Tape_Status_OK + 12)
#define Tape_Status_ERROR                           255
#define Tape_Status_ERROR_Device_Disconnected       (Tape_Status_ERROR - 1)
#define Tape_Status_ERROR_Device_Not_Configured     (Tape_Status_ERROR - 2)
#define Tape_Status_ERROR_Sense_Not_On_Record       (Tape_Status_ERROR - 3)
#define Tape_Status_ERROR_Sense_Not_On_Play         (Tape_Status_ERROR - 4)
#define Tape_Status_ERROR_Write_Interrupted_By_Stop (Tape_Status_ERROR - 5)
#define Tape_Status_ERROR_usbSendByte               (Tape_Status_ERROR - 6)
#define Tape_Status_ERROR_usbRecvByte               (Tape_Status_ERROR - 7)
#define Tape_Status_ERROR_External_Break            (Tape_Status_ERROR - 8)
//#define Tape_Status_ERROR_Wrong_Tape_Firmware       (Tape_Status_ERROR - 9) // Only for user mode applications.

#endif // TAPE_SUPPORT

/* specifiers for the IEC lines (must match values from opencbm.h) */
#define IEC_DATA    0x01
#define IEC_CLOCK   0x02
#define IEC_ATN     0x04
#define IEC_RESET   0x08
#define IEC_SRQ     0x80

/* specifiers for the IEEE-488 lines (must match values from opencbm.h) */
#define IEE_NDAC    0x01 // Not data accepted
#define IEE_NRFD    0x02 // Not ready for data
#define IEE_ATN     0x04 // Attention, similar to IEC ATN
#define IEE_IFC     0x08 // Interface clear, similar to IEC RESET
#define IEE_DAV     0x10 // Data valid
#define IEE_EOI     0x20 // End or identify
#define IEE_REN     0x40 // Remote enable (not really used)
#define IEE_SRQ     0x80 // Device service request

// Definition of EEPROM cells used for certain configuration settings
// Please always add new settings at the end of this declaration
enum {
    EEPROM_SerialNumber = 0,
};

extern volatile uint8_t eoi;
extern volatile bool doDeviceReset;
extern volatile bool device_running;

// USB IO functions and command handlers
int8_t usbHandleControl(uint8_t cmd, uint8_t *replyBuf);
int8_t usbHandleBulk(uint8_t *request, uint8_t *status);
bool TimerWorker(void);
void SetAbortState(void);
void USB_ResetConfig(void);
bool USB_ReadBlock(uint8_t *buf, uint8_t len);
bool USB_WriteBlock(uint8_t *buf, uint8_t len);
uint8_t AbortOnReset(void);
void usbInitIo(uint16_t len, uint8_t dir);
void usbIoDone(void);
int8_t usbSendByte(uint8_t data);
int8_t usbRecvByte(uint8_t *data);
void Set_usbDataLen(uint16_t Len);

// IEC functions
#define XUM_WRITE_TALK          (1 << 0)
#define XUM_WRITE_ATN           (1 << 1)
#define IEC_DELAY()             DELAY_US(2) // Time for IEC lines to change

// IEC or IEEE handlers for protocols
struct ProtocolFunctions {
    void (*cbm_reset)(bool forever);
    uint16_t (*cbm_raw_write)(uint16_t len, uint8_t flags);
    uint16_t (*cbm_raw_read)(uint16_t len);
    bool (*cbm_wait)(uint8_t line, uint8_t state);
    uint8_t (*cbm_poll)(void);
    void (*cbm_setrelease)(uint8_t set, uint8_t release);
};

// Global pointer to protocol, set by cbm_init()
extern struct ProtocolFunctions *cmds;

// Initializers for each protocol
struct ProtocolFunctions *cbm_init(void);
struct ProtocolFunctions *iec_init(void);
#ifdef IEEE_SUPPORT
struct ProtocolFunctions *ieee_init(void);
#endif

/*
 * Special protocol handlers:
 * cbm - default CBM serial or IEEE-488
 * s1 - serial
 * s2 - serial
 * p2 - parallel
 * pp - parallel
 * nib - nibbler parallel
 * Tape - 153x tape
 */
uint8_t s1_read_byte(void);
void s1_write_byte(uint8_t c);
uint8_t s2_read_byte(void);
void s2_write_byte(uint8_t c);
uint8_t p2_read_byte(void);
void p2_write_byte(uint8_t c);
void pp_read_2_bytes(uint8_t *c);
void pp_write_2_bytes(uint8_t *c);
uint8_t nib_parburst_read(void);
int8_t nib_read_handshaked(uint8_t *c, uint8_t toggle);
void nib_parburst_write(uint8_t data);
int8_t nib_write_handshaked(uint8_t data, uint8_t toggle);
#ifdef SRQ_NIB_SUPPORT
uint8_t nib_srqburst_read(void);
void nib_srqburst_write(uint8_t data);
uint8_t nib_srq_write_handshaked(uint8_t data, uint8_t toggle);
#endif // SRQ_NIB_SUPPORT
#ifdef TAPE_SUPPORT
uint16_t Tape_GetTapeFirmwareVersion(void); // Return tape firmware version for compatibility check.
uint16_t Tape_UploadConfig(void);           // Upload tape read/write configuration.
uint16_t Tape_DownloadConfig(void);         // Download tape read/write configuration.
uint16_t Tape_PrepareCapture(void);         // Configure for tape capture.
uint16_t Tape_PrepareWrite(void);           // Configure for tape write.
uint16_t Tape_GetSense(void);               // Return tape SENSE state.
uint16_t Tape_WaitForStopSense(void);       // Wait for tape <STOP> if running. Feed watchdog while waiting.
uint16_t Tape_WaitForPlaySense(void);       // Wait for tape <PLAY/RECORD> if stopped. Feed watchdog while waiting.
uint16_t Tape_MotorOn(void);                // Turns the tape drive motor on.
uint16_t Tape_MotorOff(void);               // Turns the tape drive motor off.
uint16_t Tape_Capture(void);                // Tape capture loop. Starts the actual tape capturing.
uint16_t Tape_Write(void);                  // Tape write loop. Starts the actual tape writing.
uint16_t Probe4TapeDevice(void);            // Probe for tape device presence.
void     Tape_Reset(bool whatever);         // External configuration reset. Dummy argument.
void     Enter_Tape_Mode(struct ProtocolFunctions ** protoFn); // Enter tape mode if tape device is detected.
#endif // TAPE_SUPPORT

#endif // _XUM1541_H
