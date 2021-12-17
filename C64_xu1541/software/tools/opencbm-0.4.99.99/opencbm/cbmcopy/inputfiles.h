/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2001 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
*/

#ifndef INPUTFILES_H
#define INPUTFILES_H

#include <stdio.h>

#include "opencbm.h"
#include "cbmcopy.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int (*probe)(FILE *, const char *, cbmcopy_message_cb);

    int (*read)(FILE *,               /* guess what               */
                const char *,         /* OS filename              */
                int,                  /* archive entry (0-based)  */
                char *,               /* CBM name                 */
                char *,               /* CBM filetype             */
                unsigned char **,     /* malloc'd file data       */
                size_t *,             /* CBM filesize             */
                cbmcopy_message_cb);  /* guess what               */
} input_reader;

#define DECLARE_INPUT_READER(x) \
    input_reader cbmwrite_ ## x = {probe, read}

#ifdef __cplusplus
}
#endif

#endif
