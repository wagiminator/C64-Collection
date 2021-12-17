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
** \file sys/libcommon/thread.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Common functions für initialization the WDM and NT4 driver
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"

/*! \brief Start the worker thread

 This function start the worker thread.

 \param Pdx
   Pointer to a DEVICE_EXTENSION structure. 

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. 
   Otherwise, it return one of the error status values.
*/
NTSTATUS
cbm_start_thread(IN PDEVICE_EXTENSION Pdx)
{
    NTSTATUS ntStatus;
    PDEVICE_OBJECT fdo;

    FUNC_ENTER();

    DBG_ASSERT(Pdx != NULL);

    // get the FDO out of the PDX

    fdo = Pdx->Fdo;

    DBG_ASSERT(fdo != NULL);

    // The ThreadHandle as well as the thread object pointer should be
    // 0. If not, it means that the thread has already been started.

    DBG_ASSERT(Pdx->ThreadHandle == 0);
    DBG_ASSERT(Pdx->Thread == 0);

    PERF_EVENT_THREAD_START_SCHED();

    // The thread should not be quit yet.
    //! \todo Replace Pdx->QuitThread with a event for quitting

    Pdx->QuitThread = FALSE;

    // Create the thread that will execute our IRPs

    DBG_IRQL( == PASSIVE_LEVEL);
    ntStatus = PsCreateSystemThread(&Pdx->ThreadHandle,
        THREAD_ALL_ACCESS, // Desired access
        NULL,              // No object attributes
        NULL,              // which process should the thread belong to?
        NULL,              // Client ID
        cbm_thread,        // the routine to be started
        Pdx);              // context value for the thread

    if (!NT_SUCCESS(ntStatus))
    {
        // The thread could not been started, make sure this is logged

        DBG_ERROR((DBG_PREFIX "Creation of system thread FAILED, deleting device"));
        LogErrorString(fdo, CBM_START_FAILED, L"creation of the system thread.", NULL);
    }
    else
    {
        // Find out the thread object pointer, which is needed
        // for setting the thread priority. Furthermore, as we have
        // referenced the object, the driver will refuse to be unloaded
        // until the object is dereferenced.

        DBG_IRQL( == PASSIVE_LEVEL);
        ObReferenceObjectByHandle(Pdx->ThreadHandle,
            THREAD_ALL_ACCESS, NULL, KernelMode, &Pdx->Thread, NULL);

        DBG_ASSERT(Pdx->Thread);

        if (Pdx->Thread)
        {
            // Set the priority of the thread to a realtime priority.

            DBG_IRQL( == PASSIVE_LEVEL);
            KeSetPriorityThread(Pdx->Thread, LOW_REALTIME_PRIORITY);
        }
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Stop the worker thread

 This function stops the worker thread.

 \param Pdx
   Pointer to a DEVICE_EXTENSION structure. 

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. 
   Otherwise, it return one of the error status values.
*/
VOID
cbm_stop_thread(IN PDEVICE_EXTENSION Pdx)
{
    FUNC_ENTER();

    DBG_ASSERT(Pdx != NULL);
    DBG_ASSERT(Pdx->QuitThread == FALSE);
    DBG_ASSERT(Pdx->ThreadHandle != 0);
    DBG_ASSERT(Pdx->Thread != 0);

    if ((Pdx->ThreadHandle != 0) && (Pdx->Thread != 0))
    {
        PERF_EVENT_THREAD_STOP_SCHED();

        // Signal the thread that it should QUIT

        Pdx->QuitThread = TRUE;

        // Wake up the thread so it can process the signalled QUIT event

        QueueSignal(&Pdx->IrpQueue);

        // Now, wait until the thread has been stopped

        KeWaitForSingleObject(Pdx->Thread, Executive, KernelMode, FALSE, NULL);

        // We're done, we do not need the thread's handle or object anymore

        ObDereferenceObject(Pdx->Thread);
        ZwClose(Pdx->ThreadHandle);

        // Mark that we do not have a thread anymore

        Pdx->Thread = NULL;
        Pdx->ThreadHandle = 0;
    }

    FUNC_LEAVE();
}

/*! \brief The thread function

 This function is the function of the thread itself.
 It polls the QUEUE and executed the IRPs which are on it.

 \param Context
   Pointer to the thread context. This is a PDX in reality.
*/
VOID
cbm_thread(IN PVOID Context)
{
    PDEVICE_EXTENSION pdx;
    NTSTATUS ntStatus;

    FUNC_ENTER();

    PERF_EVENT_THREAD_START_EXEC();

    // get the device extension

    pdx = Context;

    // If we are immediately quit, make sure we report STATUS_SUCCESS

    ntStatus = STATUS_SUCCESS;

    // As long as we do not have to quit the thread, poll the QUEUE

    while (!pdx->QuitThread)
    {
        PERF_EVENT_THREAD_POLL();

        // Poll the QUEUE object and execute the IRP (if any)

        ntStatus = QueuePoll(&pdx->IrpQueue, pdx->Fdo);
    }

    PERF_EVENT_THREAD_STOP_EXEC();

    // Termine the thread

    DBG_IRQL( == PASSIVE_LEVEL);
    PsTerminateSystemThread(ntStatus);

    FUNC_LEAVE();
}
