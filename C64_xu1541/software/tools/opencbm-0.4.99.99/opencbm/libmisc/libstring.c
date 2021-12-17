/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2007,2008 Spiro Trikaliotis
 *
*/

/*! ************************************************************** 
** \file libmisc/libstring.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Helper function for string handling
**
****************************************************************/

#include "arch.h"

#include <stdlib.h>

/*! \brief allocate memory for a string of a given size

 This function allocates memory for a string with
 a given size.

 \param Length
   The size of the string.
   The trailing zero does not count, that is, extra
   space for an additional zero is allocated.

 \return
   NULL on failure;
   otherwise, a pointer to the newly allocated memory.

 \remark
   The string must be freed with cbmlibmisc_strfree().
*/
char *
cbmlibmisc_stralloc(unsigned int Length)
{
    char * buffer = NULL;

    do {
        buffer = malloc(Length + 1);

        if (buffer == NULL) {
            break;
        }

        /* make sure the string is empty, and that the byte after 
         * the "usable" space is an end marker, too.
         */
        buffer[0] = 0;
        buffer[Length] = 0;

    } while (0);

    return buffer;
}

/*! \brief Duplicate a given string

 This function allocates memory for a string.
 This string is initialised with a copy of the given
 parameter string.

 \param OldString
   The string to duplicate. Can also be NULL.

 \return
   NULL on failure;
   otherwise, a pointer to the newly allocated string.

 \remark
   If OldString is given as a NULL pointer, this function
   allocates an empty string (<i>not</i> a NULL pointer)

 \remark
   The string must be freed with cbmlibmisc_strfree().
*/
char *
cbmlibmisc_strdup(const char * const OldString)
{
    const char * oldString = "";
    char * newString = NULL;
    int len;

    if (OldString) {
        oldString = OldString;
    }

    len = strlen(oldString) + 1;

    newString = malloc(len);

    if (newString) {
        memcpy(newString, oldString, len);
    }
    return newString;
}

/*! \brief Duplicate a prefix of a given string

 This function allocates memory for a string.
 This string is initialised with a copy of a 
 prefix of the given parameter string.

 \param OldString
   The string to duplicate. Can also be NULL.

 \param Length
   The size of the prefix to copy.

 \return
   NULL on failure;
   otherwise, a pointer to the newly allocated string.

 \remark
   If the OldString has Length characters or more, the
   first Length characters are returned in the new copy
   of the string.

   If the OldString has less than Length characters, a
   full copy of OldString is returned.

 \remark
   If OldString is given as a NULL pointer, this function
   allocates an empty string (<i>not</i> a NULL pointer)

 \remark
   The string must be freed with cbmlibmisc_strfree().
*/
char *
cbmlibmisc_strndup(const char * const OldString, size_t Length)
{
    char * newString = NULL;

    if (OldString)
    {
        size_t len = strlen(OldString);

        len = len < Length ? len : Length;

        newString = malloc(len + 1);

        if (newString)
        {
            memcpy(newString, OldString, len);
            newString[len] = 0;
        }
    }
    else
    {
        newString = cbmlibmisc_strdup(NULL);
    }

    return newString;
}

/*! \brief Free a string

 This function frees a string which has been
 previously allocated by any of the functions in
 this file.

 \param String
   A pointer to the string to be freed.
*/
void
cbmlibmisc_strfree(const char * String)
{
    void * p = (void *) String;

    if (String) {
        free(p);
    }
}

/*! \brief Concatenate two strings

 This function concatenates two strings and returns the
 result in a malloc()ed memory region.

 \param First
   The first string to concatenate.
   If this pointer is NULL, an empty string is assumed.

 \param Second
   The second string to concatenate.
   If this pointer is NULL, an empty string is assumed.

 \return
   The malloc()ed memory for the concatenated string, or NULL
   if there was not enough memory.

 \remark
   The string must be freed with cbmlibmisc_strfree().
*/
char *
cbmlibmisc_strcat(const char * First, const char * Second)
{
    char * ret = NULL;
    const char * string1 = "";
    const char * string2 = "";

    do {
        if (First) {
            string1 = First;
        }

        if (Second) {
            string2 = Second;
        }

        ret = malloc(strlen(string1) + strlen(string2) + 1);

        if (ret == NULL)
            break;

        strcpy(ret, string1);
        strcat(ret, string2);

    } while(0);

    return ret;
}
