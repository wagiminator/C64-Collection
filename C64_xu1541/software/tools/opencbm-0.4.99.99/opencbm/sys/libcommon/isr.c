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
** \file sys/libcommon/isr.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief The Interrupt Service Routine (ISR) for the parallel port
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "iec.h"

/*! \brief Interrupt Service Routine (ISR)

 This is the Interrupt Service Routine for the parallel port.

 \param Interrupt:
   Pointer to the interrupt object.

 \param Pdx:
   The device extension 

 \return 
   If we are responsible for this interrupt, we return TRUE.
   If not, we return FALSE and let Windows try other ISRs (if there are any).
*/
BOOLEAN
cbm_isr(IN PKINTERRUPT Interrupt, IN PVOID Pdx)
{
    // Make sure we do not do any debugging outputs
    // For this, this function ommits the FUNC_ENTER()
    // FUNC_LEAVE() sandwich and makes sure the debugging
    // flags are unset when calling the ISR routine.

    // This is needed as we cannot guard the debugging memory
    // functions against anything running at IRQL > DISPATCH_LEVEL.

    BOOLEAN result;

#if DBG

    ULONG dbgFlagsOld;

    // Make sure we do not try to write into the debugging memory
    // as long as we are executing in the ISR

    // Remember the old DbgFlags

    dbgFlagsOld = DbgFlags;

    // Now, unset the flag which tells the system to write into
    // the debugging memory

    DbgFlags &= ~DBGF_DBGMEMBUF;

#endif // #if DBG

    // let the libiec library do the hard work

    PERF_EVENT_VERBOSE(0x2000, 0);
    result = cbmiec_interrupt(Pdx);
    PERF_EVENT_VERBOSE(0x2001, 0);

#if DBG

    // Restore the debugging flags

    DbgFlags = dbgFlagsOld;

#endif // #if DBG

    return result;
}
