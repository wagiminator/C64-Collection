/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004, 2008 Spiro Trikaliotis
 *
 */

/*! **************************************************************
** \file instcbm.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Header for installation routines
**
****************************************************************/

#ifndef INSTCBM_H
#define INSTCBM_H

/*! @@@@@ \todo document */
typedef struct cbm_install_parameter_s cbm_install_parameter_t;
/*! @@@@@ \todo document */
typedef struct CbmPluginInstallProcessCommandlineData_s CbmPluginInstallProcessCommandlineData_t;
/*! @@@@@ \todo document */
typedef struct cbm_install_parameter_plugin_s cbm_install_parameter_plugin_t;

extern BOOL get_all_plugins(cbm_install_parameter_t * InstallParameter);
extern BOOL get_all_installed_plugins(cbm_install_parameter_t * InstallParameter);
extern BOOL ProcessPluginCommandlineAndAddIt(cbm_install_parameter_t * Parameter, const char * const Plugin, int Argc, char * const Argv[]);

extern void PluginListFree(cbm_install_parameter_t * InstallParameter);

/*! @@@@@ \todo document

 \param PluginInstallParameter

 \param Context

 \return
*/
typedef BOOL PluginForAll_Callback_t(cbm_install_parameter_plugin_t * PluginInstallParameter, void * Context);

extern BOOL PluginForAll(cbm_install_parameter_t * InstallParameter, PluginForAll_Callback_t * Callback, void * Context);

extern HMODULE LoadLocalOpenCBMDll(void);

#endif // #ifndef INSTCBM_H
