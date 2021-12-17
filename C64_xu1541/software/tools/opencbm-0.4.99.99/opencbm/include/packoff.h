/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2006 Spiro Trikaliotis
 */

/*! **************************************************************
** \file include/packoff.h \n
** \author Spiro Trikaliotis \n
** \n
** \brief Make sure that struct definitions are not packed anymore
****************************************************************/

#if (_MSC_VER >= 1200) // MSVC 6 or newer

 #pragma pack(pop, packonoff)

#elif (__GNUC__ >= 3)

 #pragma pack()

#else

 #pragma error "Unknown compiler!"

#endif
