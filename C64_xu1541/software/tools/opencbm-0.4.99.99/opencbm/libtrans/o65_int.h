/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2005 Spiro Trikaliotis
*/

#ifndef O65_INT_H
#define O65_INT_H

#pragma warning( push )
#pragma warning( disable: 4103 )
#include "packon.h"

/*
 * currently, v1.3 of the o65 file format as specified at
 * http://www.6502.org/users/andre/o65/fileformat.html
 * (from 31 mar 2005) is supported.
 */

typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef unsigned int   uint32;

typedef
struct o65_file_header_common_s
{
    uint8  marker[2]; /* must be $01, $00 */
    uint8  o65[3];    /* must be $6f, $36, $35 == "o65" */
    uint8  version;   /* currently, only 0 is defined */
    uint16 mode;      /* see below */
} o65_file_header_common_t;

#define O65_FILE_HEADER_MODE_65816       0x8000 /* if set, uses 65816 code; if not set, uses 6502 */
#define O65_FILE_HEADER_MODE_PAGERELOC   0x4000 /* pagewise relocation allowed only */
#define O65_FILE_HEADER_MODE_SIZE32      0x2000 /* size is 32 bit; else, it is 16 bit */
#define O65_FILE_HEADER_MODE_OBJFILE     0x1000 /* if defined, this is an OBJ file; else, it is an executable */
#define O65_FILE_HEADER_MODE_SIMPLE      0x0800 /* if defined, this file uses the "simple" approach (new for v1.3) */
#define O65_FILE_HEADER_MODE_CHAIN       0x0400 /* if set, another file follows this one */
#define O65_FILE_HEADER_MODE_BSSZERO     0x0200 /* if set, the bss segment must be zeroed out */

#define O65_FILE_HEADER_MODE_UNUSED      0x010C /* this bits should all be 0 */

#define O65_FILE_HEADER_MODE_CPU_MASK     0x00F0 /* mask for getting the CPU */
#define O65_FILE_HEADER_MODE_CPU_6502     0x0000 /* CPU is a 6502 core, no illegal opcodes */
#define O65_FILE_HEADER_MODE_CPU_65C02    0x0010 /* CPU is a 65C02 core w/ some bugfix, no illegal opcodes */
#define O65_FILE_HEADER_MODE_CPU_65SC02   0x0020 /* CPU is a 65SC02 core (enhanced 65C02), some new opcodes */
#define O65_FILE_HEADER_MODE_CPU_65CE02   0x0030 /* CPU is a 65CE02 core some 16 bit ops/branches, Z register modifiable */
#define O65_FILE_HEADER_MODE_CPU_NMOS6502 0x0040 /* CPU is an NMOS 6502 core (including undocumented opcodes) */
#define O65_FILE_HEADER_MODE_CPU_65816    0x0050 /* CPU is a 65816 in 6502 emulation mode */

#define O65_FILE_HEADER_MODE_ALIGN_MASK  0x0003 /* mask for getting the alignment */
#define O65_FILE_HEADER_MODE_ALIGN_BYTE  0x0000 /* alignment on arbitry bytes */
#define O65_FILE_HEADER_MODE_ALIGN_WORD  0x0001 /* alignment on double-byte boundaries */
#define O65_FILE_HEADER_MODE_ALIGN_QUAD  0x0002 /* alignment on quad-byte boundaries */
#define O65_FILE_HEADER_MODE_ALIGN_PAGE  0x0003 /* alignmont on pages (256 byte) only */

typedef
struct o65_file_header_16_s
{
    uint16 tbase; /* address to which text is assembled to originally */
    uint16 tlen;  /* length of text segment */
    uint16 dbase; /* originating address for data segment */
    uint16 dlen;  /* length of data segment */
    uint16 bbase; /*  originating address for bss segment */
    uint16 blen;  /* length of bss segment */
    uint16 zbase; /* originating address for zero segment */
    uint16 zlen;  /* length of zero segment */
    uint16 stack; /* minimum needed stack size, 0= not known.
                     the OS should add reasonable values for interrupt
                     handling before allocating stack space */
} o65_file_header_16_t;

typedef
struct o65_file_header_32_s
{
    uint32 tbase; /* address to which text is assembled to originally */
    uint32 tlen;  /* length of text segment */
    uint32 dbase; /* originating address for data segment */
    uint32 dlen;  /* length of data segment */
    uint32 bbase; /* originating address for bss segment */
    uint32 blen;  /* length of bss segment */
    uint32 zbase; /* originating address for zero segment */
    uint32 zlen;  /* length of zero segment */
    uint32 stack; /* minimum needed stack size, 0= not known.
                     the OS should add reasonable values for interrupt
                     handling before allocating stack space */
} o65_file_header_32_t;


/* optional headers: */
typedef
struct o65_file_header_oheader_s
{
    uint8 optionlength;
    uint8 optiontype;
    uint8 data[1];
} o65_file_header_oheader_t, *po65_file_header_oheader_t;

#define O65_FILE_HEADER_OHEADER_TYPE_FILENAME  0
#define O65_FILE_HEADER_OHEADER_TYPE_OS_SYSTEM 1
#define O65_FILE_HEADER_OHEADER_TYPE_ASSEMBLER 2
#define O65_FILE_HEADER_OHEADER_TYPE_AUTHOR    3
#define O65_FILE_HEADER_OHEADER_TYPE_CREATION  4

#define O65_FILE_HEADER_OHEADER_TYPE_OS_OSA65  1
#define O65_FILE_HEADER_OHEADER_TYPE_OS_LUNIX  2
#define O65_FILE_HEADER_OHEADER_TYPE_OS_CC65   3
#define O65_FILE_HEADER_OHEADER_TYPE_OS_OPENCBM 4


#define O65_FILE_RELOC_SEGMTYPEBYTE_UNDEF_MASK  0x18
#define O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_MASK   0x07
#define O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_MASK   0xE0

#define O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_UNDEF  0x00
#define O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_ABS    0x01
#define O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_TEXT   0x02
#define O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_DATA   0x03
#define O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_BSS    0x04
#define O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_ZERO   0x05

#define O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_WORD   0x80
#define O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_HIGH   0x40
#define O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_LOW    0x20
#define O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_SEGADR 0xc0
#define O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_SEG    0xa0


#pragma warning( disable: 4103 )
#include "packoff.h"
#pragma warning( pop )

#endif /* #ifndef O65_INT_H */
