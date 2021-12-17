/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2006-2007 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file sys/libcommon/lockunlock.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Functions for locking und unlocking the driver onto the parallel port
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "iec.h"

#include "version.h"

/*! \brief Lock the parallel port for the driver

 This function locks the driver onto the parallel port, so
 we can use the port afterwards.

 \param Pdx
   Pointer to the device extension.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/

NTSTATUS
cbm_lock_parport(IN PDEVICE_EXTENSION Pdx)
{
    NTSTATUS ntStatus;

    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "+++ LOCK PARPORT"));
    DBG_ASSERT(Pdx->ParallelPortIsLocked == FALSE);

    ntStatus = ParPortAllocate(Pdx);

    // Set the appropriate mode of the parallel port
    // Normally, this will be either BYTE MODE, or SPP

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = ParPortSetMode(Pdx);
    }

    // Try to allocate the interrupt

    if (NT_SUCCESS(ntStatus))
    {
        /*! \todo
         * As we will try to cope without interrupt,
         * do not handle it as an open failure if we
         * do not succeed!
         */

        // ntStatus =
        ParPortAllocInterrupt(Pdx, cbm_isr);
    }

    // Initialize the IEC serial port

    if (NT_SUCCESS(ntStatus))
    {
        Pdx->ParallelPortIsLocked = TRUE;

        // initialize cable type and try to detect the cable if necessary

        cbm_init_registry(NULL, Pdx);

        // initialize the IEC Bus

        cbmiec_init(Pdx);
    }

    // Did we fail any call? If yes, free and release
    // any resource we might happen to have allocated
    // before we failed


    if (!NT_SUCCESS(ntStatus))
    {
        // The functions themselves test if the resource
        // is allocated, thus, we do not need to protect
        // against freeing non-allocated resources here.

        ParPortFreeInterrupt(Pdx);
        ParPortUnsetMode(Pdx);
        ParPortFree(Pdx);
    }

    // release the bus (to be able to share it with other
    // controllers

    if (NT_SUCCESS(ntStatus) && !Pdx->DoNotReleaseBus)
    {
        cbmiec_release_bus(Pdx);
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Unlock the parallel port for the driver

 This function unlocks the driver from the parallel port
 after the port has been used.

 \param Pdx
   Pointer to the device extension.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/

NTSTATUS
cbm_unlock_parport(IN PDEVICE_EXTENSION Pdx)
{
    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "--- UNLOCK PARPORT"));
    DBG_ASSERT(Pdx->ParallelPortIsLocked == TRUE);

    Pdx->ParallelPortIsLocked = FALSE;

    // release the bus (to be able to share it with other controllers)

    if (!Pdx->DoNotReleaseBus)
    {
        cbmiec_release_bus(Pdx);
    }

    // release all resources we have previously allocated

    ParPortFreeInterrupt(Pdx);
    ParPortUnsetMode(Pdx);
    ParPortFree(Pdx);

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}

/*! \brief Lock the parallel port for the driver

 This function locks the driver onto the parallel port. This way,
 no other program or driver can allocate the parallel port and
 interfere with the communication.

 \param Pdx
   Pointer to the device extension.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
 
 \remark
 A call to cbm_lock() is undone with a call to cbm_unlock().

 Note that it is *not* necessary to call this function
 (or cbm_unlock()) when all communication is done with
 the handle to opencbm open (that is, between 
 cbm_driver_open() and cbm_driver_close(). You only
 need this function to pin the driver to the port even
 when cbm_driver_close() is to be executed (for example,
 because the program terminates).
*/

NTSTATUS
cbm_lock(IN PDEVICE_EXTENSION Pdx)
{
    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "*** LOCK"));
    Pdx->ParallelPortLock = TRUE;

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}

/*! \brief Unlock the parallel port for the driver

 This function unlocks the driver from the parallel port.
 This way, other programs and drivers can allocate the
 parallel port and do their own communication with
 whatever device they use.

 \param Pdx
   Pointer to the device extension.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
 
 \remark
 Look at cbm_lock() for an explanation of this function.
*/

NTSTATUS
cbm_unlock(IN PDEVICE_EXTENSION Pdx)
{
    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "*** UNLOCK"));
    Pdx->ParallelPortLock = FALSE;

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}
