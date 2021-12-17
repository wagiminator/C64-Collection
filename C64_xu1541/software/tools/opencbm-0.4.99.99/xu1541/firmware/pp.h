/*
  pp.h - xu1541 parallel driver interface
*/

#ifndef PP_H
#define PP_H

#define PP_VERSION_MAJOR 1
#define PP_VERSION_MINOR 0

extern uchar pp_read(uchar *data, uchar len);
extern uchar pp_write(uchar *data, uchar len);

#endif // PP_H
