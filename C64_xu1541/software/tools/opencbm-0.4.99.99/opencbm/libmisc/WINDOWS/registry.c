/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2001-2004, 2007-2009 Spiro Trikaliotis
 *
*/

/*! ************************************************************** 
** \file libmisc/WINDOWS/registry.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Registry manipulation functions
**
****************************************************************/

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#ifndef DBG_PROGNAME
    #define DBG_PROGNAME "OPENCBM-libmisc"
#endif // #ifndef DBG_PROGNAME

#include <windows.h>

#include "debug.h"
#include "cbmioctl.h"

/*! \brief Get a DWORD value from the registry

 This function gets a DWORD value in the registry. It is a simple
 wrapper for convenience.

 \param RegKey
   A handle to an already opened registry key.

 \param SubKey
   Pointer to a null-terminiated string which holds the name
   of the value to be created or changed.

 \param Value
   Pointer to a variable which will contain the value from the registry

 \return
   ERROR_SUCCESS on success, -1 otherwise

 If this function returns -1, the given Value will not be changed at all!
*/

LONG
RegGetDWORD(IN HKEY RegKey, IN char *SubKey, OUT LPDWORD Value)
{
    DWORD valueLen;
    DWORD lpType;
    DWORD value;
    DWORD rc;

    FUNC_ENTER();

    FUNC_PARAM((DBG_PREFIX "Subkey = '%s'", SubKey));

    valueLen = sizeof(value);

    rc = RegQueryValueEx(RegKey, SubKey, NULL, &lpType, (LPBYTE)&value, &valueLen);

    DBG_ASSERT(valueLen == 4);

    if ((rc == ERROR_SUCCESS) && (valueLen == 4))
    {
        DBG_SUCCESS((DBG_PREFIX "RegGetDWORD"));
        *Value = value;
    }
    else
    {
        DBG_ERROR((DBG_PREFIX "RegGetDWORD failed, returning -1"));
        rc = -1;
    }

    FUNC_LEAVE_INT(rc);
}


#if DBG

#include "libmisc.h"

/*! \brief Set the debugging flags

 This function gets the debugging flags from the registry. If there
 are any, it sets the flags to that value.
*/

void
cbm_get_debugging_flags(const char * ModuleName)
{
    DWORD ret;
    HKEY RegKey = 0;
    char * dllDebugFlagsName = NULL;

    FUNC_ENTER();

    do {
        if (ModuleName) {
            dllDebugFlagsName = cbmlibmisc_strcat(CBM_REGKEY_SERVICE_DLL_DEBUGFLAGS "-", ModuleName);
        }
        else {
            dllDebugFlagsName = cbmlibmisc_strdup(CBM_REGKEY_SERVICE_DLL_DEBUGFLAGS);
        }

        // Open a registry key to HKLM\<%REGKEY_SERVICE%>

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         CBM_REGKEY_SERVICE,
                         0,
                         KEY_QUERY_VALUE,
                         &RegKey) )
        {
            DBG_WARN((DBG_PREFIX "RegOpenKeyEx() failed!"));
            break;
        }

        // now, get the number of the port to use

        if (RegGetDWORD(RegKey, CBM_REGKEY_SERVICE_DLL_DEBUGFLAGS, &ret) != ERROR_SUCCESS) {
            DBG_WARN((DBG_PREFIX "No " CBM_REGKEY_SERVICE "\\" CBM_REGKEY_SERVICE_DLL_DEBUGFLAGS
                " value, leaving default."));
        }
        else {
            DbgFlags = ret;
        }

    } while (0);

    // We're done, close the registry handle.

    RegCloseKey(RegKey);

    cbmlibmisc_strfree(dllDebugFlagsName);

    FUNC_LEAVE();
}

#endif // #if DBG
