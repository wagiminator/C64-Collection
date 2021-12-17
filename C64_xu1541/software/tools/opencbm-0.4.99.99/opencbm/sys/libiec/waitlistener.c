/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2004 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2004 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file sys/libiec/waitlistener.c \n
** \author Spiro Trikaliotis \n
** \authors Based on code from
**    Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
** \n
** \brief Wait for listener to signal that it is ready to receive
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

#ifdef USE_DPC

/*! \brief Cancel routine if we are waiting

 This function is the cancel routine while a IRP is in the
 cbmiec_wait_for_listener() function.

 \param Fdo
   Pointer to the Fdo

 \param Irp
   Pointer to the IRP which is to be cancelled.

 This function does not cancel the IRP itself, but it only
 releases cbmiec_wait_for_listener().
*/
static VOID
WaitCancelRoutine(IN PDEVICE_OBJECT Fdo, IN PIRP Irp)
{
    PDEVICE_EXTENSION pdx;

    FUNC_ENTER();

    pdx = Fdo->DeviceExtension;

    // We do not need the cancel spin lock anymore

    DBG_DPC((DBG_PREFIX "Cancelling IRP 0x%p", Irp));
    DBG_IRQL( == DISPATCH_LEVEL);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    // Just release cbmiec_wait_for_listener(), but do NOT cancel the IRP!
    // This is left for cbmiec_wait_for_listener() as an exercise...

    DBG_IRQL( <= DISPATCH_LEVEL);
    KeSetEvent(&pdx->EventWaitForListener, IO_NO_INCREMENT, FALSE);

    FUNC_LEAVE();
}

#endif // #ifdef USE_DPC

/*! \brief Wait until listener is ready to receive

 This function waits until a listener is ready.

 \param Pdx
   Pointer to the device extension.

 \param SendEoi
   TRUE if we want to signal an EOI.
   FALSE otherwise.
*/
VOID
cbmiec_wait_for_listener(IN PDEVICE_EXTENSION Pdx, IN BOOLEAN SendEoi)
{
    ULONG NumberOfAcks = SendEoi ? 2 : 1;

    FUNC_ENTER();

    PERF_EVENT_VERBOSE(0x1100, NumberOfAcks);

    // This function has two incarnations. The first one
    // is used if we have successfully allocated the interrupt.
    // In this case, we just wait until the ISR has done the
    // essential work

    // When entering this function, DATA_IN should not be active

    DBG_ASSERT(CBMIEC_GET(PP_DATA_IN));

    if (Pdx->ParallelPortAllocatedInterrupt)
    {
        LONG ret;

        // This is implementation 1. It needs a working
        // ISR. The main work is done there

        // Tell the ISR how many interrupts to wait for

        PERF_EVENT_VERBOSE(0x1101, NumberOfAcks);
        ret = InterlockedExchange(&Pdx->IrqCount, NumberOfAcks);
        DBG_ASSERT(ret==0);
        PERF_EVENT_VERBOSE(0x1102, ret);

        // in the sequel, allow interrupts to occur

        DBG_IRQ(("Allow Interrupts"));
        CBMIEC_SET(PP_LP_IRQ);

        /*! \todo Shouldn't we make sure that there
         * is no spurious interrupt until now?
         */

        // Give the LISTENer the sign: We want to send something

        DBG_IRQ(("Release CLK_OUT"));
        CBMIEC_RELEASE(PP_CLK_OUT);

#ifdef USE_DPC

        // set the cancel routine which will wake us up if we do not get
        // an IRQ, and a cancellation is requested

        PERF_EVENT_VERBOSE(0x1103, 0);
        DBG_VERIFY(IoSetCancelRoutine(Pdx->IrpQueue.CurrentIrp, WaitCancelRoutine) DBGDO(== NULL));

        // Now, wait until we have been signalled

        PERF_EVENT_VERBOSE(0x1104, 0);
        DBG_DPC((DBG_PREFIX "CALL KeWaitForSingleObject()"));
        KeWaitForSingleObject(&Pdx->EventWaitForListener, Executive, KernelMode, FALSE, NULL);
        DBG_DPC((DBG_PREFIX "RETURN from KeWaitForSingleObject()"));

        PERF_EVENT_VERBOSE(0x1105, 0);

        // we do not need the cancel routine anymore:

        if (IoSetCancelRoutine(Pdx->IrpQueue.CurrentIrp, NULL) == NULL)
        {
            PERF_EVENT_VERBOSE(0x1106, -1);
            // the cancel routine was called!

            // Make sure the IrqCount is resetted to zero.

            InterlockedExchange(&Pdx->IrqCount, 0);
        }

#else

        // Wait until the listener has told us that it is able to listen

        while (!QueueShouldCancelCurrentIrp(&Pdx->IrpQueue) && Pdx->IrqCount)
        {
            cbmiec_schedule_timeout(libiec_global_timeouts.T_WaitForListener_Granu_T_H);
        }
#endif

        DBG_IRQ(("IrqCount = 0"));

        // from here on, no interrupts will be generated anymore

        CBMIEC_RELEASE(PP_LP_IRQ);
        DBG_IRQ(("No more Interrupts"));
    }
    else
    {
        // This is implementation 2. We do not have a working
        // ISR. Due to this, we have to busy wait until the LISTENer
        // has told us that it will accept our data.

        // This solution isn't good, as we have to busy wait
        // and the acknowledgement can take very long.

        // As we need very exact timing, don't allow anyone to
        // disturb us

        /*! \bug This implementation can lock up the whole machine,
         *  thus, do not use it!
         */

        cbmiec_block_irq(Pdx);

        // Give the LISTENer the sign: We want to send something

        CBMIEC_RELEASE(PP_CLK_OUT);

        // Wait until the listener has told us that it is able to listen

        while (!QueueShouldCancelCurrentIrp(&Pdx->IrpQueue) && NumberOfAcks)
        {
            while (!QueueShouldCancelCurrentIrp(&Pdx->IrpQueue) && CBMIEC_GET(PP_DATA_IN))
            {
                // Wait for 1 us:
                KeStallExecutionProcessor(1);
            }

            if (!CBMIEC_GET(PP_DATA_IN))
            {
                if (--NumberOfAcks == 0)
                {
                    CBMIEC_SET(PP_CLK_OUT);
                    DBG_SUCCESS((DBG_PREFIX "continue to send%s EOI", SendEoi ? "" : " no"));
                }
                else 
                {
                    while (!QueueShouldCancelCurrentIrp(&Pdx->IrpQueue) && !CBMIEC_GET(PP_DATA_IN))
                    {
                        // Wait for 1 us:
                        KeStallExecutionProcessor(1);
                    }
                }
            }
        }

        // Our timing is not critical anymore, go back to the old IRQL

        cbmiec_release_irq(Pdx);
    }

    PERF_EVENT_VERBOSE(0x1107, 0);
    FUNC_LEAVE();
}
