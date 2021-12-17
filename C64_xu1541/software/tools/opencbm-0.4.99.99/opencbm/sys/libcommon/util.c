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
** \file sys/libcommon/util.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Some utility functions for the driver
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"

/*! \brief Log an error entry in the system log

 Log an error entry in the system log.

 \param Fdo
   Pointer to a DEVICE_OBJECT structure. 
   This is the device object for the target device, 
   previously created by the driver's AddDevice routine.
 
 \param ErrorCode
   The NTSTATUS code which should be reported on behalf of
   this error log entry

 \param String1
   Pointer to the 1st (WCHAR) string which has to be included
   into the error log entry. This can be NULL if no string
   is to be inserted.

 \param String2
   Pointer to the 2nd (WCHAR) string which has to be included
   into the error log entry. This can be NULL if no string
   is to be inserted.
*/
VOID
LogError(IN PDEVICE_OBJECT Fdo,
         IN NTSTATUS ErrorCode,
         const WCHAR *String1,
         const WCHAR *String2)
{
    USHORT stringSize;
    USHORT stringOffset;
    USHORT stringSize1;
    USHORT stringSize2;
    USHORT size;
    USHORT numberOfStrings;

    FUNC_ENTER();

    // IoAllocateErrorLogEntry() and IoWriteErrorLogEntry() require this

    DBG_IRQL( <= DISPATCH_LEVEL);

    // calculate the size of the strings

    numberOfStrings = 0;

    // Check if there is a string 1, and calculate its size
    // (including the trailing zero)

    stringSize1 = String1 ? (++numberOfStrings, sizeof(WCHAR) * (wcslen(String1) + 1)) : 0;

    // Check if there is a string 2, and calculate its size
    // (including the trailing zero)

    stringSize2 = String2 ? (++numberOfStrings, sizeof(WCHAR) * (wcslen(String2) + 1)) : 0;

    // Add the sizes of both strings
    // This is the size of what has to be added to the error log entry

    stringSize = stringSize1 + stringSize2;

    // Offset where the string(s) will be written into the error log entry

    stringOffset = sizeof(IO_ERROR_LOG_PACKET);

    // The complete size of the event log entry

    size = stringOffset + stringSize;

    // Make sure we don't need more space than needed.
    // For debugging purposes, have an DBG_ASSERT(). Anyway,
    // in the wild, don't do anything if the size is too big.
    // Remember: Not being able to write a log is not an error!

    /*! \todo Would it make sense to short the strings if the
     * error log entry would be too big?
     */

    DBG_ASSERT(size <= ERROR_LOG_MAXIMUM_SIZE);

    if (size <= ERROR_LOG_MAXIMUM_SIZE) 
    {
        PIO_ERROR_LOG_PACKET pentry;

        // allocate the entry for the error log

        DBG_IRQL( <= DISPATCH_LEVEL);
        pentry = IoAllocateErrorLogEntry(Fdo, (UCHAR) size);

        DBG_ASSERT(pentry);
        if (pentry) {

            // clear the complete entry (to be sure)

            RtlZeroMemory(pentry, sizeof(*pentry));

            // Write the relevant entries

            pentry->NumberOfStrings = numberOfStrings;
            pentry->StringOffset = stringOffset;
            pentry->ErrorCode = ErrorCode;

            // If there was a string1, write that into the entry

            if (String1)
            {
                wcscpy((wchar_t*)&((UCHAR*)pentry)[stringOffset], String1);
            }

            // If there was a string2, write that into the entry

            if (String2)
            {
                wcscpy((wchar_t*)&((UCHAR*)pentry)[stringOffset + stringSize1], String2);
            }

            // Now, give that entry to the system

            DBG_IRQL( <= DISPATCH_LEVEL);
            IoWriteErrorLogEntry(pentry);
        }
    }

    FUNC_LEAVE();
}
