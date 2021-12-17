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
** \file sys/libiec/dbgwrite.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Write a RAW byte to the parallel port OUT port (Control Port)
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"


/*! The bits not to be masked off for cbmiec_iec_dbg_write() */
#define PARALLEL_CONTROL_PORT_MASK_VALUES 0x0F

/*! \brief Write a byte to the parallel port output register

 This function writes a byte to the parallel port output register.
 (CONTROL_PORT). It is a helper function for debugging the cable
 (i.e., for the XCDETECT tool) only!

 \param Pdx
   Pointer to the device extension.

 \param Value
   The value to set the control port to

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.

 \remark
   Do not use this function in anything but a debugging aid tool
   like XCDETECT!

   After this function has been called, it is NOT safe to use the
   parallel port driver unless you close the driver and open it
   again!

   This functions masks some bits off. That is, the bits not in the
   mask are not changed at all. The bits that are not masked
   off are defined in PARALLEL_CONTROL_PORT_MASK_VALUES.
*/
NTSTATUS
cbmiec_iec_dbg_write(IN PDEVICE_EXTENSION Pdx, IN UCHAR Value)
{
    UCHAR valueToSet;

    FUNC_ENTER();

    valueToSet = (Value & PARALLEL_CONTROL_PORT_MASK_VALUES) 
                 | ((Pdx->IecOutEor ^ Pdx->IecOutBits) & ~ PARALLEL_CONTROL_PORT_MASK_VALUES);

    DBG_PRINT((DBG_PREFIX "Value to set: %02x, modified to %02x", Value, valueToSet));

    WRITE_PORT_UCHAR(OUT_PORT, valueToSet);

    Pdx->IecOutBits = valueToSet ^ Pdx->IecOutEor;

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}
