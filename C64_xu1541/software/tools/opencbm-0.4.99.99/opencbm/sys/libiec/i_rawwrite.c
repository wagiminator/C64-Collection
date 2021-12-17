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
** \file sys/libiec/i_rawwrite.c \n
** \author Spiro Trikaliotis \n
** \authors Based on code from
**    Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
** \n
** \brief Write some bytes to the IEC bus, internal version
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

/*! \brief Write some bytes to the IEC bus

 \param Pdx
   Pointer to the device extension.

 \param Buffer
   Pointer to a buffer where the read bytes are written to.

 \param Count
   Maximum number of characters to read from the bus.

 \param Sent
   Pointer to the variable which will hold the number of written bytes.

 \param Atn
   If true: Sent the bytes with set ATN; else, with released ATN

 \param Talk
   If true: A talk command is to be sent (some special care has to 
   be taken at the end of the transmission).

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.

 ATN is released on return of this routine
*/
NTSTATUS
cbmiec_i_raw_write(PDEVICE_EXTENSION Pdx, const UCHAR *Buffer, ULONG Count, ULONG *Sent, BOOLEAN Atn, BOOLEAN Talk)
{
    NTSTATUS ntStatus;
    ULONG sent;
    UCHAR c;
    ULONG i;
    LONG ret;

    FUNC_ENTER();

    ntStatus = cbmiec_checkcable(Pdx);

    if (!NT_SUCCESS(ntStatus))
    {
        FUNC_LEAVE_NTSTATUS(ntStatus);
    }

    sent = 0;

    Pdx->Eoi = FALSE;

    PERF_EVENT_VERBOSE(0x1010, 0);

    ret = InterlockedExchange(&Pdx->IrqCount, 0);
    DBG_ASSERT(ret == 0);

    DBG_IEC((DBG_PREFIX "About to send %d bytes%s", Count, Atn ? " with ATN" : ""));

    PERF_EVENT_VERBOSE(0x1011, 0);

    if (Atn)
    {
        CBMIEC_SET(PP_ATN_OUT);
    }

    // Signal: We have something to send

    CBMIEC_SET(PP_CLK_OUT);
    CBMIEC_RELEASE(PP_DATA_OUT);

    // Wait for DATA to be set by the drive(s)

    PERF_EVENT_VERBOSE(0x1012, 0);

    for(i=0; (i<libiec_global_timeouts.T_9_Times) && !CBMIEC_GET(PP_DATA_IN); i++)
    {
        cbmiec_udelay(libiec_global_timeouts.T_9_SEND_WAIT_DEVICES_T_AT);
    }

    PERF_EVENT_VERBOSE(0x1013, 0);

    // cbmiec_show_state(Pdx,"!GET(PP_DATA_IN)");

    // If DATA was not set, there is no device present

    if(!CBMIEC_GET(PP_DATA_IN))
    {
        PERF_EVENT_VERBOSE(0x1014, 1);
        DBG_ERROR((DBG_PREFIX "no devices found!"));
        CBMIEC_RELEASE(PP_CLK_OUT | PP_ATN_OUT);
        ntStatus = STATUS_NO_SUCH_DEVICE;
    }
    else
    {
        PERF_EVENT_VERBOSE(0x1014, 0);
        if (Pdx->IsSMP)
        {
            cbmiec_udelay(libiec_global_timeouts.T_10_SEND_BEFORE_1ST_BYTE);
        }
        else
        {
            cbmiec_schedule_timeout(libiec_global_timeouts.T_10_SEND_BEFORE_1ST_BYTE);
        }
    }

    PERF_EVENT_VERBOSE(0x1015, 0);

    while(sent < Count && ntStatus == STATUS_SUCCESS)
    {
        c = *Buffer++;

        PERF_EVENT_WRITE_BYTE_NO(sent);
        PERF_EVENT_WRITE_BYTE(c);

        PERF_EVENT_VERBOSE(0x1016, sent);

        cbmiec_udelay(libiec_global_timeouts.T_11_SEND_BEFORE_BYTE_DELAY);

        PERF_EVENT_VERBOSE(0x1017, sent);

        if(CBMIEC_GET(PP_DATA_IN))
        {
            // Wait for the listener
            // We might have to signal an EOI. That is signaled
            // *before* the last byte has been sent.
            // Anyway, if we are sending with ATN asserted, then
            // no EOI signaling is allowed.

            PERF_EVENT_VERBOSE(0x1018, sent);

            cbmiec_wait_for_listener(Pdx, 
                ((sent == (Count-1)) && (Atn == 0)) ? TRUE : FALSE);

            PERF_EVENT_VERBOSE(0x1019, sent);

            if(QueueShouldCancelCurrentIrp(&Pdx->IrpQueue))
            {
                PERF_EVENT_VERBOSE(0x1020, 1);

                ntStatus = STATUS_CANCELLED;
            }
            else
            {
                PERF_EVENT_VERBOSE(0x1020, 0);

                if(cbmiec_send_byte(Pdx,c))
                {
                    sent++;
                    PERF_EVENT_VERBOSE(0x1021, 0);
                    cbmiec_udelay(libiec_global_timeouts.T_12_SEND_AFTER_BYTE_DELAY);
                    PERF_EVENT_VERBOSE(0x1022, 0);
                }
                else
                {
                    PERF_EVENT_VERBOSE(0x1021, 1);
                    DBG_ERROR((DBG_PREFIX "I/O error on cbmiec_send_byte()"));
                    ntStatus = STATUS_NO_SUCH_DEVICE;
                    PERF_EVENT_VERBOSE(0x1022, 1);
                }
            }
        }
        else
        {
            DBG_ERROR((DBG_PREFIX "device not present"));
            ntStatus = STATUS_NO_SUCH_DEVICE;
        }
    }

#if DBG
    if (ntStatus == STATUS_SUCCESS)
    {
        DBG_SUCCESS((DBG_PREFIX "%d bytes sent", sent));
    }
    else
    {
        DBG_ERROR((DBG_PREFIX "%d bytes sent, Status=%s", sent, DebugNtStatus(ntStatus)));
    }
#endif

    if (ntStatus == STATUS_SUCCESS)
    {
        cbmiec_setcablestate(Pdx, CABLESTATE_SUCCESSFULLY_USED);
    }
    else
    {
        cbmiec_setcablestate(Pdx, CABLESTATE_ERROR_OCCURRED);
    }

    if (ntStatus == STATUS_SUCCESS && Talk)
    {
        // Talk-attention turn around (reverse Talker and Listener)

        PERF_EVENT_VERBOSE(0x1030, 0);

        cbmiec_block_irq(Pdx);

        PERF_EVENT_VERBOSE(0x1031, 0);

        CBMIEC_SET(PP_DATA_OUT);
        CBMIEC_RELEASE(PP_ATN_OUT);

        PERF_EVENT_VERBOSE(0x1033, 0);

        CBMIEC_RELEASE(PP_CLK_OUT);

        PERF_EVENT_VERBOSE(0x1032, 0);

        while (!CBMIEC_GET(PP_CLK_IN) && !QueueShouldCancelCurrentIrp(&Pdx->IrpQueue))
              ;

        PERF_EVENT_VERBOSE(0x1034, 0);

        cbmiec_release_irq(Pdx);

        PERF_EVENT_VERBOSE(0x1035, 0);
    }
    else
    {
        CBMIEC_RELEASE(PP_ATN_OUT);
    }
    PERF_EVENT_VERBOSE(0x1036, 0);
    cbmiec_udelay(libiec_global_timeouts.T_14_SEND_AT_END_DELAY);
    PERF_EVENT_VERBOSE(0x1037, 0);

    DBG_ASSERT(Sent != NULL);
    *Sent = sent;

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
