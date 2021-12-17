/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2012 Spiro Trikaliotis
 *
*/

/*! ************************************************************** 
** \file libmisc/WINDOWS/formaterrormessage.c \n
** \author Michael Klein, Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver
**
****************************************************************/

#include "libmisc.h"

#include <windows.h>

/*! \brief Format a returned error code into a string

 This function formats a returned error code into a string.

 \param Error
   The error number to be formatted.

 \return 
   The string describing the error given by the error code.
*/

char *
cbmlibmisc_format_error_message(unsigned int ErrorNumber)
{
    static char ErrorMessageBuffer[2048];
    int n;

    // Format the message

    n = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL,
        ErrorNumber,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &ErrorMessageBuffer,
        sizeof(ErrorMessageBuffer)-1,
        NULL);

    // make sure there is a trailing zero

    ErrorMessageBuffer[n] = 0;

    return ErrorMessageBuffer;
}
