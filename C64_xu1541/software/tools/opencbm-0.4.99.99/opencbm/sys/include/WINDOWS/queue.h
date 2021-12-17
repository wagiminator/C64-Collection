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
** \file sys/include/WINDOWS/queue.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Definitions for the queueing functions
**
****************************************************************/

#ifndef SRT_QUEUE_H
#define SRT_QUEUE_H

#ifdef CSQ_STATIC
#include <csq.h>
#endif

/*! This type specifies a StartIo function for the Queue */

typedef NTSTATUS (*PCBMDRIVER_STARTIO)(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

/*! \brief A QUEUE object
 * A QUEUE object is an object which can be used to queue
 * IRPs for processing.
 * This QUEUE object is optimized for being polled from
 * an own worker thread. Anyway, a concept called FastStart
 * is also implemented, which allows some IRPs to be completed
 * immediately, without being queued, if no IRP is executed yet
 * and the caller has stated that he wants this IRP to be started
 * fast if possible.
 */
typedef
struct QUEUE
{
    /*! \brief the structure for the cancel-safe queue */
    IO_CSQ IrpQueue;

    /*! \brief the list head for the IRP list */
    LIST_ENTRY IrpListHead;

    /*! \brief the spin lock to protect the IRP list */
    KSPIN_LOCK IrpListSpinlock;

    /*! \brief signal that the queue is not empty */
    KEVENT NotEmptyEvent;

#ifdef USE_FAST_START_THREAD

    /*! \brief Signal to the caller of the QUEUE that it
     * can continue */
    KEVENT BackSignalEvent;

#endif // #ifdef USE_FAST_START_THREAD

    /*! \brief pointer to the StartIo function to be called */
    PCBMDRIVER_STARTIO DriverStartIo;

    /*! \brief counter; if != 0, this queue is stalled, that is,
       no entries are dequeued. */
    LONG IsStalled;

    /*! \brief counter; if != 0, this queue is dropping, that is,
       no new entries are queued into the queue; instead, the IRPs
       are being completed. */
    LONG IsDropping;

    /*! \brief The NTSTATUS return code with which the IRP are completed
       if we are dropping IRPs */
    NTSTATUS DroppingReturnStatus;

    /*! \brief Pointer to the IRP which is currently processed */
    PIRP CurrentIrp;

} QUEUE, *PQUEUE;

/* For performance reasons, these two functions are defined as macros
   in release builds of the driver.
   They do not need any protecting by synchronization primitives, as their
   state could have changed on return in either way. This, the caller has
   to make sure that is obtains exactly the state it wants to know.
*/
#ifndef DBG

    /*! Return: Is queue currently stalled */
    #define QueueIsStalled(_Queue_) (_Queue_->IsStalled ? 1 : 0)

    /*! Return: Is the queue currently dropping */
    #define QueueIsDropping(_Queue_) (_Queue_->IsDropping ? 1 : 0)

#endif

extern VOID
QueueInit(PQUEUE Queue, PCBMDRIVER_STARTIO DriverStartIo);

extern NTSTATUS
QueueCompleteIrp(PQUEUE Queue, PIRP Irp, NTSTATUS StatusCode, ULONG_PTR Information);

extern NTSTATUS
QueueStartPacket(PQUEUE Queue, PIRP Irp, BOOLEAN FastStart, PDEVICE_OBJECT Fdo);

extern NTSTATUS
QueuePoll(PQUEUE Queue, PDEVICE_OBJECT Fdo);

extern NTSTATUS
QueueSignal(PQUEUE Queue);

extern NTSTATUS
QueueCleanup(PQUEUE Queue, PFILE_OBJECT FileObject);

extern BOOLEAN
QueueShouldCancelCurrentIrp(PQUEUE Queue);

#endif /* #ifndef SRT_QUEUE_H */
