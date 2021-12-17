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
** \file sys/libiec/i_rawread.c \n
** \author Spiro Trikaliotis \n
** \authors Based on code from
**    Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
** \n
** \brief Read some bytes from the IEC bus, internal version
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

/*! \brief Read some bytes from the IEC bus

 \param Pdx
   Pointer to the device extension.

 \param Buffer
   Pointer to a buffer where the read bytes are written to.

 \param Count
   Maximum number of characters to read from the bus.

 \param Received
   Pointer to the variable which will hold the read bytes.

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.
*/
NTSTATUS
cbmiec_i_raw_read(IN PDEVICE_EXTENSION Pdx, OUT UCHAR *Buffer, ULONG Count, OUT ULONG *Received)
{
    NTSTATUS ntStatus;
    BOOLEAN ok;
    ULONG received;
    ULONG bit;
    ULONG b;
    ULONG i;
    ULONG flags;

    FUNC_ENTER();

    DBG_ASSERT(Received != NULL);

    ok = TRUE;
    received = 0;
    *Received = 0;

    ntStatus = cbmiec_checkcable(Pdx);

    if (!NT_SUCCESS(ntStatus))
    {
        FUNC_LEAVE_NTSTATUS(ntStatus);
    }

    // If there was already an eoi, we are ready

    if (Pdx->Eoi)
    {
        FUNC_LEAVE_NTSTATUS_CONST(STATUS_END_OF_FILE);
    }

    // loop until we have read all bytes, or there was an eoi or an error

    do
    {
        PERF_EVENT_READ_BYTE_NO(received);

        i = 0;
        while (CBMIEC_GET(PP_CLK_IN))
        {
            if (Pdx->IsSMP)
            {
                cbmiec_udelay(libiec_global_timeouts.T_1_RECV_WAIT_CLK_LOW_DATA_READY_GRANU);
            }
            else
            {
                cbmiec_schedule_timeout(libiec_global_timeouts.T_1_RECV_WAIT_CLK_LOW_DATA_READY_GRANU);
            }

            if (QueueShouldCancelCurrentIrp(&Pdx->IrpQueue))
            {
                *Received = received;
                FUNC_LEAVE_NTSTATUS_CONST(STATUS_TIMEOUT);
            }
        }

        // As we need very exact timing, don't allow anyone to disturb us

        cbmiec_block_irq(Pdx);

        // Signal "We're ready for reading" to the other side

        CBMIEC_RELEASE(PP_DATA_OUT);

        // Wait up to 400 us for a reply of the other side (TALKer)

        for(i = 0; (i < libiec_global_timeouts.T_2_Times) && !(ok=CBMIEC_GET(PP_CLK_IN)); i++)
        {
            cbmiec_udelay(libiec_global_timeouts.T_2_RECV_WAIT_CLK_HIGH_T_NE);
        }

        // Did we get an answer?

        if (!ok)
        {
            DBG_IEC((DBG_PREFIX "Got an EOI"));

            // No, so, the other device signals an EOI

            Pdx->Eoi = TRUE;

            // Tell the TALKer we recognized the EOI

            CBMIEC_SET(PP_DATA_OUT);
            cbmiec_udelay(libiec_global_timeouts.T_3_RECV_EOI_RECOGNIZED);
            CBMIEC_RELEASE(PP_DATA_OUT);
        }

        // Now, wait up to 2 ms for the TALKer 
        // If we did an non-EOI answer, this loop will not be executed at all

        for (i = 0; (i < libiec_global_timeouts.T_4_Times) && !(ok=CBMIEC_GET(PP_CLK_IN)); i++)
        {
            DBG_IEC((DBG_PREFIX "Wait after EOI"));
            cbmiec_udelay(libiec_global_timeouts.T_4_RECV_WAIT_CLK_HIGH_AFTER_EOI_GRANU);
        }

#if DBG
        if (!ok)
        {
            DBG_ERROR((DBG_PREFIX "NOT OK after Wait after EOI"));
        }
#endif // #if DBG

        for (bit = b = 0; (bit < 8) && ok; bit++)
        {
            PERF_EVENT_READ_BIT_NO(bit);

            // Wait for CLK to be activated 
            // For this to occur, wait up to 200*T_5 (=2ms)

            for (i = 0; (i < libiec_global_timeouts.T_5_Times) && !(ok=(CBMIEC_GET(PP_CLK_IN)==0)); i++)
            {
                cbmiec_udelay(libiec_global_timeouts.T_5_RECV_BIT_WAIT_CLK_HIGH);
            }

            // Did we get a CLK pulse?

            if (ok)
            {
                // Yes, shift the data to the right

                b >>= 1;

                // If the DATA pin is 0, we got a "1" bit; else, it is a "0"

                if (CBMIEC_GET(PP_DATA_IN) == 0)
                {
                    // Set the highest bit (bits are given in LSB first
                    b |= 0x80;
                }

                // Wait for CLK to be deactivated again
                // For this to occur, wait up to 100 * T_6 (=2ms)

                for (i = 0; i < libiec_global_timeouts.T_6_Times && !(ok=CBMIEC_GET(PP_CLK_IN)); i++)
                {
                    cbmiec_udelay(libiec_global_timeouts.T_6_RECV_BIT_WAIT_CLK_LOW);
                }

#if DBG
                if (!ok)
                {
                    DBG_ERROR((DBG_PREFIX "CLK WAS NOT DEACTIVATED AGAIN"));
                }
#endif // #if DBG
            }
            else
            {
                // An error occurred, it does not make sense to get more bits 
                DBG_ERROR((DBG_PREFIX "BREAKING OUT OF BIT-LOOP, no CLK pulse received. Bit %u, Value = %u", (unsigned int) bit, (unsigned int) b));
                break;
            }
        }

        // If everything went fine, acknowledge the byte

        if (ok)
        {
            // set acknowledgement
            CBMIEC_SET(PP_DATA_OUT);
        }
#if DBG
        else
        {
            DBG_ERROR((DBG_PREFIX "!OK -> no ACK"));
        }
#endif // #if DBG

        // Our timing is not critical anymore, go back to the old IRQL

        cbmiec_release_irq(Pdx);

        // If everything went fine, remember the read byte

        if (ok)
        {
            PERF_EVENT_READ_BYTE(b);

            // One more byte has been received

            received++;

            // Store the received byte into the buffer

            *Buffer++ = (UCHAR) b;

            // Wait 70us between two bytes. Anyway, as we don't
            // want to monopolize the complete CPU, schedule a
            // timeout every 256 bytes.

            if (received % 256)
            {
                cbmiec_udelay(libiec_global_timeouts.T_7_RECV_INTER_BYTE_DELAY);
            }
            else
            {
                cbmiec_schedule_timeout(libiec_global_timeouts.T_7_RECV_INTER_BYTE_DELAY);
            }
        }
    } while(received < Count && ok && !Pdx->Eoi);


    if (ok) 
    {
        DBG_SUCCESS((DBG_PREFIX "received=%d, count=%d, ok=%d, eoi=%d",
            received, Count, ok, Pdx->Eoi));

        cbmiec_setcablestate(Pdx, CABLESTATE_SUCCESSFULLY_USED);
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        DBG_ERROR((DBG_PREFIX "I/O error: received=%d, count=%d, ok=%d, eoi=%d",
            received, Count, ok, Pdx->Eoi));

        cbmiec_setcablestate(Pdx, CABLESTATE_ERROR_OCCURRED);
        Pdx->Eoi = FALSE;
        ntStatus = STATUS_UNEXPECTED_NETWORK_ERROR;
    }

    DBG_ASSERT(Received != NULL);
    *Received = received;

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
