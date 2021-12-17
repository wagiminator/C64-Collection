/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
*/

#ifndef CBM4LINUX_OLDAPI
# warning \
"cbm4linux.h has been replaced by opencbm.h and should not be used any more"
#endif

#ifndef CBM_H
#define CBM_H

#include <sys/types.h>

#define IEC_DATA   0x01
#define IEC_CLOCK  0x02
#define IEC_ATN    0x04

#ifdef __cplusplus
extern "C" {
#endif

extern const char cbm_dev[];

extern int cbm_listen(int f, unsigned char dev, unsigned char secadr);
extern int cbm_talk(int f, unsigned char dev, unsigned char secadr);

extern int cbm_open(int f, unsigned char dev, unsigned char secadr);
extern int cbm_close(int f, unsigned char dev, unsigned char secadr);

extern int cbm_unlisten(int f);
extern int cbm_untalk(int f);

extern int cbm_get_eoi(int f);

extern int cbm_reset(int f);

extern unsigned char cbm_pp_read(int f);
extern void cbm_pp_write(int f, unsigned char c);

extern int cbm_iec_poll(int f);
extern int cbm_iec_get(int f, int line);
extern void cbm_iec_set(int f, int line);
extern void cbm_iec_release(int f, int line);
extern int cbm_iec_wait(int f, int line, int state);

extern int cbm_upload(int f, unsigned char dev, int adr, unsigned char *prog, int size);

extern int cbm_device_status(int f, int drv, char *buf, int bufsize);
extern int cbm_exec_command(int f, int drv, char *cmd, int len);

#ifdef __cplusplus
}
#endif

#endif
