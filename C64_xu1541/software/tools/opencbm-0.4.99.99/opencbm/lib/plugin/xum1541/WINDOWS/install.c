/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2008 Spiro Trikaliotis
*/

/*! ************************************************************** 
** \file lib/plugin/xum1541/WINDOWS/install.c \n
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
    #define DBG_PROGNAME "OPENCBM-XUM1541.DLL"
#endif // #ifndef DBG_PROGNAME

#include "debug.h"

#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>

#include "cbmioctl.h"
#include "libmisc.h"
#include "version.h"

#define OPENCBM_PLUGIN 1 /*!< \brief mark: we are exporting plugin functions */

#include "archlib.h"
#include "archlib-windows.h"


/*! \brief The parameter which are given on the command-line */
typedef
struct xu1541_parameter_s
{
    /*! The type of the OS version */
    osversion_t OsVersion;

} xu1541_parameter_t;


static const struct option longopts[] =
{
    { "help",       no_argument,       NULL, 'h' },
    { "version",    no_argument,       NULL, 'V' },

    { NULL,         0,                 NULL, 0   }
};

static const char shortopts[] = "-hV";

static const char usagetext[] =
            "\n\nUsage: instcbm [options] xum1541 [plugin-options]\n"
            "Install the XUM1541 plugin and driver on the system, or remove it.\n"
            "\n"
            "plugin-options is one of:\n"
            "  -h, --help       display this help and exit\n"
            "  -V, --version    display version information about cbm4win\n"
            "\n";


static opencbm_plugin_install_neededfiles_t NeededFilesXUM1541[] = 
{
    { SYSTEM_DIR, "opencbm-xum1541.dll", NULL },
    { LIST_END,   "",                   NULL }
};

/*! \brief \internal Print out a hint how to get help */

static void
hint(void)
{
    fprintf(stderr, "Try \"instcbm xum1541 --help\" for more information.\n");
}


/*! \brief \internal Output version information of instcbm */

static VOID
version(VOID)
{
    printf("opencbm xum1541 plugin version " /* OPENCBM_VERSION */ ", built on " __DATE__ " at " __TIME__ "\n");
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

    xu1541_parameter_t *parameter = Data->OptionMemory;

    BOOL quitLocalProcessing = FALSE;

    FUNC_ENTER();

    DBG_ASSERT(Data);


    do {
        int c;

        /* special handling for first call: Determine the length of the OptionMemory to be allocated */

        if (Data->Argc == 0) {
            error = sizeof(xu1541_parameter_t);
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

    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "-- xum1541.install" ));

    do {
        error = FALSE;
    } while (0);

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

    DBG_PRINT((DBG_PREFIX "-- xum1541.uninstall" ));

    do {
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
    unsigned int size = sizeof(NeededFilesXUM1541);
    xu1541_parameter_t *parameter = Data->OptionMemory;

    FUNC_ENTER();

    do {
        if (NULL == Destination) {
            break;
        }

        memcpy(Destination, NeededFilesXUM1541, size);

    } while (0);

    FUNC_LEAVE_UINT(size);
}
