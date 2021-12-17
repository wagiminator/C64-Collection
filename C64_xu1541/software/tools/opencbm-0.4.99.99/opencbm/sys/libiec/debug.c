/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2001-2004 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file sys/libiec/debug.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Debug helper functions for libiec
**
****************************************************************/

#if DBG

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

/* Since we need the originals of these functions here, undefine them! */
#undef READ_PORT_UCHAR
#undef WRITE_PORT_UCHAR


static PUCHAR parportBase = (PUCHAR) 0x378;


/*! \brief Show the contents of the parallel port's registers

 \param Str
   Caller-supplied pointer to a string to output before the values.
*/
VOID
cbmiec_show_port(IN UCHAR *Str)
{
    FUNC_DEF

    DBG_ASSERT(Str != 0);

    DBG_PORT((DBG_PREFIX 
        "%s: 0x%p = 0x%02x, 0x%p = 0x%02x, 0x%p = 0x%02x",
        Str,
        parportBase,   READ_PORT_UCHAR(parportBase),
        parportBase+1, READ_PORT_UCHAR(parportBase+1),
        parportBase+2, READ_PORT_UCHAR(parportBase+2)
        ));
}

/*! \brief Write to a port address with debugging output

 \param Port
   The port address to be written.
 
 \param Value
   The value to be written.
*/
VOID
DbgWp(IN PUCHAR Port, IN UCHAR Value)
{
    FUNC_DEF 

    DBG_PORT((DBG_PREFIX "WRITE_PORT_UCHAR(0x%p, 0x%02x)", Port, (int)Value));

    cbmiec_show_port("Before Writing");

    WRITE_PORT_UCHAR(Port, Value);

    cbmiec_show_port("After Writing ");
}

/*! \brief Read from a port address with debugging output

 \param Port
   The port address to be written.
 
 \return
   The read value.
*/
UCHAR
DbgRp(IN PUCHAR Port)
{
    UCHAR Value;

    FUNC_DEF

    cbmiec_show_port("Before Reading");

    Value = READ_PORT_UCHAR(Port);

    DBG_PORT((DBG_PREFIX "READ_PORT_UCHAR(0x%p) = 0x%02x", Port, (int)Value));

    cbmiec_show_port("After Reading ");

    return Value;
}

/*! \brief Dump the input lines

 \param Pdx
   Pointer to the device extension of the driver.

 \param Str
   Caller-supplied pointer to a string to output before the values.
*/
VOID
cbmiec_show_state(IN PDEVICE_EXTENSION Pdx, IN UCHAR *Str)
{
    FUNC_DEF

    DBG_ASSERT(Pdx != 0);
    DBG_ASSERT(Str != 0);

    DBG_SUCCESS((DBG_PREFIX "%s: data=%d, clk=%d, atn=%d", Str, 
        CBMIEC_GET(PP_DATA_IN), CBMIEC_GET(PP_CLK_IN), CBMIEC_GET(PP_ATN_IN)));
}

#endif /* #if DBG */
