// ===================================================================================
// Project:   DiskMaster64 - USB to Commodore Floppy Disk Drive Adapter
// Version:   v1.2
// Year:      2022
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// Firmware for the DiskMaster64 adaper. The DiskMaster64 adapter bridges the gap
// between your PC and the Commodore 1541(II) floppy disk drives. It communicates
// with the floppy disk drives via bit banging of the IEC protocol and with the PC
// via USB_CDC. The adapter also supports a proprietary IEC protocol for faster 
// data transmission.
//
// References:
// -----------
// - DiskBuddy64: https://github.com/wagiminator/C64-Collection
// - Michael Steil: https://www.pagetable.com/?p=568
// - Blinkinlabs: https://github.com/Blinkinlabs/ch554_sdcc
// - Deqing Sun: https://github.com/DeqingSun/ch55xduino
// - Ralph Doncaster: https://github.com/nerdralph/ch554_sdcc
// - WCH Nanjing Qinheng Microelectronics: http://wch.cn
//
// Wiring:
// -------
//                                       +--\/--+
//            LED --------- SCS A1 P14  1|°     |10 V33
//       IEC  ATN --- PWM1 MOSI A2 P15  2|      |9  Vcc
//       IEC DATA ------ MISO RXD1 P16  3|      |8  GND
//       IEC  CLK ------- SCK TXD1 P17  4|      |7  P37 UDM --- USB D-
//                ---------------- RST  6|      |6  P36 UDP --- USB D+
//                                       +------+
//
// Compilation Instructions:
// -------------------------
// - Chip:  CH551, CH552 or CH554
// - Clock: 16 MHz internal
// - Adjust the firmware parameters in include/config.h if necessary.
// - Make sure SDCC toolchain and Python3 with PyUSB and PySerial is installed.
// - Press BOOT button on the board and keep it pressed while connecting it via USB
//   with your PC.
// - Run 'make flash'.
//
// Operating Instructions:
// -----------------------
// - Connect the adapter to your Commodore 1541(II) floppy disk drive via the IEC
//   connector.
// - Connect the adapter to your PC via USB. It should be detected as a CDC device.
// - Switch on your floppy disk drive.
// - Use the provided Python scripts on your PC.


// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================

// Libraries
#include "src/config.h"                   // user configurations
#include "src/system.h"                   // system clock functions
#include "src/delay.h"                    // for delays
#include "src/timer.h"                    // for timers
#include "src/usb_cdc.h"                  // for USB-CDC serial
#include "src/iec.h"                      // for IEC bus functions

// Prototypes for used interrupts
void USB_interrupt(void);
void DeviceUSBInterrupt(void) __interrupt (INT_NO_USB) {
  USB_interrupt();
}

// ===================================================================================
// Alternative Watchdog Implementation
// ===================================================================================

// Watchdog variables
volatile __idata uint8_t WDT_counter;               // watchdog cycle counter

// Start watchdog
inline void WDT_begin(void) {
  WDT_counter = T1_CYCLES;                          // reset cycle counter (~4sec)
  T1_init();                                        // init timer1
  T1_start();                                       // start timer1
}

// Restart watchdog
inline void WDT_restart(void) {
  T1_reset();                                       // reset timer1
  WDT_counter = T1_CYCLES;                          // reset cycle counter (~4sec)
}

// Watchdog interrupt service routine
void WDT_interrupt(void) __interrupt (INT_NO_TMR1) {
  if(!--WDT_counter) RST_now();                     // dec counter; reset when zero
}

// ===================================================================================
// Buffer Implementation
// ===================================================================================

// Command/argument/data buffer
__xdata uint8_t BUF_buffer[325];                    // this is the buffer

// Get command arguments
void CMD_get(void) {
  __xdata uint8_t* ptr = BUF_buffer;                // command buffer pointer
  uint8_t cnt = CDC_read();                         // read first byte = length
  *ptr++ = cnt;                                     // write length to buffer
  while(cnt--) *ptr++ = CDC_read();                 // read bytes USB -> buffer
}

// ===================================================================================
// IEC High Level Functions
// ===================================================================================

// Read data from IEC bus and send it via USB
__bit IEC_readRaw(void) {
  IEC_EOI = 0;                                      // clear EOI flag
  while(!IEC_EOI && !IEC_error) {                   // while bytes available and no error
    CDC_write(IEC_readByte());                      // transfer data byte IEC -> USB
  }
  CDC_flush();                                      // flush USB pipe
  return IEC_error;                                 // return error state
}

// Read data from IEC bus and send it via USB with EOI flag in front of each byte
__bit IEC_readBytes(void) {
  IEC_EOI = 0;                                      // clear EOI flag
  while(!IEC_EOI && !IEC_error) {                   // while bytes available and no error
    uint8_t data = IEC_readByte();                  // read data byte from device
    CDC_write(IEC_EOI | IEC_error << 1);            // send IEC and ERROR flag
    CDC_write(data);                                // send data byte via USB to PC
  }
  CDC_flush();                                      // flush USB pipe
  return IEC_error;                                 // return error state
}

// Read data block via fast IEC
void IEC_readBlock(uint16_t cnt) {
  uint8_t data;                                     // for data bytes
  uint16_t len = cnt;                               // buffer length
  __xdata uint8_t* ptr = BUF_buffer;                // buffer pointer
  WDT_restart();                                    // restart watchdog
  while(IEC_DATA_isHigh());                         // wait for 'READING BLOCK COMPLETE'
  if(IEC_CLK_isLow()) {                             // 'READ ERROR' ?
    CDC_writeflush(1);                              // send 'READ ERROR' to PC
    IEC_error = 1;                                  // raise IEC error flag
    while(IEC_CLK_isLow());                         // wait for line released
    return;                                         // return
  }
  
  CDC_writeflush(0);                                // send 'START SENDING BLOCK' to PC
  do {                                              // transfer block data
    data = IEC_readAsynch();                        // read data byte from IEC
    *ptr++ = data;                                  // write data byte to buffer
    if(cnt == 256) IEC_EOI = !data;                 // end of file flag
  } while(--cnt);                                   // loop cnt times
  ptr = BUF_buffer;                                 // reset buffer pointer
  while(len--) CDC_write(*ptr++);                   // send buffer to PC
  CDC_flush();                                      // flush CDC
  while(IEC_DATA_isLow());                          // wait for DATA line released
}

// Write data block via fast IEC
void IEC_writeBlock(uint16_t cnt) {
  uint16_t len;                                     // buffer length
  __xdata uint8_t* ptr = BUF_buffer;                // buffer pointer
  if(!cnt) cnt = 256;                               // 256 bytes if cnt = 0
  len = cnt;                                        // save length
  WDT_restart();                                    // restart watchdog
  CDC_writeflush(0);                                // request sector data from PC
  while(len--) *ptr++ = CDC_read();                 // read data block from PC to buffer
  ptr = BUF_buffer;                                 // reset buffer pointer
  IEC_DATA_setLow();                                // declare 'READY TO SEND BLOCK'
  /*
  while(IEC_CLK_isHigh());                          // wait for reply
  IEC_DATA_setHigh();                               // release DATA line
  DLY_us(IEC_PULLUP);                               // for the weak pull-ups
  if(IEC_DATA_isLow()) {                            // DATA still LOW = 'WRITE ERROR'?
    IEC_error = 1;                                  // raise IEC error flag
    while(IEC_DATA_isLow());                        // wait for line released
    return;                                         // return
  }
  */
  while(cnt--) IEC_writeAsynch(*ptr++);             // transfer block data to drive
}

// Get status from device
void IEC_getStatus(void) {
  IEC_talk(IEC_device, 0x0F);                       // device should talk on channel 15
  IEC_readRaw();                                    // read from device and write via CDC
  IEC_untalk();                                     // send 'UNTALK'
  CDC_println("");                                  // send 'END OF MESSAGE' via USB
}

// Send command from buffer to device via IEC
uint8_t IEC_sendCommand(void) {
  IEC_listen(IEC_device, 0x0F);                     // set device to LISTEN
  IEC_writeBuffer(BUF_buffer+1, BUF_buffer[0]);     // send command buffer -> device
  IEC_unlisten();                                   // set device to UNLISTEN
  CDC_writeflush(IEC_error);                        // send error state
  return IEC_error;                                 // return error state
}

// <length>"M-E"<addrLow><addrHigh><track><#sectors><sector1><sector2>...
void IEC_readTrack(void) {
  uint8_t cnt;
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  cnt = BUF_buffer[7];                              // get number of sectors to read
  while(!IEC_error && cnt--) IEC_readBlock(325);    // read sector and send data via USB
}

// <length>"M-E"<addrLow><addrHigh><track><#sectors><sector1><sector2>...
void IEC_writeTrack(void) {
  uint8_t cnt;
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  cnt = BUF_buffer[7];                              // get number of sectors to write
  while(!IEC_error && cnt--) IEC_writeBlock(325);   // read data via CDC and write sector
  if(!IEC_error) {                                  // no error until last byte?
    IEC_DATA_setLow();                              // declare 'READY'
    while(IEC_CLK_isHigh());                        // wait for status
    IEC_DATA_setHigh();                             // release DATA line
    DLY_us(IEC_PULLUP);                             // wait for the pullup
    IEC_error = IEC_DATA_isLow();                   // DATA still LOW = 'WRITE ERROR'
  }
  while(IEC_CLK_isLow());                           // wait for CLK line released
  CDC_writeflush(IEC_error);                        // send return code to PC
}

// <length>"M-E"<addrLow><addrHigh><startTrack><startSector>
void IEC_loadFile(void) {
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  IEC_EOI = 0;                                      // clear EOI flag
  while(!IEC_error && !IEC_EOI) IEC_readBlock(256); // read sectors and send data via CDC
}

// <length>"M-E"<addrLow><addrHigh><tracks><bump><clear><verify>:<name>,<ID1><ID2>
void IEC_format(void) {
  uint8_t cnt;
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  cnt = BUF_buffer[6] + 1;                          // get number of tracks
  while(!IEC_error && cnt--) {                      // for each track:
    WDT_restart();                                  // restart watchdog
    while(IEC_DATA_isHigh());                       // wait for track complete
    IEC_error = IEC_CLK_isLow();                    // get return code
    CDC_writeflush(IEC_error);                      // send return code to PC
    while(IEC_DATA_isLow());                        // wait for end of signal
  }
}

// <length>"M-R"<addrLow><addrHigh><length>
void IEC_readMem(void) {
  if(IEC_sendCommand()) return;                     // send command to drive (return if error)
  IEC_talk(IEC_device, 0x0F);                       // device should talk on channel 15
  IEC_readRaw();                                    // read from device and write via USB
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

void main(void) {
  // Check if device has to be set to boot mode
  if(RST_getKeep()) {                                 // boot command prior to reset?
    RST_keep(0);                                      // reset KEEP value
    BOOT_now();                                       // jump to bootloader
  }

  // Setup
  CLK_config();                                       // configure system clock
  DLY_ms(10);                                         // wait for clock to stabilize
  CDC_init();                                         // init USB CDC
  IEC_init();                                         // init IEC
  WDT_begin();                                        // start watchdog timer

  // Loop
  while(1) {
    uint8_t cmd;
    PIN_high(PIN_LED);                                // turn off LED
    while(!CDC_available()) WDT_restart();            // wait for command byte
    PIN_low(PIN_LED);                                 // turn on LED
    cmd = CDC_read();                                 // read command byte
    CMD_get();                                        // get arguments
    IEC_error = 0;                                    // clear error flag
    switch(cmd) {                                     // take proper action
      // High-level commands
      case 'i':         CDC_println(IDENT); break;    // send identification string
      case 'v':         CDC_println(VERSION); break;  // send version number
      case 'r':         IEC_readTrack(); break;       // read track from disk
      case 'w':         IEC_writeTrack(); break;      // write track to disk
      case 'l':         IEC_loadFile(); break;        // load a file from disk
//    case 's':         break;                        // save a file to disk
      case 'f':         IEC_format(); break;          // format disk
      case 'm':         IEC_readMem(); break;         // read memory
      case 'c':         IEC_sendCommand(); break;     // send command to IEC device
      case 't':         IEC_getStatus(); break;       // get status from IEC device
      case 'b':         RST_keep(1);RST_now(); break; // reset and start bootloader

      // Low-level commands
      case LISTEN:      IEC_listen(IEC_device, 0x0F); break;            //  1 01
      case UNLISTEN:    IEC_unlisten(); break;                          //  2 02
      case TALK:        IEC_talk(IEC_device, 0x0F); break;              //  3 03
      case UNTALK:      IEC_untalk(); break;                            //  4 04
      case READBYTE:    CDC_write(IEC_readByte()); break;               //  5 05
      case READBYTES:   IEC_readBytes(); break;                         //  6 06
      case READRAW:     IEC_readRaw(); break;                           //  7 07
      case WRITEBYTE:   IEC_sendByte(BUF_buffer[1]); break;             //  8 08
      case WRITELAST:   IEC_sendLast(BUF_buffer[1]); break;             //  9 09
      case WRITEBYTES:  IEC_writeBuffer(BUF_buffer+1, BUF_buffer[0]); break;  // 10 0A
      case READFAST:    IEC_readBlock(BUF_buffer[1]); break;            // 11 0B
      case WRITEFAST:   IEC_writeBlock(BUF_buffer[1]); break;           // 12 0C
//    case OPEN:        break;                                          // 13 0D
//    case CLOSE:       break;                                          // 14 0E
//    case RESET:       break;                                          // 15 0F
      case RELEASE:     IEC_release(); break;                           // 16 10
      case GETDEVICE:   CDC_write(IEC_device); break;                   // 17 11
      case SETDEVICE:   IEC_device = BUF_buffer[1]; break;              // 18 12
      case GETEOI:      CDC_write(IEC_EOI); break;                      // 19 13
      case SETEOI:      IEC_EOI = 1; break;                             // 20 14
      case CLREOI:      IEC_EOI = 0; break;                             // 21 15
      case GETATN:      CDC_write(PIN_read(PIN_ATN) == 0); break;       // 22 16
      case SETATN:      IEC_ATN_setLow(); break;                        // 23 17
      case RELATN:      IEC_ATN_setHigh(); break;                       // 24 18
      case GETCLK:      CDC_write(PIN_read(PIN_CLK) == 0); break;       // 25 19
      case SETCLK:      IEC_CLK_setLow(); break;                        // 26 1A
      case RELCLK:      IEC_CLK_setHigh(); break;                       // 27 1B
      case GETDATA:     CDC_write(PIN_read(PIN_DATA) == 0); break;      // 28 1C
      case SETDATA:     IEC_DATA_setLow(); break;                       // 29 1D
      case RELDATA:     IEC_DATA_setHigh(); break;                      // 30 1E
      default:          break;
    }
    if(cmd <= RELDATA) CDC_writeflush(IEC_error);
  }
}
