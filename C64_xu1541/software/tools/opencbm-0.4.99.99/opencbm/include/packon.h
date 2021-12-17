/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2006 Spiro Trikaliotis
 */

/*! **************************************************************
** \file include/packon.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Make sure that struct definitions are packed
****************************************************************/

#if (_MSC_VER >= 1200) // MSVC 6 or newer

 /*
  * Disable warnings about included file changing packing level (C4103).
  * That is the whole point of this file.
  */
 #pragma warning(disable: 4103)
 #pragma pack(push, packonoff, 1)

#elif (__GNUC__ >= 3)

 #pragma pack(1)

#else

 #pragma error "Unknown compiler!"

#endif
