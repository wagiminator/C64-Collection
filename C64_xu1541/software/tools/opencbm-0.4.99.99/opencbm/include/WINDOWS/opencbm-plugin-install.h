/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2008-2009, 2011, 2014 Spiro Trikaliotis
 */

/*! **************************************************************
** \file include/WINDOWS/opencbm-plugin-install.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Plugin DLL interface
**
****************************************************************/

#ifndef OPENCBM_PLUGIN_INSTALL_H
#define OPENCBM_PLUGIN_INSTALL_H

#include "opencbm-plugin.h"

#ifdef WIN32
    /*
     * special functions for installing on Windows
     * These functions are not used in the normal process, only for installation.
     * Thus, they are missing in opencbm_plugin_s and opencbm_plugin_t.
     */

    // int (*p_opencbm_install_getopt)(int argc, char * const argv[], const char *optstring);

    

    /*! \brief @@@@@ \todo document

     \param Argc

     \param Argv

     \param OptString

     \param Longopts

     \return
    */
    typedef int CBMAPIDECL opencbm_plugin_install_getopt_long_callback_t(int Argc, char * const Argv[], const char *Optstring, const struct option *Longopts);

    /*! \brief remember the operating system version */
    typedef
    enum osversion_e
    {
        WINUNSUPPORTED, //!< an unsupported operating system
        WINNT3,         //!< Windows NT 3.x (does the driver work there?
        WINNT4,         //!< Windows NT 4.x
        WIN2000,        //!< Windows 2000 (NT 5.0)
        WINXP,          //!< Windows XP (NT 5.1)
        WINVISTA,       //!< Windows Vista (NT 6.0)
        WIN7,           //!< Windows 7 (NT 6.1)
        WIN8,           //!< Windows 8 (NT 6.2)
        WIN8_1,         //!< Windows 8.1 (NT 6.3)
        WINNEWER        //!< newer than Windows 8.1
    } osversion_t;

    /*! \brief describe the files needed for installation */
    typedef
    struct opencbm_plugin_install_neededfiles_s opencbm_plugin_install_neededfiles_t;

    /*! \brief describe a plugin */
    typedef
    struct cbm_install_parameter_plugin_s 
    {
        struct cbm_install_parameter_plugin_s *Next;        /*!< pointer to the next plugin */
        char * FileName;                                    /*!< file name of the plugin. This is a decorated plugin name (Name). */
        char * Name;                                        /*!< plugin name */
        void * OptionMemory;                                /*!< pointer to same memory where the plugin can store options. */
        opencbm_plugin_install_neededfiles_t * NeededFiles; /*!< description of the needed files for this plugin. */

    } cbm_install_parameter_plugin_t;


    /*! \brief The parameter which are given on the command-line */
    typedef
    struct cbm_install_parameter_s
    {
        /*! Do not execute anything */
        BOOL NoExecute;

        /*! Need administrator privileges */
        BOOL AdminNeeded;

        /*! Find out of more than one "execute" parameter is given */
        BOOL ExecuteParameterGiven;

        /*! Install OpenCBM, that is, no other command has been given */
        BOOL Install;

        /*! --remove was given */
        BOOL Remove;

        /*! --purge was given. This has to go after --remote */
        BOOL Purge;

        /*! --nocopy was given */
        BOOL NoCopy;

        /*! --update was given */
        BOOL Update;

        /*! --check was given */
        BOOL CheckInstall;

        /*! --buffer was given */
        BOOL OutputDebuggingBuffer;

        /*! --debugflags was given */
        BOOL DebugFlagsDriverWereGiven;

        /*! --debugflags, a second parameter (for the DLL) was given */
        BOOL DebugFlagsDllWereGiven;

        /*! --debugflags, a third parameter (for INSTCBM itself) was given */
        BOOL DebugFlagsInstallWereGiven;

        /*! if --debugflags was given: the number which was there */
        ULONG DebugFlagsDriver;

        /*! if --debugflags with 2 parameters was given: the number which was there */
        ULONG DebugFlagsDll;

        /*! if --debugflags with 3 parameters was given: the number which was there */
        ULONG DebugFlagsInstall;

        /*! The type of the OS version */
        osversion_t OsVersion;

        /*! pointer to an array of pointers to the strings containg the names of the plugins.
         * Convention:
         * - the last entry is marked with a NULL pointer.
         * - If Plugins is not NULL, there have been two malloc()s.
         *   To free the data, one has to call:
         *   free(Plugins[0]);
         *   free(Plugins);
         */
        // @@@ char ** PluginNames;
        cbm_install_parameter_plugin_t * PluginList;

        /*! if set, there was no explicit plugin name given, but the list is a default one */
        BOOL NoExplicitPluginGiven;

        /*! adapter to be set as default adapter, or NULL if none was given */
        char * DefaultAdapter;

    } opencbm_install_parameter_t;

    /*! \brief describe necessary information to process command line in a plugin

      This struct contains all necessary data to allow the plugin to process
      the command line parameters given to instcbm on its own.

      The instcbm tools gives this structure to the plugin. The structure includes
      some pointers necessary to use the getopt_long() function of instcbm.exe.
      This way, the plugin does not need to include this function on its own.
    */
    typedef struct CbmPluginInstallProcessCommandlineData_s {
        int    Argc;          /*!< the argc value, as if given to int main(int, char **) */
        char * const * Argv;  /*!< the argv value, as if given to int main(int, char **) */
        void * OptionMemory;  /*!< pointer to the option memory where the plugin can remember its options accross multiple calls */
        char **OptArg;        /*!< pointer to the char *optarg used in getopt_long() */
        int  * OptInd;        /*!< pointer to the int optind used in getopt_long() */
        int  * OptErr;        /*!< pointer to the int opterr used in getopt_long() */
        int  * OptOpt;        /*!< pointer to the int optopt used in getopt_long() */

        opencbm_plugin_install_getopt_long_callback_t * GetoptLongCallback; /*!< pointer to a getopt_long() function */
        opencbm_install_parameter_t                   * InstallParameter;   /*!< some generic install parameters */

    } CbmPluginInstallProcessCommandlineData_t;

    
    /*! \brief @@@@@ \todo document

     \param ProcessCommandlineData

     \return
    */
    typedef unsigned int CBMAPIDECL opencbm_plugin_install_process_commandline_t(CbmPluginInstallProcessCommandlineData_t * ProcessCommandlineData); 

    
    /*! \brief @@@@@ \todo document

     \param OptionMemory

     \return
    */
    typedef BOOL CBMAPIDECL opencbm_plugin_install_do_install_t(void * OptionMemory);

    /*! \brief @@@@@ \todo document

     \param OptionMemory

     \return
    */
    typedef BOOL CBMAPIDECL opencbm_plugin_install_do_uninstall_t(void * OptionMemory);

    /*! \brief Where to install the files

      Helper enum for struct opencbm_plugin_install_neededfiles_s.
      It describes to which location each file is to be copied on installation.
    */
    typedef
    enum opencbm_plugin_install_location_e
    {
        LOCAL_DIR,        /*!< the file remains at the local dir (".") */
        LOCAL_PLUGIN_DIR, /*!< the file must be copied into the plugin dir ("./PLUGINNAME") */
        SYSTEM_DIR,       /*!< the file must be copied into the system dir ("c:\windows\system32") */
        DRIVER_DIR,       /*!< the file must be copied into the drivers dir ("c:\windows\system32\drivers") */
        LIST_END          /*!< MUST BE THE LAST ENTRY! */
    } opencbm_plugin_install_location_t;

    /*! \brief Describes the files to be copied on install

      This struct describes one file to be copied on install.
    */
    typedef
    struct opencbm_plugin_install_neededfiles_s
    {
        opencbm_plugin_install_location_t FileLocation;       /*!< the location of the file */
        const char                        Filename[100];      /*!< the file name */
        const char                      * FileLocationString; /*!< the location where the file is copied. 
                                                                   This is set to NULL by the plugin.
                                                                   instcbm uses it to keep track where the file resides. */
    } opencbm_plugin_install_neededfiles_t;

    
    /*! \brief @@@@@ \todo document

     \param Data

     \param Destination

     \return
    */
    typedef unsigned int CBMAPIDECL opencbm_plugin_install_get_needed_files_t(CbmPluginInstallProcessCommandlineData_t * Data, opencbm_plugin_install_neededfiles_t * Destination);


    #define INDEX_DEFAULT_FILENAME_LOCAL 0
    #define INDEX_DEFAULT_FILENAME_USERDIR 1
    #define INDEX_DEFAULT_FILENAME_WINDIR 2

    /*! \brief @@@@@ \todo document

     \param DefaultPluginname

     \param DefaultLocation
        Must be one of INDEX_DEFAULT_FILENAME_LOCAL,
        INDEX_DEFAULT_FILENAME_USERDIR or INDEX_DEFAULT_FILENAME_WINDIR.

     \return
    */
    typedef BOOL CBMAPIDECL opencbm_plugin_install_generic_t(const char * DefaultPluginname, unsigned int DefaultLocation);

    /*! \brief @@@@@ \todo document

     \param Pluginname

     \param Filepath

     \param CommandlineData

     \return
    */
    typedef BOOL CBMAPIDECL opencbm_plugin_install_plugin_data_t(const char * Pluginname, const char * Filepath, const CbmPluginInstallProcessCommandlineData_t * CommandlineData);


    /*! \brief @@@@@ \todo document

     \param InstallParameter

     \param Pluginname

     \return
    */
    typedef BOOL CBMAPIDECL opencbm_plugin_get_all_plugin_names_callback_t(opencbm_install_parameter_t * InstallParameter, const char * Pluginname);

    /*! \brief context for opencbm_plugin_get_all_plugin_names_t()

      This structure describe the callback to be used with opencbm_plugin_get_all_plugin_names_t().
    */
    typedef struct opencbm_plugin_get_all_plugin_names_context_s {
        opencbm_plugin_get_all_plugin_names_callback_t * Callback;         /*!< pointer to the callback function */
        opencbm_install_parameter_t                    * InstallParameter; /*!< additional data to be passed to the callback function */
    } opencbm_plugin_get_all_plugin_names_context_t;

    /*! \brief @@@@@ \todo document

     \param Callback

     \return
    */
    typedef BOOL CBMAPIDECL opencbm_plugin_get_all_plugin_names_t(opencbm_plugin_get_all_plugin_names_context_t * Callback);

    /*! \brief @@@@@ \todo document

     \return
    */
    typedef int CBMAPIDECL opencbm_plugin_self_init_plugin_t(void);

    /*! \brief @@@@@ \todo document

     \return
    */
    typedef int CBMAPIDECL opencbm_plugin_self_uninit_plugin_t(void);

#endif

#endif // #ifndef OPENCBM_PLUGIN_INSTALL_H
