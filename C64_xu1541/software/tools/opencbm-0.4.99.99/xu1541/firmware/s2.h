/*
  s2.h - xu1541 serial1 driver interface
*/

#ifndef S2_H
#define S2_H

#define S2_VERSION_MAJOR 1
#define S2_VERSION_MINOR 0

extern uchar s2_read(uchar *data, uchar len);
extern uchar s2_write(uchar *data, uchar len);

#endif // S2_H
