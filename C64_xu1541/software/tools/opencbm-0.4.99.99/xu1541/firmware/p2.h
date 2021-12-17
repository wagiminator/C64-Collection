/*
  p2.h - xu1541 parallel driver interface
*/

#ifndef P2_H
#define P2_H

#define P2_VERSION_MAJOR 1
#define P2_VERSION_MINOR 0

extern uchar p2_read(uchar *data, uchar len);
extern uchar p2_write(uchar *data, uchar len);

#endif // P2_H
