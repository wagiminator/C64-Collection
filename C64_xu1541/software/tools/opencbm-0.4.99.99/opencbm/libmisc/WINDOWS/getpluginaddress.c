/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2005 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2005,2007,2012 Spiro Trikaliotis
 *
*/

/*! ************************************************************** 
** \file libmisc/WINDOWS/getpluginaddress.c \n
** \author Michael Klein, Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver
**
****************************************************************/

#include "getpluginaddress.h"

#include "libmisc.h"

#include <windows.h>
#include <stdio.h>

/*! \brief @@@@@ \todo document

 \param name

 \return
*/
SHARED_OBJECT_HANDLE
plugin_load(const char * name)
{
    SHARED_OBJECT_HANDLE ModuleHandle = LoadLibrary(name);

    if (ModuleHandle == NULL) {
        DWORD Error = GetLastError();

        char * ErrorMessage = cbmlibmisc_format_error_message(Error);

        fprintf(stderr, "Error loading plugin '%s': %s (%u)\n", name, ErrorMessage, Error);
    }

    return ModuleHandle;
}

/*! \brief @@@@@ \todo document

 \param handle

 \param name

 \return
*/
void *
plugin_get_address(SHARED_OBJECT_HANDLE handle, const char * name)
{
    return GetProcAddress(handle, name);
}

/*! \brief @@@@@ \todo document

 \param handle

 \return
*/
void
plugin_unload(SHARED_OBJECT_HANDLE handle)
{
    FreeLibrary(handle);
}
