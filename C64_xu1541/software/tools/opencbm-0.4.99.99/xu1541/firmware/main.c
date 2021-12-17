/* Name: main.c
 * Project: xu1541
 * Author: Till Harbaum
 * Tabsize: 4
 * Copyright: (c) 2005 by Till Harbaum <till@harbaum.org>
 * License: GPL
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include <util/delay.h>

#include "xu1541.h"

typedef uchar byte_t;

#define MODE_ORIGINAL  0
#define MODE_S1        1
#define MODE_S2        2
#define MODE_PP        3
#define MODE_P2        4
#define MODE_EEPROM    5
static uchar io_mode;

#include "xu1541.h"
#include "s1.h"
#include "s2.h"
#include "pp.h"
#include "p2.h"
#include "xu1541_event_log.h"

#include "xu1541bios.h"

xu1541_bios_data_t bios_data;

#ifdef DEBUG
#define DEBUGF(format, args...) printf_P(PSTR(format), ##args)

/* -------------------------- serial debugging ---------------------------- */
static int uart_putchar(char c, FILE *stream) {
  if (c == '\n')
    uart_putchar('\r', stream);
  loop_until_bit_is_set(UCSRA, UDRE);
  UDR = c;
  return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM(uart_putchar, NULL,
                                         _FDEV_SETUP_WRITE);
#else
#define DEBUGF(format, args...)
#endif

/* ------------------------------------------------------------------------- */

uchar	usbFunctionSetup(uchar data[8], byte_t *replyBuf) {
/*  static uchar replyBuf[4]; */
  char talk;
  uchar len = data[2];

  DEBUGF("cmd %d (%d)\n", data[1], data[1] - XU1541_IOCTL);

  switch(data[1]) {

    /* ----- USB testing ----- */
  case XU1541_IRQ_PAUSE:
    xu1541_req_irq_pause(len);
    break;

  case XU1541_ECHO:
    DEBUGF("echo\n");
    memcpy(replyBuf, data+2, 4);
    return 4;

    /* ----- Debugging ----- */
#ifdef ENABLE_EVENT_LOG
  case XU1541_GET_EVENT:
    DEBUGF("get ev\n");
    replyBuf[0] = EVENT_LOG_LEN;
    replyBuf[1] = event_log_get(len);
    return 2;
#endif

    /* ----- Basic I/O ----- */
  case XU1541_INFO:
    replyBuf[0] = XU1541_VERSION_MAJOR;
    replyBuf[1] = XU1541_VERSION_MINOR;
    *(ushort*)(replyBuf+2) = XU1541_CAPABILIIES;
    *(ushort*)(replyBuf+4) = pgm_read_word_near(&bios_data.version_major);
    return 6;

  case XU1541_FLASH:
    bios_start_flash_bootloader();
    return 0;

  case XU1541_EEPROM_READ:
    io_mode = MODE_EEPROM;
    return(data[2]?0xff:0x00);

  case XU1541_EEPROM_WRITE:
    io_mode = MODE_EEPROM;
    return(data[2]?0xff:0x00);

  case XU1541_REQUEST_READ:
    xu1541_request_read(len);
    return 0;

  case XU1541_GET_RESULT:
    xu1541_get_result(replyBuf);
    return 2;

  case XU1541_READ:
    io_mode = MODE_ORIGINAL;
    DEBUGF("rd %d\n", data[2]);
    /* more data to be expected? */
    return(data[2]?0xff:0x00);

  case XU1541_WRITE:
    io_mode = MODE_ORIGINAL;
    xu1541_prepare_write(data[2]);
    return(data[2]?0xff:0x00);

  case XU1541_TALK:
  case XU1541_LISTEN:
    DEBUGF("tlk/lst(%d,%d)\n", data[2], data[3]);
    talk = (data[1] == XU1541_TALK);
    data[2] |= talk ? 0x40 : 0x20;
    data[3] |= 0x60;
    xu1541_request_async(data+2, 2, 1, talk);
    return 0;

  case XU1541_UNTALK:
  case XU1541_UNLISTEN:
    DEBUGF("untlk/unlst()\n");
    data[2] = (data[1] == XU1541_UNTALK) ? 0x5f : 0x3f;
    xu1541_request_async(data+2, 1, 1, 0);
    return 0;

  case XU1541_OPEN:
  case XU1541_CLOSE:
    DEBUGF("open/close(%d,%d)\n", data[2], data[3]);
    data[2] |= 0x20;
    data[3] |= (data[1] == XU1541_OPEN) ? 0xf0 : 0xe0;
    xu1541_request_async(data+2, 2, 1, 0);
    return 0;

  case XU1541_GET_EOI:
    replyBuf[0] = eoi ? 1 : 0;
    return 1;

  case XU1541_CLEAR_EOI:
    eoi = 0;
    return 0;

  case XU1541_RESET:
    LED_OFF();
    DEBUGF("rst\n");
    do_reset();
    return 0;

    /* ----- Low-level port access ----- */

  case XU1541_IEC_WAIT:
    xu1541_wait(data[2], data[3]);
    /* intentional fall through */

  case XU1541_IEC_POLL:
    replyBuf[0] = xu1541_poll();
    DEBUGF("poll=%x\n", replyBuf[0]);
    return 1;

  case XU1541_IEC_SETRELEASE:
    xu1541_setrelease(data[2], data[3]);
    return 0;

  case XU1541_PP_READ:
    replyBuf[0] = xu1541_pp_read();
    return 1;

  case XU1541_PP_WRITE:
    xu1541_pp_write(len);
    return 0;

    /* ----- special protocol drivers access ----- */

    /* ----- S1 ----- */
  case XU1541_S1:
    switch(data[2]) {
    case XU1541_INFO:
      replyBuf[0] = S1_VERSION_MAJOR;
      replyBuf[1] = S1_VERSION_MINOR;
      return 2;

    case XU1541_READ:
      io_mode = MODE_S1;
      /* more data to be expected? */
      return(data[4]?0xff:0x00);

    case XU1541_WRITE:
      io_mode = MODE_S1;
      return(data[4]?0xff:0x00);

    default:
      DEBUGF("ERR: s1 cmd %d not impl.\n", data[1]);
      break;
    }
    break;

    /* ----- S2 ----- */
  case XU1541_S2:
    switch(data[2]) {
    case XU1541_INFO:
      replyBuf[0] = S2_VERSION_MAJOR;
      replyBuf[1] = S2_VERSION_MINOR;
      return 2;

    case XU1541_READ:
      io_mode = MODE_S2;
      /* more data to be expected? */
      return(data[4]?0xff:0x00);

    case XU1541_WRITE:
      io_mode = MODE_S2;
      return(data[4]?0xff:0x00);

    default:
      DEBUGF("ERR: s2 cmd %d not impl.\n", data[1]);
      break;
    }
    break;

    /* ----- PP ----- */
  case XU1541_PP:
    switch(data[2]) {
    case XU1541_INFO:
      replyBuf[0] = PP_VERSION_MAJOR;
      replyBuf[1] = PP_VERSION_MINOR;
      return 2;

    case XU1541_READ:
      io_mode = MODE_PP;
      /* more data to be expected? */
      return(data[4]?0xff:0x00);

    case XU1541_WRITE:
      io_mode = MODE_PP;
      return(data[4]?0xff:0x00);

    default:
      DEBUGF("ERR: pp cmd %d not impl.\n", data[1]);
      break;
    }
    break;

    /* ----- P2 ----- */
  case XU1541_P2:
    switch(data[2]) {
    case XU1541_INFO:
      replyBuf[0] = P2_VERSION_MAJOR;
      replyBuf[1] = P2_VERSION_MINOR;
      return 2;

    case XU1541_READ:
      io_mode = MODE_P2;
      /* more data to be expected? */
      return(data[4]?0xff:0x00);

    case XU1541_WRITE:
      io_mode = MODE_P2;
      return(data[4]?0xff:0x00);

    default:
      DEBUGF("ERR: p2 cmd %d not impl.\n", data[1]);
      break;
    }
    break;

  default:
    DEBUGF("ERR: cmd %d not impl.\n", data[0]);
    // must not happen ...
    break;
  }

  return 0;  // reply len
}


/*---------------------------------------------------------------------------*/
/* usbFunctionRead                                                           */
/*---------------------------------------------------------------------------*/

uchar usbFunctionRead( uchar *data, uchar len )
{
  uchar rv = 0;

  DEBUGF("rd %d\n", len);
  LED_ON();

  switch(io_mode) {
    case MODE_ORIGINAL:
      rv = xu1541_read(data, len);
      break;

    case MODE_S1:
      rv = s1_read(data, len);
      break;

    case MODE_S2:
      rv = s2_read(data, len);
      break;

    case MODE_PP:
      rv = pp_read(data, len);
      break;

    case MODE_P2:
      rv = p2_read(data, len);
      break;

    case MODE_EEPROM:
      /* eeprom read */
      rv = 0;
      break;
  }

  LED_OFF();
  return rv;
}

/*---------------------------------------------------------------------------*/
/* usbFunctionWrite                                                          */
/*---------------------------------------------------------------------------*/

uchar usbFunctionWrite( uchar *data, uchar len )
{
  uchar rv = 0;

  DEBUGF("wt %d\n", len);
  LED_ON();

  switch(io_mode) {
    case MODE_ORIGINAL:
      rv = xu1541_write(data, len);
      break;

    case MODE_S1:
      rv = s1_write(data, len);
      break;

    case MODE_S2:
      rv = s2_write(data, len);
      break;

    case MODE_PP:
      rv = pp_write(data, len);
      break;

    case MODE_P2:
      rv = p2_write(data, len);
      break;

    case MODE_EEPROM:
      /* eeprom write */
      rv = 0;
      break;
  }

  LED_OFF();
  return rv;
}

/* ------------------------------------------------------------------------- */

#if 0
int	main(void) {

  wdt_enable(WDTO_1S);

#ifdef ENABLE_EVENT_LOG
  event_log_init();
  EVENT(EVENT_START);

  /* save reset reasons */
  if(MCUCSR & (_BV(EXTRF)|_BV(PORF))) {
    EVENT(EVENT_RESET_EXT);
    MCUCSR &= ~_BV(EXTRF);
    MCUCSR &= ~_BV(PORF);
  }

  if(MCUCSR & _BV(WDRF)) {
    EVENT(EVENT_RESET_WATCHDOG);
    MCUCSR &= ~_BV(WDRF);
  }

  if(MCUCSR & _BV(BORF)) {
    EVENT(EVENT_RESET_BROWNOUT);
    MCUCSR &= ~_BV(BORF);
  }
#endif


#if DEBUG_LEVEL > 0
  /* let debug routines init the uart if they want to */
  odDebugInit();
#else
#ifdef DEBUG
  /* quick'n dirty uart init @ 115200 bit/s */
  UCSRA |= _BV(U2X);
  UCSRB |= _BV(TXEN);
  UBRRL = F_CPU / (57600 * 16L) - 1;
#else
  /* no debugging at all, drive led on tx, led is on port d.1 */
  DDRD |= _BV(1);     /* pin is output */
  LED_ON();           /* drive led during startup */

  /* rxd (d.0) is iec_srq. we don't use it (yet) so just make it an input */
  DDRD &= ~_BV(0);     /* pin is input */
#endif
#endif

#ifdef DEBUG
  stdout = &mystdout;
#endif

  DEBUGF("xu1541 %d.%02d\n", XU1541_VERSION_MAJOR, XU1541_VERSION_MINOR);

  cbm_init();

  /* clear usb ports * @@@
  USB_CFG_IOPORT   &= (uchar)~((1<<USB_CFG_DMINUS_BIT)|(1<<USB_CFG_DPLUS_BIT));

  * make usb data lines outputs *
  USBDDR    |= ((1<<USB_CFG_DMINUS_BIT)|(1<<USB_CFG_DPLUS_BIT));

  * USB Reset by device only required on Watchdog Reset *
  _delay_ms(10);

  * make usb data lines inputs *
  USBDDR &= ~((1<<USB_CFG_DMINUS_BIT)|(1<<USB_CFG_DPLUS_BIT));

  usbInit();
*/

  sei();

  LED_OFF();

  for(;;) {	/* main event loop */
/* @@@    extern byte_t usb_idle(void); */
    wdt_reset();
    usbPoll();

    /* do async iec processing */
    if(usb_idle())
      xu1541_handle();
  }

  return 0;
}
#endif

void init(void)
{
  cbm_init();

  LED_OFF();
}
/* ------------------------------------------------------------------------- */
