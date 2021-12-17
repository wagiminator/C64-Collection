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
** \file sys/libiec/util.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Some utility functions for the IEC library
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

/*! \brief Schedule a timeout

 This function schedules a timeout.
 Scheduling means that other threads have the opportunity to
 use the processor while we're waiting.

 \param Howlong
   How long to wait (in us)
*/
VOID
cbmiec_schedule_timeout(IN ULONG Howlong)
{
    LARGE_INTEGER li;
    NTSTATUS ntStatus;
    KTIMER cbmTimer;

    FUNC_ENTER();

    // Note: The following timer is NEVER fired! We just use it
    //       to allow a KeWaitForSingleObject()-determined timeout!

    KeInitializeTimer (&cbmTimer);

    li.QuadPart  = - (LONG) Howlong;
    li.QuadPart *= 10i64;

    ntStatus = KeWaitForSingleObject(&cbmTimer, UserRequest,
                  KernelMode, FALSE, &li);

    FUNC_LEAVE();
}

/*! \brief Wait for a timeout

 This function waits for a timeout.
 Waiting means that we want to have an exact timing, so
 don't give away the processor.

 \param Howlong
   How long to wait (in us)
*/
VOID
cbmiec_udelay(IN ULONG Howlong)
{
    LARGE_INTEGER li;
    NTSTATUS ntStatus;

    FUNC_ENTER();

#if 1
    KeStallExecutionProcessor(Howlong);
#else
    li.QuadPart  = - (LONG) Howlong;
    li.QuadPart *= 10i64;

    ntStatus = KeDelayExecutionThread(KernelMode, FALSE, &li);
#endif

    FUNC_LEAVE();
}


/*! \brief Block all interrupts

 This function blocks all interrupt, thus that we cannot
 be interrupted while executing some critical things.

 This should not be used for big time periods.
*/
VOID
cbmiec_block_irq(PDEVICE_EXTENSION Pdx)
{
    KIRQL irql; // do not use Irql directly, but only indirectly,
                // as suggested by Doron Holan at
                // http://blogs.msdn.com/doronh/archive/2006/03/08/546934.aspx

    FUNC_ENTER();

    DBGDO(DBG_ASSERT(InterlockedIncrement(&Pdx->IecBlockIrqUsageCount)==1));

    KeRaiseIrql(HIGH_LEVEL, &irql);
    Pdx->IecBlockIrqPreviousIrql = irql;

    CLI();

    FUNC_LEAVE();
}

/*! \brief Release the interrupts

 This function releases all interrupt, undoing a
 previous cbmiec_block_irq() call.
*/
VOID
cbmiec_release_irq(PDEVICE_EXTENSION Pdx)
{
    FUNC_ENTER();

    STI();

    KeLowerIrql(Pdx->IecBlockIrqPreviousIrql);

    DBGDO(DBG_ASSERT(InterlockedDecrement(&Pdx->IecBlockIrqUsageCount)==0));

    FUNC_LEAVE();
}
