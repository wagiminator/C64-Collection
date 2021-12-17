/*
  s1.h - xu1541 serial1 driver interface
*/

#ifndef S1_H
#define S1_H

#define S1_VERSION_MAJOR 1
#define S1_VERSION_MINOR 0

extern uchar s1_read(uchar *data, uchar len);
extern uchar s1_write(uchar *data, uchar len);

#endif // S1_H
