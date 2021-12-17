/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004 Spiro Trikaliotis
 *
 */

/*! **************************************************************
** \file sys/include/WINDOWS/util.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Some utility helpers
**
****************************************************************/

// Some macros for specifying time values in NT's 100 ns-epoch
//! \todo Currently, these macros are unused!

/*! 1s is 1000ms */
#define SECONDS(_x_) MILLISECONDS((_x_)*1000)

/*! 1ms is 1000us */
#define MILLISECONDS(_x_) MICROSECONDS((_x_)*1000)

/*! 1us is 10 * 100ns */
#define MICROSECONDS(_x_) ((_x_) * 10)

VOID LogError(IN PDEVICE_OBJECT Fdo, IN NTSTATUS ErrorCode,
    const WCHAR *String1, const WCHAR *String2);

/*! Log without giving any strings */
#define LogErrorOnly(_Fdo_, _UniqueErrorValue_) \
    LogError(_Fdo_, _UniqueErrorValue_, NULL, NULL)

/*! Log, giving up to 2 strings */
#define LogErrorString(_Fdo_, _UniqueErrorValue_, _String1_, _String2_) \
    LogError(_Fdo_, _UniqueErrorValue_, _String1_, _String2_)
