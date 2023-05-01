// ===================================================================================
// IEC Bus Communication Functions
// ===================================================================================

#pragma once
#include <stdint.h>
#include "config.h"
#include "gpio.h"

// The weak pullup resistors need some time to pull the line HIGH
#define IEC_PULLUP          4                     // pullup time in us

// IEC bitbanging macros
#define IEC_ATN_setHigh()   PIN_high(PIN_ATN)     // ATN  released    -> FALSE
#define IEC_ATN_setLow()    PIN_low(PIN_ATN)      // ATN  pulled down -> TRUE
#define IEC_CLK_setHigh()   PIN_high(PIN_CLK)     // CLK  released    -> FALSE
#define IEC_CLK_setLow()    PIN_low(PIN_CLK)      // CLK  pulled down -> TRUE
#define IEC_DATA_setHigh()  PIN_high(PIN_DATA)    // DATA released    -> FALSE
#define IEC_DATA_setLow()   PIN_low(PIN_DATA)     // DATA pulled down -> TRUE

#define IEC_CLK_isHigh()    PIN_read(PIN_CLK)     // check if CLK  is HIGH / FALSE
#define IEC_CLK_isLow()     !PIN_read(PIN_CLK)    // check if CLK  is LOW  / TRUE
#define IEC_DATA_isHigh()   PIN_read(PIN_DATA)    // check if DATA is HIGH / FALSE
#define IEC_DATA_isLow()    !PIN_read(PIN_DATA)   // check if DATA is LOW  / TRUE

// IEC serial bus control codes
#define IEC_LISTEN          0x20
#define IEC_UNLISTEN        0x3F
#define IEC_TALK            0x40
#define IEC_UNTALK          0x5F
#define IEC_OPEN_CH         0x60
#define IEC_CLOSE           0xE0
#define IEC_OPEN            0xF0

// Global flags
extern __bit IEC_error;               // IEC error flag
extern __bit IEC_EOI;                 // IEC End-Of-Information flag
extern __xdata uint8_t IEC_device;    // Current IED device number

// IEC Implementation - Setup
void IEC_init(void);                  // IEC init

// IEC Protocol Implementation - Low Level Bitbanging Stuff
void IEC_release(void);               // Release all control lines
__bit IEC_sendByte(uint8_t data);     // Send a data byte
void IEC_sendLast(uint8_t data);      // Send last data byte in sequence
uint8_t IEC_readByte(void);           // Read a byte
__bit IEC_ATN_start(void);            // Start sending under attention
void IEC_ATN_stop(void);              // Stop sending under attention
void IEC_turnaround(void);            // Perform turnaround

// IEC Protocol Implementation - Basic Control Functions
__bit IEC_listen(uint8_t device, uint8_t secondary);  // Send 'LISTEN' command to the bus
__bit IEC_unlisten(void);                             // Send 'UNLISTEN' command to the bus
__bit IEC_talk(uint8_t device, uint8_t secondary);    // Send 'TALK' and perform turnaround
__bit IEC_untalk(void);                               // Send 'UNTALK' command to the bus

// IEC Protocol Implementation - Raw Data Transmission Functions
__bit IEC_sendStr(__xdata uint8_t* str);                      // Write a string to IEC bus
__bit IEC_writeBuffer(__xdata uint8_t* buf, uint8_t count);   // Send buffer via IEC bus

// IEC Protocol Implementation - Fast Asynchronous IEC Data Transmissions
uint8_t IEC_readAsynch(void);         // Read byte via fast IEC
void IEC_writeAsynch(uint8_t data);   // Write byte via fast IEC
