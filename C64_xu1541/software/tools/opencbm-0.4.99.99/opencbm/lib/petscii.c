/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2005 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Copyright 2001-2005 Spiro Trikaliotis
 *
 *  Parts are Copyright
 *      Jouko Valta <jopi(at)stekt(dot)oulu(dot)fi>
 *      Andreas Boose <boose(at)linux(dot)rz(dot)fh-hannover(dot)de>
*/

/*! ************************************************************** 
** \file lib/petscii.c \n
** \author Michael Klein, Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver
**
****************************************************************/

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#define DBG_PROGNAME "OPENCBM.DLL"

#include "debug.h"

#include <ctype.h>
#include <stdlib.h>

//! mark: We are building the DLL */
#define DLL
#include "opencbm.h"

/*-------------------------------------------------------------------*/
/*--------- ASCII <-> PETSCII CONVERSION FUNCTIONS ------------------*/
/*
 * 
 *  These functions are taken from VICE's charset.c,
 *  Copyright
 *      Jouko Valta <jopi(at)stekt(dot)oulu(dot)fi>
 *      Andreas Boose <boose(at)linux(dot)rz(dot)fh-hannover(dot)de>
 *
 *  You can get VICE from http://www.viceteam.org/
 */

/*! \brief Convert a PETSCII character to ASCII

 This function converts a character in PETSCII
 to ASCII.

 \param Character
   The character value to be converted in PETSCII

 \return
   Returns the value of character in ASCII if it
   can be displayed, a dot (".") otherwise.
*/

char CBMAPIDECL 
cbm_petscii2ascii_c(char Character)
{
    switch (Character & 0xff) {
      case 0x0a:
      case 0x0d:
          return '\n';
      case 0x40:
      case 0x60:
        return Character;
      case 0xa0:                                /* CBM: Shifted Space */
      case 0xe0:
        return ' ';
      default:
        switch (Character & 0xe0) {
          case 0x40: /* 41 - 7E */
          case 0x60:
            return (Character ^ 0x20);

          case 0xc0: /* C0 - DF */
            return (Character ^ 0x80);

      }
    }

    return ((isprint(Character) ? Character : '.'));
}


/*! \brief Convert an ASCII character to PETSCII

 This function converts a character in ASCII
 to PETSCII.

 \param Character
   The character value to be converted in ASCII

 \return
   Returns the value of character in PETSCII.
*/

char CBMAPIDECL
cbm_ascii2petscii_c(char Character)
{
    if ((Character >= 0x5b) && (Character <= 0x7e))
    {
        return Character ^ 0x20;
    }
    else if ((Character >= 'A') && (Character <= 'Z'))          /* C0 - DF */
    {
        return Character | 0x80;
    }
    return Character;
}


/*! \brief Convert an null-termined PETSCII string to ASCII

 This function converts a string in PETSCII
 to ASCII.

 \param Str
   Pointer to a buffer which holds a null-termined string
   in PETCII.

 \return
   Returns a pointer to the Str itself, converted to ASCII.

 If some character cannot be printer on the PC, they are
 replaced with a dot (".").
*/

char * CBMAPIDECL
cbm_petscii2ascii(char *Str)
{
    char *p;
    for (p = Str; *p; p++) 
    {
        *p = cbm_petscii2ascii_c(*p);
    }
    return Str;
}

/*! \brief Convert an null-termined ASCII string to PETSCII

 This function converts a string in ASCII
 to PETSCII.

 \param Str
   Pointer to a buffer which holds a null-termined string
   in ASCII.

 \return
   Returns a pointer to the Str itself, converted to PETSCII.
*/

char * CBMAPIDECL 
cbm_ascii2petscii(char *Str)
{
    char *p;
    for (p = Str; *p; p++)
    {
        *p = cbm_ascii2petscii_c(*p);
    }
    return Str;
}
