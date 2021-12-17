/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2005-2009, 2014 Spiro Trikaliotis
 */

#ifndef ARCHLIB_WINDOWS_H
#define ARCHLIB_WINDOWS_H

#include "opencbm.h"

#include "opencbm-plugin.h"
#include "opencbm-plugin-install.h"

#ifdef WIN32

EXTERN opencbm_plugin_install_process_commandline_t opencbm_plugin_install_process_commandline;

EXTERN opencbm_plugin_install_do_install_t          opencbm_plugin_install_do_install;
EXTERN opencbm_plugin_install_do_uninstall_t        opencbm_plugin_install_do_uninstall;
EXTERN opencbm_plugin_install_get_needed_files_t    opencbm_plugin_install_get_needed_files;

EXTERN opencbm_plugin_install_generic_t             opencbm_plugin_install_generic;
EXTERN opencbm_plugin_install_plugin_data_t         opencbm_plugin_install_plugin_data;
EXTERN opencbm_plugin_get_all_plugin_names_t        opencbm_plugin_get_all_plugin_names;
EXTERN opencbm_plugin_self_init_plugin_t            opencbm_plugin_self_init_plugin;
EXTERN opencbm_plugin_self_uninit_plugin_t          opencbm_plugin_self_uninit_plugin;

/* functions of the opencbm.dll that can be used by plugins */

#include "configuration.h"

extern int plugin_is_active(opencbm_configuration_handle Handle, const char Section[]);
extern int plugin_set_active(const char Section[]);
extern int plugin_set_inactive(const char Section[]);

#endif

#endif // #ifndef ARCHLIB_WINDOWS_H
