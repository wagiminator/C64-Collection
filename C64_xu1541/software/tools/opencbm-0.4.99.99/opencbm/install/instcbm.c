/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004, 2008, 2009 Spiro Trikaliotis
 *  Additions Copyright 2013   Arnd Menge
 *
 */

/*! ************************************************************** 
** \file instcbm.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Program to install and uninstall the OPENCBM driver
**
****************************************************************/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <direct.h>

#include "arch.h"
#include "cbmioctl.h"
#include "i_opencbm.h"
#include "libmisc.h"
#include "opencbm-plugin.h"
#include "opencbm-plugin-install.h"
#include "version.h"

#include <getopt.h>

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#define DBG_PROGNAME "INSTCBM.EXE"

/*! This file is "like" debug.c, that is, define some variables */
#define DBG_IS_DEBUG_C

/*! GetOsVersion() uses the following functions */
#define KERNEL32_DLL "kernel32.dll"
#define KERNEL32API_GetNativeSystemInfo "GetNativeSystemInfo"
typedef void (WINAPI* PGetNativeSystemInfo)(LPSYSTEM_INFO);

#include "debug.h"

#include "instcbm.h"

// defines for older DDKs
#ifndef VER_SUITE_WH_SERVER
#define VER_SUITE_WH_SERVER 0x00008000
#endif

#ifndef SM_SERVERR2
#define SM_SERVERR2 89
#endif

#ifndef VER_SUITE_STORAGE_SERVER
#define VER_SUITE_STORAGE_SERVER 0x00002000
#endif

static BOOL
SelfInitGenericOpenCBM(HMODULE OpenCbmDllHandle, const char * DefaultPluginname, BOOL NoCopy);

/*! \internal \brief Check if we have the needed access rights

 This function checks if we have the needed access rights for
 installing the driver on the machine.

 \param Parameter
   Pointer to parameter_t struct which contains the
   description of the parameters given on the command-line.

 \return 
   Return value which will be given on return from main()
   That is, 0 on success, everything else indicates an error.
*/
static BOOL
NeededAccessRights(VOID)
{
    SC_HANDLE scManager;

    FUNC_ENTER();

    // Check if we have arbitrary execute rights.
    // For this, open the service control manager

    scManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (scManager)
    {
        CloseServiceHandle(scManager);
    }

    FUNC_LEAVE_BOOL(scManager ? TRUE : FALSE);
}

/*! \brief \internal Return processor architecture as string

 This function returns the current processor architecture as string.
 wProcessorArchitecture is a SYSTEM_INFO member, determined by
 GetNativeSystemInfo or GetSystemInfo.

 \return 
   Returns the current processor architecture as string.
*/
void GetProcessorArchitecture(WORD wProcessorArchitecture, char szOS[])
{
    if (wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64)
        strcat(szOS, TEXT(" IA64"));
    else if (wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        strcat(szOS, TEXT(" x64"));
    else if (wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        strcat(szOS, TEXT(" x86"));
    else
        strcat(szOS, TEXT(" (?)"));
}

/*! \brief \internal Check out the operating system version

 This function checks out the operating system version we are
 running on.

 \return 
   The operating system version.
*/
static osversion_t
GetOsVersion(VOID)
{
    SYSTEM_INFO si;
    OSVERSIONINFOEX ovi;
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 0, 0, 0, 0, {0}, 0, 0 };
    osversion_t retValue;
    char szOS[1024] = {0};
    PGetNativeSystemInfo pGetNativeSystemInfo;
    ULONGLONG ConditionMask;
    BOOL isServer;

    FUNC_ENTER();

    // If we do not find anything, it will be an unsupported OS version

    retValue = WINUNSUPPORTED;

    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    ZeroMemory(&ovi, sizeof(OSVERSIONINFOEX));

    pGetNativeSystemInfo = (PGetNativeSystemInfo) GetProcAddress(GetModuleHandle(KERNEL32_DLL), KERNEL32API_GetNativeSystemInfo);
    if (pGetNativeSystemInfo != NULL)
        pGetNativeSystemInfo(&si);
    else
        GetSystemInfo(&si);

    ovi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    // Check out the operating system version

    if (GetVersionEx((OSVERSIONINFO*)&ovi))
    {
        DBGDO(char *platform = "";)

        strcpy(szOS, TEXT("Microsoft "));

        switch (ovi.dwPlatformId)
        {
        case VER_PLATFORM_WIN32s:
            DBGDO(platform = "WIN32s (huh?)";)
            break;

        case VER_PLATFORM_WIN32_WINDOWS:
            DBGDO(platform = "Win 95/98/Me";)
            break;

        case VER_PLATFORM_WIN32_NT:
            DBGDO(platform = "Win NT/2K/XP";)

            switch (ovi.dwMajorVersion)
            {
            case 0: case 1: case 2: /* SHOULD NOT OCCUR AT ALL! */
                /* unsupported */
                retValue = WINUNSUPPORTED;
                DBG_PRINT((DBG_PREFIX "Running on NT %u.%02u, that is, *BEFORE* 3.x! "
                    "Something is going wrong here...",
                    ovi.dwMajorVersion, ovi.dwMinorVersion));
                fprintf(stderr, "You're using Windows NT %u.%02u. THESE VERSIONS SHOULD NOT EXIST!\n",
                    ovi.dwMajorVersion, ovi.dwMinorVersion);
                break;

            case 3:
                /* NT 3.x versions are untested */
                retValue = WINNT3;
                DBG_PRINT((DBG_PREFIX "Running on NT 3.x!"));
                printf("You're using Windows NT 3.%02u. These versions are untested, but they might work.\n"
                    "If it works for you, I would be very happy if you contact me and tell me!\n",
                    ovi.dwMinorVersion);
                break;

            case 4:
                retValue = WINNT4;
                break;

            case 5:
                switch (ovi.dwMinorVersion)
                {
                case 0:
                    strcat(szOS, TEXT("Windows 2000"));
                    if (ovi.wProductType != VER_NT_WORKSTATION)
                        strcat(szOS, TEXT(" Server"));
                    GetProcessorArchitecture(si.wProcessorArchitecture, szOS);
                    retValue = WIN2000;
                    break;
                case 1:
                    strcat(szOS, TEXT("Windows XP"));
                    GetProcessorArchitecture(si.wProcessorArchitecture, szOS);
                    retValue = WINXP;
                    break;
                case 2:
                    if (GetSystemMetrics(SM_SERVERR2))
                        strcat(szOS, TEXT("Windows Server 2003 R2"));
                    else if (ovi.wSuiteMask & VER_SUITE_STORAGE_SERVER)
                        strcat(szOS, TEXT("Windows Storage Server 2003"));
                    else if (ovi.wSuiteMask & VER_SUITE_WH_SERVER)
                        strcat(szOS, TEXT("Windows Home Server"));
                    else if (ovi.wProductType == VER_NT_WORKSTATION && (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64))
                        strcat(szOS, TEXT("Windows XP Professional x64"));
                    else
                        strcat(szOS, TEXT("Windows Server 2003"));

                    if (ovi.wProductType != VER_NT_WORKSTATION)
                        GetProcessorArchitecture(si.wProcessorArchitecture, szOS);

                    retValue = WINXP; // Set to WINXP for now.
                    break;
                default:
                    retValue = WINNEWER;
                }
                break;

            case 6:
                if ((ovi.dwMinorVersion == 0) || (ovi.dwMinorVersion == 1) || (ovi.dwMinorVersion == 2))
                {
                    if (ovi.dwMinorVersion == 0)
                    {
                        if (ovi.wProductType == VER_NT_WORKSTATION)
                            strcat(szOS, TEXT("Windows Vista"));
                        else
                            strcat(szOS, TEXT("Windows Server 2008"));
                        retValue = WINVISTA;
                    }
                    else if (ovi.dwMinorVersion == 1)
                    {
                        if (ovi.wProductType == VER_NT_WORKSTATION)
                            strcat(szOS, TEXT("Windows 7"));
                        else
                            strcat(szOS, TEXT("Windows Server 2008 R2"));
                        retValue = WIN7;
                    }
                    else if (ovi.dwMinorVersion == 2)
                    {
#if _MSC_VER <= 1200
                        /*
                         * as MSVC6 does not know about the functions
                         * VerifyVersionInfo() and VerSetConditionMask(), skip this test
                         */
                        ConditionMask = 0;
                        isServer = FALSE;
                        retValue = WINNEWER;
#else
                        osvi.wProductType = VER_NT_WORKSTATION;
                        ConditionMask = VerSetConditionMask( 0, VER_PRODUCT_TYPE, VER_EQUAL );
                        isServer = !VerifyVersionInfoW(&osvi, VER_PRODUCT_TYPE, ConditionMask);

                        osvi.dwMajorVersion = 6;
                        osvi.dwMinorVersion = 3;
                        osvi.dwBuildNumber = 9600;

                        ConditionMask = VerSetConditionMask(
                            VerSetConditionMask(
                                VerSetConditionMask(0, VER_MAJORVERSION, VER_EQUAL),
                                VER_MINORVERSION, VER_EQUAL),
                            VER_BUILDNUMBER, VER_EQUAL);

                        if (VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, ConditionMask))
                        {
                            if (!isServer)
                                strcat(szOS, TEXT("Windows 8.1"));
                            else
                                strcat(szOS, TEXT("Windows Server 2012 R2"));

                            retValue = WIN8_1;
                        }

                        if (retValue == WINUNSUPPORTED)
                        {
                            if (!isServer)
                                strcat(szOS, TEXT("Windows 8"));
                            else
                                strcat(szOS, TEXT("Windows Server 2012"));

                            retValue = WIN8;
                        }
#endif
                    }

                    GetProcessorArchitecture(si.wProcessorArchitecture, szOS);
                    break;
                }

                /* else */
                /* FALL THROUGH */

            default:
                // This is a version of Windows we do not know; anyway, since
                // it is NT based, and it's major version is >= 7, we support it
                // anyway!

                retValue = WINNEWER;

                DBG_PRINT((DBG_PREFIX "Running on NT %u.%02u.",
                    ovi.dwMajorVersion, ovi.dwMinorVersion));
                fprintf(stderr, "You're using Windows NT %u.%02u.\n"
                    "I do not know it, but OpenCBM should work, anyway.\n",
                    ovi.dwMajorVersion, ovi.dwMinorVersion);
                break;
            }
            break;

        default:
            DBGDO(platform = "unknown (huh?)";)
            break;
        }

        DBG_SUCCESS((DBG_PREFIX "OS VERSION: %u.%u.%u %s (%s)",
            ovi.dwMajorVersion, ovi.dwMinorVersion, ovi.dwBuildNumber,
            platform, ovi.szCSDVersion));

//        printf("OS: %s %s\n", szOS, NeededAccessRights() ? "[admin]" : "[no admin rights]");

    }

    FUNC_LEAVE_TYPE(retValue, osversion_t, "%u");
}

/*! \brief \internal Output version information of instcbm */
static VOID
version(VOID)
{
    FUNC_ENTER();

    printf("instcbm Version " OPENCBM_VERSION ", built on " __DATE__ " at " __TIME__ "\n");

    FUNC_LEAVE();
}

/*! \brief \internal Print out the help screen */
static VOID
usage(VOID)
{
    FUNC_ENTER();

    version();

    printf("\nUsage: instcbm [options] pluginname [plugin-options] [pluginname [plugin-options] ...]\n"
            "Install OpenCBM and one or more plugins on the system, or remove it.\n"
            "\n"
            "pluginname is the name of the plugin to install. Any subsequent option is\n"
            "passed to the plugin, and it will have a different meaning! See the\n"
            "description of each plugin for details.\n"
            "\n"
            "Options:\n"
            "  -h, --help       display this help and exit\n"
            "  -V, --version    display version information about OpenCBM\n"
            "  -r, --remove    remove (uninstall) OpenCBM or a plugin\n"
            "  -u, --update    update parameters if driver is already installed.\n"
            "  -c, --check     only check if the installation is ok\n"
            "  -n, --nocopy    do not copy the driver files into the system directory\n"
            "\n");
    FUNC_LEAVE();
}

/*! \brief \internal Print out a hint how to get help */
static VOID
hint(const char *s)
{
    fprintf(stderr, "Try `%s' --help for more information.\n", s);
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

/*! \brief Make sure that only one parameter is given that results in command execution

 \param Parameter
    The outcome of the parsing of the command-line parameters so far

 \param ExecutableName
    The name of the executable (for an error message, if this function returns TRUE)

 \return
    TRUE if there is already an executing command in Parameter,
    else FALSE.

  Running the program with colliding parameters (for example, install und remove)
  is not possible. When processing this parameters, this function is called.
  
  This function ensures that it can be called only once without an error
  (returning FALSE).

  For the second (or any subsequent) call, this function gives an error message,
  and returns TRUE.
*/
static BOOL
enforceOnlyOneExecutingCommand(cbm_install_parameter_t *Parameter, const char *ExecutableName)
{
    BOOL error = FALSE;

    FUNC_ENTER();

    if (Parameter->ExecuteParameterGiven)
    {
        error = TRUE;
        printf("Colliding parameters were given, aborting!");
        hint(ExecutableName);
    }
    Parameter->ExecuteParameterGiven = TRUE;

    FUNC_LEAVE_BOOL(error);
}

/* ------------------------------------------------------------------------------------- */

/*! \internal \brief Process the command line arguments

 This function processes the command line arguments.

 \param Argc
   The count of arguments, as given to main().

 \param Argv
   The arguments vector, as given to main().

 \param Parameter
   Pointer to parameter_t struct which will contain the
   description of the parameters on return

 \return 
   TRUE on error, else FALSE.
*/
static BOOL
processargs(int Argc, char **Argv, cbm_install_parameter_t *Parameter)
{
    BOOL error;
    int option;

    struct option longopts[] =
    {
        { "help",       no_argument,       NULL, 'h' },
        { "version",    no_argument,       NULL, 'V' },
        { "remove",     no_argument,       NULL, 'r' },
        { "purge",      no_argument,       NULL, 'p' },
        { "update",     no_argument,       NULL, 'u' },
        { "check",      no_argument,       NULL, 'c' },
        { "nocopy",     no_argument,       NULL, 'n' },
        { "adapter",    required_argument, NULL, '@' },
#if DBG
        { "debugflags", required_argument, NULL, 'D' },
        { "buffer",     no_argument,       NULL, 'B' },
#endif // #if DBG
        { NULL,         0,                 NULL, 0   }
    };

    const char shortopts[] = "-hVrpucn@:"

#if DBG
                             "D:B"
#endif // #if DBG
                             ;

    BOOL quitGlobalProcessing = FALSE;

    FUNC_ENTER();

    error = FALSE;

    Parameter->Install = TRUE;


    DBG_ASSERT(Parameter);

    while ( ! quitGlobalProcessing && (option = getopt_long(Argc, Argv, shortopts, longopts, NULL)) != -1)
    {
        switch (option)
        {
        case 'h':
            usage();
            Parameter->NoExecute = TRUE;
            Parameter->Install = FALSE;
            break;

        case 'V':
            version();
            Parameter->NoExecute = TRUE;
            Parameter->Install = FALSE;
            break;

        case 'r':
            error = enforceOnlyOneExecutingCommand(Parameter, Argv[0]) || error;
            Parameter->Remove = TRUE;
            Parameter->Install = FALSE;
            break;

        case 'p':
            if (Parameter->Remove) {
                Parameter->Purge = TRUE;
            }
            else {
                error = TRUE;
                printf("--purge only allowed after --remove, aborting!");
                hint(Argv[0]);
            }
            break;

        case 'u':
            Parameter->Update = TRUE;
            Parameter->Install = FALSE;
            break;

        case 'c':
            Parameter->CheckInstall = TRUE;
            Parameter->Install = FALSE;
            break;

        case 'n':
            Parameter->NoCopy = TRUE;
            break;

        case '@': /* choose adapter */
            if (Parameter->DefaultAdapter == NULL) {
                Parameter->DefaultAdapter = cbmlibmisc_strdup(optarg);
            }
            else {
                printf("--adapter/-@ given more than once.");
                hint(Argv[0]);
                exit(1);
            }
            break;
#if DBG
        case 'D':
            {
                PCHAR next;

                error = processNumber(optarg, &next, &Parameter->DebugFlagsDriverWereGiven,
                    &Parameter->DebugFlagsDriver);

                if (!error && next && *next)
                {
                    error = processNumber(next, &next, &Parameter->DebugFlagsDllWereGiven,
                        &Parameter->DebugFlagsDll);
                }

                if (!error && next && *next)
                {
                    error = processNumber(next, NULL, &Parameter->DebugFlagsInstallWereGiven,
                        &Parameter->DebugFlagsInstall);

                    if (!error && Parameter->DebugFlagsInstallWereGiven)
                    {
                        DbgFlags = Parameter->DebugFlagsInstall;
                    }
                }
                DBG_PRINT((DBG_PREFIX "error = %s, 1st = %08x (%s), 2nd = %08x (%s), 3rd = %08x (%s)",
                    error ? "TRUE" : "FALSE", 
                    Parameter->DebugFlagsDriver, Parameter->DebugFlagsDriverWereGiven ? "TRUE" : "FALSE", 
                    Parameter->DebugFlagsDll, Parameter->DebugFlagsDllWereGiven ? "TRUE" : "FALSE",
                    Parameter->DebugFlagsInstall, Parameter->DebugFlagsInstallWereGiven ? "TRUE" : "FALSE"));
            }
            break;

        case 'B':
            Parameter->OutputDebuggingBuffer = TRUE;
            Parameter->NoExecute = TRUE;
            Parameter->Install = FALSE;
            break;

#endif // #if DBG

        case 1: /* This is not a parameter, thus, it is the name of the plugin to process */
            quitGlobalProcessing = TRUE;
            --optind;
            break;

        default:
            error = TRUE;
            hint(Argv[0]);
            break;
        }
    }


    while (! error && Argv[optind++]) {
        error = error || ProcessPluginCommandlineAndAddIt(Parameter, Argv[optind - 1], Argc, Argv);
    }

    if (Parameter->PluginList == NULL && ! error) {

        Parameter->NoExplicitPluginGiven = TRUE;

        if (Parameter->Install) {
            printf("ERROR: No plugin given.\n\n"
                "Since version 0.5.0 of OpenCBM, instcbm supports different plugins\n"
                "that correspond to different cables, connected via parallel port\n"
                "or via USB.\n\n"
                "As instcbm cannot guess which cable are available, you have to\n"
                "specifiy which plugin (='cable support software') you want to use.\n"
                "\n"
                "You have to specify it on the command-line.\n\n"
                "Available options are:\n"
                "\txa1541\tfor XA1541, XM1541, XAP1541 and XMP1541 cables (parallel port).\n"
                "\txu1541\tfor XU1541 cable (cheap USB solution, with restrictions)\n"
                "\txum1541\tfor XUM1541 cable (include ZoomFloppy).\n"
                "\n"
                "If you are upgrading from OpenCBM 0.4.2 or earlier, with the same cable\n"
                "as before, you most probably want to use the command line:\n\tinstcbm xa1541\n"
                "\n"
                "If you have the ZoomFloppy, you most probably want the command line\n"
                "\tinstcbm xum1541\n"
                );
            error = 1;
            /* error = get_all_plugins(Parameter); */
        }
        else {
            error = get_all_installed_plugins(Parameter);
        }
    }

    if (!error && Parameter->DefaultAdapter && ! Parameter->Update && ! Parameter->Install ) {
        printf("--adapter/-@ only allowed when installing or updating, aborting!");
        hint(Argv[0]);
        error = 1;
    }

    if (!error && !Parameter->PluginList) {
        printf("No plugins detected, have to abort...\n");
        error = 1;
    }

    if (!error && Parameter->Install && ! Parameter->DefaultAdapter && Parameter->PluginList->Name) {
        /* generate a default adapter: The first one given */
        Parameter->DefaultAdapter = cbmlibmisc_strdup(Parameter->PluginList->Name);
    }

/*! \todo is this needed anymore?
    if (!error && Parameter->Install && Parameter->PluginList == NULL) {
        error = TRUE;
    }
*/

    FUNC_LEAVE_BOOL(error);
}

/*! \internal \brief Copy a file from a path to another

 This function copies a file from a source directory to a
 destination directory.

 \param SourcePath
   The path from where to copy the file.
   The path has to be terminated with a backslash ("\").

 \param DestPath
   The path where to copy the file to.
   The path has to be terminated with a backslash ("\").

 \param Filename
   The name of the file to copy.

 \return
   0 on success, else 1.
*/
static int
CopyFileToNewPath(const char *SourcePath, const char *DestPath, const char *Filename)
{
    char *sourceFile = NULL;
    char *destFile = NULL;
    int error = 0;

    FUNC_ENTER();
    
    sourceFile = cbmlibmisc_strcat(SourcePath, Filename);
    destFile = cbmlibmisc_strcat(DestPath, Filename);

    if (sourceFile && destFile)
    {
        DBG_PRINT((DBG_PREFIX "Copying '%s' to '%s'", sourceFile, destFile));

        printf("Copying '%s' to '%s'", sourceFile, destFile);
        if (!CopyFile(sourceFile, destFile, FALSE))
        {
            error = 1;
            DBG_PRINT((DBG_PREFIX "--> FAILED!" ));
            printf(" FAILED!\n");
        }
        else
        {
            printf("\n");
        }
    }
    else
    {
        DBG_ERROR((DBG_PREFIX "Error allocating memory buffers for copying '%s'!", Filename));
        fprintf(stderr, "Error allocating memory buffers for copying '%s'.\n", Filename);
    }

    if (sourceFile) {
        cbmlibmisc_strfree(sourceFile);
    }

    if (destFile) {
        cbmlibmisc_strfree(destFile);
    }

    FUNC_LEAVE_INT(error);
}

/*! \brief Get the current (working) directory

 \return
    Pointer to an allocated memory buffer that contains
    the working directory, or NULL if an error occurred.
    The directory has a trailing backslash at its end.

 \remark
   The string must be freed with cbmlibmisc_strfree().
*/
static char *
GetWorkingDirectory(void)
{
    char * tmpPathString;
    unsigned int stringLength;
    BOOL error = TRUE;

    FUNC_ENTER();

    do {

        //
        // determine the length of the string
        //

        stringLength = GetCurrentDirectory(0, NULL);

        //
        // Allocate memory for the working directory
        //

        tmpPathString = cbmlibmisc_stralloc(stringLength + 1);

        if (tmpPathString == NULL)
        {
            break;
        }

        //
        // determine the working directory
        //

        if (GetCurrentDirectory(stringLength, tmpPathString) == 0)
        {
            DBG_PRINT((DBG_PREFIX "Could not determine the current working directory!"));
            printf("Could not determine the current working directory!\n");
            break;
        }

        DBG_ASSERT(strlen(tmpPathString) < stringLength);

        strcat(tmpPathString, "\\");

        error = FALSE;

    } while (0);

    if (error)
    {
        cbmlibmisc_strfree(tmpPathString);
        tmpPathString = NULL;
    }

    FUNC_LEAVE_STRING(tmpPathString);
}

/*! \brief Get the Windows system directory

 \return
    Pointer to an allocated memory buffer that contains
    the Windows system directory, or NULL if an error occurred.
    The directory has a trailing backslash at its end.

 \remark
   The string must be freed with cbmlibmisc_strfree().
*/
static char *
GetWindowsSystemDirectory(void)
{
#ifdef USE_FAKE_WIN_DIRECTORY_AS_COPY_TARGET
    FUNC_LEAVE_STRING(cbmlibmisc_strdup(USE_FAKE_WIN_DIRECTORY_AS_COPY_TARGET "\\System32\\"));
#else
    char * tmpPathString;
    unsigned int stringLength;
    BOOL error = TRUE;

    FUNC_ENTER();

    do {

        //
        // determine the length of the string
        //

        stringLength = GetSystemDirectory(NULL, 0);

        //
        // Allocate memory for the system directory
        //

        tmpPathString = cbmlibmisc_stralloc(stringLength + 1);

        if (tmpPathString == NULL)
        {
            break;
        }

        //
        // determine the system directory
        //

        if (GetSystemDirectory(tmpPathString, stringLength) == 0)
        {
            DBG_PRINT((DBG_PREFIX "Could not determine the current windows system directory!"));
            printf("Could not determine the current windows system directory!\n");
            break;
        }

        DBG_ASSERT(strlen(tmpPathString) < stringLength);

        strcat(tmpPathString, "\\");

        error = FALSE;

    } while (0);

    if (error)
    {
        cbmlibmisc_strfree(tmpPathString);
        tmpPathString = NULL;
    }

    FUNC_LEAVE_STRING(tmpPathString);
#endif
}

/*! \brief Get the Windows driver directory

 \return
    Pointer to an allocated memory buffer that contains
    the Windows driver directory, or NULL if an error occurred.
    The directory has a trailing backslash at its end.

 \remark
   The string must be freed with cbmlibmisc_strfree().
*/
static char *
GetWindowsDriverDirectory(void)
{
    char * tmpPathString = NULL;
    char * driverPathString = NULL;

    FUNC_ENTER();

    do {
        tmpPathString = GetWindowsSystemDirectory();
        if (tmpPathString == NULL) {
            break;
        }

        driverPathString = cbmlibmisc_strcat(tmpPathString, "DRIVERS\\");
    } while (0);

    cbmlibmisc_strfree(tmpPathString);

    FUNC_LEAVE_STRING(driverPathString);
}

/*! \internal \brief Update driver settings

 This function updates settings for the already installed driver.

 \param Parameter
   Pointer to parameter_t struct which contains the
   description of the parameters given on the command-line.

 \return 
   Return value which will be given on return from main()
   That is, 0 on success, everything else indicates an error.
*/
static int
UpdateOpenCBM(cbm_install_parameter_t *Parameter)
{
    int error = 0;
    HMODULE openCbmDllHandle = NULL;

    FUNC_ENTER();

    do {
        openCbmDllHandle = LoadLocalOpenCBMDll();
        if (openCbmDllHandle  == NULL) {
            DBG_PRINT((DBG_PREFIX "Could not open the OpenCBM DLL."));
            fprintf(stderr, "Could not open the OpenCBM DLL.");
            break;
        }

        error = SelfInitGenericOpenCBM(openCbmDllHandle,
                                       Parameter->DefaultAdapter,
                                       Parameter->NoCopy);
        if (error) {
            break;
        }

    } while (0);

    if (openCbmDllHandle) {
        FreeLibrary(openCbmDllHandle);
    }

   FUNC_LEAVE_INT(error);
}

/*! \brief @@@@@ \todo document */
static opencbm_plugin_install_neededfiles_t NeededFilesGeneric[] = 
{
    { SYSTEM_DIR, "opencbm.dll" },
#ifdef _X86_
    { SYSTEM_DIR, "opencbmvdd.dll" },
#endif // #ifdef _X86_
    { LIST_END,   "" }
};

/*! \brief Determine in which directory to copy a needed file

 \param NeededFile
   Structure that describes the needed file

 \param PluginName
   The name of the plugin

 \param WorkingDirectory
   A string containing the current directory

 \param SystemDirectory
   A string containing the system directory

 \param DriverDirectory
   A string containing the driver directory

 \return
   Pointer to an allocated buffer that contains the
   destination directory of this needed file.
   The directory ends with a trailing backslash.

 \remark
   The string must be freed with cbmlibmisc_strfree().
*/
static char *
GetPathForNeededFile(opencbm_plugin_install_neededfiles_t * NeededFile, const char * PluginName, const char * WorkingDirectory, const char * SystemDirectory, const char * DriverDirectory)
{
    char * filepath = NULL;

    FUNC_ENTER();

    do {
        /* if we already have a copy of the path, use that */

        if (NeededFile->FileLocationString != NULL) {
            filepath = cbmlibmisc_strdup(NeededFile->FileLocationString);
            break;
        }

        switch (NeededFile->FileLocation)
        {
        case LOCAL_PLUGIN_DIR:
            if (NULL != PluginName) {
                filepath = cbmlibmisc_strcat(WorkingDirectory, PluginName);
            }
            break;

        case LOCAL_DIR:
            filepath = cbmlibmisc_strdup(WorkingDirectory);
            break;

        case SYSTEM_DIR:
            filepath = cbmlibmisc_strdup(SystemDirectory);
            break;

        case DRIVER_DIR:
            filepath = cbmlibmisc_strdup(DriverDirectory);
            break;

        default:
            DBG_ASSERT(("wrong enum in neededfiles array!", 0));
            break;
        }

        if (filepath == NULL)
        {
            DBG_ASSERT(("internal error on building file path!", 0));
        }

        /* take a copy of the path so we can refer to it later */

        NeededFile->FileLocationString = cbmlibmisc_strdup(filepath);

    } while (0);

    FUNC_LEAVE_STRING(filepath);
}

/*! \brief Determine the destination filename of a needed file

 \param NeededFile
   Structure that describes the needed file

 \param PluginName
   The name of the plugin

 \param WorkingDirectory
   A string containing the current directory

 \param SystemDirectory
   A string containing the system directory

 \param DriverDirectory
   A string containing the driver directory

 \return
   Pointer to an allocated buffer that contains the
   file name with full destination directory of this needed file.

 \remark
   The string must be freed with cbmlibmisc_strfree().
*/
static char *
GetFilenameForNeededFile(opencbm_plugin_install_neededfiles_t * NeededFile, const char * PluginName, const char * WorkingDirectory, const char * SystemDirectory, const char * DriverDirectory)
{
    char * filename = NULL;
    char * path = NULL;

    FUNC_ENTER();

    path = GetPathForNeededFile(NeededFile, PluginName, WorkingDirectory, SystemDirectory, DriverDirectory);

    if (NULL != path) 
    {
        filename = cbmlibmisc_strcat(path, NeededFile->Filename);
        cbmlibmisc_strfree(path);
    }

    if (filename == NULL)
    {
        DBG_ASSERT(("internal error on building file name!", 0));
    }

    FUNC_LEAVE_STRING(filename);
}

/*! \brief Check if a set of files is currently installed

 \param NeededFiles
   Pointer to an array of needed files. This function
   will return TRUE only if *all* files are available.

 \return
   TRUE if all the files are installed, else FALSE.

*/
static BOOL
AreNeededFilesPresent(opencbm_plugin_install_neededfiles_t NeededFiles[])
{
    BOOL isPresent = FALSE;

    char * workingDirectory = NULL;
    char * systemDirectory = NULL;
    char * driverDirectory = NULL;

    opencbm_plugin_install_neededfiles_t * neededfiles;

    DWORD fileattributes;

    FUNC_ENTER();

    do {
        workingDirectory = GetWorkingDirectory();
        systemDirectory = GetWindowsSystemDirectory();
        driverDirectory = GetWindowsDriverDirectory();

        if ( ! workingDirectory || ! systemDirectory || ! driverDirectory)
            break;

        printf("Working directory = '%s',\n"
               "system  directory = '%s',\n"
               "driver  directory = '%s'.\n",
               workingDirectory, systemDirectory, driverDirectory);

        //
        // Check if the necessary files all exist
        //

        isPresent = TRUE; // assume all files are available

        for (neededfiles = NeededFiles; neededfiles->FileLocation != LIST_END; neededfiles++)
        {
            char * filename;
            
            filename = GetFilenameForNeededFile(neededfiles, NULL, workingDirectory, systemDirectory, driverDirectory);

            fileattributes = GetFileAttributes(filename);

            /*
             * Yes, this constant 0xFFFFFFFF is defined for the function failing,
             * not ((DWORD)-1) as one might expect.
             * Thus, this most probably holds even for 64 bit platforms
             */
            if (fileattributes == 0xFFFFFFFF)
            {
                DBG_PRINT((DBG_PREFIX "File '%s' not found.", filename));
                isPresent = FALSE;
            }

            cbmlibmisc_strfree(filename);
        }
    } while (0);

    cbmlibmisc_strfree(workingDirectory);
    cbmlibmisc_strfree(systemDirectory);
    cbmlibmisc_strfree(driverDirectory);

    FUNC_LEAVE_BOOL(isPresent);
}

/*! \brief Determine if generic OpenCBM framework is installed

 \return
    TRUE if all needed files from the OpenCBM framework are installed,
    else FALSE.
*/
static BOOL
IsPresentGenericOpenCBM(void)
{
    FUNC_ENTER();
    FUNC_LEAVE_BOOL(AreNeededFilesPresent(NeededFilesGeneric));
}

/*! \brief Type of the callback function for HandleNeededFiles()

 \param DestinationPath
   The destination path of the installed file

 \param File
   The name of the destination file.

 \return
   TRUE on error, else FALSE.
*/
typedef BOOL HandleNeededFilesCallback_t(const char * DestinationPath, const char * File);

/*! \brief Iterate through all the needed files and call a callback function for each of it

 \param NeededFiles
   Pointer to an array describing the needed files

 \param PathToInstalledPluginFile
   On return, gets the path to the copied plugin DLL file.
   This can be used to open that version of the DLL in the sequel.

 \param Callback
   Pointer to the callback function to call for each file

 \return
   TRUE on error, else FALSE

 \remark
   The returned PathToInstalledPluginFile is determined from
   NeededFiles[0]

*/
static BOOL
HandleNeededFiles(opencbm_plugin_install_neededfiles_t NeededFiles[], const char ** PathToInstalledPluginFile, HandleNeededFilesCallback_t * Callback)
{
    char * workingDirectory = NULL;
    char * systemDirectory = NULL;
    char * driverDirectory = NULL;

    char * destinationPath = NULL;

    BOOL error = FALSE;

    opencbm_plugin_install_neededfiles_t * neededfiles;

    FUNC_ENTER();

    if (PathToInstalledPluginFile != NULL)
    {
        *PathToInstalledPluginFile = NULL;
    }

    do
    {
        workingDirectory = GetWorkingDirectory();
        systemDirectory = GetWindowsSystemDirectory();
        driverDirectory = GetWindowsDriverDirectory();

        for (neededfiles = NeededFiles; (neededfiles->FileLocation != LIST_END) && ! error; neededfiles++)
        {
            destinationPath = GetPathForNeededFile(neededfiles, NULL, workingDirectory, systemDirectory, driverDirectory);

            error = Callback(destinationPath, neededfiles->Filename);

            if (PathToInstalledPluginFile && *PathToInstalledPluginFile == NULL) {
                *PathToInstalledPluginFile = cbmlibmisc_strcat(destinationPath, neededfiles->Filename);
            }

            cbmlibmisc_strfree(destinationPath);

            destinationPath = NULL;

            if (error)
                break;
        }

        if (error)
            break;

    } while (0);


    cbmlibmisc_strfree(workingDirectory);
    cbmlibmisc_strfree(systemDirectory);
    cbmlibmisc_strfree(driverDirectory);

    FUNC_LEAVE_BOOL(error);
}

/*! \brief Callback for CopyNeededFiles: Copy one file to its destination

 \param DestinationPath
    Destination path of the file

 \param File
   The name of the destination file.

 \return
   TRUE on error, else FALSE.
*/
static BOOL
CopyNeededFilesCallback(const char * DestinationPath, const char * File)
{
    FUNC_ENTER();

    FUNC_LEAVE_BOOL(CopyFileToNewPath(".\\", DestinationPath, File));
}

/*! \brief Copy all needed files to their destination

 \param NeededFiles
   Pointer to an array describing the needed files

 \param PathToInstalledPluginFile
   On return, gets the path to the copied plugin DLL file.
   This can be used to open that version of the DLL in the sequel.

 \return
   TRUE on error, else FALSE

 \remark
   The returned PathToInstalledPluginFile is determined from
   NeededFiles[0]
*/
static BOOL
CopyNeededFiles(opencbm_plugin_install_neededfiles_t NeededFiles[], const char ** PathToInstalledPluginFile)
{
    FUNC_ENTER();

    FUNC_LEAVE_BOOL(HandleNeededFiles(NeededFiles, PathToInstalledPluginFile, CopyNeededFilesCallback));
}

/*! \brief @@@@@ \todo document

 \param AtSystemDirectory

 \return
*/
static HMODULE
LoadOpenCBMDll(BOOL AtSystemDirectory)
{
    char * systemDirectory = NULL;
    char * dllPath = NULL;
    HMODULE dll = NULL;

    FUNC_ENTER();

    do {
        if (AtSystemDirectory) {
            /* 
             * make sure to load the right DLL in the system directory
             */

            systemDirectory = GetWindowsSystemDirectory();
        }
        else {
            /* 
             * make sure to load the local DLL
             */

            systemDirectory = cbmlibmisc_strdup("./");
        }

        if ( systemDirectory == NULL) {
            break;
        }

        dllPath = cbmlibmisc_strcat(systemDirectory, "opencbm.dll");
        if (dllPath == NULL) {
            break;
        }

        /* now, load the correct DLL ... */

        dll = LoadLibrary(dllPath);
        if (dll == NULL) {
            break;
        }
    } while (0);

    if (dllPath != NULL) {
        cbmlibmisc_strfree(dllPath);
    }

    if (systemDirectory != NULL) {
        cbmlibmisc_strfree(systemDirectory);
    }

    FUNC_LEAVE_HMODULE(dll);
}

/*! \brief @@@@@ \todo document

 \return
*/
static HMODULE
LoadDestinationOpenCBMDll(void)
{
    FUNC_ENTER();

    FUNC_LEAVE_HMODULE(LoadOpenCBMDll(TRUE));
}

/*! \brief @@@@@ \todo document

 \return
*/
HMODULE
LoadLocalOpenCBMDll(void)
{
    FUNC_ENTER();

    FUNC_LEAVE_HMODULE(LoadOpenCBMDll(FALSE));
}

/*! \brief @@@@@ \todo document

 \param OpenCbmDllHandle

 \param DefaultPluginname

 \param NoCopy

 \return
*/
static BOOL
SelfInitGenericOpenCBM(HMODULE OpenCbmDllHandle, const char * DefaultPluginname, BOOL NoCopy)
{
    BOOL error = TRUE;
    opencbm_plugin_install_generic_t * opencbm_plugin_install_generic = NULL;
    unsigned int DefaultLocation;

    FUNC_ENTER();

    DefaultLocation = NoCopy ? INDEX_DEFAULT_FILENAME_LOCAL : INDEX_DEFAULT_FILENAME_WINDIR;

    do {
        /* ... get the address of opencbm_plugin_install_generic() ... */

        opencbm_plugin_install_generic = (void *) GetProcAddress(OpenCbmDllHandle, 
            "opencbm_plugin_install_generic");

        if (opencbm_plugin_install_generic == NULL) {
            DBG_PRINT((DBG_PREFIX "Could not get address of "
                "opencbm.dll::opencbm_plugin_install_generic()."));
            fprintf(stderr, "Could not get address of "
                "opencbm.dll::opencbm_plugin_install_generic().\n");
            break;
        }

        /* ... and execute it */
        error = opencbm_plugin_install_generic(DefaultPluginname, DefaultLocation);

    } while (0);

    FUNC_LEAVE_BOOL(error);
}

/*! \brief @@@@@ \todo document

 \param OpenCbmDllHandle

 \param DefaultPluginname

 \return
*/
static BOOL
CopyGenericOpenCBM(HMODULE OpenCbmDllHandle, cbm_install_parameter_t *Parameter)
{
    BOOL error = TRUE;

    FUNC_ENTER();

    do {
        error = CopyNeededFiles(NeededFilesGeneric, NULL);
        if (error) {
            break;
        }

        error = SelfInitGenericOpenCBM(OpenCbmDllHandle, NULL, Parameter->NoCopy);
        if (error) {
            break;
        }

    } while (0);

    FUNC_LEAVE_BOOL(error);
}

/*! \brief @@@@@ \todo document

 \param PluginName

 \param PathToPluginDllFile

 \return
*/
static HMODULE
LoadPluginDll(const char * PluginName, const char * PathToPluginDllFile)
{
    HMODULE pluginDll = NULL;

    UINT oldErrorMode;

    FUNC_ENTER();

    /*
     * Load the DLL. Make sure that we do not get a warning dialog
     * if a dependancy DLL is not found.
     */

    oldErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    pluginDll = LoadLibrary(PathToPluginDllFile);

    SetErrorMode(oldErrorMode);

    if (pluginDll == NULL) {
        fprintf(stderr, "Error loading plugin '%s' at '%s'.\n", PluginName, PathToPluginDllFile);
        DBG_ERROR((DBG_PREFIX "Error loading plugin '%s' at '%s'.", PluginName, PathToPluginDllFile));
    }

    FUNC_LEAVE_HMODULE(pluginDll);
}

/*! \brief @@@@@ \todo document

 \param PluginDll
*/
static void
FreePluginDll(HMODULE PluginDll)
{
    FUNC_ENTER();

    FreeLibrary(PluginDll);

    FUNC_LEAVE();
}

/*! \brief @@@@@ \todo document

 \param PluginDll

 \param FunctionName

 \return
*/
static void *
GetPluginFunctionAddress(HMODULE PluginDll, const char * FunctionName)
{
    void * pointer;

    FUNC_ENTER();

    pointer = GetProcAddress(PluginDll, FunctionName);

    FUNC_LEAVE_PTR(pointer, void*);
}

/*! \brief @@@@@ \todo document

 \param PluginName

 \param PathToPluginDllFile

 \param FunctionName

 \param Callback

 \param Context

 \return
*/
static BOOL
PluginExecuteFunction(const char * PluginName,
                      const char * PathToPluginDllFile,
                      const char * FunctionName,
                      BOOL (*Callback)(const char * PluginName, void * FunctionPointer, void * Context),
                      void * Context)
{
    BOOL error = TRUE;
    HMODULE pluginDll = NULL;

    FUNC_ENTER();

    do {
        void * functionPointer = NULL;

        pluginDll = LoadPluginDll(PluginName, PathToPluginDllFile);

        if (pluginDll == NULL) {
            break;
        }

        functionPointer = GetPluginFunctionAddress(pluginDll, FunctionName);

        error = Callback(PluginName, functionPointer, Context);

    } while (0);

    if (pluginDll) {
        FreePluginDll(pluginDll);
    }

    FUNC_LEAVE_BOOL(error);
}

/*! \brief @@@@@ \todo document

 \param PluginName

 \param FunctionPointer

 \param Context

 \return
*/
static BOOL
perform_opencbm_plugin_install_do_install(const char * PluginName, void * FunctionPointer, void * Context)
{
    opencbm_plugin_install_do_install_t * opencbm_plugin_install_do_install = FunctionPointer;

    BOOL error = TRUE;

    FUNC_ENTER();

    do {
        if (opencbm_plugin_install_do_install == NULL) {
            break;
        }

        error = opencbm_plugin_install_do_install(Context);

        if (error) {
            DBG_ERROR((DBG_PREFIX "Installation of plugin '%s' failed!", PluginName));
            fprintf(stderr, "Installation of plugin '%s' failed!\n", PluginName);
            break;
        }

    } while (0);

    FUNC_LEAVE_BOOL(error);
}

/*! \brief @@@@@ \todo document */
typedef
struct InstallPluginCallback_context_s {
    HMODULE OpenCbmDllHandle;           /*!< @@@@@ \todo document */
    cbm_install_parameter_t *Parameter; /*!< @@@@@ \todo document */
} InstallPluginCallback_context_t;

/*! \brief @@@@@ \todo document

 \param PluginInstallParameter

 \param Context

 \return
*/
static BOOL
InstallPluginCallback(cbm_install_parameter_plugin_t * PluginInstallParameter, void * Context)
{
    InstallPluginCallback_context_t * context = Context;

    BOOL error = TRUE;

    opencbm_plugin_install_plugin_data_t * opencbm_plugin_install_plugin_data = NULL;

    const char * pathToInstalledPluginFile = NULL;


    FUNC_ENTER();

    do {
        printf("++++ Install: '%s' with filename '%s'.\n", PluginInstallParameter->Name, PluginInstallParameter->FileName);

        if ( context->Parameter->NoCopy) {
            char * workingDirectory = GetWorkingDirectory();

            pathToInstalledPluginFile = cbmlibmisc_strcat(workingDirectory, PluginInstallParameter->FileName);
            cbmlibmisc_strfree(workingDirectory);
        }
        else {
            if ( CopyNeededFiles(PluginInstallParameter->NeededFiles, &pathToInstalledPluginFile) ) {
                break;
            }
        }

        if ( pathToInstalledPluginFile == NULL ) {
            break;
        }

        opencbm_plugin_install_plugin_data = (void *) GetProcAddress(context->OpenCbmDllHandle, 
            "opencbm_plugin_install_plugin_data");

        if (opencbm_plugin_install_plugin_data == NULL) {
            DBG_PRINT((DBG_PREFIX "Could not get address of "
                "opencbm.dll::opencbm_plugin_install_plugin_data()"));
            fprintf(stderr, "Could not get address of "
                "opencbm.dll::opencbm_plugin_install_plugin_data()");
            break;
        }

        error = opencbm_plugin_install_plugin_data(PluginInstallParameter->Name, pathToInstalledPluginFile, PluginInstallParameter->OptionMemory);

        if (error) {
            break;
        }

        /*
         * Tell the plugin to self-init itself
         */

        error = PluginExecuteFunction(PluginInstallParameter->Name,
            pathToInstalledPluginFile, 
            "opencbm_plugin_install_do_install",
            perform_opencbm_plugin_install_do_install,
            PluginInstallParameter);

    } while (0);

    cbmlibmisc_strfree(pathToInstalledPluginFile);

    FUNC_LEAVE_BOOL(error);
}

/*! \internal \brief Install OpenCBM

 This function installs the driver on the current machine.

 \param OpenCbmDllHandle
   Handle to opencbm.dll.

 \param Parameter
   Pointer to parameter_t struct which contains the
   description of the parameters given on the command-line.

 \return 
   Return value which will be given on return from main()
   That is, 0 on success, everything else indicates an error.
*/
static int
InstallOpenCBM(cbm_install_parameter_t *Parameter)
{
    HMODULE openCbmDllHandle = NULL;
    int error = 1;

    FUNC_ENTER();

    do {
        InstallPluginCallback_context_t callbackContext;

        memset(&callbackContext, 0, sizeof(callbackContext));

        openCbmDllHandle = LoadLocalOpenCBMDll();
        if (openCbmDllHandle  == NULL) {
            DBG_PRINT((DBG_PREFIX "Could not open the OpenCBM DLL."));
            fprintf(stderr, "Could not open the OpenCBM DLL.");
            break;
        }

        if ( Parameter->NoCopy ) {
            error = SelfInitGenericOpenCBM(openCbmDllHandle, NULL, Parameter->NoCopy);
        }
        else if ( ! IsPresentGenericOpenCBM() )
        {
            // install generic OpenCBM files

            if ( CopyGenericOpenCBM(openCbmDllHandle, Parameter) ) {
                break;
            }
        }

        callbackContext.OpenCbmDllHandle = openCbmDllHandle;
        callbackContext.Parameter        = Parameter;

        error = PluginForAll(Parameter, InstallPluginCallback, &callbackContext);

        error = UpdateOpenCBM(Parameter) || error;

    } while (0);

    if (openCbmDllHandle) {
        FreeLibrary(openCbmDllHandle);
    }

    FUNC_LEAVE_INT(error);
}

/*! \internal \brief Delete a file at a path

 This function deletes a file at a specified path.

 \param DestinationPath
   The path from where to delete the file.
   The path has to be terminated with a backslash ("\").

 \param Filename
   The name of the file to copy.
*/

static BOOL
RemoveOpenCbmFilesCallback(const char * DestinationPath, const char * Filename)
{
    char * fileToDelete = NULL;
    BOOL error = TRUE;

    FUNC_ENTER();

    do
    {
        fileToDelete = cbmlibmisc_strcat(DestinationPath, Filename);

        if (fileToDelete)
        {
            DBG_PRINT((DBG_PREFIX "Trying to delete '%s'.", fileToDelete));

            fprintf(stderr, "Trying to delete '%s'.\n", fileToDelete);
            DeleteFile(fileToDelete);
            error = 0;
        }

    } while (0);

    cbmlibmisc_strfree(fileToDelete);

    FUNC_LEAVE_BOOL(error);
}

/*! \brief @@@@@ \todo document

 \param NeededFiles

 \return
*/
static BOOL
RemoveOpenCbmFiles(opencbm_plugin_install_neededfiles_t NeededFiles[])
{
    FUNC_ENTER();

    FUNC_LEAVE_BOOL(HandleNeededFiles(NeededFiles, NULL, RemoveOpenCbmFilesCallback));
}

/*! \brief @@@@@ \todo document

 \param PluginName

 \param FunctionPointer

 \param Context

 \return
*/
static BOOL
perform_opencbm_plugin_install_do_uninstall(const char * PluginName, void * FunctionPointer, void * Context)
{
    opencbm_plugin_install_do_uninstall_t * opencbm_plugin_install_do_uninstall = FunctionPointer;

    BOOL error = TRUE;

    FUNC_ENTER();

    do {
        if (opencbm_plugin_install_do_uninstall == NULL) {
            break;
        }

        error = opencbm_plugin_install_do_uninstall(Context);

        if (error) {
            DBG_ERROR((DBG_PREFIX "Uninstallation of plugin '%s' failed!", PluginName));
            fprintf(stderr, "Uninstallation of plugin '%s' failed!\n", PluginName);
            break;
        }

    } while (0);

    FUNC_LEAVE_BOOL(error);
}

/*! \brief @@@@@ \todo document

 \param PluginInstallParameter

 \param Context

 \return
*/
static BOOL
RemovePluginCallback(cbm_install_parameter_plugin_t * PluginInstallParameter, void * Context)
{
    InstallPluginCallback_context_t * context = Context;

    BOOL error = TRUE;

    FUNC_ENTER();

    do {
        error = PluginExecuteFunction(PluginInstallParameter->Name,
            PluginInstallParameter->FileName,
            "opencbm_plugin_install_do_uninstall",
            perform_opencbm_plugin_install_do_uninstall,
            PluginInstallParameter);

        if (error) {
            break;
        }

        error = RemoveOpenCbmFiles(PluginInstallParameter->NeededFiles);
        if (error) {
            break;
        }

    } while (0);

    FUNC_LEAVE_BOOL(error);
}


/*! \brief @@@@@ \todo document

 \param OpenCbmDllHandle

 \return
*/
static BOOL
RemoveGenericOpenCBM(HMODULE OpenCbmDllHandle)
{
    BOOL error = TRUE;

    FUNC_ENTER();

    do {
        error = RemoveOpenCbmFiles(NeededFilesGeneric);
        if (error) {
            break;
        }

    } while (0);

    FUNC_LEAVE_BOOL(error);
}

/*! \internal \brief Process a remove request

 This function removes the the driver from the machine.

 \param Parameter
   Pointer to parameter_t struct which contains the
   description of the parameters given on the command-line.

 \return 
   Return value which will be given on return from main()
   That is, 0 on success, everything else indicates an error.
*/
static int
RemoveOpenCBM(cbm_install_parameter_t *Parameter)
{
    HMODULE openCbmDllHandle = NULL;
    int error = 1;

    FUNC_ENTER();

    /*! \todo:
      1. Partial remove (only one or more plugins)
      2. if the default plugin is removed, make another one the default
      3. remove the plugin from the configuration file
      4. only remove OpenCBM if all plugins are removed
      5. Remove opencbm.conf if OpenCBM is removed completely
    */

    do {
        InstallPluginCallback_context_t callbackContext;

        memset(&callbackContext, 0, sizeof(callbackContext));

        openCbmDllHandle = LoadLocalOpenCBMDll();
        if (openCbmDllHandle  == NULL) {
            DBG_PRINT((DBG_PREFIX "Could not open the OpenCBM DLL."));
            fprintf(stderr, "Could not open the OpenCBM DLL.");
            break;
        }

        if ( ! IsPresentGenericOpenCBM() )  {
            DBG_PRINT((DBG_PREFIX "trying to remove OpenCBM, but it is not installed!"));
            fprintf(stderr, "trying to remove OpenCBM, but it is not installed!\n");
            error = 0;
            break;
        }

        callbackContext.OpenCbmDllHandle = openCbmDllHandle;
        callbackContext.Parameter        = Parameter;

        // Remove all plugins first

        error = PluginForAll(Parameter, RemovePluginCallback, &callbackContext);

        if (error) {
            break;
        }

        // remove generic OpenCBM files

        if ( RemoveGenericOpenCBM(openCbmDllHandle) ) {
            break;
        }

        error = 0;

    } while (0);

    if (openCbmDllHandle) {
        FreeLibrary(openCbmDllHandle);
    }

    FUNC_LEAVE_INT(error);
}

/*
 This function checks if the driver was correctly installed.

 \param Parameter
   Pointer to parameter_t struct which contains the
   description of the parameters given on the command-line.

 \param PluginNames
    Array of pointers to strings which holds the names of all plugins to process.
    This array has to be finished by a NULL pointer.

 \return 
   Return value which will be given on return from main().
   That is, 0 on success, everything else indicates an error.
*/
static int
CheckOpenCBM(cbm_install_parameter_t *Parameter)
{
    int success = 0;

    FUNC_ENTER();

    do {

        //! \todo @@@

    } while (0);

    FUNC_LEAVE_INT(success);
}

/*! \brief Main function

 This function performs the action the user has given on the command-line.

 \param Argc
   The count of arguments, as given to main().

 \param Argv
   The arguments vector, as given to main().

 \return 
   0 on success, everything else indicated an error.
*/
int __cdecl
main(int Argc, char **Argv)
{
    cbm_install_parameter_t parameter;
    int retValue = 0;

    FUNC_ENTER();

    /* initialize parameters */

    memset(&parameter, 0, sizeof(parameter));
    parameter.PluginList = NULL;

    do {
        parameter.OsVersion = GetOsVersion();

        if (parameter.OsVersion == WINUNSUPPORTED) {
            DBG_PRINT((DBG_PREFIX "This version of Windows is not supported!"));
            printf("Sorry, this version of Windows is not supported!\n");
            retValue = 2;
            break;
        }

        if (processargs(Argc, Argv, &parameter)) {
            DBG_PRINT((DBG_PREFIX "Error processing command line arguments."));
            fprintf(stderr, "Error processing command line arguments.\n");
            retValue = 1;
            break;
        }

        if (parameter.NoExecute) {
            break;
        }

        if (parameter.AdminNeeded && !NeededAccessRights()) {
            DBG_PRINT((DBG_PREFIX "You do not have necessary privileges. " 
                "Please try installing only as administrator."));
            printf("You do not have necessary privileges.\n"
                "Please try installing only as administrator.\n");

            retValue = 3;
            break;
        }

        //
        // execute the command
        //

        if (parameter.CheckInstall) {
            retValue = CheckOpenCBM(&parameter);
        }
        else if (parameter.Remove) {
            // The driver should be removed

            retValue = RemoveOpenCBM(&parameter);
        }
        else if (parameter.Update) {
            // Update driver parameters

            retValue = UpdateOpenCBM(&parameter);
        }
        else if (parameter.Install) {
            // The driver should be installed

            retValue = InstallOpenCBM(&parameter);
        }
        else {
            fprintf(stderr, "Internal problem: Which command has been given to instcbm?\n");
            DBG_PRINT((DBG_PREFIX "Internal problem: Which command has been given to instcbm?"));
        }
    } while (0);

#if DBG

    if (parameter.OutputDebuggingBuffer)
    {
// @@@        CbmOutputDebuggingBuffer();
    }

#endif // #if DBG

    PluginListFree(&parameter);

    FUNC_LEAVE_INT(retValue);
}
