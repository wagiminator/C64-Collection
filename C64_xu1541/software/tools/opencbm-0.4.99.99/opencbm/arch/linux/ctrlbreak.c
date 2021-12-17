/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2006 Spiro Trikaliotis
 *
*/

/*! ************************************************************** 
** \file arch/linux/ctrlbreak.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Helper function for setting a handler for Ctrl+C 
** and Ctrl+Break
**
****************************************************************/

#include "arch.h"

#include <signal.h>

void
arch_set_ctrlbreak_handler(ARCH_CTRLBREAK_HANDLER Handler)
{
    signal(SIGINT, Handler);
}
