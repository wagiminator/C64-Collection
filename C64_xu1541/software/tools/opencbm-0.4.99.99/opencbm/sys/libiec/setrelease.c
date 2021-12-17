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
** \file sys/libiec/setrelease.c \n
** \author Spiro Trikaliotis \n
** \authors Based on code from
**    Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
** \n
** \brief Set a specific line on the IEC bus
**
****************************************************************/

#include <wdm.h>
#include "cbm_driver.h"
#include "i_iec.h"

/*! \brief set and release line

  This macro only makes sense in the context of cbmiec_iec_setrelease().
*/
#define SET_RELEASE_LINE(_LineName, _PPName) \
        if (Set     & IEC_LINE_##_LineName) { set_mask     |= PP_##_PPName##_OUT; } \
        if (Release & IEC_LINE_##_LineName) { release_mask |= PP_##_PPName##_OUT; }

/*! \brief Activate and deactive a line on the IEC serial bus

 This function activates (sets to 0V, L) and deactivates 
 (set to 5V, H) lines on the IEC serial bus.

 \param Pdx
   Pointer to the device extension.

 \param Set
   The mask of which lines should be set. This has to be a bitwise OR
   between the constants IEC_DATA, IEC_CLOCK, IEC_ATN, and IEC_RESET

 \param Release
   The mask of which lines should be released. This has to be a bitwise
   OR between the constants IEC_DATA, IEC_CLOCK, IEC_ATN, and IEC_RESET

 \return 
   If the routine succeeds, it returns STATUS_SUCCESS. Otherwise, it
   returns one of the error status values.

 \remark
   If a bit is specified in the Set as well as in the Release mask, the
   effect is undefined.
*/
NTSTATUS
cbmiec_iec_setrelease(IN PDEVICE_EXTENSION Pdx, IN USHORT Set, IN USHORT Release)
{
    NTSTATUS ntStatus;

    FUNC_ENTER();

    FUNC_PARAM((DBG_PREFIX "set = 0x%02x, release = 0x%02x", Set, Release));

    ntStatus = STATUS_SUCCESS;

    DBG_ASSERT((Set & Release) == 0);

    // Set the correct line as given by the call

    if ( (Set & ~(IEC_LINE_DATA | IEC_LINE_CLOCK | IEC_LINE_ATN | IEC_LINE_RESET))
        || (Release & ~(IEC_LINE_DATA | IEC_LINE_CLOCK | IEC_LINE_ATN | IEC_LINE_RESET)))
    {
        // there was some bit set that is not recognized, return
        // with an error
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    else
    {
        ULONG set_mask = 0;
        ULONG release_mask = 0;

        SET_RELEASE_LINE(DATA,  DATA);
        SET_RELEASE_LINE(CLOCK, CLK);
        SET_RELEASE_LINE(ATN,   ATN);
        SET_RELEASE_LINE(RESET, RESET);

#ifdef TEST_BIDIR

        #define PP_BIDIR_OUT   PP_LP_BIDIR
        #define IEC_LINE_BIDIR PP_BIDIR_OUT

        SET_RELEASE_LINE(BIDIR, BIDIR);

        #undef PP_BIDIR_OUT
        #undef IEC_LINE_BIDIR

#endif // #ifdef TEST_BIDIR

        CBMIEC_SET_RELEASE(set_mask, release_mask);

    }

    FUNC_LEAVE_NTSTATUS(ntStatus );
}
