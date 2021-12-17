/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2004 Spiro Trikaliotis
 */

#include "arch.h"

#include <sys/stat.h>


/*! \brief Obtain the size of a given file

 This function formats a returned error code into a string,
 and outputs it onto the screen.

 \param Filename
   Name of the file of which to find out the size.

 \param Filesize
   Pointer to a location which will be set to the size of the
   file on successfull termination. If an error occurs, this
   location will not be changed at all.

 \return
   0 on success, everything else denotes an error.
*/

int arch_filesize(const char *Filename, off_t *Filesize)
{
    struct stat statrec;
    size_t ret;

    ret = stat(Filename, &statrec);
    
    if (ret == 0)
    {
        *Filesize = statrec.st_size;
        ret = 0;
    }

    return ret;
}
