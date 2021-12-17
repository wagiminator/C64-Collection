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
** \file sys/libiec/wait.c \n
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

/*! \brief Wait for a line to have a specific value

 This function waits until a listener is ready.

 \param Pdx
   Pointer to the device extension.

 \param Line
   Which line has to be monitored (one of IEC_DATA, IEC_CLOCK, IEC_ATN)

 \param State
   Type of wait
   \n =1: Wait until that line is set
   \n =0: Wait until that line is unset

 \param Result
   Pointer to a variable which will hold the value of the IEC bus
*/
NTSTATUS
cbmiec_iec_wait(IN PDEVICE_EXTENSION Pdx, IN UCHAR Line, IN UCHAR State, OUT PUCHAR Result)
{
    NTSTATUS ntStatus;
    UCHAR mask;
    ULONG i;

    FUNC_ENTER();

    FUNC_PARAM((DBG_PREFIX "line = 0x%02x, state = 0x%02x", Line, State));

    // Find the correct mask for the line which has to be tested

    switch (Line)
    {
        case IEC_LINE_DATA:
           mask = PP_DATA_IN;
           break;

        case IEC_LINE_CLOCK:
           mask = PP_CLK_IN;
           break;

        case IEC_LINE_ATN:
           mask = PP_ATN_IN;
           break;

        default:
           FUNC_LEAVE_NTSTATUS_CONST(STATUS_INVALID_PARAMETER);
    }

    // For which state do we have to wait: Set (= mask) or unset

    State = State ? mask : 0;

    i = 0;

    while ((READ_PORT_UCHAR(IN_PORT) & mask) == State)
    {
        if(i >= 20)
        {
            cbmiec_schedule_timeout(libiec_global_timeouts.T_8_IEC_WAIT_LONG_DELAY);

            if (QueueShouldCancelCurrentIrp(&Pdx->IrpQueue))
            {
                FUNC_LEAVE_NTSTATUS_CONST(STATUS_TIMEOUT);
            }
        }
        else
        {
            i++;
            cbmiec_udelay(libiec_global_timeouts.T_8_IEC_WAIT_SHORT_DELAY);
        }
    }

    ntStatus = cbmiec_iec_poll(Pdx, Result);
    FUNC_LEAVE_NTSTATUS(ntStatus);
}
