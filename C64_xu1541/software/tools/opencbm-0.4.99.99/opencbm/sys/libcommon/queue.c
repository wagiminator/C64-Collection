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
** \file sys/libcommon/queue.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Functions for queueung IRPs
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"

/*
 This file contains the quite tricky part of the drivers.
*/

/*! \internal \brief Insert an IRP into a CSQ

 This function inserts an IRP into a cancel-safe
 queue (CSQ) which is part of a QUEUE object.

 \param Csq
   Pointer to the CSQ object

 \param Irp
   Pointer to the IRP to be inserted.
   
  The cancel-safe queue object calls this function
  whenever it wants to insert an IRP into the queue.
  On entry, all necessary locks are held, so we can just
  insert the IRP and exit.

  This function assumes the CSQ is part of a QUEUE object.

  This function has been given to IoCsqInitialize().
*/
static VOID
InsertIrp(IN PIO_CSQ Csq, IN PIRP Irp)
{
    PQUEUE queue;

    FUNC_ENTER();

    PERF_EVENT_VERBOSE(0x3000, (ULONG)Irp);

    // Get the QUEUE object which contains this CSQ

    // no IRQL restrictions apply
    queue = CONTAINING_RECORD(Csq, QUEUE, IrpQueue);

    // Insert the IRP into the QUEUE

    // no IRQL restrictions apply
    InsertTailList(&queue->IrpListHead, &Irp->Tail.Overlay.ListEntry);

    PERF_EVENT_VERBOSE(0x3001, 0);

    FUNC_LEAVE();
}

/*! \internal \brief Remove an IRP from a CSQ

 This function removes  an IRP from a cancel-safe
 queue (CSQ), which is part of a QUEUE object.

 \param Csq
   Pointer to the CSQ object

 \param Irp
   Pointer to the IRP to be removed.
   
  The cancel-safe queue object calls this function
  whenever it wants to remove an IRP from the queue.
  On entry, all necessary locks are held, so we can just 
  remove the IRP and exit.

  This function assumes the CSQ is part of a QUEUE object.

  This function has been given to IoCsqInitialize().
*/
static VOID
RemoveIrp(IN PIO_CSQ Csq, IN PIRP Irp)
{
    FUNC_ENTER();

    UNREFERENCED_PARAMETER(Csq);

    PERF_EVENT_VERBOSE(0x3010, (ULONG)Irp);

    // Remove the IRP from the CSQ/QUEUE

    // no IRQL restrictions apply
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    PERF_EVENT_VERBOSE(0x3011, 0);

    FUNC_LEAVE();
}

/*! \internal \brief Find an IRP in a CSQ.

 This function tries to find an IRP which hold
 a specific requirement. If such an IRP exists,
 this function returns it, else, it returns NULL.

 \param Csq
   Pointer to the CSQ object

 \param Irp
   Pointer to a IRP which is the predecessor of 
   the first IRP to be checked against. If the 
   whole queue is to be searched, this can be
   NULL.

  \param FileObject
   Pointer to a FILE_OBJECT. If this parameter
   is given, the returned IRP has to be part of
   that FILE_OBJECT. If this parameter is NULL,
   any IRP will do.

  The cancel-safe queue object calls this function
  whenever it wants to find a specific IRP from the queue
  .
  On entry, all necessary locks are held, so we can just 
  search on the list, without worrying about race
  conditions.

  This function assumes the CSQ is part of a QUEUE object.

  This function has been given to IoCsqInitialize().

  This function is called in one of two circumstances:

  PeekNextIrp(Csq,NULL,NULL): Searches the first IRP on the queue\n
  PeekNextIrp(Csq,Irp,FileObject): Searches one IRP after the
  other which is associated to that FileObject. The purpose
  is to cancel all associated IRPs on IRP_MJ_CLEANUP.
*/
static PIRP
PeekNextIrp(IN PIO_CSQ Csq, IN PIRP Irp, IN PVOID FileObject)
{
    PLIST_ENTRY currentListEntry;
    PQUEUE queue;
    PIRP currentIrp;

    FUNC_ENTER();

    // Get the QUEUE object which contains this CSQ

    // no IRQL restrictions apply
    queue = CONTAINING_RECORD(Csq, QUEUE, IrpQueue);
    
    // Get the list entry of the first IRP to be checked 
    // into currentListEntry

    if (Irp == NULL)
    {
        // No previous IRP was given, thus, start at the
        // beginning of the queue

        currentListEntry = queue->IrpListHead.Flink;
    }
    else
    {
        // As an IRP was given, go to its successor.

        currentListEntry = Irp->Tail.Overlay.ListEntry.Flink;
    }

    // only process the list if it is not empty!

    if (currentListEntry != &queue->IrpListHead)
    {
        // get the containing IRP

        // no IRQL restrictions apply
        currentIrp = CONTAINING_RECORD(currentListEntry, IRP, Tail.Overlay.ListEntry);

        // If we were not given any file object, we have finished.
        // If we were given a file object, check if the IRP is
        // associated with that file object, thus, perform some
        // additional steps:

        if (FileObject)
        {
            BOOLEAN found;

            // We have not found any matching IRP yet

            found = FALSE;

            // For every IRP in the list, check if the condition
            // holds. If we found a matching IRP, quit this loop.

            while ((currentListEntry != &queue->IrpListHead) && !found)
            {
                PIO_STACK_LOCATION irpStack;

                // get the containing IRP

                // no IRQL restrictions apply
                irpStack = IoGetCurrentIrpStackLocation(currentIrp);
    
                if (irpStack->FileObject == (PFILE_OBJECT) FileObject)
                {
                    found = TRUE;
                }
                else
                {
                    // Proceed to next list entry (and IRP)

                    currentListEntry = currentListEntry->Flink;

                    // no IRQL restrictions apply
                    currentIrp = CONTAINING_RECORD(currentListEntry, IRP, 
                        Tail.Overlay.ListEntry);
                }
            }

            if (!found)
            {
                // If we are to quit the loop now, we have not
                // found a matching IRP

                currentIrp = NULL;
            }
        }
    }
    else
    {
        // We have not found any IRP

        currentIrp = NULL;
    }


    FUNC_LEAVE_PTR(currentIrp, PIRP);
}

/*! \internal \brief Acquire the lock for a CSQ

 This function acquires a lock for a cancel-safe
 queue (CSQ), which is part of a QUEUE object.

 \param Csq
   Pointer to the CSQ object

 \param Irql
   Pointer to a KIRQL object which will contain the
   old Irql after exit.

  The cancel-safe queue object calls this function
  whenever it needs a lock for manipulating the CSQ.

  This function assumes the CSQ is part of a QUEUE object.

  This function has been given to IoCsqInitialize().
*/
static VOID
AcquireLock(IN PIO_CSQ Csq, OUT PKIRQL Irql)
{
    PQUEUE queue;
    KIRQL irql; // do not use Irql directly, but only indirectly,
                // as suggested by Doron Holan at
                // http://blogs.msdn.com/doronh/archive/2006/03/08/546934.aspx

    FUNC_ENTER();

    // Get the containing QUEUE object

    // no IRQL restrictions apply
    queue = CONTAINING_RECORD(Csq, QUEUE, IrpQueue);

    // As we are using a spin lock, acquire that one

    DBG_IRQL( <= DISPATCH_LEVEL);
    KeAcquireSpinLock(&queue->IrpListSpinlock, &irql);

    *Irql = irql;

    FUNC_LEAVE();
}

/*! \internal \brief Release the lock for a CSQ

 This function releases a lock for a cancel-safe
 queue (CSQ), which is part of a QUEUE object.

 \param Csq
   Pointer to the CSQ object

 \param Irql
   The IRQL which was active before AcquireLock() was
   called. This is the returned value of AcquireLock().

  The cancel-safe queue object calls this function
  whenever it does not need the lock for manipulating
  the CSQ anymore.

  This function assumes the CSQ is part of a QUEUE object.

  This function has been given to IoCsqInitialize().
*/
static VOID
ReleaseLock(IN PIO_CSQ Csq, IN KIRQL Irql)
{
    PQUEUE queue;

    FUNC_ENTER();

    // Get the containing QUEUE object

    // no IRQL restrictions apply
    queue = CONTAINING_RECORD(Csq, QUEUE, IrpQueue);

    // As we are using a spin lock, release that one

    DBG_IRQL( == DISPATCH_LEVEL);
    KeReleaseSpinLock(&queue->IrpListSpinlock, Irql);

    FUNC_LEAVE();
}

/*! \internal \brief Complete a cancelled IRP

 This function completes a IRP which is part of a 
 cancel-safe queue (CSQ), which is part of a QUEUE
 object.

 \param Csq
   Pointer to the CSQ object

 \param Irp
   Pointer to the IRP to be cancelled.

  The cancel-safe queue object calls this function
  whenever it wants to cancel an IRP. The CSQ library
  makes sure we do not need to worry about race conditions,
  as long as we use an appropriate lock for AcquireLock()
  and ReleaseLock().

  This function assumes the CSQ is part of a QUEUE object.

  This function has been given to IoCsqInitialize().
*/
static VOID
CompleteCanceledIrp(IN PIO_CSQ Csq, IN PIRP Irp)
{
    PQUEUE queue;

    FUNC_ENTER();

    PERF_EVENT_CANCELIRP(Irp);

    // Get the containing QUEUE object

    // no IRQL restrictions apply
    queue = CONTAINING_RECORD(Csq, QUEUE, IrpQueue);

    // Complete the IRP

    QueueCompleteIrp(queue, Irp, STATUS_CANCELLED, 0);

    FUNC_LEAVE();
}

/*! \brief Initialize a QUEUE object

 This function initializes a QUEUE object before it can be used by
 the other Queue...() functions.

 \param Queue
   Pointer to the QUEUE object.

 \param DriverStartIo
   Pointer to the StartIo function for this queue
*/
VOID
QueueInit(PQUEUE Queue, PCBMDRIVER_STARTIO DriverStartIo)
{
    FUNC_ENTER();

    // can be run at arbitrary IRQL
    KeInitializeSpinLock(&Queue->IrpListSpinlock);

    // can be run at arbitrary IRQL
    InitializeListHead(&Queue->IrpListHead);

    // Initialize the event which is used to wake up the thread
    DBG_IRQL( == PASSIVE_LEVEL);
    KeInitializeEvent(&Queue->NotEmptyEvent, SynchronizationEvent, FALSE);

#ifdef USE_FAST_START_THREAD

    // Initialize the event which is used to wake up "the caller of the thread"
    DBG_IRQL( == PASSIVE_LEVEL);
    KeInitializeEvent(&Queue->BackSignalEvent, SynchronizationEvent, FALSE);

#endif // #ifdef USE_FAST_START_THREAD

    // Initialize the cancel-safe queue
    // no IRQL restrictions apply!
    IoCsqInitialize(&Queue->IrpQueue,
        InsertIrp,
        RemoveIrp,
        PeekNextIrp,
        AcquireLock,
        ReleaseLock,
        CompleteCanceledIrp);

    // remember the function pointers
    Queue->DriverStartIo = DriverStartIo;

    // start in stalled state
    Queue->IsStalled = TRUE;

    // do not start with dropping IRPs
    Queue->IsDropping = FALSE;

    FUNC_LEAVE();
}

/*! \brief Insert an IRP into a QUEUE object

 This function inserts an IRP into the given QUEUE object.

 \param Queue
   Pointer to the QUEUE object.

 \param Irp
   Pointer to the IRP to be queued.

 \param FastStart
   If this value is TRUE, than this IRP does not last long.
   Thus, it can be started immediately if the QUEUE is
   empty.
   If this value is FALSE, this IRP can take much time,
   so a fast start is not allowed

 \param Fdo
   Pointer to the DEVICE_OBJECT. This is only needed if
   FastStart is TRUE.

 \return 
   If the IRP has been successfully inserted into the QUEUE,
   this function returns STATUS_PENDING. If the IRP could
   not be inserted, it returns the status code given by the
   abortion status.

 If the queue is in the dropping state, this function does
 not insert the IRP into the queue, but it immediately completes
 the IRP with the given abortion status.

 The return value can be directly used in the dispatch function 
 which calls QueueStartPacket() as the return status.
*/
NTSTATUS
QueueStartPacket(PQUEUE Queue, PIRP Irp, BOOLEAN FastStart, PDEVICE_OBJECT Fdo)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    FUNC_ENTER();

    // Are we currently dropping IRPs?
    if (!Queue->IsDropping)
    {
        // No: Process the IRP

        PERF_EVENT_VERBOSE(0x3020, (ULONG)Irp);

        if (FastStart)
        {
            DBG_ASSERT(Fdo != NULL);

            PERF_EVENT_VERBOSE(0x3021, (ULONG)Irp);

            // The caller has chosen FastStart.
            // This means that he wants to IRP to be executed
            // immediately (and not be queue) if there is not
            // IRP currently being executed in the thread.

            // is there already an IRP in progress?

            if (Queue->CurrentIrp == NULL)
            {
                // no, thus, we can do a fast start

                // Mark this IRP as the current one

                Queue->CurrentIrp = Irp;

                // Immediately execute this IRP

                PERF_EVENT_VERBOSE(0x3022, (ULONG)Irp);

                ntStatus = Queue->DriverStartIo(Fdo, Irp);

                PERF_EVENT_VERBOSE(0x3023, 0);
            }
            else
            {
                // There is an IRP being processed. 
                // Unfortunately, this means that a fast 
                // start is not possible.

                FastStart = FALSE;
            }
        }

        // Has this IRP already been fast started?

        if (!FastStart)
        {
            // No: Add the IRP to the Queue

            PERF_EVENT_VERBOSE(0x3024, (ULONG)Irp);

            // First of all, since we will pend the IRP later,
            // mark it pending to prevent the race condition

            DBG_IRQL( <= DISPATCH_LEVEL);
            IoMarkIrpPending(Irp);

            // Insert the IRP into the queue.

            // no IRQL restrictions apply!
            IoCsqInsertIrp(&Queue->IrpQueue, Irp, NULL);

            PERF_EVENT_VERBOSE(0x3025, 0);

            // Wake up the waiting thread which can execute
            // the queued IRP

            QueueSignal(Queue);

            PERF_EVENT_VERBOSE(0x3026, 0);

            // Return to the caller that the IRP has been pended.
            // This tells the caller that it has to wait until the
            // IRP is completed by another means.

            ntStatus = STATUS_PENDING;
        }
    }
    else
    {
        // We are currently dropping the IRP, thus,
        // just return the value set before, and complete the IRP
        // without doing anything.

        ntStatus = QueueCompleteIrp(NULL, Irp, Queue->DroppingReturnStatus, 0);
    }

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief Remove the next IRP from a QUEUE object

 This function removes an IRP from the given QUEUE object.

 \param Queue
   Pointer to the QUEUE object.

 \return
   Returns the pointer to the next IRP.

 The returned IRP is removed from the QUEUE list.

 \internal
   This function does not perform any synchronization other
   than what ExInterlockedRemoveHeadList() provides. Because
   of this, it is not safe to be used directly from a driver.
   Because of this, this function is marked INTERNAL, and it
   has static binding.
*/
static PIRP
QueueRemoveNextIrp(PQUEUE Queue)
{
    PIRP irp;

    FUNC_ENTER();

    // Just let the CSQ perform all the necessary steps

    irp = IoCsqRemoveNextIrp(&Queue->IrpQueue, NULL);

    PERF_EVENT_VERBOSE(0x3030, (ULONG)irp);

    FUNC_LEAVE_PTR(irp, PIRP);
}

/*! \brief Sets a QUEUE into stalled state.

 This function unconditionally stalls the given QUEUE.

 \param Queue
   Pointer to the QUEUE object.

 This function is not to be called recursively.

 This function is needed for correct PnP management.
 As currently, we do not do any PnP, this function is
 unused currently.
*/
VOID
QueueStall(PQUEUE Queue)
{
    LONG ret;

    FUNC_ENTER();

    // Increment the number of stall requests

    ret = InterlockedIncrement(&Queue->IsStalled);

    DBG_ASSERT(ret == 0);

    FUNC_LEAVE();
}

/*! \brief Sets a QUEUE into unstalled state.

 This function unstalls the given QUEUE.

 \param Queue
   Pointer to the QUEUE object.

 This function is not to be called recursively.

 This function is needed for correct PnP management.
 As currently, we do not do any PnP, this function is
 unused currently.
*/
VOID
QueueUnstall(PQUEUE Queue)
{
    LONG ret;

    FUNC_ENTER();

    // Decrement the number of stall requests

    ret = InterlockedDecrement(&Queue->IsStalled);

    DBG_ASSERT(ret == 1);

    FUNC_LEAVE();
}

#if DBG

/*! \brief Check if a QUEUE is in stalled state.

 This function cheks if the given QUEUE is in the
 stalled state.

 \param Queue
   Pointer to the QUEUE object.

 \return 
    TRUE if the QUEUE is currently stalled, FALSE else.

 Without any further protection, the state which is reported
 could have changed at the time this function returns! Thus,
 handle with care. 
 This means, the caller has to make sure that QueueIsStalled() 
 cannot compete with QueueStall() and/or QueueUnstall().
*/
BOOLEAN
QueueIsStalled(PQUEUE Queue)
{
    BOOLEAN Ret;

    FUNC_ENTER();

    Ret = Queue->IsStalled ? TRUE : FALSE;

    FUNC_LEAVE_BOOLEAN(Ret);
}

/*! \brief Check if a QUEUE is in DROPPING state.

 This function checks if the given QUEUE is in the
 DROPPING state.

 \param Queue
   Pointer to the QUEUE object.

 \return 
    TRUE if the QUEUE is currently dropping, FALSE else.

 A QUEUE is in the DROPPING state if it does not take
 any new IRP, but completes it immediately with a given
 completion status.

 Without any further protection, the state which is reported
 could have changed at the time this function returns! Thus,
 handle with care.

 This function is needed for correct PnP management.
 As currently, we do not do any PnP, this function is
 unused currently.

 \bug
 Currently, there is no function for setting a Queue into
 dropping state. So, this function is not needed at all (yet).
*/
BOOLEAN
QueueIsDropping(PQUEUE Queue)
{
    BOOLEAN Ret;

    FUNC_ENTER();

    Ret = Queue->IsDropping ? TRUE : FALSE;

    FUNC_LEAVE_BOOLEAN(Ret);
}

#endif // #if DBG

/*! \brief Complete an IRP which is on a QUEUE

 This function completes an IRP which is on a QUEUE object

 \param Queue
   Pointer to the QUEUE object, or NULL if the IRP is on 
   no QUEUE at all.

 \param Irp
   Pointer to the Irp to be completed

 \param NtStatus
   The NTSTATUS code which is to be used for the completion

 \param Information
   The value to give the Information field of the IRP

 \return 
    Returns the given NtStatus

 Without any further protection, the state which is reported
 could have changed at the time this function returns! Thus,
 handle with care.
*/
NTSTATUS
QueueCompleteIrp(PQUEUE Queue, PIRP Irp, NTSTATUS NtStatus, ULONG_PTR Information)
{
    FUNC_ENTER();

    // If there was a Queue given, the IRP can be the current
    // executing IRP of the Queue. Because of this, make sure
    // to unset it from executing state ("CurrentIrp") if it
    // happens to be the current IRP.

    if (Queue)
    {
        DBG_ASSERT(Queue->CurrentIrp != NULL);
        DBG_ASSERT(Queue->CurrentIrp == Irp);

        if (Queue->CurrentIrp == Irp)
        {
            Queue->CurrentIrp = NULL;
        }
    }

    // Set the return values into the IRP

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = NtStatus;

    PERF_EVENT_COMPLETEIRP(Irp);

    // Now, complete the IRP.

    DBG_IRQL( <= DISPATCH_LEVEL);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    // For convenience, return the NTSTATUS which was used to
    // complete the IRP.

    FUNC_LEAVE_NTSTATUS(NtStatus);
}

/*! \brief Poll the QUEUE

 This function polls the QUEUE object. 
 If there is an IRP in there, the IRP is executed by 
 calling the specified DriverStartIo() function.

 If there is no IRP, this function just sleeps until
 there is one.

 \param Queue
   Pointer to the QUEUE object.

 \param Fdo
   Pointer to the DEVICE_OBJECT on which the Queue is located.
   This parameter is given to the call of StartIo().

 \bug
   This function does not perform any synchronization other
   than what ExInterlockedRemoveHeadList() provides. Because
   of this, it is not safe to be used directly from a driver.
*/
NTSTATUS
QueuePoll(PQUEUE Queue, PDEVICE_OBJECT Fdo)
{
    PIRP irp;

    FUNC_ENTER();

    DBG_IRQL( == PASSIVE_LEVEL);

    DBG_ASSERT(Queue->CurrentIrp == NULL);

    // First, check if there is an IRP in the QUEUE

    PERF_EVENT_VERBOSE(0x3040, 0);

    irp = QueueRemoveNextIrp(Queue);

    PERF_EVENT_VERBOSE(0x3041, (ULONG)irp);

    if (!irp)
    {
        // There is no IRP in the QUEUE, wait for a new one

        PERF_EVENT_VERBOSE(0x3042, (ULONG)irp);

        KeWaitForSingleObject(&Queue->NotEmptyEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL);

        PERF_EVENT_VERBOSE(0x3043, (ULONG)irp);

#ifdef USE_FAST_START_THREAD

        // Signal to the caller that it can continue

        DBG_IRQL( <= DISPATCH_LEVEL);
        KeSetEvent(&Queue->BackSignalEvent, IO_NO_INCREMENT, FALSE);

        PERF_EVENT_VERBOSE(0x3044, (ULONG)irp);

#endif USE_FAST_START_THREAD

        // Get the next IRP from the QUEUE
        // Remember: There might not be any IRP on the
        // QUEUE. The caller has to handle this state
        // properly.

        irp = QueueRemoveNextIrp(Queue);

        PERF_EVENT_VERBOSE(0x3045, (ULONG)irp);
    }

    // If there is an IRP, execute and complete that

    if (irp)
    {
        DBG_ASSERT(Queue->CurrentIrp == NULL);

        // Set the IRP we just got as the current one

        Queue->CurrentIrp = irp;

        // Execute the IRP by calling DriverStartIo()

//        KIRQL oldIrql;
//        KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
        Queue->DriverStartIo(Fdo, irp);
//        KeLowerIrql(oldIrql);
    }

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}

/*! \brief Signal to the QUEUE need for processing

 This function signals the QUEUE object that there is
 something to be processed. In general, this means that
 some IRP has been queued.

 \param Queue
   Pointer to the QUEUE object.
*/
NTSTATUS
QueueSignal(PQUEUE Queue)
{
#ifdef USE_FAST_START_THREAD

    LARGE_INTEGER timeout;

#endif // #ifdef USE_FAST_START_THREAD

    FUNC_ENTER();

    // Wake up the waiting thread

#ifdef USE_FAST_START_THREAD

    // The fast start of the thread is only allowed if we are running
    // at PASSIVE_LEVEL.

    PERF_EVENT_VERBOSE(0x3050, 0);

    if (KeGetCurrentIrql() == PASSIVE_LEVEL)
    {
        // Make sure the "backsignal event" is not set before we start the thread

        DBG_IRQL( <= DISPATCH_LEVEL);
        KeClearEvent(&Queue->BackSignalEvent);

        PERF_EVENT_VERBOSE(0x3051, 0);

        // Initialize the timeout value

        timeout.QuadPart = 1;

        // Signal the other thread. Make sure no-one else can disturb us
        // by setting the last argument (Wait) to TRUE

        DBG_IRQL( == PASSIVE_LEVEL);
        KeSetEvent(&Queue->NotEmptyEvent, IO_NO_INCREMENT, TRUE);

        PERF_EVENT_VERBOSE(0x3052, 0);

        // Make sure the other thread is scheduled by waiting for the
        // "back event"

        // This IRQL restriction "normally" applies. Anyway, as the above
        // KeSetEvent() was called with Wait == TRUE, in fact, we *are* at
        // DISPATCH_LEVEL, so this tests does not make sense.

        // DBG_IRQL( < DISPATCH_LEVEL);
        KeWaitForSingleObject(&Queue->BackSignalEvent,
            Executive,
            KernelMode,
            FALSE,
            &timeout);

        PERF_EVENT_VERBOSE(0x3053, 0);
    }
    else
    {
        // As no fast start is allowed, just signal the thread

        PERF_EVENT_VERBOSE(0x3054, 0);

        DBG_IRQL( <= DISPATCH_LEVEL);
        KeSetEvent(&Queue->NotEmptyEvent, IO_NO_INCREMENT, FALSE);

        PERF_EVENT_VERBOSE(0x3055, 0);
    }

#else // #ifdef USE_FAST_START_THREAD

    DBG_IRQL( <= DISPATCH_LEVEL);
    KeSetEvent(&Queue->NotEmptyEvent, IO_NO_INCREMENT, FALSE);

#endif // #ifdef USE_FAST_START_THREAD


    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}

/*! \brief Process an IRP_MJ_CLEANUP on the QUEUE

 Whenever an IRP_MJ_CLEANUP is received by the driver,
 it has to complete all IRPs that belong to the given
 file object. This function takes all appropriate IRPs
 out of the QUEUE and completes them.

 \param Queue
   Pointer to the QUEUE object.

 \param FileObject
   Pointer to the FILE_OBJECT of which the associated
   IRPs have to be cancelled.

 \todo
   What if the current IRP is associated to that FILE_OBJECT?
   Will that one be cancelled separately?
*/
NTSTATUS
QueueCleanup(PQUEUE Queue, PFILE_OBJECT FileObject)
{
    PIRP irp;

    FUNC_ENTER();

    // Check if there is an IRP associated with that FILE_OBJECT

    // No IRQL restrictions apply
    irp = IoCsqRemoveNextIrp(&Queue->IrpQueue, FileObject);

    // While there are still IRPs which are associated with the
    // given FILE_OBJECT, take them out of the QUEUE and complete them.

    while (irp)
    {
        QueueCompleteIrp(NULL, irp, STATUS_CANCELLED, 0);

        // Check for next IRP associated with that FILE_OBJECT

        // No IRQL restrictions apply
        irp = IoCsqRemoveNextIrp(&Queue->IrpQueue, FileObject);
    }

    //! \todo What if the current irp is associated with that FILE_OBJECT?

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}

/*! \brief Should the current IRP be cancelled?

 Find out if the caller of the current IRP is not interested 
 anymore in the result.

 \param Queue
   Pointer to the QUEUE object.

 This function should be called in the processing of an IRP whenever
 there is a change that some processing will take some considerable
 time. An alternative to polling this function is to install a cancel
 routine.
*/
BOOLEAN
QueueShouldCancelCurrentIrp(PQUEUE Queue)
{
    FUNC_ENTER();

    // Check if the Cancel field of the current IRP has been set

    FUNC_LEAVE_BOOLEAN(Queue->CurrentIrp->Cancel ? TRUE : FALSE);
}
