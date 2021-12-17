/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2007      Spiro Trikaliotis
 *
*/

/*! ************************************************************** 
** \file lib/LINUX/configuration_name.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver
**        Read configuration file
**
****************************************************************/

#include "configuration.h"
#include "libmisc.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

/*! \brief The name (including path) of the configuration file.
  
  This defines the name of the configuration file for this platform.
  
  \remark
    OPENCBM_CONFIG_FILE is defined in LINUX/config.make, and it
    is given to the compiler via the command line!
*/

#define OPENCBM_DEFAULT_CONFIGURATION_FILE_NAME OPENCBM_CONFIG_FILE

// Special case: config file location relative to OPENCBM_HOME environment variable
// This string get appended to the OPENCBM_HOME environment variable if it exists
#define OPENCBM_HOME_CONFIG_FILEPATH "/etc/opencbm.conf"

/*! \brief Get the default filename for the configuration file

 Get the default filename of the configuration file.

 \return 
   Returns a newly allocated memory area with the default file name.
*/
const char *
configuration_get_default_filename(void)
{
    char* opencbm_home = getenv("OPENCBM_HOME");
    if (opencbm_home != NULL) {
      return cbmlibmisc_strcat(opencbm_home, OPENCBM_HOME_CONFIG_FILEPATH);
    }
    return cbmlibmisc_strdup(OPENCBM_DEFAULT_CONFIGURATION_FILE_NAME);
}
