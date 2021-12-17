/*
  xu1541.h - xu1541
*/

#ifndef XU1541_H
#define XU1541_H

typedef unsigned char uchar;
typedef unsigned short ushort;

#include "xu1541_types.h"
#include "version.h"

/* use port c for cbm io */
#define CBM_PORT  PORTC
#define CBM_DDR   DDRC
#define CBM_PIN   PINC

/* mapping of iec lines */
#define CLK   _BV(2)
#define DATA  _BV(3)
#define ATN   _BV(4)
#define RESET _BV(5)

/* the parallel port is mapped onto two avr ports */
#define PAR_PORT0_MASK   0xf8   /* port d pins 7-3 */
#define PAR_PORT0_DDR    DDRD
#define PAR_PORT0_PIN    PIND
#define PAR_PORT0_PORT   PORTD

#define PAR_PORT1_MASK   0x07   /* port b pins 2-0 */
#define PAR_PORT1_DDR    DDRB
#define PAR_PORT1_PIN    PINB
#define PAR_PORT1_PORT   PORTB

#define IEC_DELAY  (0.5)   // 500ns

#ifndef DEBUG
#define LED_ON()     { PORTD &= ~_BV(1); }
#define LED_OFF()    { PORTD |=  _BV(1); }
#else
#define LED_ON()
#define LED_OFF()
#endif

extern uchar eoi;

/* exported functions */

/* basic port access routines */
extern uchar iec_poll(void);
extern void  iec_set(uchar line);
extern void  iec_release(uchar line);
extern void  iec_set_release(uchar s, uchar r);
extern uchar iec_get(uchar line);

extern void  cbm_init(void);
extern void  do_reset(void);
extern uchar cbm_raw_write(const uchar *buf, uchar cnt, uchar atn, uchar talk);
extern void  xu1541_request_async(const uchar *buf, uchar cnt, uchar atn, uchar talk);
extern void  xu1541_request_read(uchar len);
extern uchar xu1541_read(uchar *data, uchar len);
extern void  xu1541_prepare_write(uchar len);
extern uchar xu1541_write(uchar *data, uchar len);
extern void  xu1541_handle(void);
extern void  xu1541_get_result(uchar *data);

/* low level io on single lines */
extern uchar xu1541_wait(uchar line, uchar state);
extern uchar xu1541_poll(void);
extern void  xu1541_setrelease(uchar set, uchar release);

/* low level io on parallel lines */
extern uchar xu1541_pp_read(void);
extern void  xu1541_pp_write(uchar);

/* debugging */
extern void  xu1541_req_irq_pause(uchar len);

#endif // XU1541_H
