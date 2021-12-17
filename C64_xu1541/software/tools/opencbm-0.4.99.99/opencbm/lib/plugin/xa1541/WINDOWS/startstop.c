/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004, 2008, 2012 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file lib/plugin/xa1541/WINDOWS/startstop.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Functions for starting and stopping the driver
**
****************************************************************/


#include <windows.h>
#include <stdio.h>
#include "cbmioctl.h"

#include "i_opencbm.h"

#include "archlib.h"

#include "version.h"

#include <stdlib.h>

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#define DBG_PROGNAME "INSTCBM.EXE"

#include "debug.h"

#include "libmisc.h"

static BOOL CheckVersions(PCBMT_I_INSTALL_OUT InstallOutBuffer);
static BOOL CbmCheckCorrectInstallation(BOOL HaveAdminRights);


/*! \internal \brief Output a path

 \param Text
   Pointer to a string that will be printed before
   the path string.

 \param Path
   Pointer to a string which contains the path.

*/
static VOID
OutputPathString(IN PCHAR Text, IN PCHAR Path)
{
    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "%s%s", Text, Path));
    printf("%s%s\n", Text, Path);

    FUNC_LEAVE();
}

/*! \internal \brief Output a version string

 \param Text
   Pointer to a string that will be printed before
   the version string.

 \param Version
   A (coded) version information, as defined by
   CBMT_I_INSTALL_OUT_MAKE_VERSION()

*/
static VOID
OutputVersionString(IN PCHAR Text, IN ULONG Version, IN ULONG VersionEx)
{
    char buffer[100];
    char buffer2[100];

    FUNC_ENTER();

    if (Version != 0)
    {
        char patchlevelVersion[] = "pl0";

        if (CBMT_I_INSTALL_OUT_GET_VERSION_EX_BUGFIX(VersionEx) != 0)
        {
            patchlevelVersion[2] = 
                (char) (CBMT_I_INSTALL_OUT_GET_VERSION_EX_BUGFIX(VersionEx) + '0');
        }
        else
        {
            patchlevelVersion[0] = 0;
        }

        _snprintf(buffer2, sizeof(buffer)-1,
            (CBMT_I_INSTALL_OUT_GET_VERSION_DEVEL(Version) 
                ? "%u.%u.%u.%u" 
                : "%u.%u.%u"),
            (unsigned int) CBMT_I_INSTALL_OUT_GET_VERSION_MAJOR(Version),
            (unsigned int) CBMT_I_INSTALL_OUT_GET_VERSION_MINOR(Version),
            (unsigned int) CBMT_I_INSTALL_OUT_GET_VERSION_SUBMINOR(Version),
            (unsigned int) CBMT_I_INSTALL_OUT_GET_VERSION_DEVEL(Version));

        _snprintf(buffer, sizeof(buffer)-1,
            (CBMT_I_INSTALL_OUT_GET_VERSION_DEVEL(Version) 
                ? "%s%s (Development)" 
                : "%s%s"),
            buffer2,
            patchlevelVersion);
    }
    else
    {
        _snprintf(buffer, sizeof(buffer)-1, "COULD NOT DETERMINE VERSION");
    }
    buffer[sizeof(buffer)-1] = 0;

    OutputPathString(Text, buffer);

    FUNC_LEAVE();
}


/*! \internal \brief Read parameters of the driver

 This function determines the path of the driver as well as some 
 basic configuration parameters of it

 \param DriverPath
   Pointer to a buffer that will hold the path of the driver on exit

 \param DriverPathLen
   The length of the buffer on which DriverPath points to.

 \param StartMode
   Pointer to a variable which will hold the start mode of the driver

 \param LptPort
   Pointer to a variable which will hold the LPT port number of the driver

 \param LptLocking
   Pointer to a variable which will determine if the driver being kept locked or not

 \param CableType
   Pointer to a variable which will hold the cable type for the driver

 \return
   TRUE if an error occurred, else FALSE.
*/
static BOOL
ReadDriverData(char *DriverPath, ULONG DriverPathLen, DWORD *StartMode, DWORD *LptPort, DWORD *LptLocking, DWORD *CableType)
{
    BOOL error = TRUE;

    HKEY regKey = NULL;

    FUNC_ENTER();

    do {
        DWORD regLength;
        DWORD regReturn;
        DWORD regType;
        char driverPathFromRegistry[MAX_PATH];
        char *pColon;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         CBM_REGKEY_SERVICE,
                         0,
                         KEY_QUERY_VALUE,
                         &regKey)
           )
        {
            DBG_WARN((DBG_PREFIX "RegOpenKeyEx() failed!"));
            fprintf(stderr, "Could not open registry key\n");
            regKey = NULL;
            break;
        }

        // now, get the number of the port to use

        regLength = sizeof(driverPathFromRegistry);

        regReturn = RegQueryValueEx(regKey, "ImagePath", NULL, &regType, 
            (LPBYTE)driverPathFromRegistry, &regLength);

        if (regReturn != ERROR_SUCCESS)
        {
            DBG_ERROR((DBG_PREFIX "No HKLM\\" CBM_REGKEY_SERVICE "\\ImagePath"
                " value: %s", cbmlibmisc_format_error_message(regReturn)));

            break;
        }


        //! \todo rewrite using libmisc string functions

        // Make sure there is a trailing zero

        driverPathFromRegistry[sizeof(driverPathFromRegistry)-1] = 0;

        // Find out if there is a colon ":" in the path

        pColon = strchr(driverPathFromRegistry, ':');

        if (pColon)
        {
            // There is a colon, that is, the path is absolute

            if (strncmp(driverPathFromRegistry, "\\??\\", sizeof("\\??\\")-1) == 0)
            {
                // There is the \??\ prefix, skip that

                strncpy(DriverPath, &driverPathFromRegistry[sizeof("\\??\\")-1],
                    DriverPathLen);
            }
            else
            {
                strncpy(DriverPath, driverPathFromRegistry, DriverPathLen);
            }
        }
        else
        {
            DWORD lengthString;

            // There is no colon, that is, the path is relative (to the windows directory)
            // Thus, make sure the windows directory is appended in front of it

            lengthString = GetWindowsDirectory(DriverPath, DriverPathLen);

            if ((lengthString != 0) && (lengthString < DriverPathLen))
            {
                strncat(&DriverPath[lengthString], "\\", DriverPathLen-lengthString);
                ++lengthString;

                strncat(&DriverPath[lengthString], driverPathFromRegistry,
                    DriverPathLen-lengthString);
            }
        }


#if DBG
        // find out the start mode of the driver

        RegGetDWORD(regKey, "Start", StartMode);

        // find out the default lpt port of the driver

        RegGetDWORD(regKey, CBM_REGKEY_SERVICE_DEFAULTLPT, LptPort);

        // find out the configured LPT port locking behaviour

        RegGetDWORD(regKey, CBM_REGKEY_SERVICE_PERMLOCK, LptLocking);

        // find out the configured cable type

        RegGetDWORD(regKey, CBM_REGKEY_SERVICE_IECCABLE, CableType);

#endif

        error = FALSE;

    } while (0);

    // We're done, close the registry handle.

    if (regKey != NULL) {
        RegCloseKey(regKey);
    }

    FUNC_LEAVE_BOOL(error);
}



/*! \internal \brief Read the file version info out of a file

 This function reads the file version information, as written
 in the version resource, and returns it in the format needed
 by instcbm.

 \param Filename
    The name of the file of which to determine the version

 \param VersionNumber
    Pointer to a variable which will hold the version information
    of the file

 \param PatchLevelNumber
    Pointer to a variable which will hold the patchlevel
    of the file.

 \return
    TRUE if an error occurred, else FALSE.
*/
static BOOL
getVersionInfoOfFile(char * Filename, ULONG *VersionNumber, ULONG *PatchLevelNumber)
{
    ULONG error = TRUE;

    DWORD versionInfoSize;
    DWORD dummyhandle;

    FUNC_ENTER();

    DBG_ASSERT(PatchLevelNumber);

    *PatchLevelNumber = 0;

    versionInfoSize = GetFileVersionInfoSize(Filename, &dummyhandle);

    if (versionInfoSize > 0)
    {
        void *versionInfo = malloc(versionInfoSize);

        if (versionInfo && GetFileVersionInfo(Filename, 0, versionInfoSize, versionInfo))
        {
            VS_FIXEDFILEINFO *fileInfo;
            DWORD fileInfoSize = 0;
            TCHAR *versionInfoText = 0;

            if (VerQueryValue(versionInfo, TEXT("\\"), &fileInfo, &fileInfoSize))
            {
                *VersionNumber   = ((fileInfo->dwFileVersionMS & 0x00FF0000) >> 8) | fileInfo->dwFileVersionMS & 0xFF;
                *VersionNumber <<= 16;
                *VersionNumber  |= ((fileInfo->dwFileVersionLS & 0x00FF0000) >> 8) | fileInfo->dwFileVersionLS & 0xFF;

                error = FALSE;
            }

            // determine if there is some patch-level involved

            if (VerQueryValue(versionInfo, TEXT("\\StringFileInfo\\000004B0\\FileVersion"), &versionInfoText, &fileInfoSize))
            {
                char *patch_level = strstr(versionInfoText, "pl");

                if (patch_level)
                {
                    *PatchLevelNumber = atoi(patch_level + 2);
                }
                else
                {
                    // handling of old 0.1.0a version, which handled the patch-level in a different format:

                    if (strcmp(versionInfoText, TEXT("0.1.0a")) == 0)
                        *PatchLevelNumber = 1;
                }
            }
        }

        free(versionInfo);
    }

    FUNC_LEAVE_ULONG(error);
}

/*! \internal \brief Determine version of a DLL

 This function opens a DLL and reads out the version information as well
 as the path of it. Additionally, a callback is called if it is specified.
 This callback gets the HMODULE of the DLL, so, it can do additional initializing.

 \param Filename
    The name of the DLL of which to determine the version

 \param Path
    Pointer to a buffer which will hold the path of the DLL

 \param PathLen
    The length of the buffer pointed to with Path.

 \param VersionNumber
    Pointer to a variable which will hold the version information
    of the dLL

 \param VersionNumberEx
    Pointer to a variable which will hold the patchlevel of the DLL

 \return
    TRUE if an error occurred, else FALSE.
*/
static BOOL
ReadDllVersion(char * Filename, char * Path, ULONG PathLen, ULONG * VersionNumber, ULONG * VersionNumberEx)
{
    BOOL error = TRUE;

    HMODULE handleDll;

    FUNC_ENTER();

    // Try to load the DLL. If this succeeds, get the version information
    // from there

    handleDll = LoadLibrary(Filename);

    if (handleDll)
    {
        DWORD length;

        length = GetModuleFileName(handleDll, Path, PathLen);

        if (length >= PathLen)
        {
            Path[PathLen-1] = 0;
        }

        error = getVersionInfoOfFile(Path, VersionNumber, VersionNumberEx);

        FreeLibrary(handleDll);
    }

    FUNC_LEAVE_BOOL(error);
}

/*! \internal \brief Complete the installation of a plugin DLL

 This function is a callback for ReadDllVersion.
 It calls a special function which completes the plugin
 installation.

 \param HandleDll
    A handle to the plugin DLL which is currently processed

 \return
    TRUE if an error occurred, else FALSE.
*/
static BOOL
CompleteDriverInstallation(HMODULE HandleDll)
{
    BOOL error = TRUE;

#if 0 //! \todo rewrite, it does not make any sense to call an internal function via GetProcAddress()!

    cbm_install_complete_t * p_cbm_install_complete;
    CBMT_I_INSTALL_OUT dllInstallOutBuffer;

    FUNC_ENTER();

    memset(&dllInstallOutBuffer, 0, sizeof(dllInstallOutBuffer));

    p_cbm_install_complete = 
        (cbm_install_complete_t *) GetProcAddress(HandleDll, "cbm_install_complete");

    if (p_cbm_install_complete) {
        error = p_cbm_install_complete((PULONG) &dllInstallOutBuffer, sizeof(dllInstallOutBuffer));
    }

#endif

    FUNC_LEAVE_BOOL(error);
}

/*! \brief Check for the correct installation

 This function checks if the driver was correctly installed.

 \return 
   Return value which will be given on return from main().
   That is, 0 on success, everything else indicates an error.
*/
int
CbmCheckDriver(void)
{
    int error;

    FUNC_ENTER();

    DBG_PRINT((DBG_PREFIX "Checking configuration for OpenCBM"));
    printf("Checking configuration for OpenCBM\n");

    if (CbmCheckCorrectInstallation(TRUE /*! \todo NeededAccessRights() */))
    {
        DBG_PRINT((DBG_PREFIX "There were errors in the current configuration."
            "Please fix them before trying to use the driver!"));
        fprintf(stderr, "*** There were errors in the current configuration.\n"
            "*** Please fix them before trying to use the driver!\n");
        error = 11;
    }
    else
    {
        error = 0;
        DBG_PRINT((DBG_PREFIX "No problems found in current configuration"));
        printf("No problems found in current configuration\n\n");

        /*! \todo Suggested output from WoMo:
            Checking configuration for OpenCBM:
            No problems found in current configuration:

            Driver configuration:
             Port:               automatic (0), currently using LPT 1
             IRQ mode:           enabled
             Driver start mode:  manually (3)
        */
    }

    FUNC_LEAVE_INT(error);
}


/*! \internal \brief Check if versions differ
*/

static BOOL
checkIfDifferentVersions(Version1, VersionEx1, Version2, VersionEx2)
{
    return ((Version1 != Version2) || (VersionEx1 != VersionEx2)) ? TRUE : FALSE;
}

static char *
get_plugin_filename(char *PluginName)
{
    char *filename;

    FUNC_ENTER();

    filename = malloc(sizeof("opencbm-.dll") + strlen(PluginName));

    if (filename)
        sprintf(filename, "opencbm-%s.dll", PluginName);

    FUNC_LEAVE_STRING(filename);
}

/*! \brief Check version information

 This function checks (and outputs) the version
 information for OpenCBM.

 \param InstallOutBuffer
    A buffer which will hold the version information for the
    driver and the main DLL.

 \param PluginNames
    Array of pointers to strings which holds the names of all plugins to process.
    This array has to be finished by a NULL pointer.

 \return
    If mixed versions are found, or other errors occurred,
    this function returns TRUE.

*/
static BOOL
CheckVersions(PCBMT_I_INSTALL_OUT InstallOutBuffer)
{
    ULONG instcbmVersion;
    ULONG instcbmVersionEx;
    DWORD startMode;
    DWORD lptPort;
    DWORD lptLocking;
    DWORD cableType;
    char dllPath[MAX_PATH] = "<unknown>";
    char driverPath[MAX_PATH] = "<unknown>";
    BOOL error;
    BOOL differentVersion = FALSE;
    char **PluginNames = NULL; //! @@@ \todo

    FUNC_ENTER();

    error = FALSE;

    // Default value for unset/unconfigured registry settings

    startMode = -1;
    lptPort = 0;
    lptLocking = 1;
    cableType = -1;

    // Try to find out the version and path of the DLL

    error = ReadDllVersion("OPENCBM.DLL", dllPath, sizeof(dllPath),
        &InstallOutBuffer->DllVersion, &InstallOutBuffer->DllVersionEx);

    // Try to find the path to the driver

    error = ReadDriverData(driverPath, sizeof(driverPath), &startMode, &lptPort, &lptLocking, &cableType);

    // Print out the configuration we just obtained

    printf("\n\nThe following configuration is used:\n\n");

    instcbmVersion =
        CBMT_I_INSTALL_OUT_MAKE_VERSION(OPENCBM_VERSION_MAJOR,
                                        OPENCBM_VERSION_MINOR,
                                        OPENCBM_VERSION_SUBMINOR,
                                        OPENCBM_VERSION_DEVEL);

    instcbmVersionEx =
        CBMT_I_INSTALL_OUT_MAKE_VERSION_EX(OPENCBM_VERSION_PATCHLEVEL);

    OutputVersionString("INSTCBM version: ", instcbmVersion, instcbmVersionEx);

    OutputVersionString("Driver version:  ", InstallOutBuffer->DriverVersion,
        InstallOutBuffer->DriverVersionEx);
    OutputPathString   ("Driver path:     ", driverPath);
    differentVersion |= checkIfDifferentVersions(instcbmVersion, instcbmVersionEx,
        InstallOutBuffer->DriverVersion, InstallOutBuffer->DriverVersionEx);

    OutputVersionString("DLL version:     ", InstallOutBuffer->DllVersion,
        InstallOutBuffer->DllVersionEx);
    OutputPathString   ("DLL path:        ", dllPath);
    differentVersion |= checkIfDifferentVersions(instcbmVersion, instcbmVersionEx,
        InstallOutBuffer->DllVersion, InstallOutBuffer->DllVersionEx);

#if 0 // def _X86_
    {
        ULONG dllVersionVdd = 0;
        ULONG dllVersionVddEx = 0;

        dllPath[0] = 0;

        ReadDllVersion("opencbmvdd.dll", dllPath, sizeof(dllPath),
                &dllVersionVdd, &dllVersionVddEx);

        OutputVersionString("VDD version:     ", dllVersionVdd, dllVersionVddEx);
        OutputPathString   ("VDD path:        ", dllPath);
        differentVersion |= checkIfDifferentVersions(instcbmVersion, instcbmVersionEx,
            dllVersionVdd, dllVersionVddEx);
    }
#endif // #ifdef _X86_

    if (PluginNames)
    {
        unsigned int i;

        for (i = 0; PluginNames[i] != NULL; i++)
        {
            ULONG dllVersionPlugin = 0;
            ULONG dllVersionPluginEx = 0;

            char *filename = get_plugin_filename(PluginNames[i]);

            dllPath[0] = 0;

            if (!filename)
            {
                error = TRUE;
                break;
            }

            error = ReadDllVersion(filename, dllPath, sizeof(dllPath),
                &dllVersionPlugin, &dllVersionPluginEx);

            free(filename);

            printf("\nPlugin '%s':\n", PluginNames[i]);
            OutputVersionString("Plugin version:  ", dllVersionPlugin, dllVersionPluginEx);
            OutputPathString   ("Plugin path:     ", dllPath);

            /*! \todo do we insist on plugins having the same version?
            differentVersion |= checkIfDifferentVersions(instcbmVersion, instcbmVersionEx,
                dllVersionPlugin, dllVersionPluginEx);
            /**/
        }
    }

    printf("\n");

    if (differentVersion)
    {
        error = TRUE;
        printf("There are mixed versions, THIS IS NOT RECOMMENDED!\n\n");
    }

    printf("Driver configuration:\n");
    DBG_PRINT((DBG_PREFIX "Driver configuration:"));

    printf(               "  Default port: ........ LPT%i\n", lptPort ? lptPort : 1);
    DBG_PRINT((DBG_PREFIX "  Default port: ........ LPT%i", lptPort ? lptPort : 1));

    {
        const char *startModeName;

        switch (startMode)
        {
            case -1:
                startModeName = "NO ENTRY FOUND!";
                break; 

            case SERVICE_BOOT_START:
                startModeName = "boot";
                break;

            case SERVICE_SYSTEM_START:
                startModeName = "system";
                break;

            case SERVICE_AUTO_START:
                startModeName = "auto";
                break;

            case SERVICE_DEMAND_START:
                startModeName = "demand";
                break;

            case SERVICE_DISABLED:
                startModeName = "disabled";
                break;

            default:
                startModeName = "<UNKNOWN>";
                break;
        }

        printf("  Driver start mode: ... %s (%i)\n", startModeName, startMode);
        DBG_PRINT((DBG_PREFIX "  Driver start mode: ... %s (%i)", startModeName,
            startMode));
    }

    printf(               "  LPT port locking: .... %s\n", lptLocking ? "yes" : "no");
    DBG_PRINT((DBG_PREFIX "  LPT port locking: .... %s", lptLocking ? "yes" : "no"));

    {
        const char *cableTypeName;

        switch (cableType)
        {
            case -1:
                cableTypeName = "auto";
                break; 

            case 0:
                cableTypeName = "xm1541";
                break;

            case 1:
                cableTypeName = "xa1541";
                break;

            default:
                cableTypeName = "<UNKNOWN>";
                break;
        }

        printf("  Cable type: .......... %s (%i)\n\n", cableTypeName, cableType);
        DBG_PRINT((DBG_PREFIX "  Cable type: .......... %s (%i)", cableTypeName,
            cableType));
    }

    FUNC_LEAVE_BOOL(error);
}

/*! \brief Check if the driver was correctly installed

 This function checks if the driver was correctly installed.

 \param HaveAdminRights
   TRUE if we are running with admin rights; FALSE if not.

 \return
   FALSE on success, TRUE on error,

 This function opens the opencbm driver, tests if anything is
 ok - especially, if the interrupt could be obtained - and reports
 this status.

 If there was no interrupt available, it tries to enable it
 on the parallel port.

 If we are running without administrator rights, we do not try to
 start the driver, as we will not be able to do it.
*/

static BOOL
CbmCheckCorrectInstallation(BOOL HaveAdminRights)
{
    CBMT_I_INSTALL_OUT outBuffer;
    BOOL error;
    BOOL driverAlreadyStarted = FALSE;
    int tries;

    FUNC_ENTER();

    memset(&outBuffer, 0, sizeof(outBuffer));

    for (tries = 1; tries >= 0; --tries)
    {
        if (driverAlreadyStarted) {
            cbm_driver_stop();
        }

        error = HaveAdminRights ? (cbm_driver_start() ? FALSE : TRUE) : FALSE;

        error = FALSE;

        driverAlreadyStarted = TRUE;

        if (error)
        {
            DBG_PRINT((DBG_PREFIX "Driver or DLL not correctly installed."));
            printf("Driver or DLL not correctly installed.\n");
            break;
        }
        else
        {
            error = cbm_driver_install((PULONG) &outBuffer, sizeof(outBuffer));

            outBuffer.DllVersion = 0;

            if (error)
            {
                DBG_PRINT((DBG_PREFIX "Driver problem: Could not check install."));
                printf("Driver problem: Could not check install.\n");
                break;
            }

            // did we fail to gather an interrupt?

            if (outBuffer.ErrorFlags & CBM_I_DRIVER_INSTALL_0M_NO_INTERRUPT)
            {
                if (tries > 0)
                {
                    //
                    // stop the driver to be able to restart the parallel port
                    //

                    cbm_driver_stop();
                    driverAlreadyStarted = FALSE;

                    //
                    // No IRQ available: Try to restart the parallel port to enable it.
                    //

                    printf("Please wait some seconds...\n");

                    CbmParportRestart();
                }
                else
                {
                    DBG_PRINT((DBG_PREFIX "No interrupt available."));
                    printf("\n*** Could not get an interrupt. Please try again after a reboot.\n");
                    error = TRUE;
                }
            }
            else
            {
                error = FALSE;

                // no problem so far. Now, check if the IRQ is actually working
//                error = CbmTestIrq();

                // no problem, we can stop the loop

                break;
            }
        }
    }

    //
    // If the driver is not set to be started automatically, stop it now
    //

    if (!IsDriverStartedAutomatically())
    {
        cbm_driver_stop();
    }

    if (CheckVersions(&outBuffer))
    {
        error = TRUE;
    }

    FUNC_LEAVE_BOOL(error);
}
