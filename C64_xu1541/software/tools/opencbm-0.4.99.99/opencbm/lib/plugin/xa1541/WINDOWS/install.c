/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2008 Spiro Trikaliotis
*/

/*! ************************************************************** 
** \file lib/plugin/xa1541/WINDOWS/install.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Helper functions for installing the plugin
**        on a Windows machine
**
****************************************************************/

#include <windows.h>
#include <windowsx.h>

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#ifndef DBG_PROGNAME
    #define DBG_PROGNAME "OPENCBM-XA1541.DLL"
#endif // #ifndef DBG_PROGNAME

#include "debug.h"

#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>

#include "cbmioctl.h"
#include "i_opencbm.h"
#include "libmisc.h"
#include "version.h"

#define OPENCBM_PLUGIN 1 /*!< \brief mark: we are exporting plugin functions */

#include "archlib.h"
#include "archlib-windows.h"


/*! \brief The parameter which are given on the command-line */
typedef
struct xa1541_parameter_s
{
    /*! --enum was given */
    BOOL EnumerateParport;

    /*! --forcent4 was given */
    BOOL ForceNt4;

    /*! --lpt was given, the number which was there */
    ULONG Lpt;

    /*! the IEC cable type was specified */
    IEC_CABLETYPE IecCableType;
    
    /*! It was specified that the driver is to be permanently locked */
    ULONG PermanentlyLock;

    /*! --automatic or --on-demand was given */
    BOOL AutomaticOrOnDemandStart;

    /*! --automatic was given, start the driver automatically */
    BOOL AutomaticStart;

    /*! The type of the OS version */
    osversion_t OsVersion;

} xa1541_parameter_t;


static const struct option longopts[] =
{
    { "help",       no_argument,       NULL, 'h' },
    { "version",    no_argument,       NULL, 'V' },

//    { "enumpport",  no_argument,       NULL, 'e' },
#ifdef _X86_
    { "forcent4",   no_argument,       NULL, 'F' },
#endif // #ifdef _X86_
    { "lpt",        required_argument, NULL, 'l' },
    { "cabletype",  required_argument, NULL, 't' },
    { "lock",       required_argument, NULL, 'L' },
#if DBG
    { "debugflags", required_argument, NULL, 'D' },
    { "buffer",     no_argument,       NULL, 'B' },
#endif // #if DBG
    { "automatic",  no_argument,       NULL, 'A' },
    { "on-demand",  no_argument,       NULL, 'O' },

    { NULL,         0,                 NULL, 0   }
};

static const char shortopts[] = "-hVFl:t:L:DBAO";

static const char usagetext[] =
            "\n\nUsage: instcbm [options] xa1541 [plugin-options]\n"
            "Install the XA1541 plugin and driver on the system, or remove it.\n"
            "\n"
            "plugin-options is one of:\n"
            "  -h, --help       display this help and exit\n"
            "  -V, --version    display version information about cbm4win\n"
//            "  -e, --enumpport re-enumerate the parallel port driver\n"
            "  -l, --lpt=no    set default LPT port\n"
            "  -t, --cabletype=TYPE set cabletype to 'auto', 'xa1541' or 'xm1541'.\n"
            "                  If not specified, --cabletype=auto is assumed.\n"
            "  -L, --lock=WHAT automatically lock the driver 'yes' or not 'no'.\n"
            "                  If not specified, --lock=yes is assumed.\n"
#ifdef _X86_
            "  -F, --forcent4  force NT4 driver on a Win 2000, XP, or newer systems\n" 
            "                  (NOT RECOMMENDED!)\n"
#endif // #ifdef _X86_
            "  -A, --automatic (default) automatically start the driver on system boot.\n"
            "                  The driver can be used from a normal user, no need for\n"
            "                  administrator rights.\n"
            "                  The opposite of --on-demand.\n"
            "  -O, --on-demand start the driver only on demand.\n"
            "                  The opposite of --automatic.\n"
            "\n";


static opencbm_plugin_install_neededfiles_t NeededFilesXA1541[] = 
{
    { SYSTEM_DIR, "opencbm-xa1541.dll", NULL },
    { DRIVER_DIR, "cbm4wdm.sys",        NULL },  /* this MUST be the same index as driver_cbm4wdm is defined */
#ifdef _X86_
    { DRIVER_DIR, "cbm4nt.sys",         NULL },  /* this MUST be the same index as driver_cbm4nt is defined */
#endif
    { LIST_END,   "",                   NULL }
};

/*! \brief use WDM driver or NT4 driver */
typedef
enum driver_to_use_e {
    driver_cbm4wdm = 1,  /*!< use WDM driver */
#ifdef _X86_
    driver_cbm4nt = 2    /*!< use NT4 driver */
#endif
} driver_to_use_t;

/*! \brief \internal Print out a hint how to get help */

static void
hint(void)
{
    fprintf(stderr, "Try \"instcbm xa1541 --help\" for more information.\n");
}


/*! \brief \internal Output version information of instcbm */

static VOID
version(VOID)
{
    printf("opencbm xa1541 plugin version " /* OPENCBM_VERSION */ ", built on " __DATE__ " at " __TIME__ "\n");
}

/*! \brief \internal Print out the help screen */

static void
usage(void)
{
    version();

    printf("%s", usagetext);
}


/*! \internal \brief Process a number

 This function processes a number which was given as a string.

 \param Argument:
   Pointer to the number in ASCII representation

 \param NextChar:
   Pointer to a PCHAR which will had the address of the next
   char not used on return. This can be NULL.

 \param ParameterGiven:
   Pointer to a BOOL which will be set to TRUE if the value
   could be calculated correctly. Can be NULL.

 \param ParameterValue:
   Pointer to a ULONG which will get the result.

 \return
   TRUE on error, FALSE on success.

 If this parameter is given more than once, the last occurence
 takes precedence.

 The number can be specified in octal (0***), hex (0x***), or
 decimal (anything else).

 If NextChar is NULL, the Argument *must* terminate at the
 end of the number. If NextChar is not NULL, the Argument might
 contain a comma.
*/
static BOOL
processNumber(const PCHAR Argument, PCHAR *NextChar, PBOOL ParameterGiven, PULONG ParameterValue)
{
    PCHAR p;
    BOOL error;
    int base;

    FUNC_ENTER();

    DBG_ASSERT(ParameterValue != NULL);

    error = FALSE;
    p = Argument;

    if (p)
    {
        // Find out which base to use (0***, 0x***, or anything else)

        switch (*p)
        {
        case 0:
            error = TRUE;
            break;

        case '0':
            switch (*++p)
            {
            case 'x': case 'X':
                base = 16;
                ++p;
                break;

            default:
                base = 8;
                break;
            };
            break;

        default:
            base = 10;
            break;
        }

        // Convert the value

        if (!error)
        {
            *ParameterValue = strtoul(p, &p, base);

            if (NextChar)
            {
                error = ((*p != 0) && (*p != ',')) ? TRUE : FALSE;
            }
            else
            {
                error = *p != 0 ? TRUE : FALSE;
            }

            if (!error)
            {
                if (NextChar != NULL)
                {
                    *NextChar = p + ((*p) ? 1 : 0);
                }

                if (ParameterGiven != NULL)
                {
                    *ParameterGiven = TRUE;
                }
            }
        }
    }

    FUNC_LEAVE_BOOL(error);
}

/*-------------------------------------------------------------------*/
/*--------- OPENCBM INSTALL HELPER FUNCTIONS ------------------------*/

/*! \brief @@@@@ \todo document

 \param Data

 \return
*/
unsigned int CBMAPIDECL
opencbm_plugin_install_process_commandline(CbmPluginInstallProcessCommandlineData_t * Data)
{
    int error = 0;
    char **localOptarg = Data->OptArg;

    xa1541_parameter_t *parameter = Data->OptionMemory;

    BOOL quitLocalProcessing = FALSE;

    FUNC_ENTER();

    DBG_ASSERT(Data);


    do {
        int c;

        /* special handling for first call: Determine the length of the OptionMemory to be allocated */

        if (Data->Argc == 0) {
            error = sizeof(xa1541_parameter_t);
            break;
        }

        DBG_ASSERT(Data->OptionMemory != NULL);
        DBG_ASSERT(Data->GetoptLongCallback != NULL);
        DBG_ASSERT(Data->OptInd != NULL);
        DBG_ASSERT(Data->OptErr != NULL);
        DBG_ASSERT(Data->OptOpt != NULL);
        DBG_ASSERT(Data->InstallParameter != NULL);

        /* as we are interested in the OS version for installation, copy it */

        parameter->OsVersion = Data->InstallParameter->OsVersion;


        // We have not specified an LPT port yet

        parameter->Lpt = (ULONG) -1;

        // No IEC cable type was specified

        parameter->IecCableType = IEC_CABLETYPE_UNSPEC;

        // It was not specified if the driver is to be permenently locked

        parameter->PermanentlyLock = (ULONG) -1;

        // set the default: automaticstart -A

        parameter->AutomaticStart = TRUE;


        if (Data->Argv) {
        while ( ! quitLocalProcessing && (c = Data->GetoptLongCallback(Data->Argc, Data->Argv, shortopts, longopts)) != -1) {
            switch (c) {
                case 'h':
                    usage();
                    Data->InstallParameter->NoExecute = TRUE;
                    break;

                case 'V':
                    version();
                    Data->InstallParameter->NoExecute = TRUE;
                    break;

                case 't':
                    if ((*localOptarg == NULL) || (strcmp(*localOptarg, "auto") == 0))
                        parameter->IecCableType = IEC_CABLETYPE_AUTO;
                    else if (strcmp(*localOptarg, "xa1541") == 0)
                        parameter->IecCableType = IEC_CABLETYPE_XA;
                    else if (strcmp(*localOptarg, "xm1541") == 0)
                        parameter->IecCableType = IEC_CABLETYPE_XM;
                    else
                    {
                        fprintf(stderr, "you must specify 'xa1541', 'xm1541' or 'auto' for --cabletype\n");
                        error = TRUE;
                    }
                    break;

                case 'L':
                    if (*localOptarg == NULL
                        || (strcmp(*localOptarg, "+") == 0)
                        || (strcmp(*localOptarg, "yes") == 0)
                        || (strcmp(*localOptarg, "true") == 0)
                       )
                    {
                        parameter->PermanentlyLock = 1;
                    }
                    else if (*localOptarg != NULL &&
                            (   (strcmp(*localOptarg, "-") == 0)
                             || (strcmp(*localOptarg, "no") == 0)
                             || (strcmp(*localOptarg, "false") == 0)
                            )
                            )
                    {
                        parameter->PermanentlyLock = 0;
                    }
                    else
                    {
                        fprintf(stderr, "you must specify 'yes' or 'no' for --lock\n");
                        error = TRUE;
                    }
                    break;

#ifdef _X86_
                case 'F':
                    if (Data->InstallParameter->ExecuteParameterGiven)
                    {
                        error = TRUE;
                        printf("Colliding parameters were given, aborting!");
                        hint();
                    }
                    else
                    {
                        Data->InstallParameter->ExecuteParameterGiven = TRUE;
                        parameter->ForceNt4 = TRUE;
                        //parameter->Remove = FALSE;
                    }
                    break;
#endif // #ifdef _X86_

                case 'l':
                    error = processNumber(*localOptarg, NULL, NULL, &parameter->Lpt);
                    break;

                case 'A':
                    if (parameter->AutomaticOrOnDemandStart)
                    {
                        fprintf(stderr, "--automatic and --on-demand cannot be specified at the same time!\n");
                        error = TRUE;
                    }
                    else
                    {
                        parameter->AutomaticStart = TRUE;
                        parameter->AutomaticOrOnDemandStart = TRUE;
                    }
                    break;

                case 'O':
                    if (parameter->AutomaticOrOnDemandStart)
                    {
                        fprintf(stderr, "--automatic and --on-demand cannot be specified at the same time!\n");
                        error = TRUE;
                    }
                    else
                    {
                        parameter->AutomaticStart = FALSE;
                        parameter->AutomaticOrOnDemandStart = TRUE;
                    }
                    break;

                case 1:
                    quitLocalProcessing = 1;
                    -- * Data->OptInd;
                    break;

                default:
                    fprintf(stderr, "error...\n");
                    error = TRUE;
                    hint();
                    break;
            }
        }
        }

    } while (0);

    FUNC_LEAVE_UINT(error);
}

/*! \brief @@@@@ \todo document

 \param Context

 \return
*/
BOOL CBMAPIDECL
opencbm_plugin_install_do_install(void * Context)
{
    BOOL error = TRUE;
    char * driverLocation = NULL;


    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "-- xa1541.install" ));

    do {
        cbm_install_parameter_plugin_t * pluginInstallParameter = Context;
        xa1541_parameter_t *parameter;
        driver_to_use_t driverToUse;

        if (pluginInstallParameter == NULL || pluginInstallParameter->OptionMemory == NULL) {
            break;
        }

        parameter = pluginInstallParameter->OptionMemory;

        //
        // Find out which driver to use (cbm4wdm.sys, cbm4nt.sys)
        //

#ifdef _X86_
        driverToUse = ((parameter->OsVersion > WINNT4) && ! parameter->ForceNt4) ? driver_cbm4wdm : driver_cbm4nt;
#else
        driverToUse = driver_cbm4wdm;
#endif

        driverLocation = cbmlibmisc_strcat(
            pluginInstallParameter->NeededFiles[driverToUse].FileLocationString,
            pluginInstallParameter->NeededFiles[driverToUse].Filename);

        if (driverLocation == NULL) {
            DBG_ERROR((DBG_PREFIX "Could not get the location of the driver file, aborting."));
            fprintf(stderr, "Could not get the location of the driver file, aborting.\n");
            break;
        }

        printf("Using driver '%s'\n", driverLocation);

        error = CbmInstall(OPENCBM_DRIVERNAME, driverLocation, parameter->AutomaticStart);

        if (error) {
            break;
        }

        error = CbmCheckDriver();

        if (error) {
            break;
        }

    } while (0);

    cbmlibmisc_strfree(driverLocation);

    FUNC_LEAVE_BOOL(error);
}

/*! \brief @@@@@ \todo document

 \param Context

 \return
*/
BOOL CBMAPIDECL
opencbm_plugin_install_do_uninstall(void * Context)
{
    BOOL error = TRUE;

    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "-- xa1541.uninstall" ));

    do {
        cbm_install_parameter_plugin_t * pluginInstallParameter = Context;
        xa1541_parameter_t *parameter;
        
        if (pluginInstallParameter == NULL || pluginInstallParameter->OptionMemory == NULL) {
            break;
        }

        parameter = pluginInstallParameter->OptionMemory;


        if (!CbmCheckPresence(OPENCBM_DRIVERNAME))
        {
            printf("No driver installed, cannot remove.\n");
            error = FALSE;
            break;
        }

        printf("REMOVING driver...\n");

        CbmRemove(OPENCBM_DRIVERNAME);

        error = FALSE;

    } while (0);

    FUNC_LEAVE_BOOL(error);
}

/*! \brief @@@@@ \todo document

 \param Data

 \param Destination

 \return
*/
unsigned int CBMAPIDECL
opencbm_plugin_install_get_needed_files(CbmPluginInstallProcessCommandlineData_t * Data, opencbm_plugin_install_neededfiles_t * Destination)
{
    unsigned int size = sizeof(NeededFilesXA1541);
    xa1541_parameter_t *parameter = Data->OptionMemory;

    FUNC_ENTER();

    do {
        if (NULL == Destination) {
            break;
        }

        memcpy(Destination, NeededFilesXA1541, size);

    } while (0);

    FUNC_LEAVE_UINT(size);
}
