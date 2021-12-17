/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * Copyright (C) 2006 Wolfgang Moser (http://d81.de)
 */

#ifndef __CBMRPM41_HEADER_INCLUDE_
#define __CBMRPM41_HEADER_INCLUDE_

#include "opencbm.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <getopt.h>
#include <string.h>

#include "arch.h"

#define CBMRPM41_DEBUG      // enable state logging and debugging


    /*
     * The following are some really ugly macro definitions, so that a
     * structural storage declaration can be shared between the ca65
     * assembler and ANSI-C. The structural declaration is made within
     * the interface definition header file named cbmforng.idh, it
     * contains several macro calls which are defined here.
     */

#   include "packon.h"
#   define _CMT(        str         )
#   define _CONSTDECL(  name,value  ) typedef unsigned char name[value];
#   define _BEGINSTRUCT(name        ) struct name {
#   define _ENDSTRUCT(              ) };
#   define _OCTETARRAY( name, size  ) unsigned char name[size];
#   define _OCTETDECL(  name        ) unsigned char name;

#   define _BEGINENUM(  name        ) enum name {
#   define _ENDENUM(                ) };
#   define _ENUMENTRY(  name,value  ) name = value,

#   define _BEGIN_UXT(  name        ) enum name { ResetUxVectorTable = '0',
#   define _UX_EENTRY(  asym,no,name) name = no,

#   define _BEGINMACRO( name        ) struct name {
#   define _ENDMACRO(               ) };
#   define _BEGLSCOPE(  name        ) struct name {
#   define _ENDLSCOPE(  impl        ) } impl;
#   define _TAGLJUMP(   name        ) unsigned char name[3];
#   define _TAGSTRUCT(  name, type  ) struct type name;
#   define _TAGOCTET(   name, value ) unsigned char name;
#   define _TAGSHORT(   name, value ) unsigned short name;

#   include "cbmrpm41.idh"

#   undef _CMT
#   undef _CONSTDECL
#   undef _BEGINSTRUCT
#   undef _ENDSTRUCT
#   undef _OCTETARRAY
#   undef _OCTETDECL

#   undef _BEGINENUM
#   undef _ENDENUM
#   undef _ENUMENTRY

#   undef _BEGIN_UXT
#   undef _UX_EENTRY

#   undef _BEGINMACRO
#   undef _ENDMACRO
#   undef _BEGLSCOPE
#   undef _ENDLSCOPE
#   undef _TAGLJUMP
#   undef _TAGSTRUCT
#   undef _TAGOCTET
#   undef _TAGSHORT
#   include "packoff.h"

#endif
