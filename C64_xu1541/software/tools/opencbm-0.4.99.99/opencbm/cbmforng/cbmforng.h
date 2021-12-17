/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Copyright (C) 1999 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 * Copyright (C) 2006 Wolfgang Moser (http://d81.de)
 */

#ifndef __CBMFORMAT_NEXT_GENERATION_INTERFACE_DESCFRIPTION_HEADER_INCLUDE_
#define __CBMFORMAT_NEXT_GENERATION_INTERFACE_DESCFRIPTION_HEADER_INCLUDE_

#include "opencbm.h"

#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch.h"

    /*
     * Enable additional formatting debug reporting, this also requires
     * the floppy stub part
     */
#define DebugFormat 1

    /*
     * Do report the name "cbmforng" on all self reports instead of the
     * future name "cbmformat", when having replaced the former tool.
     */
#define CBMFORNG 1

    /*
     * The following are some really ugly macro definitions, so that a
     * structural storage declaration can be shared between the ca65
     * assembler and ANSI-C. The structural declaration is made within
     * the interface definition header file named cbmforng.idh, it
     * contains several macro calls which are defined here.
     */
#   define _CMT(str)
#   define _BEGINSTRUCT(parm)      struct parm {
#   define _ENDSTRUCT()            };
#   define _OCTETARRAY(name, size) unsigned char name[size];
#   define _OCTETDECL(name)        unsigned char name;

#   include  "cbmforng.idh"

#   undef _CMT
#   undef _BEGINSTRUCT
#   undef _ENDSTRUCT
#   undef _OCTETARRAY
#   undef _OCTETDECL

#endif
