/* Name: xu1541.c
 * Project: xu1541
 * Author: Till Harbaum
 * Tabsize: 4
 * Copyright: (c) 2007 by Till Harbaum <till@harbaum.org>
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
#include "xu1541_event_log.h"

#ifdef DEBUG
#define DEBUGF(format, args...) printf_P(PSTR(format), ##args)
#else
#define DEBUGF(format, args...)
#endif

/* specifiers for the lines (must match values from opencbm.h) */
#define IEC_DATA   0x01 /*!< Specify the DATA line */
#define IEC_CLOCK  0x02 /*!< Specify the CLOCK line */
#define IEC_ATN    0x04 /*!< Specify the ATN line */
#define IEC_RESET  0x08 /*!< Specify the RESET line */

uchar io_buffer[XU1541_IO_BUFFER_SIZE];
uchar io_buffer_fill, io_request, io_offset, io_result;

/* fast conversion between logical and physical mapping */
static const uchar iec2hw_table[] PROGMEM = {
  0,
  DATA,
         CLK,
  DATA | CLK,
               ATN,
  DATA |       ATN,
         CLK | ATN,
  DATA | CLK | ATN,
                     RESET,
  DATA |             RESET,
         CLK |       RESET,
  DATA | CLK |       RESET,
               ATN | RESET,
  DATA |       ATN | RESET,
         CLK | ATN | RESET,
  DATA | CLK | ATN | RESET
};

/* the timers don't allow for accurate 1us and 1ms clock speeds */
/* thus use macros to adjust the offsets */
#define DELAY_US(a) delay_us(256 - (1.5*(a)))
#define DELAY_MS(a) delay_ms(256 - (8.533*(a)))

/* don't use busy waiting delay routines from util/delay.h. use timer */
/* based aproach instead since this is more accurate when being */
/* interrupted by an USB irq */
static void delay_wait(uchar a) {
  wdt_reset();

  TCNT0 = a;
  TIFR |= _BV(TOV0);

  /* wait until counter overflows */
  while(!(TIFR & _BV(TOV0)));
}

/* counter runs at 12/8 mhz */
/* a max 256 -> max 170us */
static void delay_us(uchar a) {
  /* use 8 bit timer 0 as system timer, prescaler/8 */
  TCCR0 = _BV(CS01);
  delay_wait(a);
}

/* counter runs at 12/1024 mhz */
/* a max 256 -> max 21.8ms */
static void delay_ms(uchar a) {
  /* use 8 bit timer 0 as system timer, prescaler/1024 */
  TCCR0 = _BV(CS02) | _BV(CS00);
  delay_wait(a);
}

static uchar iec2hw(char iec) {
  return pgm_read_byte(iec2hw_table + iec);
}

uchar iec_poll(void) {
  _delay_us(IEC_DELAY);
  return CBM_PIN;
}

/* set line means: make it an output and drive it low */
void iec_set(uchar line) {
  _delay_us(IEC_DELAY);
  CBM_PORT &= ~line;
  CBM_DDR |= line;
}

/* release means: make it an input and enable the pull-ups */
void iec_release(uchar line) {
  _delay_us(IEC_DELAY);
  CBM_DDR &= ~line;
  CBM_PORT |= line;
}

void iec_set_release(uchar s, uchar r) {
  _delay_us(IEC_DELAY);
  CBM_PORT &= ~s;
  CBM_DDR = (CBM_DDR | s) & ~r;
  CBM_PORT |= r;
}

uchar iec_get(uchar line) {
  _delay_us(IEC_DELAY);
  return ((CBM_PIN & line) == 0 )?1:0;
}

/* global variable to keep track of eoi state */
uchar eoi = 0;

void cbm_init(void) {
  DEBUGF("init\n");

  io_buffer_fill = 0;
  io_request = XU1541_IO_IDLE;

  iec_release(ATN | CLK | DATA | RESET);
  DELAY_US(100);

#if 0 // timer test
#define TEST_DELAY  100
 {
   uchar x;

   TCCR2 = _BV(CS21) | _BV(CS20);  /* prescaler 32 -> 375khz/2.66667us */
   TCNT2 = 0;
   DELAY_US(TEST_DELAY);
   x = TCNT2;

   DEBUGF("counted %u (%u)\n", x, (uchar)(TEST_DELAY/2.666667));
 }
#endif

 do_reset();
}

static char check_if_bus_free(void) {
  iec_release(ATN | CLK | DATA | RESET);

  // wait for the drive to have time to react
  DELAY_US(100);

  // assert ATN
  iec_set(ATN);

  // now, wait for the drive to have time to react
  DELAY_US(100);

  // if DATA is still unset, we have a problem.
  if (!iec_get(DATA)) {
    iec_release(ATN | CLK | DATA | RESET);
    return 0;
  }

  // ok, at least one drive reacted. Now, test releasing ATN:
  iec_release(ATN);
  DELAY_US(100);

  if (!iec_get(DATA)) {
    iec_release(ATN | CLK | DATA | RESET);
    return 1;
  }

  iec_release(ATN | CLK | DATA | RESET);
  return 0;
}

static void wait_for_free_bus(void) {
  short i=1;

  while (1) {
    wdt_reset();

    if (check_if_bus_free())
      break;

    ++i;

    if (i == 2000) {
      EVENT(EVENT_TIMEOUT_FREE_BUS);
      DEBUGF("wait4free bus to\n");
      break;
    }
    DELAY_MS(1);
  }
}

void do_reset( void ) {
  DEBUGF("reset\n");
  iec_release(DATA | ATN | CLK);
  iec_set(RESET);
  DELAY_MS(20);
  iec_release(RESET);

  wait_for_free_bus();
}

/* timeout in milliseconds, max 5.4 */
#define IEC_TIMEOUT_MS(a)  ((uchar)((a) * F_CPU / 256000))

static uchar iec_wait_timeout_2ms(uchar mask, uchar state) {

  /* use 8 bit timer 2 for precise timeout */
  TCCR2 = _BV(CS22) | _BV(CS21);  /* prescaler 256 */
  TCNT2 = 256 - IEC_TIMEOUT_MS(2);
  TIFR |= _BV(TOV2);

  /* wait until counter reaches expected value or counter overflows */
  /* e.g. 2ms timeout == 94 ticks @ 46.8khz, or of course until expected */
  /* event arrives. this is in negative notation since iec_poll returns */
  /* 0 for "active" bits */
  while(!(TIFR & _BV(TOV2)) && (((iec_poll() & mask) == state)));

  return ((iec_poll() & mask) != state);
}

/*
 *  send byte
 */
static char send_byte(uchar b) {
  uchar i, ack = 0;

  for( i = 0; i < 8; i++ ) {

    /* each _bit_ takes a total of 90us to send ... */
    DELAY_US(70);

    if( !(b & 1) )
      iec_set(DATA);

    iec_release(CLK);
    DELAY_US(20);

    iec_set_release(CLK, DATA);

    b >>= 1;
  }

  /* wait 2ms for data to be driven */
  ack = iec_wait_timeout_2ms(DATA, DATA);

#ifdef ENABLE_EVENT_LOG
  if(!ack) {
    EVENT(EVENT_BYTE_NAK);
  }
#endif

  return ack;
}

/* wait for listener to release DATA line, N second timeout */
static char wait_for_listener(void) {
  unsigned short a,b;

  /* release the clock line to indicate that we are ready */
  iec_release(CLK);

  /* wait for client to do the same with the DATA line */
  for(a=0;iec_get(DATA)&&(a < XU1541_W4L_TIMEOUT * 100);a++) {
    for(b=0;iec_get(DATA)&&(b < 1000);b++) {
      DELAY_US(10);
    }
  }

  return !iec_get(DATA);
}

/* return number of successful written bytes or 0 on error */
uchar cbm_raw_write(const uchar *buf, uchar len, uchar atn, uchar talk) {
  uchar rv = len;

  eoi = 0;

  DEBUGF("wr %d, atn %d\n", len, atn);

  iec_release(DATA);
  iec_set(CLK | (atn ? ATN : 0));

  /* wait for any device to pull data */
  if(!iec_wait_timeout_2ms(DATA, DATA)) {
    DEBUGF("write: no devs\n");
    iec_release(CLK | ATN);
    EVENT(EVENT_WRITE_NO_DEV);
    return 0;
  }

  while(len && rv) {

    /* wait 50 us */
    DELAY_US(50);

    /* data line must be pulled by device */
    if(iec_get(DATA)) {

      /* release clock and wait for listener to release data */
      if(!wait_for_listener()) {
	DEBUGF("w4l to\n");
	EVENT(EVENT_TIMEOUT_LISTENER);
	iec_release(CLK | ATN);
	return 0;
      }

      /* this is timing critical and if we are not sending an eoi */
      /* the iec_set(CLK) must be reached in less than ~150us. The USB */
      /* at 1.5MBit/s transfers 160 bits (20 bytes) in ~100us, this */
      /* should not interfere */

      if((len == 1) && !atn) {
	/* signal eoi by waiting so long (>200us) that listener */
	/* pulls data */

	/* wait 2ms for data to be pulled */
	iec_wait_timeout_2ms(DATA, DATA);

	/* wait 2ms for data to be release */
	iec_wait_timeout_2ms(DATA, 0);
      }

      /* wait 10 us, why 10?? This delay is the most likely to be hit */
      /* by an USB irq */
      DELAY_US(10);

      iec_set(CLK);

      if(send_byte(*buf++)) {
	len--;
	DELAY_US(100);
      } else {
	EVENT(EVENT_WRITE_FAILED);
	DEBUGF("write: io err\n");
	rv = 0;
      }
    } else {
      EVENT(EVENT_WRITE_DEV_NOT_PRESENT);
      DEBUGF("write: dev not pres\n");
      rv = 0;
    }
  }

  if(talk) {
    iec_set(DATA);
    iec_release(CLK | ATN);
    while(!iec_get(CLK));
  } else {
    iec_release(ATN);
  }
  DELAY_US(100);

  DEBUGF("rv=%d\n", rv);

  return rv;
}

void xu1541_req_irq_pause(uchar len) {
  io_request = XU1541_IO_IRQ_PAUSE;
  io_buffer[0] = len;
}

/* frequently check for outstanding requests */
void xu1541_handle(void) {

  /* irq pause is for testing in debugging only */
  if(io_request == XU1541_IO_IRQ_PAUSE) {
    DEBUGF("h-ps %d0\n", io_buffer[0]);
    cli();
    while(io_buffer[0]--) {
      DELAY_MS(10);
    }
    sei();

    io_request = XU1541_IO_IDLE;
  }

  if(io_request == XU1541_IO_ASYNC) {
    DEBUGF("h-as\n");
    LED_ON();
    /* write async cmd byte(s) used for (un)talk/(un)listen, open and close */
    io_result = !cbm_raw_write(io_buffer+2, io_buffer_fill,
			       io_buffer[0], io_buffer[1]);
    LED_OFF();

    io_request = XU1541_IO_RESULT;
  }

  if(io_request == XU1541_IO_WRITE) {
    DEBUGF("h-wr %d\n", io_buffer_fill);
    LED_ON();
    io_result = cbm_raw_write(io_buffer, io_buffer_fill, 0, 0);
    LED_OFF();

    io_request = XU1541_IO_RESULT;
  }

  if(io_request == XU1541_IO_READ) {
    uchar ok, bit, b;
    uchar received = 0;
    unsigned short to;

    DEBUGF("h-rd %d\n", io_buffer_fill);

    io_offset = 0;

    LED_ON();

    do {
      to = 0;

      /* wait for clock to be released. typically times out during: */
      /* directory read */
      while(iec_get(CLK)) {
	if( to >= 50000 ) {
	  /* 1.0 (50000 * 20us) sec timeout */
	  EVENT(EVENT_READ_TIMEOUT);
	  DEBUGF("rd to\n");

	  io_request = XU1541_IO_READ_DONE;
	  io_buffer_fill = 0;

	  LED_OFF();
	  return;
	} else {
	  to++;
	  DELAY_US(20);
	}
      }

      if (eoi) {
          /* re-enable interrupts and return */
          io_request = XU1541_IO_READ_DONE;
          io_buffer_fill = received;
          LED_OFF();
          return;
      }

      /* disable IRQs to make sure IEC transfer goes uninterrupted */

      /* release DATA line */
      iec_release(DATA);

      /* use special "timer with wait for clock" */
      /* use 8 bit timer 2, prescaler 32 */
      TCCR2 = _BV(CS21) | _BV(CS20);  /* prescaler 32 -> 375khz/2.66667us */
      TCNT2 = 0;
      TIFR |= _BV(TOV2);

      /* wait until counter reaches expected value or counter overflows */
      /* 400us timeout == 150 ticks @ 375khz */
      while((TCNT2 < 150) && !(TIFR & _BV(TOV2)) && !iec_get(CLK));


      if(!iec_get(CLK)) {
	/* device signals eoi */
	eoi = 1;
	iec_set(DATA);
	DELAY_US(70);
	iec_release(DATA);
      }

      cli();

      /* wait 2ms for clock to be asserted */
      ok = iec_wait_timeout_2ms(CLK, CLK);

      /* read all bits of byte */
      for(bit = b = 0; (bit < 8) && ok; bit++) {

	/* wait 2ms for clock to be released */
	if((ok = iec_wait_timeout_2ms(CLK, 0))) {
	  b >>= 1;
	  if(!iec_get(DATA))
	    b |= 0x80;

	  /* wait 2ms for clock to be asserted */
	  ok = iec_wait_timeout_2ms(CLK, CLK);
	}
      }

      sei();

      /* acknowledge byte */
      if(ok)
	iec_set(DATA);

      if(ok) {
	io_buffer[received++] = b;
	DELAY_US(50);
      }

    } while(received < io_buffer_fill && ok && !eoi);

    if(!ok) {
      EVENT(EVENT_READ_ERROR);
      DEBUGF("read io err\n");
      io_buffer_fill = 0;
    }

    /* re-enable interrupts and return */
    io_request = XU1541_IO_READ_DONE;
    io_buffer_fill = received;

    LED_OFF();
  }
}

void xu1541_request_read(uchar len) {
  DEBUGF("req rd %d\n", len);

  /* check for buffer in use etc ... */
  /* ... hmmm, some checks just eat up too much flash space ... */

  /* store request */
  io_buffer_fill = len;   // save requested lenght
  io_request = XU1541_IO_READ;
}

uchar xu1541_read(uchar *data, uchar len) {
  if(io_request != XU1541_IO_READ_DONE) {
    DEBUGF("no rd (%d)\n", io_request);
    return 0;
  }

  if(io_buffer_fill) {
    if(len > io_buffer_fill)
      len = io_buffer_fill;

    /* fetch data from buffer */
    memcpy(data, io_buffer + io_offset, len);
    io_offset += len;
    io_buffer_fill -= len;

  } else
    len = 0;

  /* stop after last byte has been transferred */
  if(!io_buffer_fill)
    io_request = XU1541_IO_IDLE;

  return len;
}

void xu1541_prepare_write(uchar len) {
  io_request = XU1541_IO_WRITE_PREPARED;
  io_offset = 0;
  io_buffer_fill = len;
}

/* write to buffer */
uchar xu1541_write(uchar *data, uchar len) {
  if(io_request != XU1541_IO_WRITE_PREPARED) {
    DEBUGF("no wr (%d)\n", io_request);
    return 0;
  }

  DEBUGF("st %d\n", len);
  memcpy(io_buffer + io_offset, data, len);
  io_offset += len;

  /* done transfering data? */
  if(io_offset >= io_buffer_fill) {
    DEBUGF("st done\n");

    io_request = XU1541_IO_WRITE;
  }
  return len;
}

/* return result code from async operations */
void xu1541_get_result(unsigned char *data) {

  data[0] = io_request;
  data[1] = io_result;

  DEBUGF("r %d/%d\n", data[0], data[1]);

#if 0
  /* reset flag if result has been delivered */
  if(io_request == XU1541_IO_RESULT)
     io_request = XU1541_IO_IDLE;
#endif
}

void xu1541_request_async(const uchar *buf, uchar cnt,
			   uchar atn, uchar talk) {

  io_request = XU1541_IO_ASYNC;
  memcpy(io_buffer+2, buf, cnt);
  io_buffer_fill = cnt;
  io_buffer[0] = atn;
  io_buffer[1] = talk;
}

/* wait while a specific line to reach a certain state */
uchar xu1541_wait(uchar line, uchar state) {
  uchar hw_mask, hw_state;
  ushort i,j;

  /* calculate hw mask and expected state */
  hw_mask = iec2hw(line);
  hw_state = state ? hw_mask : 0;

  j = i = 0;
  while((iec_poll() & hw_mask) == hw_state) {
    if(i >= 1000) {
      if(j++ > XU1541_W4L_TIMEOUT * 100) {
	EVENT(EVENT_TIMEOUT_IEC_WAIT);
	DEBUGF("iec_wait to\n");
	return 0xff;
      }

      i = 0;
    } else {
      i++;
      DELAY_US(10);
    }
  }
  return 0;
}

uchar xu1541_poll(void) {
  uchar rv = 0;

  if((iec_poll() & DATA) == 0) rv |= IEC_DATA;
  if((iec_poll() & CLK)  == 0) rv |= IEC_CLOCK;
  if((iec_poll() & ATN)  == 0) rv |= IEC_ATN;

  return rv;
}

void xu1541_setrelease(uchar set, uchar release) {
  iec_set_release(iec2hw(set), iec2hw(release));
}

/* read byte from parallel port */
uchar xu1541_pp_read(void) {
  uchar retval;

  /* make port(s) input */
  PAR_PORT0_DDR &= ~PAR_PORT0_MASK;
  PAR_PORT1_DDR &= ~PAR_PORT1_MASK;

  /* disable pullups */
  PAR_PORT0_PORT &= ~PAR_PORT0_MASK;
  PAR_PORT1_PORT &= ~PAR_PORT1_MASK;

  /* and read value */
  retval  = PAR_PORT0_PIN & PAR_PORT0_MASK;
  retval |= PAR_PORT1_PIN & PAR_PORT1_MASK;

  return retval;
}

void xu1541_pp_write(uchar val) {

  /* make ports output */
  PAR_PORT0_DDR |= PAR_PORT0_MASK;
  PAR_PORT1_DDR |= PAR_PORT1_MASK;

  /* mask pins */
  PAR_PORT0_PORT &= ~PAR_PORT0_MASK;
  PAR_PORT1_PORT &= ~PAR_PORT1_MASK;

  /* and put data bits on port */
  PAR_PORT0_PORT |= val & PAR_PORT0_MASK;
  PAR_PORT1_PORT |= val & PAR_PORT1_MASK;
}
