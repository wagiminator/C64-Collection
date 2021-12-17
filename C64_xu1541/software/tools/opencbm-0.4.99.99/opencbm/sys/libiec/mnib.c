/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2000-2005 Markus Brenner
 *  Copyright 2000-2005 Pete Rittwage
 *  Copyright 2005      Tim Schürmann
 *  Copyright 2005,2009 Spiro Trikaliotis
 *  Copyright 2009      Arnd M.
 *
 */

/*! ************************************************************** 
** \file sys/libiec/mnib.c \n
** \author Tim Schürmann, Spiro Trikaliotis, Arnd M. \n
** \authors Based on code from
**    Markus Brenner
** \n
** \brief Nibble a complete track
**
****************************************************************/

#define TO_HANDSHAKED_READ  libiec_global_timeouts.T_PARALLEL_BURST_READ_BYTE_HANDSHAKED  /*!< timeout value for handshaked read */
#define TO_HANDSHAKED_WRITE libiec_global_timeouts.T_PARALLEL_BURST_WRITE_BYTE_HANDSHAKED /*!< timeout value for handshaked write */

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

#define PERF_EVENT_PARBURST_PAR_READ_ENTER()           PERF_EVENT(0x5000, 0)      /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_PAR_READ_DELAY1(_x_)       PERF_EVENT(0x5001, _x_)    /*!< Mark performance event: @@@@@ */ 
#define PERF_EVENT_PARBURST_PAR_READ_PP_READ()         PERF_EVENT(0x5002, 0)      /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_PAR_READ_RELEASED(_x_)     PERF_EVENT(0x5003, _x_)    /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_PAR_READ_DELAY2(_x_)       PERF_EVENT(0x5004, _x_)    /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_PAR_READ_TIMEOUT(_x_)      PERF_EVENT(0x5005, _x_)    /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_PAR_READ_EXIT(_x_)         PERF_EVENT(0x5006, _x_)    /*!< Mark performance event: @@@@@ */

#define PERF_EVENT_PARBURST_PAR_WRITE_ENTER()          PERF_EVENT(0x5100, 0)      /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_PAR_WRITE_DELAY1(_x_)      PERF_EVENT(0x5101, _x_)    /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_PAR_WRITE_PP_WRITE(_x_)    PERF_EVENT(0x5102, _x_)    /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_PAR_WRITE_RELEASE()        PERF_EVENT(0x5103, 0)      /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_PAR_WRITE_DELAY2(_x_)      PERF_EVENT(0x5104, _x_)    /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_PAR_WRITE_DUMMY_READ(_x_)  PERF_EVENT(0x5105, _x_)    /*!< Mark performance event: @@@@@ */

#define PERF_EVENT_PARBURST_SEND_CMD(_x_)              PERF_EVENT(0x5200, _x_)    /*!< Mark performance event: @@@@@ */

#define PERF_EVENT_PARBURST_NREAD_RELEASE()            PERF_EVENT(0x5300, 0)      /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_NREAD_AFTERDELAY()         PERF_EVENT(0x5301, 0)      /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_NREAD_EXIT(_x_)            PERF_EVENT(0x5302, _x_)    /*!< Mark performance event: @@@@@ */

#define PERF_EVENT_PARBURST_NWRITE_RELEASE()           PERF_EVENT(0x5400, 0)      /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_NWRITE_VALUE(_x_)          PERF_EVENT(0x5401, _x_)    /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_NWRITE_EXIT(_x_)           PERF_EVENT(0x5402, _x_)    /*!< Mark performance event: @@@@@ */

#define PERF_EVENT_PARBURST_READ_TRACK_ENTER()         PERF_EVENT(0x5500, 0)      /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_READ_TRACK_STARTLOOP()     PERF_EVENT(0x5500, 0)      /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_READ_TRACK_VALUE(_x_)      PERF_EVENT(0x5500, _x_)    /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_READ_TRACK_TIMEOUT(_x_)    PERF_EVENT(0x5500, _x_)    /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_READ_TRACK_READ_DUMMY(_x_) PERF_EVENT(0x5500, _x_)    /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_READ_TRACK_EXIT(_x_)       PERF_EVENT(0x5500, _x_)    /*!< Mark performance event: @@@@@ */

#define PERF_EVENT_PARBURST_WRITE_TRACK_ENTER()        PERF_EVENT(0x5500, 0)      /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_WRITE_TRACK_STARTLOOP()    PERF_EVENT(0x5500, 0)      /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_WRITE_TRACK_VALUE(_x_)     PERF_EVENT(0x5500, _x_)    /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_WRITE_TRACK_TIMEOUT(_x_)   PERF_EVENT(0x5500, _x_)    /*!< Mark performance event: @@@@@ */
#define PERF_EVENT_PARBURST_WRITE_TRACK_EXIT(_x_)      PERF_EVENT(0x5500, _x_)    /*!< Mark performance event: @@@@@ */

/*! \brief @@@@@ \todo document

 \param Pdx

 \param Byte

 \return
*/
NTSTATUS
cbmiec_parallel_burst_read(IN PDEVICE_EXTENSION Pdx, OUT UCHAR* Byte)
{
    FUNC_ENTER();

    PERF_EVENT_PARBURST_PAR_READ_ENTER();

    CBMIEC_RELEASE(PP_DATA_OUT|PP_CLK_OUT);
    CBMIEC_SET(PP_ATN_OUT);

    PERF_EVENT_PARBURST_PAR_READ_DELAY1(0);
    cbmiec_udelay(200);
    PERF_EVENT_PARBURST_PAR_READ_DELAY1(1);

    while(CBMIEC_GET(PP_DATA_IN))
    {
        if (QueueShouldCancelCurrentIrp(&Pdx->IrpQueue))
        {
            PERF_EVENT_PARBURST_PAR_READ_TIMEOUT(0);
            FUNC_LEAVE_NTSTATUS_CONST(STATUS_TIMEOUT);
        }
    }

    PERF_EVENT_PARBURST_PAR_READ_PP_READ();
    cbmiec_pp_read(Pdx, Byte);

    cbmiec_udelay(5);
    PERF_EVENT_PARBURST_PAR_READ_RELEASED(0);
    CBMIEC_RELEASE(PP_ATN_OUT);
    PERF_EVENT_PARBURST_PAR_READ_RELEASED(1);

    PERF_EVENT_PARBURST_PAR_READ_DELAY2(0);
    cbmiec_udelay(10);
    PERF_EVENT_PARBURST_PAR_READ_DELAY2(1);
    while(!CBMIEC_GET(PP_DATA_IN))
    {
        if (QueueShouldCancelCurrentIrp(&Pdx->IrpQueue))
        {
            PERF_EVENT_PARBURST_PAR_READ_TIMEOUT(1);
            FUNC_LEAVE_NTSTATUS_CONST(STATUS_TIMEOUT);
        }
    }

    PERF_EVENT_PARBURST_PAR_READ_EXIT(*Byte);
    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}

/*! \brief @@@@@ \todo document

 \param Pdx

 \param Byte

 \return
*/
NTSTATUS
cbmiec_parallel_burst_write(IN PDEVICE_EXTENSION Pdx, IN UCHAR Byte)
{
    UCHAR dummy;
    int j,to;

    FUNC_ENTER();

    PERF_EVENT_PARBURST_PAR_WRITE_ENTER();

    CBMIEC_RELEASE(PP_DATA_OUT|PP_CLK_OUT);
    CBMIEC_SET(PP_ATN_OUT);
    PERF_EVENT_PARBURST_PAR_WRITE_DELAY1(0);
    cbmiec_udelay(200);
    PERF_EVENT_PARBURST_PAR_WRITE_DELAY1(1);

    while(CBMIEC_GET(PP_DATA_IN))
    {
        if (QueueShouldCancelCurrentIrp(&Pdx->IrpQueue))
        {
            FUNC_LEAVE_NTSTATUS_CONST(STATUS_TIMEOUT);
        }
    }

    PERF_EVENT_PARBURST_PAR_WRITE_PP_WRITE(Byte);
    cbmiec_pp_write(Pdx, Byte);

    cbmiec_udelay(5);
    CBMIEC_RELEASE(PP_ATN_OUT);
    PERF_EVENT_PARBURST_PAR_WRITE_DELAY2(0);
    cbmiec_udelay(20);
    PERF_EVENT_PARBURST_PAR_WRITE_DELAY2(1);

    while(!CBMIEC_GET(PP_DATA_IN))
    {
        if (QueueShouldCancelCurrentIrp(&Pdx->IrpQueue))
        {
            FUNC_LEAVE_NTSTATUS_CONST(STATUS_TIMEOUT);
        }
    }

    PERF_EVENT_PARBURST_PAR_WRITE_DUMMY_READ(0);
    cbmiec_pp_read(Pdx, &dummy);
    PERF_EVENT_PARBURST_PAR_WRITE_DUMMY_READ(dummy);

    FUNC_LEAVE_NTSTATUS_CONST(STATUS_SUCCESS);
}

/*! \internal \brief @@@@@ \todo document

 \param Pdx

 \param Toggle

 \return
*/
static int
cbm_handshaked_read(PDEVICE_EXTENSION Pdx, int Toggle)
{
    unsigned int to = 0;
    int j;
    int returnValue;

    FUNC_ENTER();

    PERF_EVENT_PARBURST_NREAD_RELEASE();
    CBMIEC_RELEASE(PP_DATA_OUT); // @@@ DATA_IN ???

    cbmiec_udelay(2); 
    PERF_EVENT_PARBURST_NREAD_AFTERDELAY();

    if (!Toggle)
    {
        while (CBMIEC_GET(PP_DATA_IN))
        {
            if (to++ > TO_HANDSHAKED_READ)
                break;
        }
    }
    else
    {
        while (!CBMIEC_GET(PP_DATA_IN))
        {
            if (to++ > TO_HANDSHAKED_READ)
                break;
        }
    }

    PERF_EVENT_PARBURST_NREAD_EXIT(to > TO_HANDSHAKED_READ ? -1 : 0);

    if (to > TO_HANDSHAKED_READ)
    {
        returnValue = -1;
    }
    else
    {
        returnValue = cbmiec_i_pp_read_debounced(Pdx);
    }

    return returnValue;
}

/*! \internal \brief @@@@@ \todo document

 \param Pdx

 \param Data

 \param Toggle

 \return
*/
static int
cbm_handshaked_write(PDEVICE_EXTENSION Pdx, char Data, int Toggle)
{
    unsigned int to=0;
    int retval;

    FUNC_ENTER();

    PERF_EVENT_PARBURST_NWRITE_RELEASE();
    CBMIEC_RELEASE(PP_CLK_IN);

    do 
    {
        retval = 1;

        if (!Toggle)
        {
            while (CBMIEC_GET(PP_DATA_IN)) // @@@ CLK_IN ???
            {
                if (to++ > TO_HANDSHAKED_WRITE)
                    break;
            }
        }
        else
        {
            while (!CBMIEC_GET(PP_DATA_IN)) // @@@ CLK_IN ???
            {
                if (to++ > TO_HANDSHAKED_WRITE)
                    break;
            }
        }

        if (to++ <= TO_HANDSHAKED_WRITE)
        {
            PERF_EVENT_PARBURST_NWRITE_VALUE(Data);
            cbmiec_pp_write(Pdx, Data);
            retval = 0; // @@@ retval = 1 ???
        }

    } while (0);

    PERF_EVENT_PARBURST_NWRITE_EXIT(retval);
    FUNC_LEAVE_INT(retval);
}

#define enable()  cbmiec_release_irq(Pdx)    /*!< enable interrupts for transfer */
#define disable() cbmiec_block_irq(Pdx)      /*!< disable interrupts for transfer */

/*! \brief @@@@@ \todo document

 \param Pdx

 \param Buffer

 \param ReturnLength

 \return
*/
NTSTATUS
cbmiec_parallel_burst_read_track(IN PDEVICE_EXTENSION Pdx, OUT UCHAR* Buffer, IN ULONG ReturnLength)
{
    NTSTATUS ntStatus;
    ULONG i;
    UCHAR dummy;

    int timeout = 0;
    int byte;

    FUNC_ENTER();

    PERF_EVENT_PARBURST_READ_TRACK_ENTER();

    disable();

    PERF_EVENT_PARBURST_READ_TRACK_STARTLOOP();

    for (i = 0; i < ReturnLength; i ++)
    {
        byte = cbm_handshaked_read(Pdx, i&1);
        PERF_EVENT_PARBURST_READ_TRACK_VALUE(byte);
        if (byte == -1)
        {
            PERF_EVENT_PARBURST_READ_TRACK_TIMEOUT(0);
            timeout = 1;
            break;
        }
        Buffer[i] = (UCHAR) byte;

        if (QueueShouldCancelCurrentIrp(&Pdx->IrpQueue))
        {
            PERF_EVENT_PARBURST_READ_TRACK_TIMEOUT(1);
            timeout = 1; // FUNC_LEAVE_NTSTATUS_CONST(STATUS_TIMEOUT);
        }
    }

    if(!timeout)
    {
        cbmiec_parallel_burst_read(Pdx, &dummy);
        PERF_EVENT_PARBURST_READ_TRACK_READ_DUMMY(dummy);
        enable();
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        enable();
        DBG_PRINT((DBG_PREFIX "timeout failure! Wanted to read %u, but only read %u",
            ReturnLength, i));
        ntStatus = STATUS_DATA_ERROR;
    }

    PERF_EVENT_PARBURST_READ_TRACK_EXIT(ntStatus);

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

/*! \brief @@@@@ \todo document

 \param Pdx

 \param Buffer

 \param BufferLength

 \return
*/
NTSTATUS
cbmiec_parallel_burst_read_track_var(IN PDEVICE_EXTENSION Pdx, OUT UCHAR* Buffer, IN ULONG ReturnLength)
{
    NTSTATUS ntStatus;
    ULONG i;
    UCHAR dummy;

    int timeout = 0;
    int byte;

    FUNC_ENTER();

    PERF_EVENT_PARBURST_READ_TRACK_ENTER();

    disable();

    PERF_EVENT_PARBURST_READ_TRACK_STARTLOOP();

    for (i = 0; i < ReturnLength; i ++)
    {
        byte = cbm_handshaked_read(Pdx, i&1);
        PERF_EVENT_PARBURST_READ_TRACK_VALUE(byte);
        if (byte == -1)
        {
            PERF_EVENT_PARBURST_READ_TRACK_TIMEOUT(0);
            timeout = 1;
            break;
        }
        Buffer[i] = (UCHAR) byte;
        if (byte == 0x55) break;

        if (QueueShouldCancelCurrentIrp(&Pdx->IrpQueue))
        {
            PERF_EVENT_PARBURST_READ_TRACK_TIMEOUT(1);
            timeout = 1; // FUNC_LEAVE_NTSTATUS_CONST(STATUS_TIMEOUT);
        }
    }

    if(!timeout)
    {
        cbmiec_parallel_burst_read(Pdx, &dummy);
        PERF_EVENT_PARBURST_READ_TRACK_READ_DUMMY(dummy);
        enable();
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        enable();
        DBG_PRINT((DBG_PREFIX "timeout failure! Wanted to read %u, but only read %u",
            ReturnLength, i));
        ntStatus = STATUS_DATA_ERROR;
    }

    PERF_EVENT_PARBURST_READ_TRACK_EXIT(ntStatus);

    FUNC_LEAVE_NTSTATUS(ntStatus);
}

NTSTATUS
cbmiec_parallel_burst_write_track(IN PDEVICE_EXTENSION Pdx, IN UCHAR* Buffer, IN ULONG BufferLength)
{
    NTSTATUS ntStatus;
    UCHAR dummy;

    ULONG i = 0;

    int timeout = 0;

    FUNC_ENTER();

    PERF_EVENT_PARBURST_WRITE_TRACK_ENTER();

    disable();

    PERF_EVENT_PARBURST_WRITE_TRACK_STARTLOOP();

    for (i = 0; i < BufferLength; i++)
    {
        PERF_EVENT_PARBURST_WRITE_TRACK_VALUE(Buffer[i]);
        if(cbm_handshaked_write(Pdx, Buffer[i], i&1))
        {
            PERF_EVENT_PARBURST_WRITE_TRACK_TIMEOUT(0);
            timeout = 1;
            break;
        }

        if (QueueShouldCancelCurrentIrp(&Pdx->IrpQueue))
        {
            PERF_EVENT_PARBURST_WRITE_TRACK_TIMEOUT(1);
            timeout = 1; // FUNC_LEAVE_NTSTATUS_CONST(STATUS_TIMEOUT);
        }
    }

    if(!timeout)
    {
        cbm_handshaked_write(Pdx, 0, i&1);
        cbmiec_parallel_burst_read(Pdx, &dummy);
        enable();
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        enable();
        DBG_PRINT((DBG_PREFIX "timeout failure! Wanted to write %u, but only wrote %u",
            BufferLength, i));
        ntStatus = STATUS_DATA_ERROR;
    }

    PERF_EVENT_PARBURST_WRITE_TRACK_EXIT(ntStatus);

    FUNC_LEAVE_NTSTATUS(ntStatus);
}
