/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2007,2008,2012 Spiro Trikaliotis
 *
*/

/*! ************************************************************** 
** \file lib/WINDOWS/configuration_name.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver
**        Read configuration file
**
****************************************************************/

#include "configuration.h"
#include "libmisc.h"
#include "opencbm-plugin-install.h"
#include "version.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <windows.h>

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! Mark: We are building the DLL */
/* #define DBG_DLL */

/*! The name of the executable */
#define DBG_PROGNAME "OPENCBM.DLL"

/*! This file is "like" debug.c, that is, define some variables */
/* #define DBG_IS_DEBUG_C */

#include "debug.h"

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

/*! \brief The filename of the configuration file */
#define FILENAME_CONFIGFILE "opencbm.conf"

static char *
get_environment(const char * environment)
{
    char * buffer = NULL;

    DWORD length;

    length = GetEnvironmentVariable(environment, NULL, 0);

    while (length > 0) {

        DWORD length2;

        free(buffer);
        buffer = malloc(length + 1);

        length2 = GetEnvironmentVariable(environment, buffer, length + 1);

        if (length2 > length) {
            length = length2;
        }
        else {
            length = 0;
        }
    }
    return buffer;
}

static char *
get_windir()
{
    return get_environment("WINDIR");
}

static char *
get_userdir()
{
    return get_environment("USERPROFILE");
}

/*! \internal \brief Get a possible filename for the configuration file

 This function returns the possible filenames for the configuration
 file, one after the other.
 
 For this, the function is called with an index, starting with zero, 
 being incremented with every call. If the list is exhaused, the function
 returns NULL.

 \return 
   pointer to a newly allocated memory area which contains the
   candidate for the given index. If there is no candidate with
   the given index, the returned value is NULL.

 \remark
   There is not a single default filename for the configuration;
   instead, there are some locations that are searched one after
   the other. This function returns the candidates for these locations.

 \remark
   The caller is responsible for free()ing the returned buffer pointer.
*/
static char *
configuration_get_default_filename_candidate(unsigned int index)
{
    char * buffer = NULL;

    switch (index)
    {
        case INDEX_DEFAULT_FILENAME_LOCAL /* 0 */:
            buffer = cbmlibmisc_strdup(FILENAME_CONFIGFILE);
            break;

        case INDEX_DEFAULT_FILENAME_USERDIR /* 1 */:
            {
            char * userdir = get_userdir();
            buffer = cbmlibmisc_strcat(userdir, "/" FILENAME_CONFIGFILE);
            free(userdir);
            }
            break;

        case INDEX_DEFAULT_FILENAME_WINDIR /* 2 */:
            {
            char * windir = get_windir();
            buffer = cbmlibmisc_strcat(windir, "/System32/" FILENAME_CONFIGFILE);
            free(windir);
            }
            break;

        default:
            /* no more candidates */
            buffer = NULL;
            break;
    }

    return buffer;
}

static int
FileExists(const char * filename)
{
    DWORD attrib = GetFileAttributes(filename);

    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

/*! \brief Get the index of the default filename for the configuration file

 Get the index of the default filename of the configuration file.

 \param pstring_candidate
   Pointer to a 'const char *': Returns a newly allocated string which contains
   the name of the candidate, or NULL of there isn't any.

 \return
   Returns the index of the configuration file candidate, or -1 if none was found.
 
 \remark
   The memory returned in *pstring_candidate must be free()d be the caller.
*/
static unsigned int
configuration_get_default_filename_index(const char ** pstring_candidate)
{
    unsigned int index = 0;
    *pstring_candidate = NULL;

    while (NULL != (*pstring_candidate = configuration_get_default_filename_candidate(index)))
    {
        if (FileExists(*pstring_candidate)) {
DBG_PRINT((DBG_PREFIX "Candidate #%u '%s' does exist, DONE!", index, *pstring_candidate));
            break;
        };

        ++index;
    };

    return (*pstring_candidate == NULL) ? -1 : index;
}

/*! \brief Get the default filename for the configuration file

 Get the default filename of the configuration file.

 \return 
   Returns a newly allocated memory area with the default file name.
*/
const char *
configuration_get_default_filename(void)
{
    char * string_candidate;

    configuration_get_default_filename_index(&string_candidate);

    return string_candidate;
}

/*! \brief Get the default filename for the configuration file on installation

 Get the default filename of the configuration file.
 This function returns a filename even if no configuration exists yet.
 This is used for the installation, where the name for the initial configuration
 file must be found.

 \param DefaultLocation
   In case a new file has to be created, give the preference of one of
   the locations to use.

 \return 
   Returns a newly allocated memory area with the default file name.
*/
const char *
configuration_get_default_filename_for_install(unsigned int DefaultLocation)
{
    return configuration_get_default_filename_candidate(DefaultLocation);
}


