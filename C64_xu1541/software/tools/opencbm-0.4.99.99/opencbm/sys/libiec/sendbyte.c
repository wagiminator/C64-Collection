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
** \file sys/libiec/sendbyte.c \n
** \author Spiro Trikaliotis \n
** \authors Based on code from
**    Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
** \n
** \brief Write one byte to the IEC bus
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

/*! \brief Write one byte to the IEC bus

 \param Pdx
   Pointer to the device extension.

 \param Byte
   The byte to be output

 \return 
   If the routine succeeds - that is, the listener acknowledged 
   the byte -, it returns TRUE. Otherwise, it returns FALSE.
*/
BOOLEAN
cbmiec_send_byte(IN PDEVICE_EXTENSION Pdx, IN UCHAR Byte)
{
    BOOLEAN ack;
    ULONG i;

    FUNC_ENTER();

    PERF_EVENT_VERBOSE(0x1050, 0);

    DBG_SUCCESS((DBG_PREFIX "send_byte %02x", Byte));
    
    PERF_EVENT_VERBOSE(0x1051, 0);

    ack = FALSE;

    PERF_EVENT_VERBOSE(0x1052, 0);

    cbmiec_block_irq(Pdx);

    PERF_EVENT_VERBOSE(0x1053, 0);

    for(i = 0; i<8; i++)
    {
        PERF_EVENT_WRITE_BIT_NO(i);

        PERF_EVENT_VERBOSE(0x1054, 0);

        cbmiec_udelay(libiec_global_timeouts.T_15_SEND_BEFORE_BIT_DELAY_T_S);

        PERF_EVENT_VERBOSE(0x1055, 0);

        if(!((Byte>>i) & 1))
        {
            CBMIEC_SET(PP_DATA_OUT);
        }
        CBMIEC_RELEASE(PP_CLK_OUT);
        PERF_EVENT_VERBOSE(0x1056, 0);
        cbmiec_udelay(libiec_global_timeouts.T_16_SEND_BIT_TIME_T_V);
        PERF_EVENT_VERBOSE(0x1057, 0);
        CBMIEC_SET_RELEASE(PP_CLK_OUT, PP_DATA_OUT);
    }

    PERF_EVENT_VERBOSE(0x1058, 0);
    cbmiec_release_irq(Pdx);
    PERF_EVENT_VERBOSE(0x1059, 0);

    for(i=0; (i<libiec_global_timeouts.T_17_Times) && !(ack=CBMIEC_GET(PP_DATA_IN)); i++)
    {
        PERF_EVENT_VERBOSE(0x105A, 0);
        cbmiec_udelay(libiec_global_timeouts.T_17_SEND_FRAME_HANDSHAKE_T_F);
    }
    PERF_EVENT_VERBOSE(0x105B, 0);

    DBG_SUCCESS((DBG_PREFIX "ack=%s", ack ? "TRUE" : "FALSE" ));

    FUNC_LEAVE_BOOLEAN(ack);
}
