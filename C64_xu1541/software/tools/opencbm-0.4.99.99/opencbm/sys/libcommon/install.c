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
** \file sys/libcommon/install.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Install the driver and check if anything needed is there
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"

#include "cbmioctl.h"
#include "version.h"

/*! \brief Complete driver installation

 This function performs anything that is needed for completing
 the driver installation.

 \param Pdx
   Pointer to the device extension.

 \param ReturnBuffer
   Pointer to a buffer which will contain the result.

 \param ReturnLength
   Pointer to a ULONG which contains the length of the ReturnBuffer
   on entry, and which will contain the length of the written
   ReturnBuffer on exit.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
 
 If ReturnLength is smaller then the length of CBMT_I_INSTALL_OUT,
 then only the the first settings are copied there. This way, a caller
 can get information even if newer versions have more informations to
 give.
*/
NTSTATUS
cbm_install(IN PDEVICE_EXTENSION Pdx, OUT PCBMT_I_INSTALL_OUT ReturnBuffer, IN OUT PULONG ReturnLength)
{
    CBMT_I_INSTALL_OUT result;

    FUNC_ENTER();

    RtlZeroMemory(&result, sizeof(result));

    // Set the driver version

    result.DriverVersion = 
        CBMT_I_INSTALL_OUT_MAKE_VERSION(OPENCBM_VERSION_MAJOR, OPENCBM_VERSION_MINOR, 
                                        OPENCBM_VERSION_SUBMINOR, OPENCBM_VERSION_DEVEL);

    result.DriverVersionEx = 
        CBMT_I_INSTALL_OUT_MAKE_VERSION_EX(OPENCBM_VERSION_PATCHLEVEL);

    // first of all, assume there is nothing to report

    result.ErrorFlags = 0;

    // Now, check if we have an interrupt

    if (!Pdx->ParallelPortAllocatedInterrupt)
    {
        // try to allow the allocation of the interrupt
        // by setting the appropriate registry settings

        if (!Pdx->IsNT4)
        {
            ParPortAllowInterruptIoctl(Pdx);
        }

        // report that we do not have any interrupt access currently.

        result.ErrorFlags |= CBM_I_DRIVER_INSTALL_0M_NO_INTERRUPT;
    }

    // Report the size of returned information.
    // Make sure that it is not longer then the maximum info we can give

    if (*ReturnLength > sizeof(CBMT_I_INSTALL_OUT))
    {
        *ReturnLength = sizeof(CBMT_I_INSTALL_OUT);
    }

    // Copy the return value into the return buffer

    RtlCopyMemory(ReturnBuffer, &result, *ReturnLength);

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}
