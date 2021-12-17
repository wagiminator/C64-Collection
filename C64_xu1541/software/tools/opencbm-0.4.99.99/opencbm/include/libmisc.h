/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2007-2008, 2012 Spiro Trikaliotis
 */

/*! **************************************************************
** \file include/libmisc.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Some functions for string handling
**
****************************************************************/

#ifndef CBM_LIBMISC_H
#define CBM_LIBMISC_H

#include <stddef.h>

extern char * cbmlibmisc_stralloc(unsigned int Length);

extern char * cbmlibmisc_strdup(const char * const OldString);
extern char * cbmlibmisc_strndup(const char * const OldString, size_t Length);

extern void   cbmlibmisc_strfree(const char * String);
extern char * cbmlibmisc_strcat(const char * first, const char * second);

/* Windows only: */
extern char * cbmlibmisc_format_error_message(unsigned int ErrorNumber);


#endif /* #ifndef CBM_LIBMISC_H */
