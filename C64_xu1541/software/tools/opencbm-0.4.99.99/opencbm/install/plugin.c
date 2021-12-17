/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2007,2008 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file plugin.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Program to install and uninstall the OPENCBM driver; handling of plugins
**
****************************************************************/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>

#include <getopt.h>

#include "archlib.h"
#include "archlib-windows.h"
#include "cbmioctl.h"
#include "configuration.h"
#include "version.h"
#include "arch.h"
#include "i_opencbm.h"

#include "libmisc.h"

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#define DBG_PROGNAME "INSTCBM.EXE"

/*! This file is "like" debug.c, that is, define some variables */
#define DBG_IS_DEBUG_C

#include "debug.h"

#include "instcbm.h"

#include "opencbm-plugin.h"

/*! \brief name prefix for the plugin executable

  \remark
    The "main" executable for the plugin has to be named as follows:

    If the plugin if named "AAA", the plugin file must be named

    PLUGIN_PREFIX "AAA" PLUGIN_SUFFIX
*/
#define PLUGIN_PREFIX "opencbm-"

/*! \brief name suffix for the plugin executable

  \remark
    The "main" executable for the plugin has to be named as follows:

    If the plugin if named "AAA", the plugin file must be named

    PLUGIN_PREFIX "AAA" PLUGIN_SUFFIX
*/
#define PLUGIN_SUFFIX ".dll"

/*! \brief \internal Get the file name of a plugin

 This function returns the file name for a plugin
 with the given name.

 \param PluginName
   The name of the plugin.

 \return
   Pointer to a string containing the name of the plugin main executable;
   NULL if not enough memory to hold the name.

   This string is allocated from the heap with malloc().

 \remark
   The plugin functions are not multi-thread safe!

 \remark
   This implementation is not very efficient (strcat()), but we do
   not care here.
*/
static char *
malloc_plugin_file_name(const char *PluginName)
{
    char * filename = NULL;

    FUNC_ENTER();

    do {
        filename = cbmlibmisc_stralloc( sizeof(PLUGIN_PREFIX) - 1
                                        + strlen(PluginName) 
                                        + sizeof(PLUGIN_SUFFIX) - 1 );

        if (filename == NULL)
            break;

        strcpy(filename, PLUGIN_PREFIX);
        strcat(filename, PluginName);
        strcat(filename, PLUGIN_SUFFIX);

    } while (0);

    FUNC_LEAVE_STRING(filename);
}

/*! \brief \internal Check if a plugin is already in the list of plugins

 This function checks if the given plugin is already in the list
 of plugins to be processed.

 \param PluginName
   The name of the plugin to check.

 \param InstallParameter
   The install parameters which contain the plugin list.

 \return
   TRUE if the plugin already exists, else FALSE:

 \remark
   The plugin functions are not multi-thread safe!
*/
static BOOL
PluginAlreadyInList(const char * const PluginName, cbm_install_parameter_t * InstallParameter)
{
    BOOL exists = FALSE;
    cbm_install_parameter_plugin_t * currentPlugin;

    FUNC_ENTER();

    DBG_ASSERT(InstallParameter != NULL);
    DBG_ASSERT(PluginName != NULL);

    for (currentPlugin = InstallParameter->PluginList; currentPlugin != NULL; currentPlugin = currentPlugin->Next) {
        if (strcmp(currentPlugin->Name, PluginName) == 0) {
            exists = TRUE;
            break;
        }
    }

    FUNC_LEAVE_BOOL(exists);
}

/*! \brief \internal Add a plugin to the list of plugins

 This function adds data of a plugin to the list of plugins to
 be processed.

 \param InstallParameter
   The install parameters which contain the plugin list.

 \param Plugin
   The plugin to add. This pointer must point to dynamically
   allocated memory (via malloc()). Once added, the caller
   looses ownership if this memory area.

 \remark
   The plugin functions are not multi-thread safe!

 \remark
   The memory pointer to by Plugin can be freed
   for all plugins with a call to PluginListFree().
*/
static void
PluginAddToList(cbm_install_parameter_t * InstallParameter, cbm_install_parameter_plugin_t * Plugin)
{
    cbm_install_parameter_plugin_t * previousPlugin;

    FUNC_ENTER();

    DBG_ASSERT(InstallParameter != NULL);
    DBG_ASSERT(Plugin != NULL);

    Plugin->Next = NULL;

    previousPlugin = (cbm_install_parameter_plugin_t *) & InstallParameter->PluginList;

    while (previousPlugin->Next) {
        previousPlugin = previousPlugin->Next;
    }

    DBG_ASSERT(previousPlugin);

    previousPlugin->Next = Plugin;

    FUNC_LEAVE();
}

/*! \brief Free all the memory occupied by plugin management

 This function frees all the memory that was needed in order
 to remember plugins and their data.

 \param InstallParameter
   The install parameters from which to free the plugin list.

 \remark
   The plugin functions are not multi-thread safe!
*/
void
PluginListFree(cbm_install_parameter_t * InstallParameter)
{
    cbm_install_parameter_plugin_t * currentPlugin;
    cbm_install_parameter_plugin_t * nextPlugin;

    FUNC_ENTER();

    DBG_ASSERT(InstallParameter != NULL);

    nextPlugin = InstallParameter->PluginList;

    while (nextPlugin != NULL) {
        currentPlugin = nextPlugin;

        nextPlugin = currentPlugin->Next;

        free(currentPlugin->Name);
        free(currentPlugin->FileName);
        free(currentPlugin->OptionMemory);
        free(currentPlugin);
    }

    FUNC_LEAVE();
}

/*! \brief Callback for getopt_long()

 This function is used as a callback from the plugin
 to process the command-line parameters.
 It resembles the getopt_long() POSIX call.

 \param Argc
   see getopt_long()

 \param Argv
   see getopt_long()

 \param Optstring
   see getopt_long()

 \param Longopts
   see getopt_long()

 \return
   see getopt_long()
*/
static int CBMAPIDECL
getopt_long_callback(int Argc,
                     char * const Argv[],
                     const char *Optstring,
                     const struct option *Longopts)
{
    int retValue;

    FUNC_ENTER();

    retValue = getopt_long(Argc, Argv, Optstring, Longopts, NULL);

    FUNC_LEAVE_INT(retValue);
}

static cbm_install_parameter_plugin_t *
GetPluginData(const char * const PluginName, cbm_install_parameter_t * InstallParameter, int Argc, char * const Argv[])
{
    HINSTANCE library = NULL;
    char *plugin_file_name = NULL;
    void *option_memory = NULL;

    BOOL error = TRUE;

    cbm_install_parameter_plugin_t * returnValue = NULL;

    FUNC_ENTER();

    do {
        unsigned int option_memory_size = 0;

        opencbm_plugin_install_process_commandline_t * opencbm_plugin_install_process_commandline;
        opencbm_plugin_install_get_needed_files_t * opencbm_plugin_install_get_needed_files;

        cbm_install_parameter_plugin_t * pluginData;

        CbmPluginInstallProcessCommandlineData_t commandLineData;

        memset(&commandLineData, 0, sizeof(commandLineData));

        if (PluginAlreadyInList(PluginName, InstallParameter)) {
            fprintf(stderr, "Please, do not add plugin '%s' multiple times!\n", PluginName);
            break;
        }

        pluginData = malloc(sizeof(*pluginData));
        if ( ! pluginData)
            break;

        memset(pluginData, 0, sizeof(*pluginData));

        plugin_file_name = malloc_plugin_file_name(PluginName);

        if (plugin_file_name == NULL)
            break;

        /*
         * Load the DLL. Make sure that we do not get a warning dialog
         * if a dependancy DLL is not found.
         */

        {
            UINT oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

            library = LoadLibrary(plugin_file_name);

            SetErrorMode(oldErrorMode);
        }

        if (library == NULL) {
            fprintf(stderr, "Error loading plugin '%s', ABORTING!.\n\n", PluginName);
            DBG_ERROR((DBG_PREFIX "Error loading plugin '%s', ABORTING!", PluginName));
            break;
        }

        opencbm_plugin_install_process_commandline = (void *) GetProcAddress(library, "opencbm_plugin_install_process_commandline");
        opencbm_plugin_install_get_needed_files = (void *) GetProcAddress(library, "opencbm_plugin_install_get_needed_files");

        if (0 == (opencbm_plugin_install_process_commandline && opencbm_plugin_install_get_needed_files))
            break;

        option_memory_size = opencbm_plugin_install_process_commandline(&commandLineData);

        /* make sure option_memory_size is at least 1,
         * because malloc(0) can return either NULL or a valid memory block.
         * This way, we are consistent, regardless of how malloc() handles this.
         */

        option_memory_size = (option_memory_size > 0) ? option_memory_size : 1;

        option_memory = malloc(option_memory_size);

        if (option_memory == NULL)
            break;

        memset(option_memory, 0, option_memory_size);

        commandLineData.Argc               = Argc;
        commandLineData.Argv               = Argv;
        commandLineData.OptArg             = &optarg;
        commandLineData.OptInd             = &optind;
        commandLineData.OptErr             = &opterr;
        commandLineData.OptOpt             = &optopt;
        commandLineData.OptionMemory       = option_memory;
        commandLineData.GetoptLongCallback = &getopt_long_callback;
        commandLineData.InstallParameter   = InstallParameter;

        error = opencbm_plugin_install_process_commandline(&commandLineData);

        if ( ! error && pluginData) {
            pluginData->Name = cbmlibmisc_strdup(PluginName);
            pluginData->FileName = cbmlibmisc_strdup(plugin_file_name);

            if (pluginData->Name == NULL || pluginData->FileName == NULL) {
                error = TRUE;
            }
        }

        if (error) {
            free(pluginData->FileName);
            free(pluginData->Name);
            free(pluginData);

            free(commandLineData.OptionMemory);

            break;
        }

        pluginData->OptionMemory = commandLineData.OptionMemory;

        error = TRUE; // assume error unless proven otherwise

        {
            int needed_files_length = opencbm_plugin_install_get_needed_files(&commandLineData, NULL);

            pluginData->NeededFiles = malloc(needed_files_length);

            if (NULL == pluginData->NeededFiles)
                break;

            opencbm_plugin_install_get_needed_files(&commandLineData, pluginData->NeededFiles);

            error = FALSE;
        }

        returnValue = pluginData;

    } while (0);

    if (library) {
        FreeLibrary(library);
    }

    free(plugin_file_name);

    if (error) {
        free(option_memory);
    }

    FUNC_LEAVE_PTR(returnValue, cbm_install_parameter_plugin_t *);
}

/*! \brief Process the command-line parameters for a plugin and add that plugin to the plugin list.

 This function lets the specified plugin process its own
 command-line parameters. Afterwards, an entry for that
 plugin with the plugin-specific data is added to the
 plugin list.

 \param InstallParameter
   The install parameters which contain the plugin list.

 \param PluginName
   The name of the plugin.

 \param Argc
   The (remaining) argc value, as if given to
   int main(int, char **);

 \param Argv
   The (remaining) argv value, as if given to
   int main(int, char **);

 \return
    TRUE on error, else FALSE.
*/
BOOL
ProcessPluginCommandlineAndAddIt(cbm_install_parameter_t * InstallParameter,
                                 const char * const PluginName,
                                 int Argc,
                                 char * const Argv[])
{
    cbm_install_parameter_plugin_t *pluginParameter;

    BOOL error = TRUE;

    FUNC_ENTER();

    pluginParameter = GetPluginData(PluginName, InstallParameter, Argc, Argv);

    error = pluginParameter == NULL ? TRUE : FALSE;

    if ( ! error ) {
        PluginAddToList(InstallParameter, pluginParameter);
    }

    FUNC_LEAVE_BOOL(error);
}

/*! \brief Get all the plugins

 This function searches for all plugins available 
 and adds an entry for that plugin to the plugin list.

 \param InstallParameter
   The install parameters which contain the plugin list.

 \return
    TRUE on error, else FALSE.

 \remark
    An available plugin is a plugin in the current
    directory. A plugin is determined by all files
    that match the pattern PLUGIN_PREFIX "*" PLUGIN_SUFFIX.
*/
BOOL
get_all_plugins(cbm_install_parameter_t * InstallParameter)
{
    BOOL error = FALSE;
    WIN32_FIND_DATA finddata;
    HANDLE findhandle;

    const char *findfilename = NULL; 

    FUNC_ENTER();

    do {
        findfilename = malloc_plugin_file_name("*");

        if ( ! findfilename) {
            error = TRUE;
            break;
        }

        findhandle = FindFirstFile(findfilename, &finddata);

        do {
            if (findhandle != INVALID_HANDLE_VALUE) {
                BOOL quit = FALSE;

                do {
                    if (finddata.dwFileAttributes != FILE_ATTRIBUTE_DIRECTORY) {
                        /* extract the plugin name from the file name */

                        cbm_install_parameter_plugin_t * pluginParameter = NULL;

                        int len = strlen(finddata.cFileName) + 1;
                        char *pluginName = malloc(len);

                        if (pluginName == NULL) {
                            error = TRUE;
                            break;
                        }

                        strcpy(pluginName, finddata.cFileName + sizeof(PLUGIN_PREFIX) - 1);
                        pluginName[strlen(pluginName) - (sizeof(PLUGIN_SUFFIX) - 1)] = '\0';

                        pluginParameter = GetPluginData(pluginName, InstallParameter, 1, NULL);

                        if (pluginParameter) {
                            PluginAddToList(InstallParameter, pluginParameter);
                        }
                        else {
                            error = TRUE;
                        }

                        free(pluginName);
                    }

                    if (FindNextFile(findhandle, &finddata) == 0) {
                        quit = TRUE;
                    }
                } while ( ! quit);

                FindClose(findhandle);

            }
        } while (0);
    } while (0);

    cbmlibmisc_strfree(findfilename);

    FUNC_LEAVE_BOOL(error);
}

/*! \brief Callback for get_all_installed_plugins()

 This function adds the specified plugin to the
 plugins list.

 \param InstallParameter
   The install parameters which contain the plugin list.

 \param PluginName
   The name of the plugin to add.

 \return
    TRUE on error, else FALSE.

 \remark
    This function is called for every installed plugin
    from the call to opencbm_plugin_get_all_plugin_names() 
    in get_all_installed_plugins().
*/
static BOOL CBMAPIDECL
get_all_installed_plugins_callback(cbm_install_parameter_t * InstallParameter, const char * PluginName)
{
    FUNC_LEAVE_BOOL(ProcessPluginCommandlineAndAddIt(InstallParameter, PluginName, 1, NULL));
}

/*! \brief Get all the installed plugins

 This function searches all installed plugins 
 and adds an entry for that plugin to the plugin list.

 \param InstallParameter
   The install parameters which contain the plugin list.

 \return
    TRUE on error, else FALSE.

 \remark
    An installed plugin is a plugin which is mentioned
    in the configuration file.
*/
BOOL
get_all_installed_plugins(cbm_install_parameter_t * InstallParameter)
{
    HMODULE openCbmDllHandle = NULL;
    BOOL error = TRUE;
    opencbm_configuration_handle configuration_handle = NULL;

    FUNC_ENTER();

    do {
        opencbm_plugin_get_all_plugin_names_context_t opencbm_plugin_get_all_plugin_names_context;
        opencbm_plugin_get_all_plugin_names_t * opencbm_plugin_get_all_plugin_names;

        openCbmDllHandle = LoadLocalOpenCBMDll();
        if (openCbmDllHandle  == NULL) {
            DBG_PRINT((DBG_PREFIX "Could not open the OpenCBM DLL."));
            fprintf(stderr, "Could not open the OpenCBM DLL.");
            break;
        }

        opencbm_plugin_get_all_plugin_names = (opencbm_plugin_get_all_plugin_names_t *) 
            GetProcAddress(openCbmDllHandle, "opencbm_plugin_get_all_plugin_names");

        if ( ! opencbm_plugin_get_all_plugin_names ) {
            break;
        }

        opencbm_plugin_get_all_plugin_names_context.Callback = get_all_installed_plugins_callback;
        opencbm_plugin_get_all_plugin_names_context.InstallParameter = InstallParameter;

        if ( opencbm_plugin_get_all_plugin_names(&opencbm_plugin_get_all_plugin_names_context) ) {
            break;
        }

        error = FALSE;

    } while (0);

    if (openCbmDllHandle) {
        FreeLibrary(openCbmDllHandle);
    }

    opencbm_configuration_close(configuration_handle);

    FUNC_LEAVE_BOOL(error);
}

/*! \brief Execute a callback function for all plugins in the plugin list

 \param InstallParameter
   The install parameters which contain the plugin list.

 \param Callback
   Callback function to execute for each plugin.

 \param Context
   Context data to give to the Callback function.
   This data is Callback specific; it is not interpreted in any way
   by this function here.

 \return
   TRUE if an error occurred (that is, a callback returned TRUE);
   else FALSE:

 \remark
   If a callback function returns TRUE, this indicates an error.
   In this case, all subsequent plugins will not be processed.
*/
BOOL PluginForAll(cbm_install_parameter_t * InstallParameter, PluginForAll_Callback_t * Callback, void * Context)
{
    cbm_install_parameter_plugin_t * plugin = NULL;

    BOOL error = FALSE;

    FUNC_ENTER();

    for (plugin = InstallParameter->PluginList; (plugin != NULL) && ! error; plugin = plugin->Next) {
        printf("Using plugin: '%s' with filename '%s'.\n", plugin->Name, plugin->FileName);
        error = Callback(plugin, Context);
    }

    FUNC_LEAVE_BOOL(error);
}
