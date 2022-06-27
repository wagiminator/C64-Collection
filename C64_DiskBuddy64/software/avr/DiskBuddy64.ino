// ===================================================================================
// Project:   DiskBuddy64 - USB to Commodore Floppy Disk Drive Adapter
// Version:   v1.5
// Year:      2022
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// Firmware for the DiskBuddy64 adaper. The DiskBuddy64 adapter bridges the gap
// between your PC and the Commodore 1541(II) floppy disk drives. It communicates
// with the floppy disk drives via bit banging of the IEC protocol and with the PC
// via the integrated USB-to-serial converter. The adapter also supports a
// proprietary IEC protocol for faster data transmission.
//
// References:
// -----------
// Michael Steil: https://www.pagetable.com/?p=568
//
// Wiring:
// -------
//                        +-\/-+
//                  Vdd  1|°   |8  GND
//  USB RXD --- TXD PA6  2|    |7  PA3 AIN3 -------- IEC CLK
//  USB TXD --- RXD PA7  3|    |6  PA0 AIN0 UPDI --- UPDI
// IEC DATA --- SDA PA1  4|    |5  PA2 AIN2 SCL ---- IEC ATN
//                        +----+
//
// Compilation Settings:
// ---------------------
// Core:    megaTinyCore (https://github.com/SpenceKonde/megaTinyCore)
// Board:   ATtiny412/402/212/202
// Chip:    Choose the chip you use
// Clock:   16 MHz internal
//
// Leave the rest on default settings. Select "SerialUPDI" as programmer in the
// Arduino IDE and set the selector switch on the board to "UPDI". Don't forget
// to "Burn bootloader"! Compile and upload the code.
//
// No Arduino core functions or libraries are used. To compile and upload without
// Arduino IDE download AVR 8-bit toolchain at:
// https://www.microchip.com/mplab/avr-support/avr-and-arm-toolchains-c-compilers
// and extract to tools/avr-gcc. Use the makefile to compile and upload.
//
// Fuse Settings: 0:0x00 1:0x00 2:0x01 4:0x00 5:0xC5 6:0x04 7:0x00 8:0x00
//
// Operating Instructions:
// -----------------------
// - Set the serial mode switch on your DiskBuddy64 adapter to "UART".
// - Connect the adapter to your Commodore 1541(II) floppy disk drive via an IEC cable.
// - Connect the adapter to your PC via a USB cable.
// - Switch on your floppy disk drive.
// - Use the provided Python scripts on your PC.


// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================

// Libraries
#include <avr/io.h>               // for GPIO
#include <avr/interrupt.h>        // for interrupts
#include <util/delay.h>           // for delays

// Pin definitions
#define PIN_DATA      PA1         // pin connected to IEC DATA
#define PIN_ATN       PA2         // pin connected to IEC ATN (attention)
#define PIN_CLK       PA3         // pin connected to IEC CLK (clock)
#define PIN_TXD       PA6         // UART TX pin connected to usb-to-serial converter
#define PIN_RXD       PA7         // UART RX pin connected to usb-to-serial converter

// Configuration parameters
#define UART_BAUD     460800      // UART: baud rate (max 1/16 of F_CPU)
#define CMD_BUF_LEN   64          // command buffer length (don't change)

// Identifiers
#define VERSION     "1.5"         // version number sent via serial if requested
#define IDENT       "DiskBuddy64" // identifier sent via serial if requested

// Pin manipulation macros
enum {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7};      // enumerate pin designators
#define pinInput(x)       VPORTA.DIR &= ~(1<<(x))   // set pin to INPUT
#define pinOutput(x)      VPORTA.DIR |=  (1<<(x))   // set pin to OUTPUT
#define pinLow(x)         VPORTA.OUT &= ~(1<<(x))   // set pin to LOW
#define pinHigh(x)        VPORTA.OUT |=  (1<<(x))   // set pin to HIGH
#define pinToggle(x)      VPORTA.IN  |=  (1<<(x))   // TOGGLE pin
#define pinRead(x)        (VPORTA.IN &   (1<<(x)))  // READ pin
#define pinPullup(x)      (&PORTA.PIN0CTRL)[x] |= PORT_PULLUPEN_bm  // enable pullup

// ===================================================================================
// UART Implementation (8N1, no calibration, no buffer, no interrupts)
// ===================================================================================

// UART definitions and macros
#define UART_BAUD_RATE    4.0 * F_CPU / UART_BAUD + 0.5
#define UART_send(x)      USART0.TXDATAL = (x)
#define UART_ready()      (USART0.STATUS & USART_DREIF_bm)
#define UART_available()  (USART0.STATUS & USART_RXCIF_bm)

// UART init
void UART_init(void) {
  pinOutput(PIN_TXD);                               // set TX pin as output
  USART0.BAUD   = UART_BAUD_RATE;                   // set BAUD
  USART0.CTRLB  = USART_RXEN_bm                     // enable RX
                | USART_TXEN_bm;                    // enable TX
}

// UART receive data byte
uint8_t UART_read(void) {
  while(!UART_available());                         // wait until received something
  return USART0.RXDATAL;                            // read and return received data
}

// UART transmit data byte
void UART_write(uint8_t data) {
  while(!UART_ready());                             // wait until ready for next data
  UART_send(data);                                  // send data byte
}

// UART print string
void UART_print(const char *str) {
  while(*str) UART_write(*str++);                   // write characters of string
}

// UART print string with new line
void UART_println(const char *str) {
  UART_print(str);                                  // print string
  UART_write('\n');                                 // send new line command
}

// ===================================================================================
// Watchdog Timer (WDT) Implementation
// ===================================================================================

// WDT reset
#define WDT_reset()   asm("wdr")                    // reset watchdog timer

// WDT init
void WDT_init(void) {
  while(WDT.STATUS & WDT_SYNCBUSY_bm);              // wait for synchronization
  _PROTECTED_WRITE(WDT.CTRLA, WDT_PERIOD_4KCLK_gc); // set timer to 4 seconds
}

// ===================================================================================
// Command Buffer Implementation
// ===================================================================================

// Command/argument buffer
uint8_t CMD_buf[CMD_BUF_LEN];                       // this is the command buffer

// Get command arguments
void CMD_get(void) {
  uint8_t ptr = 0;                                  // command buffer pointer
  uint8_t cnt = UART_read();                        // read first byte = length
  CMD_buf[ptr++] = cnt;                             // write length to buffer
  while(cnt--) {                                    // loop <length> times
    CMD_buf[ptr++] = UART_read();                   // read bytes UART -> buffer
  }
}

// Ring buffer functions
uint8_t buf_head, buf_tail;                         // head and tail buffer pointer
uint8_t buf_requested;                              // requested bytes counter
#define buf_available()  (buf_head != buf_tail)     // something in the ring buffer?

// Get number of items in ring buffer
uint8_t buf_items(void) {
  if(buf_head >= buf_tail) return(buf_head - buf_tail);
  return(CMD_BUF_LEN - buf_tail + buf_head);
}

// Push byte to ring buffer
void buf_push(uint8_t data) {
  CMD_buf[buf_head++] = data;                       // data byte -> buffer; increase pointer
  if(buf_head >= CMD_BUF_LEN) buf_head = 0;         // make it a ring buffer
}

// Pop byte from ring buffer
uint8_t buf_pop(void) {
  uint8_t data = CMD_buf[buf_tail++];               // buffer -> data; increase pointer
  if(buf_tail >= CMD_BUF_LEN) buf_tail = 0;         // make it a ring buffer
  return data;
}

void buf_check(void) {
  if(UART_available()) {                            // something coming in via UART?
    buf_push(USART0.RXDATAL);                       // get data byte UART -> buffer
    if(buf_requested) buf_requested -= 1;           // decrease requested bytes counter
  }
}

uint8_t buf_request(uint16_t maxcnt) {
  if(buf_requested) return 0;                       // still something requested?
  buf_requested = CMD_BUF_LEN - buf_items() - 1;    // get free bytes in buffer
  if(buf_requested > maxcnt) buf_requested = maxcnt;// limit requested bytes
  if(buf_requested) UART_write(buf_requested);      // send request via UART
  return buf_requested;                             // return number of requested bytes
}

// ===================================================================================
// IEC Implementation - Setup
// ===================================================================================

// Regarding the signals on the bus:
// 0V = TRUE or PULLED DOWN
// 5V = FALSE or RELEASED
// Regarding the data on the bus:
// 0V = logical 0
// 5V = logical 1
// Bytes are sent with low bit first. Data is valid on rising edge of clock

// The weak internal pullup resistors need some time to pull the line HIGH
#define IEC_PULLUP    4                             // pullup time in us

// Global flags
uint8_t IEC_error;
uint8_t IEC_EOI;
uint8_t IEC_device = 8;

// IEC init
void IEC_init(void) {
  pinPullup(PIN_ATN);                               // activate pullup resistors
  pinPullup(PIN_CLK);
  pinPullup(PIN_DATA);
}

// ===================================================================================
// IEC Protocol Implementation - Low Level Bitbanging Stuff
// ===================================================================================

// IEC bitbanging macros
#define IEC_ATN_setHigh()    pinInput(PIN_ATN)      // ATN  released    -> FALSE
#define IEC_ATN_setLow()     pinOutput(PIN_ATN)     // ATN  pulled down -> TRUE
#define IEC_CLK_setHigh()    pinInput(PIN_CLK)      // CLK  released    -> FALSE
#define IEC_CLK_setLow()     pinOutput(PIN_CLK)     // CLK  pulled down -> TRUE
#define IEC_DATA_setHigh()   pinInput(PIN_DATA)     // DATA released    -> FALSE
#define IEC_DATA_setLow()    pinOutput(PIN_DATA)    // DATA pulled down -> TRUE

#define IEC_CLK_isHigh()     pinRead(PIN_CLK)       // check if CLK  is HIGH / FALSE
#define IEC_CLK_isLow()      !pinRead(PIN_CLK)      // check if CLK  is LOW  / TRUE
#define IEC_DATA_isHigh()    pinRead(PIN_DATA)      // check if DATA is HIGH / FALSE
#define IEC_DATA_isLow()     !pinRead(PIN_DATA)     // check if DATA is LOW  / TRUE

// Release all control lines
void IEC_release(void) {
  IEC_ATN_setHigh();                                // release ATN line
  IEC_CLK_setHigh();                                // release CLK line
  IEC_DATA_setHigh();                               // release DATA line
  while(IEC_CLK_isLow());                           // make sure CLK is high
  while(IEC_DATA_isLow());                          // make sure DATA is high
}

// Send a data byte
uint8_t IEC_sendByte(uint8_t data) {
  IEC_CLK_setHigh();                                // declare 'READY TO SEND'
  while(IEC_DATA_isLow());                          // wait for listener 'READY FOR DATA'
  _delay_us(40);                                    // wait 'NON-EOI RESPONSE TIME' (max 200us)

  IEC_CLK_setLow();                                 // declare 'TRANSMISSION STARTS'
  for(uint8_t i=8; i; i--, data>>=1) {              // 8 bits, LSB first
    (data & 1) ? IEC_DATA_setHigh() : IEC_DATA_setLow();  // set DATA line according to bit
    _delay_us(60);                                  // wait 'BIT SETUP TIME' (min 60us)
    IEC_CLK_setHigh();                              // declare 'DATA VALID'
    _delay_us(60);                                  // wait 'DATA VALID TIME' (min 60us)
    IEC_CLK_setLow();                               // end of clock period
  }
  IEC_DATA_setHigh();                               // release DATA line for handshake
  uint8_t cnt = 250;                                // waiting counter in 4us steps
  _delay_us(IEC_PULLUP);                            // some time for the weak pull-ups
  while(--cnt && IEC_DATA_isHigh()) _delay_us(4);   // wait for listener 'DATA ACCEPTED' (max 1ms)
  if(IEC_DATA_isHigh()) {                           // no response??
    IEC_error = 1;                                  // raise IEC_error
    IEC_CLK_setHigh();                              // release CLK line
  }
  _delay_us(100);                                   // wait 'BETWEEN BYTES TIME' (min 100us)
  return IEC_error;                                 // return error state
}

// Send last data byte in sequence (EOI: End-Or-Identify / End-Of-Information)
void IEC_sendLast(uint8_t data) {
  IEC_CLK_setHigh();                                // declare 'READY TO SEND'
  while(IEC_DATA_isLow());                          // wait for listener 'READY FOR DATA'
                                                    // do nothing now to declare 'EOI' (min 200us)
  while(IEC_DATA_isHigh());                         // wait for listener 'EOI RECEIVED'
  IEC_sendByte(data);                               // send the data byte
  IEC_CLK_setHigh();                                // release CLK line
}

// Read a byte
uint8_t IEC_readByte(void) {
  IEC_EOI = 0;                                      // clear EOI flag
  while(IEC_CLK_isLow());                           // wait for talker 'READY TO SEND'
  IEC_DATA_setHigh();                               // declare 'READY FOR DATA'

  // Wait for transmission start or EOI
  uint8_t waittime = 25;                            // timer to identify EOI (last byte)
  while(IEC_CLK_isHigh()) {                         // wait for talker 'TRANSMISSION STARTS' or 'EOI'
    if(!--waittime) break;                          // leave when timeout
    _delay_us(10);                                  // 20 * 10us = 200us, EOI after 200us timeout
  }

  // Handle EOI if necessary
  if(!waittime) {                                   // timeout? => EOI
    IEC_EOI = 1;                                    // set EOI flag
    IEC_DATA_setLow();                              // declare 'EOI RECEIVED'
    _delay_us(60);                                  // wait 'EOI RESPONSE HOLD TIME' (min 60us)
    IEC_DATA_setHigh();                             // declare 'READY FOR DATA'
    while(IEC_CLK_isHigh());                        // wait for talker 'TRANSMISSION STARTS'
  }

  // Receive the data byte
  uint8_t data = 0;                                 // variable to store received byte
  for(uint8_t i=8; i; i--) {                        // 8 bits, LSB first
    data >>= 1;                                     // shift byte to the right
    while(IEC_CLK_isLow());                         // wait for talker 'DATA VALID'
    if(IEC_DATA_isHigh()) data |= 0x80;             // read the data bit
    while(IEC_CLK_isHigh());                        // wait end of clock period
  }
  IEC_DATA_setLow();                                // declare 'DATA ACCEPTED'

  // Release system line if last byte received
  if(IEC_EOI) {                                     // was it the last byte?
    _delay_us(60);                                  // wait 'EOI ACKNOWLEDGE TIME' (min 60us)
    IEC_DATA_setHigh();                             // release DATA line
  }

  return data;                                      // return received data byte
}

// Start sending under attention
uint8_t IEC_ATN_start(void) {
  IEC_ATN_setLow();                                 // declare 'ATTENTION!'
  IEC_CLK_setLow();                                 // pull CLK line true
  _delay_us(1000);                                  // wait 'ATN RESPONSE TIME' (1000us)
  if(IEC_DATA_isHigh()) {                           // no response?
    IEC_error = 1;                                  // raise error
    IEC_ATN_setHigh(); IEC_CLK_setHigh();           // release control lines
  }
  return IEC_error;                                 // return error state
}

// Stop sending under attention
void IEC_ATN_stop(void) {
  IEC_ATN_setHigh();                                // release ATN line
  _delay_us(20);                                    // wait 'ATN RELEASE TIME' (20us - 100us)
}

// Perform turnaround -> Disk Drive becomes talker, ATtiny becomes listener
void IEC_turnaround(void) {
  IEC_DATA_setLow();                                // take over DATA line
  IEC_CLK_setHigh();                                // declare 'I AM LISTENER NOW'
  _delay_us(IEC_PULLUP);                            // some time for the weak pull-ups
  while(IEC_CLK_isHigh());                          // wait for listener 'I AM TALKER NOW'
}

// ===================================================================================
// IEC Protocol Implementation - Basic Control Functions
// ===================================================================================

// IEC serial bus control codes
#define IEC_LISTEN        0x20
#define IEC_UNLISTEN      0x3F
#define IEC_TALK          0x40
#define IEC_UNTALK        0x5F
#define IEC_OPEN_CH       0x60
#define IEC_CLOSE         0xE0
#define IEC_OPEN          0xF0

// Send 'LISTEN' command to the bus
uint8_t IEC_listen(uint8_t device, uint8_t secondary) {
  if(IEC_ATN_start()) return 1;                     // start sending under 'ATTENTION'
  if(IEC_sendByte(IEC_LISTEN + device)) return 1;   // send 'LISTEN' + device address
  if(IEC_sendByte(IEC_OPEN_CH + secondary)) return 1;  // open channel (secondary address)
  IEC_ATN_stop();                                   // stop sending under 'ATTENTION'
  return 0;                                         // return success
}

// Send 'UNLISTEN' command to the bus
uint8_t IEC_unlisten(void) {
  if(IEC_ATN_start()) return 1;                     // start sending under 'ATTENTION'
  if(IEC_sendByte(IEC_UNLISTEN)) return 1;          // send 'UNLISTEN'
  IEC_ATN_stop();                                   // stop sending under 'ATTENTION'
  IEC_release();                                    // release all lines
  return 0;                                         // return success
}

// Send 'TALK' command to the bus and perform turnaround
uint8_t IEC_talk(uint8_t device, uint8_t secondary) {
  if(IEC_ATN_start()) return 1;                     // start sending under 'ATTENTION'
  if(IEC_sendByte(IEC_TALK + device)) return 1;     // send 'TALK' + device address
  if(IEC_sendByte(IEC_OPEN_CH + secondary)) return 1;  // open channel (secondary address)
  IEC_ATN_stop();                                   // stop sending under 'ATTENTION'
  IEC_turnaround();                                 // turnaround; device becomes talker
  return 0;                                         // return success
}

// Send 'UNTALK' command to the bus
uint8_t IEC_untalk(void) {
  if(IEC_ATN_start()) return 1;                     // start sending under 'ATTENTION'
  if(IEC_sendByte(IEC_UNTALK)) return 1;            // send 'UNTALK'
  IEC_ATN_stop();                                   // stop sending under 'ATTENTION'
  IEC_release();                                    // release all lines
  return 0;                                         // return success
}

// ===================================================================================
// IEC Protocol Implementation - Raw Data Transmission Functions
// ===================================================================================

// Write a string to IEC bus
uint8_t IEC_sendStr(const uint8_t* str) {
  while(*str && !IEC_error) IEC_sendByte(*str++);   // send each character of string
  if(!IEC_error) IEC_sendLast(0x0D);                // send 'RETURN' char
  return IEC_error;                                 // return error state
}

// Read data from IEC bus and send it via UART
uint8_t IEC_readRaw(void) {
  IEC_EOI = 0;                                      // clear EOI flag
  while(!IEC_EOI && !IEC_error) {                   // while bytes available and no error
    UART_write(IEC_readByte());                     // transfer data byte IEC -> UART
  }
  return IEC_error;                                 // return error state
}

// Read data from IEC bus and send it via UART with EOI flag in front of each byte
uint8_t IEC_readBytes(void) {
  IEC_EOI = 0;                                      // clear EOI flag
  while(!IEC_EOI && !IEC_error) {                   // while bytes available and no error
    uint8_t data = IEC_readByte();                  // read data byte from device
    UART_write(IEC_EOI | IEC_error << 1);           // send IEC and ERROR flag
    UART_write(data);                               // send data byte via UART to PC
  }
  return IEC_error;                                 // return error state
}

// Send buffer via IEC bus
uint8_t IEC_writeBuffer(uint8_t* buf, uint8_t count) {
  while(count && !IEC_error) {                      // until no more bytes or error
    if(--count) IEC_sendByte(*buf++);               // send data byte buffer -> IEC
    else        IEC_sendLast(*buf);                 // send last byte with EOI
  }
  return IEC_error;                                 // return error state
}

// ===================================================================================
// IEC Protocol Implementation - Fast Asynchronous IEC Data Transmissions
// ===================================================================================

#define MFCYCLES             F_CPU / 1000000UL      // MCU cycles per Floppy cycle

// Read byte via fast IEC
uint8_t IEC_readAsynch(void) {
  uint8_t data = 0;                                 // preload data byte
  while(IEC_CLK_isHigh());                          // wait for 'READY TO SEND BYTE'
  __builtin_avr_delay_cycles(16 * MFCYCLES);        // wait 16 floppy cycles
  if(IEC_CLK_isLow())  data |= 0b10000000;          // get data bit 7
  if(IEC_DATA_isLow()) data |= 0b00100000;          // get data bit 5
  __builtin_avr_delay_cycles(8 * MFCYCLES - 4);     // wait 8 floppy cycles
  if(IEC_CLK_isLow())  data |= 0b01000000;          // get data bit 6
  if(IEC_DATA_isLow()) data |= 0b00010000;          // get data bit 4
  __builtin_avr_delay_cycles(8 * MFCYCLES - 4);     // wait 8 floppy cycles
  if(IEC_CLK_isLow())  data |= 0b00001000;          // get data bit 3
  if(IEC_DATA_isLow()) data |= 0b00000010;          // get data bit 1
  __builtin_avr_delay_cycles(8 * MFCYCLES - 4);     // wait 8 floppy cycles
  if(IEC_CLK_isLow())  data |= 0b00000100;          // get data bit 2
  if(IEC_DATA_isLow()) data |= 0b00000001;          // get data bit 0
  while(IEC_CLK_isLow());                           // wait for CLK released
  return data;                                      // return received data byte
}

// Read data block via fast IEC
void IEC_readBlock(uint16_t cnt) {
  WDT_reset();                                      // reset watchdog
  do {                                              // transfer block data
    uint8_t data = IEC_readAsynch();                // read data byte from IEC
    UART_send(data);                                // send data byte via UART
    if(cnt == 256) IEC_EOI = !data;                 // end of file flag
  } while(--cnt);                                   // loop cnt times
}

// Write byte via fast IEC
void IEC_writeAsynch(uint8_t data) {
  while(IEC_CLK_isHigh()) buf_check();              // wait for 'READY TO RECEIVE'
  IEC_DATA_setLow();                                // declare  'READY TO SEND'
  while(IEC_CLK_isLow()) buf_check();               // wait for 'LETS GO'
  IEC_CLK_setHigh(); IEC_DATA_setHigh();            // initially '0' - bits
  if(data & 0b00001000) IEC_CLK_setLow();           // set data bit 3
  if(data & 0b00000010) IEC_DATA_setLow();          // set data bit 1
  __builtin_avr_delay_cycles(6 * MFCYCLES - 6);     // wait 6 floppy cycles
  IEC_CLK_setHigh(); IEC_DATA_setHigh();            // initially '0' - bits
  if(data & 0b00000100) IEC_CLK_setLow();           // set data bit 2
  if(data & 0b00000001) IEC_DATA_setLow();          // set data bit 0
  __builtin_avr_delay_cycles(9 * MFCYCLES - 6);     // wait 9 floppy cycles
  IEC_CLK_setHigh(); IEC_DATA_setHigh();            // initially '0' - bits
  if(data & 0b10000000) IEC_CLK_setLow();           // set data bit 7
  if(data & 0b00100000) IEC_DATA_setLow();          // set data bit 5
  __builtin_avr_delay_cycles(6 * MFCYCLES - 6);     // wait 6 floppy cycles
  IEC_CLK_setHigh(); IEC_DATA_setHigh();            // initially '0' - bits
  if(data & 0b01000000) IEC_CLK_setLow();           // set data bit 6
  if(data & 0b00010000) IEC_DATA_setLow();          // set data bit 4
  buf_check();                                      // check inbound UART
  __builtin_avr_delay_cycles(6 * MFCYCLES - 6);     // wait 6 floppy cycles
  IEC_CLK_setHigh(); IEC_DATA_setHigh();            // release lines
}

// Write data block via fast IEC
void IEC_writeBlock(uint16_t cnt) {
  if(!cnt) cnt = 256;                               // zero means 256
  uint16_t reqcnt = cnt;                            // reset request counter
  buf_requested  = 0;                               // reset requested counter
  buf_head = 0; buf_tail = 0;                       // reset buffer pointer
  WDT_reset();                                      // reset watchdog
  do {
    if(reqcnt) reqcnt -= buf_request(reqcnt);       // request next data package
    while(!buf_available()) buf_check();            // wait for data if buffer empty
    IEC_writeAsynch(buf_pop());                     // data bytes buffer -> IEC
  } while(--cnt);                                   // loop for 325 bytes
}

// ===================================================================================
// High Level Functions
// ===================================================================================

// Get status from device
void IEC_getStatus(void) {
  IEC_talk(IEC_device, 0x0F);                       // device should talk on channel 15
  IEC_readRaw();                                    // read from device and write via UART
  IEC_untalk();                                     // send 'UNTALK'
  UART_write('\n');                                 // send 'END OF MESSAGE' via UART
}

// Send command from buffer to device via IEC
uint8_t IEC_sendCommand(void) {
  IEC_listen(IEC_device, 0x0F);                     // set device to LISTEN
  IEC_writeBuffer(CMD_buf+1, CMD_buf[0]);           // send command buffer -> device
  IEC_unlisten();                                   // set device to UNLISTEN
  UART_write(IEC_error);                            // send error state
  return IEC_error;                                 // return error state
}

// <length>"M-E"<addrLow><addrHigh><track><#sectors><sector1><sector2>...
void IEC_readTrack(void) {
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  uint8_t cnt = CMD_buf[7];                         // get number of sectors to read
  while(cnt--) IEC_readBlock(325);                  // read sector and send data via UART
}

// <length>"M-E"<addrLow><addrHigh><track><#sectors><sector1><sector2>...
void IEC_writeTrack(void) {
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  uint8_t cnt = CMD_buf[7];                         // get number of sectors to write
  while(cnt--) IEC_writeBlock(325);                 // read data via UART and write sector
  UART_write(0);                                    // send 'job finished'
}

// <length>"M-E"<addrLow><addrHigh><startTrack><startSector>
void IEC_loadFile(void) {
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  do {
    IEC_readBlock(256);                             // read sector and send data via UART
  } while(!IEC_error && !IEC_EOI);                  // repeat until error or end of file
}

// <length>"M-E"<addrLow><addrHigh><tracks><bump><clear><verify>:<name>,<ID1><ID2>
void IEC_format(void) {
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  uint8_t cnt = CMD_buf[6] + 1;                     // get number of tracks
  while(!IEC_error && cnt--) {                      // for each track:
    WDT_reset();                                    // reset watchdog
    while(IEC_DATA_isHigh());                       // wait for track complete
    IEC_error = IEC_CLK_isLow();                    // get return code
    UART_write(IEC_error);                          // send return code to pc
    while(IEC_DATA_isLow());                        // wait for end of signal
  }
}

// <length>"M-R"<addrLow><addrHigh><length>
void IEC_readMem(void) {
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  IEC_talk(IEC_device, 0x0F);                       // device should talk on channel 15
  IEC_readRaw();                                    // read from device and write via UART
  IEC_untalk();                                     // send 'UNTALK'
}

// ===================================================================================
// Main Function
// ===================================================================================

// List of low-level commands
enum {NOP, LISTEN, UNLISTEN, TALK, UNTALK, READBYTE, READBYTES, READRAW, 
      WRITEBYTE, WRITELAST, WRITEBYTES, READFAST, WRITEFAST, OPEN, CLOSE,
      RESET, RELEASE, GETDEVICE, SETDEVICE, GETEOI, SETEOI, CLREOI,
      GETATN, SETATN, RELATN, GETCLK, SETCLK, RELCLK, GETDATA, SETDATA, RELDATA};
      
// Main function
int main(void) {
  // Setup
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);             // set clock frequency to 16 MHz
  UART_init();                                        // setup serial communication
  WDT_init();                                         // setup watchdog timer
  IEC_init();                                         // setup IEC interface

  // Loop
  while(1) {
    while(!UART_available()) WDT_reset();             // wait for command byte
    uint8_t cmd = UART_read();                        // read command byte
    CMD_get();                                        // get arguments
    IEC_error = 0;                                    // clear error flag
    switch(cmd) {                                     // take proper action
      // High-level commands
      case 'i':         UART_println(IDENT); break;   // send identification string
      case 'v':         UART_println(VERSION); break; // send version number
      case 'r':         IEC_readTrack(); break;       // read track from disk
      case 'w':         IEC_writeTrack(); break;      // write track to disk
      case 'l':         IEC_loadFile(); break;        // load a file from disk
//    case 's':         break;                        // save a file to disk
      case 'f':         IEC_format(); break;          // format disk
      case 'm':         IEC_readMem(); break;         // read memory
      case 'c':         IEC_sendCommand(); break;     // send command to IEC device
      case 't':         IEC_getStatus(); break;       // get status from IEC device

      // Low-level commands
      case LISTEN:      IEC_listen(IEC_device, 0x0F); break;            //  1 01
      case UNLISTEN:    IEC_unlisten(); break;                          //  2 02
      case TALK:        IEC_talk(IEC_device, 0x0F); break;              //  3 03
      case UNTALK:      IEC_untalk(); break;                            //  4 04
      case READBYTE:    UART_write(IEC_readByte()); break;              //  5 05
      case READBYTES:   IEC_readBytes(); break;                         //  6 06
      case READRAW:     IEC_readRaw(); break;                           //  7 07
      case WRITEBYTE:   IEC_sendByte(CMD_buf[1]); break;                //  8 08
      case WRITELAST:   IEC_sendLast(CMD_buf[1]); break;                //  9 09
      case WRITEBYTES:  IEC_writeBuffer(CMD_buf+1, CMD_buf[0]); break;  // 10 0A
      case READFAST:    IEC_readBlock(CMD_buf[1]); break;               // 11 0B
      case WRITEFAST:   IEC_writeBlock(CMD_buf[1]); break;              // 12 0C
//    case OPEN:        break;                                          // 13 0D
//    case CLOSE:       break;                                          // 14 0E
//    case RESET:       break;                                          // 15 0F
      case RELEASE:     IEC_release(); break;                           // 16 10
      case GETDEVICE:   UART_write(IEC_device); break;                  // 17 11
      case SETDEVICE:   IEC_device = CMD_buf[1]; break;                 // 18 12
      case GETEOI:      UART_write(IEC_EOI); break;                     // 19 13
      case SETEOI:      IEC_EOI = 1; break;                             // 20 14
      case CLREOI:      IEC_EOI = 0; break;                             // 21 15
      case GETATN:      UART_write(pinRead(PIN_ATN) == 0); break;       // 22 16
      case SETATN:      IEC_ATN_setLow(); break;                        // 23 17
      case RELATN:      IEC_ATN_setHigh(); break;                       // 24 18
      case GETCLK:      UART_write(pinRead(PIN_CLK) == 0); break;       // 25 19
      case SETCLK:      IEC_CLK_setLow(); break;                        // 26 1A
      case RELCLK:      IEC_CLK_setHigh(); break;                       // 27 1B
      case GETDATA:     UART_write(pinRead(PIN_DATA) == 0); break;      // 28 1C
      case SETDATA:     IEC_DATA_setLow(); break;                       // 29 1D
      case RELDATA:     IEC_DATA_setHigh(); break;                      // 30 1E
      default:          break;
    }
    if(cmd <= RELDATA) UART_write(IEC_error);
  }
}
