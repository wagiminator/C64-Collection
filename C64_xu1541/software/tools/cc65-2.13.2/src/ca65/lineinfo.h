/*****************************************************************************/
/*                                                                           */
/*				  lineinfo.h                                 */
/*                                                                           */
/*			Source file line info structure                      */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 2001      Ullrich von Bassewitz                                       */
/*               Wacholderweg 14                                             */
/*               D-70597 Stuttgart                                           */
/* EMail:        uz@cc65.org                                                 */
/*                                                                           */
/*                                                                           */
/* This software is provided 'as-is', without any expressed or implied       */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated but is not required.                                       */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/



/* Note: The line infos kept here are additional line infos supplied by the
 * ".dbg line" command. The native line infos are always kept in the fragments
 * itself (because one fragment always originates from one line). The
 * additional line infos (which may not exist if none are supplied in the
 * source) may have several fragments attached (as is the case with sources
 * generated by the C compiler).
 */



#ifndef LINEINFO_H
#define LINEINFO_H



/* common */
#include "coll.h"
#include "filepos.h"

/* ca65 */
#include "global.h"



/*****************************************************************************/
/*				     Data                                    */
/*****************************************************************************/



/* The LineInfo structure is shared between several fragments, so we need a
 * reference counter.
 */
typedef struct LineInfo LineInfo;
struct LineInfo {
    unsigned   	    Usage;                /* Usage counter */
    unsigned        Index;                /* Index */
    FilePos         Pos;                  /* File position */
};

/* Collection containing all line infos */
extern Collection LineInfoColl;
extern unsigned  LineInfoValid;           /* Valid, that is, used entries */

/* Global pointer to last line info or NULL if not active */
extern LineInfo* CurLineInfo;



/*****************************************************************************/
/*     	       	      	      	     Code			     	     */
/*****************************************************************************/



LineInfo* UseLineInfo (LineInfo* LI);
/* Increase the reference count of the given line info and return it. The
 * function will gracefully accept NULL pointers and do nothing in this case.
 */

void GenLineInfo (unsigned FileIndex, unsigned long LineNum);
/* Generate a new line info */

void ClearLineInfo (void);
/* Clear the current line info */

void MakeLineInfoIndex (void);
/* Walk over the line info list and make an index of all entries ignoring
 * those with a usage count of zero.
 */

void WriteLineInfo (void);
/* Write a list of all line infos to the object file. */



/* End of lineinfo.h */
#endif


