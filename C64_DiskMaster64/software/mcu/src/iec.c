// ===================================================================================
// IEC Bus Communication Functions
// ===================================================================================

#include "delay.h"
#include "timer.h"
#include "iec.h"

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

// Global flags
__bit IEC_error;
__bit IEC_EOI;
__xdata uint8_t IEC_device;

// IEC init
void IEC_init(void) {
  IEC_device = 8;                                   // set default device number
  PIN_output_OD(PIN_ATN);                           // pins to open-drain output
  PIN_output_OD(PIN_CLK);
  PIN_output_OD(PIN_DATA);
  T0_init();                                        // init timer0
}

// ===================================================================================
// IEC Protocol Implementation - Low Level Bitbanging Stuff
// ===================================================================================

// Release all control lines
void IEC_release(void) {
  IEC_ATN_setHigh();                                // release ATN line
  IEC_CLK_setHigh();                                // release CLK line
  IEC_DATA_setHigh();                               // release DATA line
  while(IEC_CLK_isLow());                           // make sure CLK is high
  while(IEC_DATA_isLow());                          // make sure DATA is high
}

// Send a data byte
__bit IEC_sendByte(uint8_t data) {
  uint8_t i;
  IEC_CLK_setHigh();                                // declare 'READY TO SEND'
  while(IEC_DATA_isLow());                          // wait for listener 'READY FOR DATA'
  DLY_us(40);                                       // wait 'NON-EOI RESPONSE TIME' (max 200us)

  IEC_CLK_setLow();                                 // declare 'TRANSMISSION STARTS'
  for(i=8; i; i--, data>>=1) {                      // 8 bits, LSB first
    (data & 1) ? (IEC_DATA_setHigh()) : (IEC_DATA_setLow());  // set DATA line according to bit
    DLY_us(60);                                     // wait 'BIT SETUP TIME' (min 60us)
    IEC_CLK_setHigh();                              // declare 'DATA VALID'
    DLY_us(60);                                     // wait 'DATA VALID TIME' (min 60us)
    IEC_CLK_setLow();                               // end of clock period
  }
  IEC_DATA_setHigh();                               // release DATA line for handshake
  i = 250;                                          // waiting counter in 4us steps
  DLY_us(IEC_PULLUP);                               // some time for the weak pull-ups
  while(--i && IEC_DATA_isHigh()) DLY_us(4);        // wait for listener 'DATA ACCEPTED' (max 1ms)
  if(IEC_DATA_isHigh()) {                           // no response??
    IEC_error = 1;                                  // raise IEC_error
    IEC_CLK_setHigh();                              // release CLK line
  }
  DLY_us(100);                                      // wait 'BETWEEN BYTES TIME' (min 100us)
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
  uint8_t i, data;
  IEC_EOI = 0;                                      // clear EOI flag
  while(IEC_CLK_isLow());                           // wait for talker 'READY TO SEND'
  IEC_DATA_setHigh();                               // declare 'READY FOR DATA'

  // Wait for transmission start or EOI
  i = 25;                                           // timer to identify EOI (last byte)
  while(IEC_CLK_isHigh()) {                         // wait for talker 'TRANSMISSION STARTS' or 'EOI'
    if(!--i) break;                                 // leave when timeout
    DLY_us(10);                                     // 20 * 10us = 200us, EOI after 200us timeout
  }

  // Handle EOI if necessary
  if(!i) {                                          // timeout? => EOI
    IEC_EOI = 1;                                    // set EOI flag
    IEC_DATA_setLow();                              // declare 'EOI RECEIVED'
    DLY_us(60);                                     // wait 'EOI RESPONSE HOLD TIME' (min 60us)
    IEC_DATA_setHigh();                             // declare 'READY FOR DATA'
    while(IEC_CLK_isHigh());                        // wait for talker 'TRANSMISSION STARTS'
  }

  // Receive the data byte
  data = 0;                                         // variable to store received byte
  for(i=8; i; i--) {                                // 8 bits, LSB first
    data >>= 1;                                     // shift byte to the right
    while(IEC_CLK_isLow());                         // wait for talker 'DATA VALID'
    if(IEC_DATA_isHigh()) data |= 0x80;             // read the data bit
    while(IEC_CLK_isHigh());                        // wait end of clock period
  }
  IEC_DATA_setLow();                                // declare 'DATA ACCEPTED'

  // Release system line if last byte received
  if(IEC_EOI) {                                     // was it the last byte?
    DLY_us(60);                                     // wait 'EOI ACKNOWLEDGE TIME' (min 60us)
    IEC_DATA_setHigh();                             // release DATA line
  }

  return data;                                      // return received data byte
}

// Start sending under attention
__bit IEC_ATN_start(void) {
  IEC_ATN_setLow();                                 // declare 'ATTENTION!'
  IEC_CLK_setLow();                                 // pull CLK line true
  DLY_us(1000);                                     // wait 'ATN RESPONSE TIME' (1000us)
  if(IEC_DATA_isHigh()) {                           // no response?
    IEC_error = 1;                                  // raise error
    IEC_ATN_setHigh(); IEC_CLK_setHigh();           // release control lines
  }
  return IEC_error;                                 // return error state
}

// Stop sending under attention
void IEC_ATN_stop(void) {
  IEC_ATN_setHigh();                                // release ATN line
  DLY_us(20);                                       // wait 'ATN RELEASE TIME' (20us - 100us)
}

// Perform turnaround -> Disk Drive becomes talker, ATtiny becomes listener
void IEC_turnaround(void) {
  IEC_DATA_setLow();                                // take over DATA line
  IEC_CLK_setHigh();                                // declare 'I AM LISTENER NOW'
  DLY_us(IEC_PULLUP);                               // some time for the weak pull-ups
  while(IEC_CLK_isHigh());                          // wait for listener 'I AM TALKER NOW'
}

// ===================================================================================
// IEC Protocol Implementation - Basic Control Functions
// ===================================================================================

// Send 'LISTEN' command to the bus
__bit IEC_listen(uint8_t device, uint8_t secondary) {
  if(IEC_ATN_start()) return 1;                     // start sending under 'ATTENTION'
  if(IEC_sendByte(IEC_LISTEN + device)) return 1;   // send 'LISTEN' + device address
  if(IEC_sendByte(IEC_OPEN_CH + secondary)) return 1;  // open channel (secondary address)
  IEC_ATN_stop();                                   // stop sending under 'ATTENTION'
  return 0;                                         // return success
}

// Send 'UNLISTEN' command to the bus
__bit IEC_unlisten(void) {
  if(IEC_ATN_start()) return 1;                     // start sending under 'ATTENTION'
  if(IEC_sendByte(IEC_UNLISTEN)) return 1;          // send 'UNLISTEN'
  IEC_ATN_stop();                                   // stop sending under 'ATTENTION'
  IEC_release();                                    // release all lines
  return 0;                                         // return success
}

// Send 'TALK' command to the bus and perform turnaround
__bit IEC_talk(uint8_t device, uint8_t secondary) {
  if(IEC_ATN_start()) return 1;                     // start sending under 'ATTENTION'
  if(IEC_sendByte(IEC_TALK + device)) return 1;     // send 'TALK' + device address
  if(IEC_sendByte(IEC_OPEN_CH + secondary)) return 1;  // open channel (secondary address)
  IEC_ATN_stop();                                   // stop sending under 'ATTENTION'
  IEC_turnaround();                                 // turnaround; device becomes talker
  return 0;                                         // return success
}

// Send 'UNTALK' command to the bus
__bit IEC_untalk(void) {
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
__bit IEC_sendStr(__xdata uint8_t* str) {
  while(*str && !IEC_error) IEC_sendByte(*str++);   // send each character of string
  if(!IEC_error) IEC_sendLast(0x0D);                // send 'RETURN' char
  return IEC_error;                                 // return error state
}

// Send buffer via IEC bus
__bit IEC_writeBuffer(__xdata uint8_t* buf, uint8_t count) {
  while(count && !IEC_error) {                      // until no more bytes or error
    if(--count) IEC_sendByte(*buf++);               // send data byte buffer -> IEC
    else        IEC_sendLast(*buf);                 // send last byte with EOI
  }
  return IEC_error;                                 // return error state
}

// ===================================================================================
// IEC Protocol Implementation - Fast Asynchronous IEC Data Transmissions
// ===================================================================================

// Read byte via fast IEC
uint8_t IEC_readAsynch(void) {
  uint8_t data = 0;                                 // preload data byte
  T0_setperiod(8);                                  // set timer period to 8 floppy cycles
  EA = 0;                                           // disable interrupts
  while(IEC_CLK_isHigh());                          // wait for 'READY TO SEND BYTE'
  T0_start();                                       // start timer
  T0_waitperiod();                                  // wait 8 floppy cycles
  T0_waitperiod();                                  // wait 8 floppy cycles
  if(IEC_CLK_isLow())  data |= 0b10000000;          // get data bit 7
  if(IEC_DATA_isLow()) data |= 0b00100000;          // get data bit 5
  T0_waitperiod();                                  // wait 8 floppy cycles
  if(IEC_CLK_isLow())  data |= 0b01000000;          // get data bit 6
  if(IEC_DATA_isLow()) data |= 0b00010000;          // get data bit 4
  T0_waitperiod();                                  // wait 8 floppy cycles
  if(IEC_CLK_isLow())  data |= 0b00001000;          // get data bit 3
  if(IEC_DATA_isLow()) data |= 0b00000010;          // get data bit 1
  T0_waitperiod();                                  // wait 8 floppy cycles
  if(IEC_CLK_isLow())  data |= 0b00000100;          // get data bit 2
  if(IEC_DATA_isLow()) data |= 0b00000001;          // get data bit 0
  EA = 1;                                           // enable interrupts again
  T0_stop();                                        // stop timer
  while(IEC_CLK_isLow());                           // wait for CLK released
  return data;                                      // return received data byte
}

// Write byte via fast IEC
void IEC_writeAsynch(uint8_t data) {
  T0_setperiod(3);                                  // set timer period to 3 floppy cycles
  while(IEC_CLK_isHigh());                          // wait for 'READY TO RECEIVE'
  IEC_DATA_setHigh();                               // release DATA line
  EA = 0;                                           // disable interrupts
  while(IEC_CLK_isLow());                           // wait for 'LETS GO'
  T0_start();                                       // start timer
  if(data & 0b00001000) IEC_CLK_setLow();           // set data bit 3
  if(data & 0b00000010) IEC_DATA_setLow();          // set data bit 1
  T0_waitperiod();                                  // wait 3 floppy cycles
  T0_waitperiod();                                  // wait 3 floppy cycles
  IEC_CLK_setHigh(); IEC_DATA_setHigh();            // initially '0' - bits
  if(data & 0b00000100) IEC_CLK_setLow();           // set data bit 2
  if(data & 0b00000001) IEC_DATA_setLow();          // set data bit 0
  T0_waitperiod();                                  // wait 3 floppy cycles
  T0_waitperiod();                                  // wait 3 floppy cycles
  T0_waitperiod();                                  // wait 3 floppy cycles
  IEC_CLK_setHigh(); IEC_DATA_setHigh();            // initially '0' - bits
  if(data & 0b10000000) IEC_CLK_setLow();           // set data bit 7
  if(data & 0b00100000) IEC_DATA_setLow();          // set data bit 5
  T0_waitperiod();                                  // wait 3 floppy cycles
  T0_waitperiod();                                  // wait 3 floppy cycles
  IEC_CLK_setHigh(); IEC_DATA_setHigh();            // initially '0' - bits
  if(data & 0b01000000) IEC_CLK_setLow();           // set data bit 6
  if(data & 0b00010000) IEC_DATA_setLow();          // set data bit 4
  T0_waitperiod();                                  // wait 3 floppy cycles
  T0_waitperiod();                                  // wait 3 floppy cycles
  IEC_CLK_setHigh(); IEC_DATA_setHigh();            // release lines
  EA = 1;                                           // enable interrupts again
  T0_stop();                                        // stop timer
  while(IEC_CLK_isLow());                           // wait for the pullup
}
