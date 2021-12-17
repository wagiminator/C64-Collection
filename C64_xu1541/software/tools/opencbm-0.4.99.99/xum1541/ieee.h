//-------------------------------------------------------------------------
// Titel:    XS-1541 - IEE488 Handler
// Funktion: handle communication to parallel IEEE-488 floppy drives
//-------------------------------------------------------------------------
// Copyright (C) 2008  Thomas Winkler <t.winkler@tirol.com>
//-------------------------------------------------------------------------
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version
// 2 of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//-------------------------------------------------------------------------

#ifndef IEEE_H
#define IEEE_H

#ifdef IEEE_EOI_IO    
  #define IEEE_EOI_I    IEEE_EOI_IO
  #define IEEE_EOI_O    IEEE_EOI_IO
#endif
#ifdef IEEE_ATN_IO
  #define IEEE_ATN_I    IEEE_ATN_IO
  #define IEEE_ATN_O    IEEE_ATN_IO
#endif
#ifdef IEEE_DAV_IO
  #define IEEE_DAV_I    IEEE_DAV_IO
  #define IEEE_DAV_O    IEEE_DAV_IO
#endif
#ifdef IEEE_IFC_IO
  #define IEEE_IFC_I    IEEE_IFC_IO
  #define IEEE_IFC_O    IEEE_IFC_IO
#endif
#ifdef IEEE_SRQ_IO
  #define IEEE_SRQ_I    IEEE_SRQ_IO
  #define IEEE_SRQ_O    IEEE_SRQ_IO
#endif
#ifdef IEEE_NDAC_IO
  #define IEEE_NDAC_I   IEEE_NDAC_IO
  #define IEEE_NDAC_O   IEEE_NDAC_IO
#endif
#ifdef IEEE_NRFD_IO
  #define IEEE_NRFD_I   IEEE_NRFD_IO
  #define IEEE_NRFD_O   IEEE_NRFD_IO
#endif
#ifdef IEEE_REN_IO
  #define IEEE_REN_I    IEEE_REN_IO
  #define IEEE_REN_O    IEEE_REN_IO
#endif

#define IEEE_ST_WRTO    _BV(0) // write timeout
#define IEEE_ST_RDTO    _BV(1) // read timeout
#define IEEE_ST_ATTO    _BV(2) // ATN/DAV timeout
#define IEEE_ST_EOI     _BV(6) // EOI
#define IEEE_ST_DNP     _BV(7) // device not present

#endif // IEEE_H
