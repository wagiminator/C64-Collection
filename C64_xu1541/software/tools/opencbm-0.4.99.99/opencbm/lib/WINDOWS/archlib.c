/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2005,2007,2008 Spiro Trikaliotis
 *
*/

/*! ************************************************************** 
** \file lib/WINDOWS/archlib.c \n
** \author Michael Klein, Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver, windows specific code
**
****************************************************************/

#include <windows.h>
#include <stdlib.h>

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! Mark: We are building the DLL */
#define DBG_DLL

/*! The name of the executable */
#define DBG_PROGNAME "OPENCBM.DLL"

/*! This file is "like" debug.c, that is, define some variables */
#define DBG_IS_DEBUG_C

#include "configuration.h"
#include "debug.h"
#include "getpluginaddress.h"


#define OPENCBM_PLUGIN 1 /*!< \brief mark: we are exporting plugin functions */
#include "archlib.h"
#include "archlib-windows.h"
#include "libmisc.h"

/*! \brief DLL initialization und unloading

 This function is called whenever the DLL is loaded or unloaded.
 It ensures that the driver is loaded to be able to call its
 functions.

 \param Module
   Handle of the module; this is not used.

 \param Reason
   DLL_PROCESS_ATTACH if the DLL is loaded,
   DLL_PROCESS_DETACH if it is unloaded.

 \param Reserved
   Not used.

 \return 
   Returns TRUE on success, else FALSE.

 If this function returns FALSE, windows reports that loading the DLL
 was not successful. If the DLL is linked statically, the executable
 refuses to load with STATUS_DLL_INIT_FAILED (0xC0000142)
*/

BOOL WINAPI
DllMain(IN HANDLE Module, IN DWORD Reason, IN LPVOID Reserved)
{
    static BOOL bIsOpen = FALSE;

    FUNC_ENTER();

#if DBG

    if (Reason == DLL_PROCESS_ATTACH)
    {
        // Read the debugging flags from the registry

        cbm_get_debugging_flags(NULL);
    }

#endif

    FUNC_LEAVE_BOOL(TRUE);
}


/*! \brief @@@@@ \todo document

 \param DefaultPluginname

 \param DefaultLocation
    Must be one of INDEX_DEFAULT_FILENAME_LOCAL,
    INDEX_DEFAULT_FILENAME_USERDIR or INDEX_DEFAULT_FILENAME_WINDIR.

 \return
*/
BOOL CBMAPIDECL opencbm_plugin_install_generic(const char * DefaultPluginname, unsigned int DefaultLocation)
{
    BOOL error = TRUE;

    opencbm_configuration_handle configuration_handle;

    const char * configurationFilename = NULL;

    FUNC_ENTER();

    do {
        /* create the INI file, if not present, and create the needed keys */

        configurationFilename = configuration_get_default_filename_for_install(DefaultLocation);
        if (configurationFilename == NULL) {
            break;
        }

        configuration_handle = opencbm_configuration_create(configurationFilename);

        if (configuration_handle == NULL) {
            break;
        }

        if (DefaultPluginname != NULL) {
            error = opencbm_configuration_set_data(configuration_handle, 
                       "plugins", "default", DefaultPluginname);
        }
        else {
            error = FALSE;
        }

        error = opencbm_configuration_close(configuration_handle) || error;

    } while (0);

    cbmlibmisc_strfree(configurationFilename);

    FUNC_LEAVE_BOOL(error);
}

static BOOL
cbm_plugin_call_self_init_plugin(const char * Pluginname, const char * Filepath, const CbmPluginInstallProcessCommandlineData_t * CommandlineData)
{
    BOOL error = TRUE;
    SHARED_OBJECT_HANDLE pluginHandle = SHARED_OBJECT_HANDLE_INVALID;

    FUNC_ENTER();

    do {
        opencbm_plugin_self_init_plugin_t * opencbm_plugin_self_init_plugin = NULL;

        pluginHandle = plugin_load(Filepath);
        if (pluginHandle == SHARED_OBJECT_HANDLE_INVALID) {
            DBG_ERROR((DBG_PREFIX "Could not load plugin '%s' at '%s' for self-init!\n", Pluginname, Filepath));
            break;
        }

        opencbm_plugin_self_init_plugin = plugin_get_address(pluginHandle, "opencbm_plugin_self_init_plugin");
        if ( ! opencbm_plugin_self_init_plugin ) {
            error = FALSE;
            break;
        }

        error = opencbm_plugin_self_init_plugin();
        if (error) {
            break;
        }

        error = plugin_set_active(Pluginname);

    } while (0);

    if (pluginHandle != SHARED_OBJECT_HANDLE_INVALID) {
        plugin_unload(pluginHandle);
    }

    FUNC_LEAVE_BOOL(error);
}

/*! \brief @@@@@ \todo document

 \param Pluginname

 \param Filepath

 \param CommandlineData

 \return
*/
BOOL CBMAPIDECL opencbm_plugin_install_plugin_data(const char * Pluginname, const char * Filepath, const CbmPluginInstallProcessCommandlineData_t * CommandlineData)
{
    BOOL error = TRUE;

    opencbm_configuration_handle configuration_handle;

    const char * configurationFilename = NULL; 

    FUNC_ENTER();

    do {
        /* create the INI file, if not present, and create the needed keys */

        configurationFilename = configuration_get_default_filename();

        if (configurationFilename == NULL) {
            break;
        }

        configuration_handle = opencbm_configuration_open(configurationFilename);
        if (configuration_handle == NULL) {
            DBG_PRINT((DBG_PREFIX "No configuration file present. Is OpenCBM installed at all?"));
            break;
        }

        error = opencbm_configuration_set_data(configuration_handle, 
                   Pluginname, "location", Filepath);

        if (error == 0) {
            error = cbm_plugin_call_self_init_plugin(Pluginname, Filepath, CommandlineData->OptionMemory);
        }

        error = opencbm_configuration_close(configuration_handle) || error;

    } while (0);

    cbmlibmisc_strfree(configurationFilename);

    FUNC_LEAVE_BOOL(error);
}

/*! \brief @@@@@ \todo document */
opencbm_configuration_enum_sections_callback_t callback;

/*! \brief @@@@@ \todo document

 \param Handle

 \param Section

 \param Data

 \return
*/
static int opencbm_plugin_get_all_plugin_names_callback(opencbm_configuration_handle Handle,
                                                    const char Section[],
                                                    void * Data)
{
    opencbm_plugin_get_all_plugin_names_context_t * Context = Data;

    BOOL error = 0;

    FUNC_ENTER();

    do {
        if ( ! Section || strcmp(Section, "plugins") == 0 ) {
            break;
        }

        /*
         * check if the plugin is marked as active.
         * Only active plugins are reported back,
         */

        if ( ! plugin_is_active(Handle, Section) ) {
            break;
        }

       error = Context->Callback(Context->InstallParameter, Section);

    } while (0);

    FUNC_LEAVE_BOOL(error);
}

/*! \brief @@@@@ \todo document

 \param Context

 \return
*/
EXTERN BOOL CBMAPIDECL opencbm_plugin_get_all_plugin_names(opencbm_plugin_get_all_plugin_names_context_t * Context)
{
    BOOL error = TRUE;
    opencbm_configuration_handle configuration_handle = NULL;
    const char * configurationFilename = NULL;

    FUNC_ENTER();

    do {
        configurationFilename = configuration_get_default_filename();
        if (configurationFilename == NULL) {
            break;
        }

        configuration_handle = opencbm_configuration_open(configurationFilename);
        if (configuration_handle == NULL) {
            DBG_PRINT((DBG_PREFIX "No configuration file present. Is OpenCBM installed at all?"));
            break;
        }

        error = opencbm_configuration_enum_sections(configuration_handle,
            opencbm_plugin_get_all_plugin_names_callback, Context);

    } while (0);

    opencbm_configuration_close(configuration_handle);

    cbmlibmisc_strfree(configurationFilename);

    FUNC_LEAVE_BOOL(error);
}
