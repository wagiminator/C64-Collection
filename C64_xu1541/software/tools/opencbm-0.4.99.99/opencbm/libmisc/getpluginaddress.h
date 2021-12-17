/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2007 Spiro Trikaliotis
 *
*/

/*! **************************************************************
** \file libmisc/getpluginaddress.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver
**        Functions for obtaining the addresses of plugin functions
**
****************************************************************/

#ifndef OPENCBM_LIBMISC_GETPLUGINADDRESS_H
#define OPENCBM_LIBMISC_GETPLUGINADDRESS_H

#ifdef WIN32

  #include <windows.h>

  /*! \brief @@@@@ \todo document */
  typedef HINSTANCE SHARED_OBJECT_HANDLE;

  /*! \brief @@@@@ \todo document */
  #define SHARED_OBJECT_HANDLE_INVALID NULL

#else

  /*! \brief @@@@@ \todo document */
  typedef void * SHARED_OBJECT_HANDLE;

  /*! \brief @@@@@ \todo document */
  #define SHARED_OBJECT_HANDLE_INVALID NULL

#endif

extern SHARED_OBJECT_HANDLE plugin_load(const char * name);

extern void * plugin_get_address(SHARED_OBJECT_HANDLE handle, const char * name);

extern void plugin_unload(SHARED_OBJECT_HANDLE handle);

#endif /* #ifndef OPENCBM_LIBMISC_GETPLUGINADDRESS_H */
