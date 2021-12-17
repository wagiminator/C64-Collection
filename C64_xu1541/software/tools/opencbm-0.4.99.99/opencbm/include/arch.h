/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004-2007 Spiro Trikaliotis
 */

/*! **************************************************************
** \file include/arch.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Define makros and functions which account for differences
** between the different architectures
**
****************************************************************/

#ifndef CBM_ARCH_H
#define CBM_ARCH_H

#include <sys/types.h> /* for off_t */

#define ARCH_EMPTY

#include "version.h"
#define OPENCBM_VERSION OPENCBM_VERSION_STRING

#ifdef WIN32
# define ARCH_CBM_LINUX_WIN( _linux, _win) _win

# include <stdio.h>
# include <io.h>
# include <fcntl.h>

#if (_MSC_VER <= 1200) // MSVC 6 or older
typedef unsigned long ULONG_PTR;
#endif

#else

# define ARCH_CBM_LINUX_WIN( _linux, _win) _linux

# include <unistd.h>
# include <errno.h>
# include <stdbool.h>

#ifdef __LP64__
typedef unsigned long UINT_PTR;
#else
typedef unsigned int UINT_PTR;
#endif

/* error.h is only available on Linux */
#ifdef __linux__
# include <error.h>
#endif

typedef unsigned long ULONG_PTR;

typedef bool BOOL;

# ifndef TRUE
#  define TRUE true
# endif
# ifndef FALSE
#  define FALSE false
# endif

#endif // WIN32

#include <string.h>

#define arch_strcasecmp(_x,_y)     ARCH_CBM_LINUX_WIN(strcasecmp(_x,_y), _stricmp(_x,_y))
#define arch_strncasecmp(_x,_y,_z) ARCH_CBM_LINUX_WIN(strncasecmp(_x,_y,_z), _strnicmp(_x,_y,_z))

#define arch_sleep(_x)  ARCH_CBM_LINUX_WIN(sleep(_x), Sleep((_x) * 1000))
#define arch_usleep(_x) ARCH_CBM_LINUX_WIN(usleep(_x), Sleep( ((_x) + 999) / 1000))

#ifdef WIN32
 extern void arch_error(int AUnused, unsigned int ErrorCode, const char *format, ...);
 extern char *arch_strerror(unsigned int ErrorCode);
#else
#if defined(__APPLE__) || defined(__FreeBSD__)
 extern void arch_error(int AUnused, unsigned int ErrorCode, const char *format, ...);
#else
# define arch_error error
#endif
# define arch_strerror strerror
#endif

/*! set errno variable */
#define arch_set_errno(_x) ARCH_CBM_LINUX_WIN((errno = (_x)), SetLastError(_x))
#define arch_get_errno()   ARCH_CBM_LINUX_WIN((errno), GetLastError())

#define arch_atoc(_x) ((unsigned char) atoi(_x))

/* dummys for compiling */

#ifdef WIN32
# include <stdlib.h> /* for getenv */

/* Make sure that getenv() will not be defined with a prototype
   in arch/windows/getopt.c, which would result in a compiler error
   "error C2373: 'getenv' : redefinition; different type modifiers".
*/
# define getenv getenv
#endif

#define arch_unlink(_x) ARCH_CBM_LINUX_WIN(unlink(_x), _unlink(_x))

int arch_filesize(const char *Filename, off_t *Filesize);

#define arch_strdup(_x) ARCH_CBM_LINUX_WIN(strdup(_x), _strdup(_x))

#define arch_fileno(_x) ARCH_CBM_LINUX_WIN(fileno(_x), _fileno(_x))

#define arch_setbinmode(_x) ARCH_CBM_LINUX_WIN(ARCH_EMPTY/* already in bin mode */, _setmode(_x, _O_BINARY))

#define arch_ftruncate(_x, _y) ARCH_CBM_LINUX_WIN(ftruncate(_x, _y), _chsize(_x, _y))

#define arch_fdopen(_x, _y) ARCH_CBM_LINUX_WIN(fdopen(_x, _y), _fdopen(_x, _y))

#define arch_snprintf ARCH_CBM_LINUX_WIN(snprintf, _snprintf)

#define ARCH_MAINDECL   ARCH_CBM_LINUX_WIN(ARCH_EMPTY, __cdecl)
#define ARCH_SIGNALDECL ARCH_CBM_LINUX_WIN(ARCH_EMPTY, __cdecl)

typedef void (ARCH_SIGNALDECL *ARCH_CTRLBREAK_HANDLER)(int dummy);
extern void arch_set_ctrlbreak_handler(ARCH_CTRLBREAK_HANDLER Handler);

#endif /* #ifndef CBM_ARCH_H */
