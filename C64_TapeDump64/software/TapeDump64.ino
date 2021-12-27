// ===================================================================================
// Project:   TapeDump64 - Transfer files from a Commodore Datasette to a PC
// Version:   v1.1
// Year:      2021
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// TapeDump64 is a simple and inexpensive adapter that can interface a Commodore
// Datasette to your PC via USB in order to dump your software saved on tapes as
// TAP files.
//
// References:
// -----------
// - https://ist.uwaterloo.ca/~schepers/formats/TAP.TXT
// - https://wav-prg.sourceforge.io/tape.html
// - https://github.com/francescovannini/truetape64
//
// Wiring:
// -------
//                       +-\/-+
//                 Vdd  1|°   |8  GND
// USB RXD --- TXD PA6  2|    |7  PA3 AIN3 -------- TAPE SENSE
// USB TXD --- RXD PA7  3|    |6  PA0 AIN0 UPDI --- UPDI
//     LED --- SDA PA1  4|    |5  PA2 AIN2 SCL ---- TAPE READ
//                       +----+
//
// Compilation Settings:
// ---------------------
// Core:    megaTinyCore (https://github.com/SpenceKonde/megaTinyCore)
// Board:   ATtiny412/402/212/202
// Chip:    Choose the chip you use
// Clock:   16 MHz internal
//
// Leave the rest on default settings. Select "SerialUPDI" as programmer in the
// Arduino IDE and set the serial mode switch on the board to "UPDI". Don't forget
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
// - Set the serial mode switch on your TapeDump64 to "UART"
// - Connect your TapeDump64 to your Commodore Datasette
// - Connect your TapeDump64 to a USB port of your PC
// - Execute the tapedump python script on your PC: tapedump.py outputfile.tap
// - Press PLAY on your Datasette
// - The dumping is done fully automatically. It stops when the end of the cassette
//   is reached, when there are no more signals on the tape for a certain time or
//   when the STOP button on the Datasette is pressed.
//
// The tapedump python script controls the device via three simple commands:
//
// Command    Function                            Response
// "i"        transmit indentification string     "TapeDump64\n"
// "v"        transmit firmware version number    e.g. "v1.0\n"
// "r"        read file from tape                 raw data stream
//
// The raw datastream starts with a 0x00 as soon as PLAY on tape was pressed. Each
// valid pulse the device reads from the tape will be converted into a single non-zero
// byte which represents the pulse length and immediately transmitted via UART. If
// STOP on tape was pressed or a timeout waiting for valid pulses occurs, the end of
// the stream is shown with a 0x00 byte. Afterwards, the 16-bit checksum, which is
// formed from the addition of all data bytes, is transmitted least significant byte
// first (little endian). Finally, the tape buffer overflow flag is transmitted as a
// single byte (0x00 means no overflow occured).
//
// If PLAY was not pressed within the defined period of time at the beginning, a 0x01
// is sent instead of a 0x00 and the procedure is ended.


// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================

// Libraries
#include <avr/io.h>               // for GPIO
#include <avr/interrupt.h>        // for interrupts
#include <util/delay.h>           // for delays

// Pin definitions
#define PIN_LED       PA1         // pin connected to builtin LED
#define PIN_READ      PA2         // pin connected to READ on datasette port
#define PIN_SENSE     PA3         // pin connected to SENSE on datasette port
#define PIN_TXD       PA6         // UART TX pin connected to usb-to-serial converter
#define PIN_RXD       PA7         // UART RX pin connected to usb-to-serial converter

// Configuration parameters
#define UART_BAUD     460800      // UART: baud rate (max 1/16 of F_CPU)
#define TAP_WAIT_PLAY 10          // TAPE: time in seconds to wait for PLAY pressed (max 63)
#define TAP_TIMEOUT   25          // TAPE: time in seconds to wait for pulses (max 63)
#define TAP_BUF_LEN   64          // tape buffer length (must be power of 2)

// Identifiers
#define VERSION     "1.1"         // version number sent via serial if requested
#define IDENT       "TapeDump64"  // identifier sent via serial if requested

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
// Timeout Counter using RTC (Real Time Counter)
// ===================================================================================

volatile uint8_t RTC_timeout;                       // timeout flag

// Setup Real Time Counter (RTC)
void RTC_init(void) {
  RTC.CLKSEL  = RTC_CLKSEL_INT1K_gc;                // choose internal 1024Hz clock
  RTC.INTCTRL = RTC_OVF_bm;                         // enable overflow interrupt
}

// Start timeout counter (timeout in seconds)
void RTC_start(uint8_t timeout) {
  while(RTC.STATUS > 0);                            // wait for RTC to be ready
  RTC.CNT = 0;                                      // clear counter value
  RTC.PER = (uint16_t)timeout << 10;                // set timeout (seconds * 1024)
  RTC.CTRLA = RTC_RTCEN_bm;                         // start RTC, no prescaler
  RTC_timeout = 0;                                  // reset timeout flag
}

// Reset timeout counter
inline void RTC_reset(void) {
  RTC.CNT = 0;                                      // clear counter value
}

// Interrupt service routine for RTC (reached timeout)
ISR(RTC_CNT_vect) {
  RTC.INTFLAGS = RTC_OVF_bm;                        // clear interrupt flag
  RTC.CTRLA = 0;                                    // stop RTC
  RTC_timeout = 1;                                  // raise timeout flag
}

// ===================================================================================
// Datasette Port Interface Implementation
// ===================================================================================

// Variables, definitions and macros
volatile uint8_t TAP_buf[TAP_BUF_LEN];              // tape buffer
volatile uint8_t TAP_buf_ovf;                       // tape buffer overflow flag
volatile uint8_t TAP_buf_head;                      // tape buffer pointer for writing
volatile uint8_t TAP_buf_tail;                      // tape buffer pointer for reading
volatile uint8_t TAP_timer_ovf;                     // tape timer overflow flag
#define TAP_available()  (TAP_buf_head != TAP_buf_tail)

// Setup TAP interface
void TAP_init(void) {
  // Setup Pins
  pinPullup(PIN_READ);                              // enable pullup on READ pin
  pinPullup(PIN_SENSE);                             // enable pullup on SENSE pin
  pinOutput(PIN_LED);                               // set LED pin as output

  // Setup event system with channel between READ pin and TCB
  EVSYS.ASYNCCH0   = PIN_READ + 0x0A;               // READ pin as input for asynch channel 0
  EVSYS.ASYNCUSER0 = EVSYS_ASYNCUSER0_ASYNCCH0_gc;  // TCB as output for asynch channel 0

  // Setup timer/counter B (TCB) in input capture frequency measurement mode
  TCB0.CTRLA   = TCB_CLKSEL_CLKDIV2_gc;             // prescaler 2 -> 8 MHz
  TCB0.CTRLB   = TCB_CNTMODE_FRQ_gc;                // set frequency measurement mode
  TCB0.EVCTRL  = TCB_FILTER_bm                      // enable noise cancellation filter
               | TCB_EDGE_bm                        // capture on falling edges
               | TCB_CAPTEI_bm;                     // enable capture mode
  TCB0.INTCTRL = TCB_CAPT_bm;                       // enable interrupt on capture events
}

// Read from Datasette
void TAP_read(void) {
  // Wait until PLAY has been pressed on the tape or timeout occurs
  RTC_start(TAP_WAIT_PLAY);                         // start timeout counter for PLAY
  while(pinRead(PIN_SENSE) && !RTC_timeout) {       // wait for PLAY on tape pressed
    pinToggle(PIN_LED);                             // flash LED
    _delay_ms(100);                                 // short delay
  }
  if(RTC_timeout) {                                 // timout waiting for PLAY?
    pinLow(PIN_LED);                                // switch off LED
    UART_write(1);                                  // send 'TIMEOUT' to PC
    return;                                         // stop and return
  }

  // Prepare reading from tape
  uint16_t checksum = 0;                            // 16-bit checksum
  TAP_timer_ovf = 1;                                // ignore first pulse
  TAP_buf_ovf   = 0;                                // reset tape buffer overflow flag
  TAP_buf_head  = 0; TAP_buf_tail = 0;              // reset tape buffer
  TCB0.CNT = 0;                                     // reset pulse timer
  UART_write(0);                                    // send 'PLAY' to PC
  pinHigh(PIN_LED);                                 // light up LED
  _delay_ms(100);                                   // wait for motor to speed up
  RTC_start(TAP_TIMEOUT);                           // start timeout counter for pulses
  TCB0.CTRLA |= TCB_ENABLE_bm;                      // start timer

  // Read from tape
  while(1) {
    if(pinRead(PIN_SENSE)) break;                   // finish if STOP was pressed on tape
    if(RTC_timeout) break;                          // finish if no pulses for certain time
    if(TAP_buf_ovf) break;                          // finish if buffer overflow occured
    if(TCB0.CNT > 16384) TAP_timer_ovf = 1;         // check if timer overflow occurs
    if(TAP_available() && UART_ready()) {           // ready to send something via UART?
      uint8_t data = TAP_buf[TAP_buf_tail++];       // get data byte and increase pointer
      TAP_buf_tail &= (TAP_BUF_LEN - 1);            // limit buffer pointer
      UART_send(data);                              // send data byte via UART
      checksum += data;                             // update checksum
      RTC_reset();                                  // reset timeout counter
    }
  }

  // Finish reading
  TCB0.CTRLA &= ~TCB_ENABLE_bm;                     // stop timer
  pinLow(PIN_LED);                                  // switch off LED
  UART_write(0);                                    // send 'STOP' to PC
  UART_write(checksum); UART_write(checksum >> 8);  // send checksum to PC
  UART_write(TAP_buf_ovf);                          // send overflow flag
}

// TCB0 interrupt service routine (on each pulse capture)
ISR(TCB0_INT_vect) {
  uint16_t tmp = TCB0.CCMP;                         // read pulse length in 1/8 us
  if((tmp >= 16320) || (TAP_timer_ovf)) tmp = 16320;// limit to max value
  if(tmp >= 1024) {                                 // above minimum pulse length?
    TAP_buf[TAP_buf_head++] = tmp >> 6;             // calculate and write .tap value
    TAP_buf_head &= (TAP_BUF_LEN - 1);              // limit buffer pointer
    if(TAP_buf_head == TAP_buf_tail) TAP_buf_ovf = 1;   // set overflow flag if necessary
  }
  TAP_timer_ovf = 0;                                // reset timer overflow flag
}

// ===================================================================================
// Main Function
// ===================================================================================

int main(void) {
  // Setup
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);           // set clock frequency to 16 MHz
  UART_init();                                      // setup serial communication
  RTC_init();                                       // setup RTC as timeout counter
  TAP_init();                                       // setup datasette interface
  sei();                                            // enable global interrupts

  // Loop
  while(1) {
    uint8_t cmd = UART_read();                      // wait for and read command byte
    switch(cmd) {                                   // take proper action
      case 'i':   UART_println(IDENT); break;       // send identification string
      case 'v':   UART_println(VERSION); break;     // send version number
      case 'r':   TAP_read(); break;                // start reading from tape
      default:    break;
    }
  }
}
