/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2005 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file sys/libwin/processor.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Functions for determining processor number
**
****************************************************************/

#include <ntddk.h>
#include "cbm_driver.h"

/*! \brief Wrapper for KeGetCurrentProcessorNumber()

 See KeGetCurrentProcessorNumber()
 
 This function is needed as KeGetCurrentProcessorNumber() is
 only defined in NTDDK.H, not in WDM.H. Anyway, for debugging
 purposes, we need to access it from anywhere..
*/
ULONG
CbmGetCurrentProcessorNumber(VOID)
{
#ifdef COMPILE_W98_API
    return 0;
#else
    return KeGetCurrentProcessorNumber();
#endif
}

/*! \brief Get the number of processors in the system
 
 This function returns the count of processors available in the
 current system.

 \return
    The number of processors in the system

 \remark
    This function is necessary to account between differences
    between the W2K API and later APIs.
    Additionally, on Windows 95/98/Me, there is no variable
    available that returns this info. On the other hand, on
    Win 95/98/Me, there is no chance for more than one processor,
    as these systems do not support more than one.
*/
ULONG
CbmGetNumberProcessors(VOID)
{
#ifdef COMPILE_W98_API

    return 1;

#elif COMPILE_W2K_API

    return *KeNumberProcessors;

#else

    return KeNumberProcessors;

#endif
}
