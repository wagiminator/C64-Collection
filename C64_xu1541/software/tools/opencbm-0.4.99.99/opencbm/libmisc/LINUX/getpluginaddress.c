/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2005 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2005,2007 Spiro Trikaliotis
 *
*/

/*! ************************************************************** 
** \file libmisc/LINUX/getpluginaddress.c \n
** \author Michael Klein, Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver
**
****************************************************************/

#include "getpluginaddress.h"

#include <stdio.h>
#include <dlfcn.h>

/*! \brief @@@@@ \todo document

 \param name

 \return
*/
SHARED_OBJECT_HANDLE 
plugin_load(const char * name)
{
    void * ret = NULL;
    char * error;

    if (name != NULL) {
       ret = dlopen(name, RTLD_NOW);
       error = dlerror();
       if (error){
           fprintf(stderr, "%s\n", error);
       }
    }
    return ret;
}

/*! \brief @@@@@ \todo document

 \param handle

 \param name

 \return
*/
void *
plugin_get_address(SHARED_OBJECT_HANDLE handle, const char * name)
{
    return dlsym(handle, name);
}

/*! \brief @@@@@ \todo document

 \param handle

 \return
*/
void
plugin_unload(SHARED_OBJECT_HANDLE handle)
{
    dlclose(handle);
}
