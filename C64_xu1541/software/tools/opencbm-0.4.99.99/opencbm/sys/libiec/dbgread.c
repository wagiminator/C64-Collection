/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2008 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file sys/libiec/dbgread.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Read a RAW byte from the parallel port IN port (Status Port)
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"


/*! The bits not to be masked off for cbmiec_iec_dbg_read() */
#define PARALLEL_STATUS_PORT_MASK_VALUES 0xF8

/*! \brief Read a byte from the parallel port input register

 This function reads a byte from the parallel port input register.
 (STATUS_PORT). It is a helper function for debugging the cable
 (i.e., for the XCDETECT tool) only!

 \param Pdx
   Pointer to the device extension.

 \param Return
   Pointer to an UCHAR where the read byte is written to.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.

 \remark
   Do not use this function in anything but a debugging aid tool
   like XCDETECT!

   This functions masks some bits off. The bits that are not masked
   off are defined in PARALLEL_STATUS_PORT_MASK_VALUES.
*/
NTSTATUS
cbmiec_iec_dbg_read(IN PDEVICE_EXTENSION Pdx, OUT UCHAR *Return)
{
    UCHAR valueRead;

    FUNC_ENTER();

    valueRead = READ_PORT_UCHAR(IN_PORT);

    *Return = valueRead & PARALLEL_STATUS_PORT_MASK_VALUES;

    DBG_PRINT((DBG_PREFIX "Read from Status Port: %02x, converted to %02x", valueRead, *Return));

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}
