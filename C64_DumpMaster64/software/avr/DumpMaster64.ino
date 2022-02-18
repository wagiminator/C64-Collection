// ===================================================================================
// Project:   DumpMaster64 - Adapter Firmware
// Version:   v1.3
// Year:      2022
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// The DumpMaster64 adapter bridges the gap between your modern PC and your ancient 
// mass storage devices for the Commodore C64. It can interface Commodore 1541(II)
// floppy disk drives as well as Commodore C2N/1530 Datasettes. It is a combination
// of the DiskBuddy64 and the TapeBuddy64.
//
// References:
// -----------
// - Schepers/Sundell/Brenner: https://ist.uwaterloo.ca/~schepers/formats/TAP.TXT
// - Vannini: https://github.com/francescovannini/truetape64
// - Steil: https://www.pagetable.com/?p=568
//
// Wiring:
// -------
//                              +-\/-+
//                        Vcc  1|°   |14  GND
//   IEC CLK --- !SS AIN4 PA4  2|    |13  PA3 AIN3 SCK ---- IEC DATA
//   IEC ATN ------- AIN5 PA5  3|    |12  PA2 AIN2 MISO --- TAPE SENSE
//   IEC RST --- DAC AIN6 PA6  4|    |11  PA1 AIN1 MOSI --- TAPE READ
//  BUSY LED ------- AIN7 PA7  5|    |10  PA0 AIN0 UPDI --- UPDI
//       USB -------- RXD PB3  6|    |9   PB0 AIN11 SCL --- TAPE WRITE
//       USB ---------TXD PB2  7|    |8   PB1 AIN10 SDA --- TAPE MOTOR
//                              +----+
//
// Compilation Settings:
// ---------------------
// Core:    megaTinyCore (https://github.com/SpenceKonde/megaTinyCore)
// Board:   ATtiny1614/1604/814/804/414/404
// Chip:    ATtiny804 or 814 or 1604 or 1614 (depending on your chip)
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
// - Set the serial mode switch on your DumpMaster64 adapter to "UART".
// - Connect the adapter to your Datasette and/or your floppy disk drive(s).
// - Connect the adapter to a USB port of your PC.
// - Switch on your floppy disk drive(s) if connected.
// - Execute the desired Python script on your PC to interface the adapter.


// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================

// Libraries
#include <avr/io.h>               // for GPIO
#include <avr/interrupt.h>        // for interrupts
#include <util/delay.h>           // for delays

// Pin definitions
#define PIN_READ      PA1         // pin connected to READ on datasette port
#define PIN_SENSE     PA2         // pin connected to SENSE on datasette port
#define PIN_DATA      PA3         // pin connected to DATA in IEC port
#define PIN_CLK       PA4         // pin connected to CLK on IEC port
#define PIN_ATN       PA5         // pin connected to ATN on IEC port
#define PIN_RST       PA6         // pin connected to RESET on IEC port
#define PIN_LED       PA7         // pin connected to BUSY LED
#define PIN_WRITE     PB0         // pin connected to WRITE on datasette port
#define PIN_MOTOR     PB1         // pin connected to MOTOR control
#define PIN_TXD       PB2         // UART TX pin connected to usb-to-serial converter
#define PIN_RXD       PB3         // UART RX pin connected to usb-to-serial converter

// Configuration parameters
#define UART_BAUD     460800      // UART: baud rate (max 1/16 of F_CPU)
#define TAP_WAIT_PLAY 10          // TAPE: time in seconds to wait for PLAY pressed (max 63)
#define TAP_TIMEOUT   25          // TAPE: time in seconds to wait for pulses (max 63)
#define TAP_PACKSIZE  64          // length of data package for writing (32, 64, or 128)

// Identifiers
#define VERSION   "1.3"           // version number sent via serial if requested
#define IDENT     "DumpMaster64"  // identifier sent via serial if requested

// Pin manipulation macros
enum {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PB0, PB1, PB2, PB3};    // enumerate pin designators
#define pinInput(x)       (&VPORTA.DIR)[((x)&8)>>1] &= ~(1<<((x)&7))  // set pin to INPUT
#define pinOutput(x)      (&VPORTA.DIR)[((x)&8)>>1] |=  (1<<((x)&7))  // set pin to OUTPUT
#define pinLow(x)         (&VPORTA.OUT)[((x)&8)>>1] &= ~(1<<((x)&7))  // set pin to LOW
#define pinHigh(x)        (&VPORTA.OUT)[((x)&8)>>1] |=  (1<<((x)&7))  // set pin to HIGH
#define pinToggle(x)      (&VPORTA.IN )[((x)&8)>>1] |=  (1<<((x)&7))  // TOGGLE pin
#define pinRead(x)        ((&VPORTA.IN)[((x)&8)>>1] &   (1<<((x)&7))) // READ pin
#define pinPullup(x)      (&PORTA.PIN0CTRL)[(((x)&8)<<2)+((x)&7)] |= PORT_PULLUPEN_bm
#define pinInvert(x)      (&PORTA.PIN0CTRL)[(((x)&8)<<2)+((x)&7)] |= PORT_INVEN_bm
#define pinIntEnable(x)   (&PORTA.PIN0CTRL)[(((x)&8)<<2)+((x)&7)] |= PORT_ISC_BOTHEDGES_gc
#define pinIntDisable(x)  (&PORTA.PIN0CTRL)[(((x)&8)<<2)+((x)&7)] &= ~PORT_ISC_gm
#define pinIntFlagClr(x)  (&VPORTA.INTFLAGS)[((x)&8)>>1] |= (1<<((x)&7))

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

// UART empty RX pipeline
void UART_flushRX(void) {
  do {
    USART0.RXDATAL;                                 // read to clear RXCIF flag
    _delay_ms(1);                                   // wait a bit
  } while(UART_available());                        // repeat if there is still data
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
// Universal Ring Buffer Implementation (256 Bytes)
// ===================================================================================

// Buffer variables
volatile uint8_t BUF_buffer[325];                   // ring buffer + 69 overflow bytes
volatile uint8_t BUF_head;                          // buffer pointer for writing
volatile uint8_t BUF_tail;                          // buffer pointer for reading
volatile uint8_t BUF_overflow;                      // buffer overflow flag
volatile uint8_t BUF_underrun;                      // buffer underrun flag

// Buffer macros
#define BUF_push(x)      BUF_buffer[BUF_head++]=(x) // push item into buffer
#define BUF_pull()       (BUF_buffer[BUF_tail++])   // pull item from buffer
#define BUF_available()  (BUF_head != BUF_tail)     // something in the ring buffer?
#define BUF_items()      (BUF_head - BUF_tail)      // get number of items in buffer

// Reset buffer
void BUF_reset(void) {
  BUF_head = 0; BUF_tail = 0;                       // reset buffer poiners
  BUF_overflow = 0; BUF_underrun = 0;               // reset buffer flags
}

// Get command arguments
void CMD_get(void) {
  BUF_reset();                                      // reset buffer
  uint8_t cnt = UART_read();                        // read first byte = length
  BUF_push(cnt);                                    // write length to buffer
  while(cnt--) {                                    // loop <length> times
    BUF_push(UART_read());                          // read bytes UART -> buffer
  }
}

// ===================================================================================
// Datasette Port Interface Implementation - Setup
// ===================================================================================

// Variables, definitions and macros
volatile uint8_t TAP_timer_ovf;                     // tape timer overflow flag
volatile uint8_t TAP_started;                       // tape pulses started flag

// Setup TAP interface
void TAP_init(void) {
  // Setup Pins
  pinPullup(PIN_READ);                              // enable pullup on READ pin
  pinPullup(PIN_SENSE);                             // enable pullup on SENSE pin
  pinOutput(PIN_WRITE);                             // set WRITE pin as output
  pinOutput(PIN_MOTOR);                             // set MOTOR pin as output  
  pinIntEnable(PIN_SENSE);                          // enable interrupt on SENSE pin

  // Setup event system with channel between READ pin and TCB
  EVSYS.ASYNCCH0   = PIN_READ + 0x0A;               // READ pin as input for asynch channel 0
  EVSYS.ASYNCUSER0 = EVSYS_ASYNCUSER0_ASYNCCH0_gc;  // TCB as output for asynch channel 0

  // Setup timer/counter B (TCB) in input capture frequency measurement mode
  TCB0.CTRLA   = TCB_CLKSEL_CLKTCA_gc;              // TCA0 as clock source -> 250kHz
  TCB0.CTRLB   = TCB_CNTMODE_FRQ_gc;                // set frequency measurement mode
  TCB0.EVCTRL  = TCB_FILTER_bm                      // enable noise cancellation filter
               | TCB_EDGE_bm                        // capture on falling edges
               | TCB_CAPTEI_bm;                     // enable capture mode
  TCB0.INTCTRL = TCB_CAPT_bm;                       // enable interrupt on capture events

  // Setup PWM on WRITE pin using timer/counter A (TCA)
  TCA0.SINGLE.CTRLA   = TCA_SINGLE_CLKSEL_DIV64_gc;       // prescaler 64 -> 250kHz
  TCA0.SINGLE.CTRLB   = TCA_SINGLE_CMP0EN_bm              // enable output on PB0
                      | TCA_SINGLE_WGMODE_SINGLESLOPE_gc; // single slope PWM
}

// Interrupt service routine for SENSE pin (MOTOR control)
ISR(PORTA_PORT_vect) {
  pinIntFlagClr(PIN_SENSE);                         // clear interrupt flag
  if(pinRead(PIN_SENSE)) pinLow(PIN_MOTOR);         // button released -> stop  motor
  else                  pinHigh(PIN_MOTOR);         // button pressed  -> start motor
}

// ===================================================================================
// Datasette Port Interface Implementation - Read from Tape
// ===================================================================================

// Read from Datasette
void TAP_read(void) {
  // Wait until PLAY has been pressed on the tape or timeout occurs
  RTC_start(TAP_WAIT_PLAY);                         // start timeout counter for PLAY
  while(pinRead(PIN_SENSE) && !RTC_timeout) {       // wait for PLAY on tape pressed
    WDT_reset();                                    // reset watchdog
    pinToggle(PIN_LED);                             // flash LED
    _delay_ms(100);                                 // short delay
  }
  if(RTC_timeout) {                                 // timout waiting for PLAY?
    UART_write(1);                                  // send 'TIMEOUT' to PC
    return;                                         // stop and return
  }

  // Prepare reading from tape
  uint16_t checksum = 0;                            // 16-bit checksum
  uint8_t  count = 0;                               // data byte counter
  TAP_started   = 0;                                // first pulse flag
  TAP_timer_ovf = 1;                                // ignore first pulse
  BUF_reset();                                      // reset buffer
  TCB0.CNT = 0;                                     // reset pulse timer
  UART_write(0);                                    // send 'PLAY' to PC
  pinHigh(PIN_LED);                                 // light up LED
  _delay_ms(100);                                   // wait for motor to speed up
  RTC_start(TAP_TIMEOUT);                           // start timeout counter for pulses
  TCA0.SINGLE.CTRLA  |= TCA_SINGLE_ENABLE_bm;       // start TCA as clock source
  TCB0.CTRLA |= TCB_ENABLE_bm;                      // start TCB

  // Read from tape
  while(1) {
    // Check finish conditions
    if(pinRead(PIN_SENSE)) break;                   // finish if STOP was pressed on tape
    if(RTC_timeout) break;                          // finish if no pulses for certain time
    if(BUF_overflow) break;                         // finish if buffer overflow occured
    WDT_reset();                                    // reset watchdog

    // Check timer overflow conditions
    if(TAP_started) {                               // pulses already started?
      cli();                                        // atomic sequence ahead
      if(TAP_timer_ovf) {                           // overflow ahead?
        if(TCB0.CNT < 65500) {                      // is it an actual overflow?          
          BUF_push(0xFF);                           // low  byte TAP value of max pause
          BUF_push(0x7F);                           // high byte TAP value of max pause
          if(!BUF_available()) BUF_overflow = 1;    // set overflow flag if necessary
          TAP_timer_ovf = 0;                        // clear timer overflow flag
        }
      } else {                                      // no overflow ahead?
        if(TCB0.CNT > 65500) TAP_timer_ovf = 1;     // check if timer overflow is coming
      }
      sei();                                        // atomic sequence ended
    }

    // Check send data conditions
    if(BUF_available() && UART_ready()) {           // ready to send something via UART?
      uint8_t data  = BUF_pull();                   // pull data byte from buffer
      UART_send(data);                              // send data byte via UART
      checksum += data;                             // update checksum
      count++;                                      // increase counter
      RTC_reset();                                  // reset timeout counter
    }
  }

  // Finish reading
  TCB0.CTRLA &= ~TCB_ENABLE_bm;                     // stop timer
  if(count & 1) UART_write(1);                      // ensure even number of bytes
  UART_write(0); UART_write(0);                     // send 'STOP' to PC
  UART_write(checksum); UART_write(checksum >> 8);  // send checksum to PC
  UART_write(BUF_overflow);                         // send overflow flag
  pinLow(PIN_MOTOR);                                // stop motor
  while(!pinRead(PIN_SENSE)) {                      // wait for STOP button pressed
    WDT_reset();                                    // reset watchdog
    pinToggle(PIN_LED);                             // flash LED
    _delay_ms(100);                                 // wait a bit
  }
}

// TCB0 interrupt service routine (on each pulse capture)
ISR(TCB0_INT_vect) {
  uint16_t tmp = TCB0.CCMP;                         // read pulse length in 4 us
  if((TAP_timer_ovf) && (tmp<65500)) tmp = 0xFFFF;  // set max value on overflow
  tmp >>= 1;                                        // make it 8 us
  if(tmp) {                                         // minimum pulse length?
    BUF_push(tmp);                                  // push low byte TAP value to buffer
    BUF_push(tmp >> 8);                             // push high byte TAP value to buffer
    if(!BUF_available()) BUF_overflow = 1;          // set overflow flag if necessary
  }
  TAP_timer_ovf = 0;                                // reset timer overflow flag
  TAP_started = 1;                                  // set first pulse flag
}

// ===================================================================================
// Datasette Port Interface Implementation - Write to Tape
// ===================================================================================

// Write to Datasette
void TAP_write(void) {
  // Variables
  uint8_t requested = 0;                            // number of data bytes requested
  uint8_t endofdata = 0;                            // end of data flag
  uint8_t data;                                     // temporary variable for data
  uint16_t checksum = 0;                            // checksum

  // Wait until RECORD has been pressed on the tape or timeout occurs
  RTC_start(TAP_WAIT_PLAY);                         // start timeout counter for RECORD
  while(pinRead(PIN_SENSE) && !RTC_timeout) {       // wait for RECORD on tape pressed
    WDT_reset();                                    // reset watchdog
    pinToggle(PIN_LED);                             // flash LED
    _delay_ms(100);                                 // wait a bit
  }
  if(RTC_timeout) {                                 // timout occured?
    UART_write(1);                                  // send 'TIMEOUT' to PC
    return;                                         // stop and return
  }

  // Fill buffer with first data package
  BUF_reset();                                      // reset buffer
  UART_write(0);                                    // send 'READY' to PC
  UART_write(254);                                  // request first data package
  for(uint8_t i=254; i; i--) {                      // read first data package
    data = UART_read();                             // get data byte
    BUF_push(data);                                 // push data byte to buffer
    checksum += data;                               // update checksum
  }

  // Prepare writing to tape
  pinHigh(PIN_LED);                                 // light up LED
  _delay_ms(100);                                   // wait for motor to speed up
  uint16_t tmp = BUF_pull();                        // get length of first pulse
  tmp += (uint16_t)BUF_pull() << 8;                 // high byte
  TCA0.SINGLE.CNT = 0;                              // reset TCA counter
  TCA0.SINGLE.CMP0BUF = tmp;                        // set TCA compare (duty)
  TCA0.SINGLE.PERBUF  = (tmp << 1) - 1;             // set TCA period (pulse length)
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;          // enable overflow interrupt
  TCA0.SINGLE.CTRLA  |= TCA_SINGLE_ENABLE_bm;       // start timer for PWM

  // Write to tape
  while(1) {
    WDT_reset();                                    // reset watchdog
    if(pinRead(PIN_SENSE)) break;                   // finish if STOP was pressed on tape
    if(BUF_underrun) break;                         // finish if buffer underrun
    if(~TCA0.SINGLE.CTRLA & TCA_SINGLE_ENABLE_bm) break; // finish if timer was stopped
    if(UART_available()) {                          // something coming in via UART?
      uint8_t dataL = USART0.RXDATAL;               // read data low  byte
      uint8_t dataH = UART_read();                  // read data high byte
      BUF_push(dataL);                              // push data low  byte to buffer
      BUF_push(dataH);                              // push data high byte to buffer
      if(requested) requested -= 2;                 // decrease requested bytes counter
      checksum += dataL;                            // update checksum
      checksum += dataH;                            // update checksum
      if(!(dataL | dataH)) endofdata = 1;           // set end of data flag on zero-byte
    }
    if(!requested && !endofdata && ((254 - BUF_items()) > TAP_PACKSIZE)) {
      requested = 254 - BUF_items();                // set requested data counter
      if(requested) UART_write(requested);          // request next package
    }
  }

  // Finish writing
  TCA0.SINGLE.CTRLA  &= ~TCA_SINGLE_ENABLE_bm;      // make sure timer is stopped
  TCA0.SINGLE.INTCTRL = 0;                          // disable overflow interrupt
  UART_write(0);                                    // send 'STOP' to PC
  UART_write(checksum); UART_write(checksum >> 8);  // send checksum to PC
  UART_write(BUF_underrun);                         // send underrun flag
  UART_write(pinRead(PIN_SENSE));                   // send RECORD state
  _delay_ms(500);                                   // write a little pause
  pinLow(PIN_MOTOR);                                // stop motor
  while(!pinRead(PIN_SENSE)) {                      // wait for STOP button pressed
    WDT_reset();                                    // reset watchdog
    pinToggle(PIN_LED);                             // flash LED
    _delay_ms(100);                                 // wait a bit
  }
  UART_flushRX();                                   // make sure RX pipeline is empty
}

// TCA0 interrupt service routine (on each overflow to prepare next pulse)
ISR(TCA0_OVF_vect) {
  if(!BUF_available()) BUF_underrun = 1;            // set underrun flag if necessary
  uint16_t tmp = BUF_pull();                        // get length of next pulse
  tmp += (uint16_t)BUF_pull() << 8;                 // high byte
  TCA0.SINGLE.CMP0BUF = tmp;                        // set PWM value for next pulse
  TCA0.SINGLE.PERBUF  = (tmp << 1) - 1;             // set length of next pulse
  if(!tmp) TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm; // stop timer if end of file
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;         // clear interrupt flag
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
#define IEC_PULLUP    2                             // pullup time in us

// Global flags
uint8_t IEC_error;
uint8_t IEC_EOI;
uint8_t IEC_device = 8;

// IEC init
void IEC_init(void) {
  pinPullup(PIN_RST);                               // activate pullup resistors
  pinPullup(PIN_ATN);
  pinPullup(PIN_CLK);
  pinPullup(PIN_DATA);
}

// ===================================================================================
// IEC Protocol Implementation - Low Level Bitbanging Stuff
// ===================================================================================

// IEC bitbanging macros
#define IEC_RST_setHigh()    pinInput(PIN_RST)      // RST  released    -> FALSE
#define IEC_RST_setLow()     pinOutput(PIN_RST)     // RST  pulled down -> TRUE
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
  IEC_RST_setHigh();                                // release RST line
  while(IEC_CLK_isLow());                           // make sure CLK is high
  while(IEC_DATA_isLow());                          // make sure DATA is high
}

// Perform reset
void IEC_reset(void) {
  IEC_RST_setLow();                                 // pull RESET line low
  _delay_ms(20);                                    // wait a bit
  IEC_RST_setHigh();                                // release RESET line
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
  _delay_us(IEC_PULLUP);                            // some time for the weak pull-up
  uint8_t cnt = 250;                                // waiting counter in 4us steps
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
  _delay_us(IEC_PULLUP);                            // some time for the weak pull-up
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
uint8_t IEC_writeBuffer(volatile uint8_t* buf, uint8_t count) {
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
  cli();                                            // timed sequence coming up
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
  while(IEC_CLK_isLow());                           // wait for CLK low
  sei();                                            // timed sequence ended
  return data;                                      // return received data byte
}

// Read data block via fast IEC
void IEC_readBlock(uint16_t cnt) {
  WDT_reset();                                      // reset watchdog
  while(IEC_DATA_isHigh());                         // wait for 'READING BLOCK COMPLETE'
  if(IEC_CLK_isLow()) {                             // 'READ ERROR' ?
    UART_write(1);                                  // send 'READ ERROR' to PC
    IEC_error = 1;                                  // raise IEC error flag
    while(IEC_CLK_isLow());                         // wait for line released
    return;                                         // return
  }
  UART_write(0);                                    // send 'START SENDING BLOCK' to PC
  do {                                              // transfer block data
    uint8_t data = IEC_readAsynch();                // read data byte from IEC
    UART_send(data);                                // send data byte via UART
    if(cnt == 256) IEC_EOI = !data;                 // end of file flag
  } while(--cnt);                                   // loop cnt times
  while(IEC_DATA_isLow());                          // wait for DATA line released
}

// Write byte via fast IEC
void IEC_writeAsynch(uint8_t data) {
  cli();                                            // timed sequence coming up
  while(IEC_CLK_isHigh());                          // wait for 'READY TO RECEIVE BYTE'
  while(IEC_CLK_isLow());                           // wait for 'LETS GO'
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
  __builtin_avr_delay_cycles(6 * MFCYCLES - 6);     // wait 6 floppy cycles
  IEC_CLK_setHigh(); IEC_DATA_setHigh();            // release lines
  while(IEC_CLK_isLow());                           // wait for the pullup
  sei();                                            // timed sequence ended
}

// Write data block via fast IEC
void IEC_writeBlock(uint16_t cnt) {
  if(!cnt) cnt = 256;                               // 256 bytes if cnt = 0
  BUF_reset();                                      // reset buffer
  WDT_reset();                                      // reset watchdog
  UART_write(0);                                    // request sector data from PC
  for(uint16_t i=0; i<cnt; i++)                     // get data block from PC
    BUF_buffer[i] = UART_read();                    // data byte UART -> buffer
  IEC_DATA_setLow();                                // declare 'READY TO SEND BLOCK'
  while(IEC_CLK_isHigh());                          // wait for reply
  IEC_DATA_setHigh();                               // release DATA line
  _delay_us(IEC_PULLUP);                            // for the weak pull-ups
  if(IEC_DATA_isLow()) {                            // DATA still LOW = 'WRITE ERROR'?
    IEC_error = 1;                                  // raise IEC error flag
    while(IEC_DATA_isLow());                        // wait for line released
    return;                                         // return
  }
  for(uint16_t i=0; i<cnt; i++)                     // transfer block data to drive
    IEC_writeAsynch(BUF_buffer[i]);                 // data byte buffer -> IEC
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
  IEC_writeBuffer(BUF_buffer+1, BUF_buffer[0]);     // send command buffer -> device
  IEC_unlisten();                                   // set device to UNLISTEN
  UART_write(IEC_error);                            // send error state
  return IEC_error;                                 // return error state
}

// <length>"M-E"<addrLow><addrHigh><track><#sectors><sector1><sector2>...
void IEC_readTrack(void) {
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  uint8_t cnt = BUF_buffer[7];                      // get number of sectors to read
  while(!IEC_error && cnt--) IEC_readBlock(325);    // read sector and send data via UART
}

// <length>"M-E"<addrLow><addrHigh><track><#sectors><sector1><sector2>...
void IEC_writeTrack(void) {
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  uint8_t cnt = BUF_buffer[7];                      // get number of sectors to write
  while(!IEC_error && cnt--) IEC_writeBlock(325);   // read data via UART and write sector
  if(!IEC_error) {                                  // no error until last byte?
    IEC_DATA_setLow();                              // declare 'READY'
    while(IEC_CLK_isHigh());                        // wait for status
    IEC_DATA_setHigh();                             // release DATA line
    _delay_us(IEC_PULLUP);                          // wait for the pullup
    IEC_error = IEC_DATA_isLow();                   // DATA still LOW = 'WRITE ERROR'
  }
  while(IEC_CLK_isLow());                           // wait for CLK line released
  UART_write(IEC_error);                            // send return code to PC
}

// <length>"M-E"<addrLow><addrHigh><startTrack><startSector>
void IEC_loadFile(void) {
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  IEC_EOI = 0;                                      // clear EOI flag
  while(!IEC_error && !IEC_EOI) IEC_readBlock(256); // read sectors and send data via UART
}

// <length>"M-E"<addrLow><addrHigh><tracks><bump><clear><verify>:<name>,<ID1><ID2>
void IEC_format(void) {
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  uint8_t cnt = BUF_buffer[6] + 1;                  // get number of tracks
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
      GETATN, SETATN, RELATN, GETCLK, SETCLK, RELCLK, GETDATA, SETDATA, RELDATA,
      SETRST, RELRST, GETSENSE, MOTORON, MOTOROFF};
      
// Main function
int main(void) {
  // Setup
  _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);             // set clock frequency to 16 MHz
  UART_init();                                        // setup serial communication
  WDT_init();                                         // setup watchdog timer
  RTC_init();                                         // setup RTC as timeout counter
  TAP_init();                                         // setup datasette interface
  IEC_init();                                         // setup IEC interface
  IEC_reset();                                        // reset IEC devices
  pinOutput(PIN_LED);                                 // set LED pin as output
  sei();                                              // enable global interrupts

  // Loop
  while(1) {
    pinLow(PIN_LED);                                  // turn off BUSY LED
    while(!UART_available()) WDT_reset();             // wait for command byte
    pinHigh(PIN_LED);                                 // turn on BUSY LED
    uint8_t cmd = UART_read();                        // read command byte
    CMD_get();                                        // get arguments
    IEC_error = 0;                                    // clear error flag
    switch(cmd) {                                     // take proper action
      // High-level commands
      case 'i':         UART_println(IDENT); break;   // send identification string
      case 'v':         UART_println(VERSION); break; // send version number
      case 'R':         TAP_read(); break;            // start reading from tape
      case 'W':         TAP_write(); break;           // start writing to tape
      case 'r':         IEC_readTrack(); break;       // read track from disk
      case 'w':         IEC_writeTrack(); break;      // write track to disk
      case 'l':         IEC_loadFile(); break;        // load a file from disk
      case 's':         break;                        // save a file to disk
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
      case WRITEBYTE:   IEC_sendByte(BUF_buffer[1]); break;             //  8 08
      case WRITELAST:   IEC_sendLast(BUF_buffer[1]); break;             //  9 09
      case WRITEBYTES:  IEC_writeBuffer(BUF_buffer+1, BUF_buffer[0]); break;  // 10 0A
      case READFAST:    IEC_readBlock(BUF_buffer[1]); break;            // 11 0B
      case WRITEFAST:   IEC_writeBlock(BUF_buffer[1]); break;           // 12 0C
      case OPEN:        break;                                          // 13 0D
      case CLOSE:       break;                                          // 14 0E
      case RESET:       IEC_reset(); break;                             // 15 0F
      case RELEASE:     IEC_release(); break;                           // 16 10
      case GETDEVICE:   UART_write(IEC_device); break;                  // 17 11
      case SETDEVICE:   IEC_device = BUF_buffer[1]; break;              // 18 12
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
      case SETRST:      IEC_RST_setLow(); break;                        // 31 1F
      case RELRST:      IEC_RST_setHigh(); break;                       // 32 20
      case GETSENSE:    UART_write(pinRead(PIN_SENSE) == 0); break;     // 33 21
      case MOTORON:     pinHigh(PIN_MOTOR); break;                      // 34 22
      case MOTOROFF:    pinLow(PIN_MOTOR); break;                       // 35 23
      default:          break;
    }
    if(cmd <= RELRST) UART_write(IEC_error);
  }
}
