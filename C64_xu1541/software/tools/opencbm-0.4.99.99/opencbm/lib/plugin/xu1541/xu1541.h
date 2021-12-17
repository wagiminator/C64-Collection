#ifndef XU1541_H
#define XU1541_H

#include <stdio.h>
#include <usb.h>

#include "opencbm.h"

#ifndef FUNC_ENTER
  #define FUNC_ENTER()
#endif

/* time out 10% after device itself times out */
/* so make sure we should normally never time out on usb */
#define USB_TIMEOUT (XU1541_W4L_TIMEOUT * 1100)

#include "xu1541_types.h"

/* vendor and product id (donated by ftdi) */
#define XU1541_VID  0x0403
#define XU1541_PID  0xc632

/* calls required for standard io */
extern int xu1541_init(void);
extern void xu1541_close(void);
extern int xu1541_ioctl(unsigned int cmd, unsigned int addr, unsigned int secaddr);
extern int xu1541_write(const unsigned char *data, size_t len);
extern int xu1541_read(unsigned char *data, size_t len);

/* calls for speeder supported modes */
extern int xu1541_special_write(int mode, const unsigned char *data, size_t size);
extern int xu1541_special_read(int mode, unsigned char *data, size_t size);

#endif // XU1541_H
