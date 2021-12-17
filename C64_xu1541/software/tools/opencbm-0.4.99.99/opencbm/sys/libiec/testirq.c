/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2006 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file sys/libiec/testirq.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Test for IRQ capabilities
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "iec.h"
#include "i_iec.h"

/*! \brief Test for IRQ capabilities

 This function tests if the parallel port is able to
 generate interrupts.

 \param Pdx
   Pointer to the device extension of the device to be
   tested.

 \param Buffer
   @@@@@ \todo document

 \param BufferLength
   @@@@@ \todo document
*/

NTSTATUS
cbmiec_test_irq(IN PDEVICE_EXTENSION Pdx, OUT PVOID Buffer, IN ULONG BufferLength)
{
    PCBMT_I_TESTIRQ bufferTestIrq = Buffer;
    NTSTATUS ntStatus;
    LONG ret;
#if DBG
    LONG oldDbgFlags = DbgFlags;
#endif

    FUNC_ENTER();

    if (bufferTestIrq)
    {
        if (sizeof(*bufferTestIrq) > BufferLength)
        {
            // the buffer is not long enough; don't use it at all!
            bufferTestIrq = NULL;
        }
        else
        {
            RtlZeroMemory(bufferTestIrq, BufferLength);
        }
    }

    ntStatus = STATUS_SUCCESS;

    do {
        PUCHAR ecrPort = Pdx->ParPortEcpPortAddress + ECR_OFFSET;
        UCHAR  ecr     = READ_PORT_UCHAR(ecrPort);
        UCHAR  ecp0, ecp1;

#if DBG
//        DbgFlags |= 0x7;
//        DbgFlags |= 0x7fffffff;
#endif

        //
        // Did we get a an interrupt at all? If not, no need
        // to do ANY test!
        //

        if (!Pdx->ParallelPortAllocatedInterrupt)
        {
            ntStatus = STATUS_BIOS_FAILED_TO_CONNECT_INTERRUPT;
            if (bufferTestIrq)
                bufferTestIrq->ErrAcquireIrq = 1;
            break;
        }

        //
        // Now, do the test
        //

        DBG_CABLE((DBG_PREFIX "Release all lines"));
        CBMIEC_RELEASE(PP_ATN_OUT | PP_CLK_OUT | PP_DATA_OUT | PP_RESET_OUT);

        DBG_CABLE((DBG_PREFIX "Pdx->IrqCount = 100"));
        ret = InterlockedExchange(&Pdx->IrqCount, 100);
        DBG_ASSERT(ret==0);

        DBG_CABLE((DBG_PREFIX "Allow Interrupts"));
        CBMIEC_SET(PP_LP_IRQ);

        if (Pdx->ParPortEcpPortAddress)
        {
            ecr  = READ_PORT_UCHAR(ecrPort);

            DBG_CABLE((DBG_PREFIX ""));
            DBG_CABLE((DBG_PREFIX "" __DATE__ " " __TIME__));
            DBG_CABLE((DBG_PREFIX "Setting ECP to configuration mode"));

            WRITE_PORT_UCHAR(Pdx->ParPortEcpPortAddress, ecr | 0xe0);
            ecp0 = READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + 0);
            ecp1 = READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + 1);

            DBG_CABLE((DBG_PREFIX "Addresses: %p = (%02x, %02x, %02x)",
                Pdx->ParPortEcpPortAddress,
                ecp0,
                ecp1,
                READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + 2)));

            DBG_CABLE((DBG_PREFIX "Interrupt bit = %s",
                (READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + 0) & 0x40)
                ? "TRUE"
                : "FALSE"));

            DBG_CABLE((DBG_PREFIX "Interrupt bit = %s",
                (READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + 0) & 0x40)
                ? "TRUE"
                : "FALSE"));

            DBG_CABLE((DBG_PREFIX "Resetting ECP to old mode"));

            WRITE_PORT_UCHAR(Pdx->ParPortEcpPortAddress, ecr);

            DBG_CABLE((DBG_PREFIX "Interrupt bit ECR = %s",
                (READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + ECR_OFFSET) & 0x04)
                ? "TRUE"
                : "FALSE"));

            DBG_CABLE((DBG_PREFIX "Interrupt bit ECR = %s",
                (READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + ECR_OFFSET) & 0x04)
                ? "TRUE"
                : "FALSE"));

            WRITE_PORT_UCHAR(Pdx->ParPortEcpPortAddress + ECR_OFFSET, 
                READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + ECR_OFFSET) & ~0x04);

            DBG_CABLE((DBG_PREFIX "Interrupt bit ECR = %s",
                (READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + ECR_OFFSET) & 0x04)
                ? "TRUE"
                : "FALSE"));

            DBG_CABLE((DBG_PREFIX ""));
            DBG_CABLE((DBG_PREFIX ""));

            DBG_CABLE((DBG_PREFIX "Before: ECR (%p) = %02x",
                Pdx->ParPortEcpPortAddress + ECR_OFFSET, 
                READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + ECR_OFFSET)));

            WRITE_PORT_UCHAR(Pdx->ParPortEcpPortAddress + ECR_OFFSET,
                READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + ECR_OFFSET) | 0x10);

            DBG_CABLE((DBG_PREFIX "After:  ECR (%p) = %02x",
                Pdx->ParPortEcpPortAddress + ECR_OFFSET, 
                READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + ECR_OFFSET)));
        }

        DBG_CABLE((DBG_PREFIX "Set all lines"));
        CBMIEC_SET(PP_ATN_OUT | PP_CLK_OUT | PP_DATA_OUT);

        DBG_CABLE((DBG_PREFIX "Wait 1s"));
        cbmiec_udelay(1000000);

        ret = InterlockedExchange(&Pdx->IrqCount, 100);
        DBG_CABLE((DBG_PREFIX "Pdx->IrqCount = 100, old Value = %u", ret));

        if (ret != 100)
        {
            DBG_ERROR((DBG_PREFIX "Interrupt generated when SETTING"));
            LogErrorOnly(Pdx->Fdo, CBM_IRQ_WHEN_SETTING);

            if (bufferTestIrq)
                bufferTestIrq->ErrIrqRisingEdge = -1;

            /* But: This is NO error, thus, no need for setting ntStatus! */
            /* ntStatus = STATUS_UNSUCCESSFUL; */
        }

        DBG_CABLE((DBG_PREFIX "Release all lines"));
        CBMIEC_RELEASE(PP_ATN_OUT | PP_CLK_OUT | PP_DATA_OUT);

        DBG_CABLE((DBG_PREFIX "Wait 1s"));
        cbmiec_udelay(1000000);

        DBG_CABLE((DBG_PREFIX "Disallow Interrupts"));
        CBMIEC_RELEASE(PP_LP_IRQ);

        if (Pdx->ParPortEcpPortAddress)
        {
            DBG_CABLE((DBG_PREFIX "Before: ECR (%p) = %02x",
                Pdx->ParPortEcpPortAddress + ECR_OFFSET, 
                READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + ECR_OFFSET)));

            WRITE_PORT_UCHAR(Pdx->ParPortEcpPortAddress + ECR_OFFSET,
                READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + ECR_OFFSET) & ~0x10);

            DBG_CABLE((DBG_PREFIX "After:  ECR (%p) = %02x", 
                Pdx->ParPortEcpPortAddress + ECR_OFFSET, 
                READ_PORT_UCHAR(Pdx->ParPortEcpPortAddress + ECR_OFFSET)));
        }

        ret = InterlockedExchange(&Pdx->IrqCount, 0);
        DBG_CABLE((DBG_PREFIX "Pdx->IrqCount = 0, old Value = %u", ret));

        /*
         * Check if an interrupt occurred
         *
         * Note: Check for > 99 (and not !=99) because the 1581 issues 
         *       two IRQs in this short test!
         */

        if (ret > 99)
        {
            DBG_ERROR((DBG_PREFIX "No interrupt generated when RELEASING"));
            ntStatus = STATUS_NO_SUCH_DEVICE;

            if (bufferTestIrq)
                bufferTestIrq->ErrIrqFallingEdge = 1;
        }

    } while (0);

#if DBG
    DbgFlags = oldDbgFlags;
#endif

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
