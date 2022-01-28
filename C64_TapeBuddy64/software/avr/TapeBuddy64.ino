// ===================================================================================
// Project:   TapeBuddy64 - Connect your Commodore Datasette to a PC
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
// TapeBuddy64 is a simple and inexpensive adapter that can interface a Commodore
// Datasette to your PC via USB in order to read from or write to tapes.
//
// When reading from tape, the ATtiny measures the pulse lengths with its timer/
// counter B (TCB) in frequency measurement mode. The TCB continuously counts upwards
// at 250kHz until a falling edge is detected at the READ output of the Datasette.
// The current value of the counter is automatically saved, the counter is then reset
// to zero and restarted. In addition, an interrupt is triggered, in whose service
// routine the stored value is read out, divided by 2 and pushed into the output
// buffer for transmission to the PC.
//
// When writing to tape, the ATtiny uses its timer/counter A (TCA) to output a PWM
// signal with 50% duty cycle at the WRITE pin of the cassette port. After each
// period a timer overflow interrupt is triggered, in whose service routine the pulse
// length of the next period is set according to the respective next TAP value which
// is pulled from the buffer. This creates a very precise, pulse-length modulated
// square wave.
//
// References:
// -----------
// - https://ist.uwaterloo.ca/~schepers/formats/TAP.TXT
// - https://wav-prg.sourceforge.io/tape.html
// - https://github.com/francescovannini/truetape64
//
// Wiring:
// -------
//                              +-\/-+
//                        Vcc  1|°   |14  GND
//           --- !SS AIN4 PA4  2|    |13  PA3 AIN3 SCK ---- TAPE SENSE
//           ------- AIN5 PA5  3|    |12  PA2 AIN2 MISO --- TAPE WRITE
//  READ LED --- DAC AIN6 PA6  4|    |11  PA1 AIN1 MOSI --- TAPE READ
// WRITE LED ------- AIN7 PA7  5|    |10  PA0 AIN0 UPDI --- UPDI
//       USB -------- RXD PB3  6|    |9   PB0 AIN11 SCL --- 
//       USB ---------TXD PB2  7|    |8   PB1 AIN10 SDA --- TAPE MOTOR
//                              +----+
//
// Compilation Settings:
// ---------------------
// Core:    megaTinyCore (https://github.com/SpenceKonde/megaTinyCore)
// Board:   ATtiny1614/1604/814/804/414/404
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
// - Set the serial mode switch on your TapeBuddy64 to "UART".
// - Connect your TapeBuddy64 to your Commodore Datasette.
// - Connect your TapeBuddy64 to a USB port of your PC.
// - Execute the desired Python script on your PC.
//
// The Python scripts control the device via four simple serial commands:
//
// Command    Function                            Response
// "i"        transmit indentification string     "TapeBuddy64\n"
// "v"        transmit firmware version number    e.g. "v1.0\n"
// "r"        read file from tape                 send raw data stream
// "w"        write file to tape                  receive raw data stream


// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================

// Libraries
#include <avr/io.h>               // for GPIO
#include <avr/interrupt.h>        // for interrupts
#include <util/delay.h>           // for delays

// Pin definitions
#define PIN_READ      PA1         // pin connected to READ on datasette port
#define PIN_WRITE     PA2         // pin connected to WRITE on datasette port
#define PIN_SENSE     PA3         // pin connected to SENSE on datasette port
#define PIN_LED_R     PA6         // pin connected to READ LED
#define PIN_LED_W     PA7         // pin connected to WRITE LED
#define PIN_PWM       PB0         // pin for PWM generation
#define PIN_MOTOR     PB1         // pin connected to MOTOR control
#define PIN_TXD       PB2         // UART TX pin connected to usb-to-serial converter
#define PIN_RXD       PB3         // UART RX pin connected to usb-to-serial converter

// Configuration parameters
#define UART_BAUD     460800      // UART: baud rate (max 1/16 of F_CPU)
#define TAP_WAIT_PLAY 10          // TAPE: time in seconds to wait for PLAY pressed (max 63)
#define TAP_TIMEOUT   25          // TAPE: time in seconds to wait for pulses (max 63)
#define TAP_PACKSIZE  32          // length of data package for writing (max TAP_BUF_LEN / 2)
#define TAP_BUF_LEN   128         // tape buffer length (must be power of 2)

// Identifiers
#define VERSION     "1.1"         // version number sent via serial if requested
#define IDENT       "TapeBuddy64" // identifier sent via serial if requested

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
// Datasette Port Interface Implementation - Setup
// ===================================================================================

// Variables, definitions and macros
volatile uint8_t TAP_buf[TAP_BUF_LEN];              // tape buffer
volatile uint8_t TAP_buf_ovf;                       // tape buffer overflow flag
volatile uint8_t TAP_buf_unr;                       // tape buffer underrun flag
volatile uint8_t TAP_buf_head;                      // tape buffer pointer for writing
volatile uint8_t TAP_buf_tail;                      // tape buffer pointer for reading
volatile uint8_t TAP_timer_ovf;                     // tape timer overflow flag
volatile uint8_t TAP_started;                       // tape pulses started flag
#define TAP_available()  (TAP_buf_head != TAP_buf_tail)

// Setup TAP interface
void TAP_init(void) {
  // Setup Pins
  pinOutput(PIN_PWM);                               // set PWM pin as output
  pinOutput(PIN_LED_R);                             // set READ LED pin as output
  pinOutput(PIN_LED_W);                             // set WRITE LED pin as output
  pinPullup(PIN_READ);                              // enable pullup on READ pin
  pinPullup(PIN_SENSE);                             // enable pullup on SENSE pin
  pinOutput(PIN_WRITE);                             // set WRITE pin as output
  pinOutput(PIN_MOTOR);                             // set MOTOR pin as output  
  pinIntEnable(PIN_SENSE);                          // enable interrupt on SENSE pin

  // Setup event system with channel between READ pin and TCB
  EVSYS.ASYNCCH0   = PIN_READ + 0x0A;               // READ pin as input for asynch channel 0
  EVSYS.ASYNCUSER0 = EVSYS_ASYNCUSER0_ASYNCCH0_gc;  // TCB as output for asynch channel 0

  // Setup event system with channel from pin PB1 to PA2 (PWM pass through for WRITE)
  EVSYS.ASYNCCH1   = PIN_PWM + 0x02;                // PWM as input for asynch channel 1
  EVSYS.ASYNCUSER8 = EVSYS_ASYNCUSER8_ASYNCCH1_gc;  // EVOUT0 (PA2) as output
  PORTMUX.CTRLA   |= PORTMUX_EVOUT0_bm;             // enable event out 0

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

// Get number of items in tape buffer
uint8_t TAP_buf_items(void) {
  uint8_t head = TAP_buf_head;
  uint8_t tail = TAP_buf_tail;
  if(head > tail) return(head - tail);
  return(TAP_BUF_LEN - tail + head);
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
    pinToggle(PIN_LED_R);                           // flash LED
    _delay_ms(100);                                 // short delay
  }
  if(RTC_timeout) {                                 // timout waiting for PLAY?
    pinLow(PIN_LED_R);                              // switch off LED
    UART_write(1);                                  // send 'TIMEOUT' to PC
    return;                                         // stop and return
  }

  // Prepare reading from tape
  uint16_t checksum = 0;                            // 16-bit checksum
  uint8_t  count = 0;                               // data byte counter
  TAP_started   = 0;                                // first pulse flag
  TAP_timer_ovf = 1;                                // ignore first pulse
  TAP_buf_ovf   = 0;                                // reset tape buffer overflow flag
  TAP_buf_head  = 0; TAP_buf_tail = 0;              // reset tape buffer
  TCB0.CNT = 0;                                     // reset pulse timer
  UART_write(0);                                    // send 'PLAY' to PC
  pinHigh(PIN_LED_R);                               // light up LED
  _delay_ms(100);                                   // wait for motor to speed up
  RTC_start(TAP_TIMEOUT);                           // start timeout counter for pulses
  TCA0.SINGLE.CTRLA  |= TCA_SINGLE_ENABLE_bm;       // start TCA as clock source
  TCB0.CTRLA |= TCB_ENABLE_bm;                      // start TCB

  // Read from tape
  while(1) {
    // Check finish conditions
    if(pinRead(PIN_SENSE)) break;                   // finish if STOP was pressed on tape
    if(RTC_timeout) break;                          // finish if no pulses for certain time
    if(TAP_buf_ovf) break;                          // finish if buffer overflow occured

    // Check timer overflow conditions
    if(TAP_started) {                               // pulses already started?
      cli();                                        // atomic sequence ahead
      if(TAP_timer_ovf) {                           // overflow ahead?
        if(TCB0.CNT < 65500) {                      // is it an actual overflow?          
          TAP_buf[TAP_buf_head++] = 0xFF;           // low  byte TAP value of max pause
          TAP_buf[TAP_buf_head++] = 0x7F;           // high byte TAP value of max pause
          TAP_buf_head &= (TAP_BUF_LEN - 1);        // limit buffer pointer
          if(TAP_buf_head==TAP_buf_tail) TAP_buf_ovf = 1; // set overflow flag if necessary
          TAP_timer_ovf = 0;                        // clear overflow flag
        }
      } else {                                      // no overflow ahead?
        if(TCB0.CNT > 65500) TAP_timer_ovf = 1;     // check if timer overflow is coming
      }
      sei();                                        // atomic sequence ended
    }

    // Check send data conditions
    if(TAP_available() && UART_ready()) {           // ready to send something via UART?
      uint8_t data  = TAP_buf[TAP_buf_tail++];      // get data byte
      TAP_buf_tail &= (TAP_BUF_LEN - 1);            // limit buffer pointer
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
  UART_write(TAP_buf_ovf);                          // send overflow flag
  pinLow(PIN_MOTOR);                                // stop motor
  while(!pinRead(PIN_SENSE)) {                      // wait for STOP button pressed
    pinToggle(PIN_LED_R);                           // toggle write LED
    _delay_ms(100);                                 // wait a bit
  }
  pinLow(PIN_LED_R);                                // switch off LED
}

// TCB0 interrupt service routine (on each pulse capture)
ISR(TCB0_INT_vect) {
  uint16_t tmp = TCB0.CCMP;                         // read pulse length in 4 us
  if((TAP_timer_ovf) && (tmp<65500)) tmp = 0xFFFF;  // set max value on overflow
  tmp >>= 1;                                        // make it 8 us
  if(tmp) {                                         // minimum pulse length?
    TAP_buf[TAP_buf_head++] = tmp;                  // low byte TAP value
    TAP_buf[TAP_buf_head++] = tmp >> 8;             // high byte TAP value
    TAP_buf_head &= (TAP_BUF_LEN - 1);              // limit buffer pointer
    if(TAP_buf_head==TAP_buf_tail) TAP_buf_ovf = 1; // set overflow flag if necessary
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
    pinToggle(PIN_LED_W);                           // toggle write LED
    _delay_ms(100);                                 // wait a bit
  }
  if(RTC_timeout) {                                 // timout occured?
    UART_write(1);                                  // send 'TIMEOUT' to PC
    pinLow(PIN_LED_W);                              // switch off WRITE LED
    return;                                         // stop and return
  }

  // Fill tape buffer with first data package
  UART_write(0);                                    // send 'READY' to PC
  UART_write(TAP_BUF_LEN - 2);                      // request first data package
  for(uint8_t i=0; i<TAP_BUF_LEN - 2; i++) {        // read first data package
    data = UART_read();                             // get data byte
    TAP_buf[i] = data;                              // write data byte to buffer
    checksum += data;                               // update checksum
  }
  TAP_buf_head = TAP_BUF_LEN - 2; TAP_buf_tail = 0; // set tape buffer pointers
  TAP_buf_unr  = 0;                                 // clear buffer underrun flag

  // Prepare writing to tape
  pinHigh(PIN_LED_W);                               // light up WRITE LED
  _delay_ms(100);                                   // wait for motor to speed up
  uint16_t tmp = TAP_buf[TAP_buf_tail++];           // get length of first pulse
  tmp += (uint16_t)TAP_buf[TAP_buf_tail++] << 8;    // high byte
  TCA0.SINGLE.CNT = 0;                              // reset TCA counter
  TCA0.SINGLE.CMP0BUF = tmp;                        // set TCA compare (duty)
  TCA0.SINGLE.PERBUF  = (tmp << 1) - 1;             // set TCA period (pulse length)
  TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm;          // enable overflow interrupt
  TCA0.SINGLE.CTRLA  |= TCA_SINGLE_ENABLE_bm;       // start timer for PWM

  // Write to tape
  while(1) {
    if(pinRead(PIN_SENSE)) break;                   // finish if STOP was pressed on tape
    if(TAP_buf_unr) break;                          // finish if buffer underrun
    if(~TCA0.SINGLE.CTRLA & TCA_SINGLE_ENABLE_bm) break; // finish if timer was stopped
    if(UART_available()) {                          // something coming in via UART?
      uint8_t dataL = USART0.RXDATAL;               // read data low  byte
      uint8_t dataH = UART_read();                  // read data high byte
      TAP_buf[TAP_buf_head++] = dataL;              // write data low  byte to buffer
      TAP_buf[TAP_buf_head++] = dataH;              // write data high byte to buffer
      TAP_buf_head &= (TAP_BUF_LEN - 1);            // limit buffer pointer
      if(requested) requested -= 2;                 // decrease requested bytes counter
      checksum += dataL;                            // update checksum
      checksum += dataH;                            // update checksum
      if(!(dataL | dataH)) endofdata = 1;           // set end of data flag on zero-byte
    }
    if(!requested && !endofdata && ((TAP_BUF_LEN - TAP_buf_items()) > TAP_PACKSIZE)) {
      requested = TAP_BUF_LEN - TAP_buf_items() - 2;// set requested data counter
      UART_write(requested);                        // request next package
    }
  }

  // Finish writing
  TCA0.SINGLE.CTRLA  &= ~TCA_SINGLE_ENABLE_bm;      // make sure timer is stopped
  TCA0.SINGLE.INTCTRL = 0;                          // disable overflow interrupt
  UART_write(0);                                    // send 'STOP' to PC
  UART_write(checksum); UART_write(checksum >> 8);  // send checksum to PC
  UART_write(TAP_buf_unr);                          // send underrun flag
  UART_write(pinRead(PIN_SENSE));                   // send RECORD state
  _delay_ms(500);                                   // write a little pause
  pinLow(PIN_MOTOR);                                // stop motor
  while(!pinRead(PIN_SENSE)) {                      // wait for STOP button pressed
    pinToggle(PIN_LED_W);                           // toggle write LED
    _delay_ms(100);                                 // wait a bit
  }
  pinLow(PIN_LED_W);                                // switch off WRITE LED
  UART_flushRX();                                   // make sure RX pipeline is empty
}

// TCA0 interrupt service routine (on each overflow to prepare next pulse)
ISR(TCA0_OVF_vect) {
  uint16_t tmp = TAP_buf[TAP_buf_tail];             // get length of next pulse
  tmp += (uint16_t)TAP_buf[TAP_buf_tail + 1] << 8;  // high byte
  TCA0.SINGLE.CMP0BUF = tmp;                        // set PWM value for next pulse
  TCA0.SINGLE.PERBUF  = (tmp << 1) - 1;             // set length of next pulse
  if(!tmp) TCA0.SINGLE.CTRLA &= ~TCA_SINGLE_ENABLE_bm; // stop timer if end of file
  if(TAP_buf_head==TAP_buf_tail) TAP_buf_unr = 1;   // set underrun flag if necessary
  TAP_buf_tail += 2;                                // increase buffer pointer
  TAP_buf_tail &= (TAP_BUF_LEN - 1);                // limit buffer pointer
  TCA0.SINGLE.INTFLAGS = TCA_SINGLE_OVF_bm;         // clear interrupt flag
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
      case 'w':   TAP_write(); break;               // start writing to tape
      default:    break;
    }
  }
}
