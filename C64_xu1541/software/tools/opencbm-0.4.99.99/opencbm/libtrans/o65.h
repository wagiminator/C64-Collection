/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2006, 2009 Spiro Trikaliotis
*/

#ifndef O65_H
#define O65_H

typedef
enum O65ERR {
    O65ERR_NO_ERROR                       =   0,
    O65ERR_UNSPECIFIED                    =  -1,
    O65ERR_NO_DATA                        =  -2,
    O65ERR_NO_FILE                        =  -3,
    O65ERR_FILE_EMPTY                     =  -4,
    O65ERR_FILE_TOO_LARGE                 =  -5,
    O65ERR_UNEXPECTED_END_OF_FILE         =  -6,
    O65ERR_STRING_TOO_LONG                =  -7,
    O65ERR_OPTIONAL_SYSTEM_TOO_SHORT      =  -8,
    O65ERR_OPTIONAL_HEADER_TOO_SHORT      =  -9,
    O65ERR_OPTIONAL_HEADER_NOT_TERMINATED = -10,
    O65ERR_UNKNOWN_SYSTEM                 = -11,
    O65ERR_UNKNOWN_HEADER_OPTION          = -12,
    O65ERR_OUT_OF_MEMORY                  = -13,
    O65ERR_STRING_ERROR_OR_MEMORY         = -14,
    O65ERR_65816_NOT_ALLOWED              = -15,
    O65ERR_UNDEFINED_REFERENCE            = -16,
    O65ERR_NO_O65_FILE                    = -17,
    O65ERR_UNKNOWN_VERSION                = -18,
    O65ERR_FILE_HANDLING_ERROR            = -19,
    O65ERR_UNKNOWN_CPU_SPECIFICATION      = -20
} O65ERR;

extern int o65_file_process(char *Buffer, unsigned Length, void **PO65file);
extern int o65_file_load(const char * const Filename, void **PO65file);
extern int o65_file_reloc(void *O65file, unsigned int Address);
extern void o65_file_delete(void *O65file);

#endif /* #ifndef O65_H */
