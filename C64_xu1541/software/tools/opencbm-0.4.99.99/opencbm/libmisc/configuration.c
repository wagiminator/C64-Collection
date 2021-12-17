/*
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *  Copyright 2007-2009 Spiro Trikaliotis
 *
 */

/*! ************************************************************** 
** \file libmisc/configuration.c \n
** \author Spiro Trikaliotis \n
** \n
** \brief Shared library / DLL for accessing the driver
**        Process configuration file
**
****************************************************************/

#include "arch.h"
#include "configuration.h"
#include "libmisc.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*! \brief the maximum line length we expect in a configuration file 
 * \remark: 
 *   If a line is longer, it will still be processed. The only
 *   drawback is that the line is read in with multiple fgets()
 *   calls. Thus, it is slower and produces more work and possibly
 *   fragmentation on the heap.
 */
#define ASSUMED_MAX_LINE_LENGTH 256

/*! a convenient name for opencbm_configuration_entry_s */
typedef struct opencbm_configuration_entry_s opencbm_configuration_entry_t;

/*! this struct holds an element in the configuration file.
 * That is, it holds the equivalent of a line:
 * NAME=VALUE # Comment
 *
 * \remark
 *  If Name == NULL, then this line is NOT of the form
 *  NAME=VALUE # Comment.
 *  Instead, no equal sign is present on that line at all.
 */
struct opencbm_configuration_entry_s {
    opencbm_configuration_entry_t * Next;    /*!< pointer to the next entry; NULL if this is the last one */
    char *                          Name;    /*!< the name of this entry. This can be NULL, cf. remarks*/
    char *                          Value;   /*!< the value of this entry */
    char *                          Comment; /*!< an (optional) comment for this entry */
};


/*! a convenient name for opencbm_configuration_section_s */
typedef struct opencbm_configuration_section_s opencbm_configuration_section_t;

/*! this structs holds a complete section in a configuration file.
 * That is, it is the equivalent of a line of the form
 * [NAME] # Comment
 * and a list of the following lines (Entries) until the next section begins.
 * If the stored section name is a NULL pointer, we are in the first section,
 * before the first line of the form [...]. This is a dummy section only.
 */
struct opencbm_configuration_section_s {
    opencbm_configuration_section_t * Next;    /*!< pointer to the next section; NULL if this is the last one */
    opencbm_configuration_entry_t   * Entries; /*!< pointer to a linked list of the entries in this section */
    char *                            Name;    /*!< the name of this section */
    char *                            Comment; /*!< an (optional) comment which is on the line where the section starts, if any */
};

/*! this struct holds a complete configuration file
 * The handle to the configuration file is a pointer to this struct, actually.
 */
typedef
struct opencbm_configuration_s {
    opencbm_configuration_section_t * Sections;         /*!< pointer to a linked list of sections contained in the configuration file */
    const char *                      FileName;         /*!< the file name of the configuration file */
    const char *                      FileNameForWrite; /*!< the special file name used when the configuration file is written */

    unsigned int Changed;                               /*!< marker if this file has been changed after it has been read. 0 if no changed occurred, 1 otherwise. */
} opencbm_configuration_t;

/*! \brief \internal allocate memory for a new configuration entry

 \param CurrentSection
   Pointer to the current section to which this entry is to be added.

 \param PreviousEntry
   Pointer to the previous entry. That is, the new entry will be added
   after this entry.
   If this is given as NULL, the new entry will be added as the first
   one.

 \param EntryName
   The name of the entry which is to be allocated. That is, this
   is the left-hand side of the equal sign.
   If this is given as NULL, there is no real entry available, but
   an (incorrect) line without an equal sign.

 \param Value
   The value of the entry. That is, this is the right-hand side of
   the equal sign (excluding the commentary).

 \param Comment
   The (optional) comment on this line. All characters (including
   whitespace and the comment delimiter '#') are included.

 \return 
   Pointer to the new configuration entry.
   NULL if we run out of memory.

 \remark
   If any of the parameters Value or Comment are given as NULL, there
   is an empty string allocated for them, anyway.
   This is not true for the parameter EntryName.
*/
static opencbm_configuration_entry_t *
entry_alloc_new(opencbm_configuration_section_t * CurrentSection,
                opencbm_configuration_entry_t * PreviousEntry,
                const char * EntryName,
                const char * Value,
                const char * Comment)
{
    opencbm_configuration_entry_t * newEntry = NULL;

    assert(CurrentSection != NULL);

    do {
        newEntry = malloc(sizeof(*newEntry));

        if (newEntry == NULL)
            break;

        memset(newEntry, 0, sizeof(*newEntry));
        newEntry->Comment = cbmlibmisc_strdup(Comment);
        newEntry->Name = cbmlibmisc_strdup(EntryName);
        newEntry->Next = NULL;

        if (Value) {
            newEntry->Value = cbmlibmisc_strdup(Value);
        }
        else {
            newEntry->Value = NULL;
        }

        if (PreviousEntry != NULL) {
            PreviousEntry->Next = newEntry;
        }
        else {
            newEntry->Next          = CurrentSection->Entries;
            CurrentSection->Entries = newEntry;
        }

    } while (0);

    return newEntry;
}

/*! \brief \internal free the memory of a configuration entry

 \param Entry
   Pointer to the entry to be removed

 \return
   Pointer to the entry that was next after the entry just removed.
*/
static opencbm_configuration_entry_t *
configuration_entry_free(opencbm_configuration_entry_t * Entry)
{
    opencbm_configuration_entry_t * next_entry = NULL;

    assert(Entry != NULL);

    if (Entry != NULL) {
        next_entry = Entry->Next;

        cbmlibmisc_strfree(Entry->Comment);
        cbmlibmisc_strfree(Entry->Name);
        cbmlibmisc_strfree(Entry->Value);

        free(Entry);
    }

    return next_entry;
}

/*! \brief \internal allocate memory for a new configuration section

 \param Configuration
   Pointer to the configuration file structure to which this
   section is to be added.

 \param PreviousSection
   Pointer to the previous section. That is, the new section will be
   added after this entry.
   If this is given as NULL, the new section will be added as the first
   one.

 \param SectionName
   The name of the section which is to be allocated. That is, this
   is the name between the brackets [...] in the configuration file.
   If this is given as NULL, this is the special "first" section,
   which is an unnamed "global" section.

 \param Comment
   The (optional) comment on this line. All characters (including
   whitespace and the comment delimiter '#') are included.

 \return 
   Pointer to the new configuration section.
   NULL if we run out of memory.

 \remark
   If Comment is given as NULL, there is an empty string allocated
   for it, anyway.
   This is not true for the parameter SectionName.
*/
static opencbm_configuration_section_t *
section_alloc_new(opencbm_configuration_t * Configuration,
                  opencbm_configuration_section_t * PreviousSection,
                  const char * const SectionName,
                  const char * const Comment)
{
    opencbm_configuration_section_t * newSection = NULL;

    do {
        newSection = malloc(sizeof(*newSection));

        if (newSection == NULL) {
            break;
        }

        memset(newSection, 0, sizeof(*newSection));
        newSection->Entries = NULL;
        newSection->Comment = cbmlibmisc_strdup(Comment);
        newSection->Next = NULL;

        if (SectionName) {
            newSection->Name = cbmlibmisc_strdup(SectionName);
        }
        else {
            newSection->Name = NULL;
        }

        if (PreviousSection != NULL) {
            PreviousSection->Next = newSection;
        }
        else {
            newSection->Next        = Configuration->Sections;
            Configuration->Sections = newSection;
        }

    } while (0);

    return newSection;
}

/*! \brief \internal free the memory of a complete configuration section

 \param Section
   Pointer to the section to be removed

 \return
   Pointer to the section that was next after the entry just removed.
*/
static opencbm_configuration_section_t *
configuration_section_free(opencbm_configuration_section_t * Section)
{
    opencbm_configuration_section_t * next_section = NULL;

    assert(Section != NULL);

    if (Section != NULL) {
        opencbm_configuration_entry_t * entry;

        next_section = Section->Next;

        entry = Section->Entries;

        while (entry) {
            entry = configuration_entry_free(entry);
        }

        cbmlibmisc_strfree(Section->Comment);
        cbmlibmisc_strfree(Section->Name);

        free(Section);
    }

    return next_section;
}

/*! \brief \internal handle the comment when reading a line of the configuration file

 This function is an internal helper function which is called whenever a
 comment is encountered in the configuration file.

 \param Buffer
   Pointer to the configuration file structure to which this
   section is to be added.

 \return 
   1 if there was a comment on that line and it has been handled,
   0 otherwise.
*/
static unsigned int
configuration_read_line_handle_comment(char * Buffer, char ** Comment)
{
    unsigned int handledComment = 0;
    char * commentBuffer = NULL;

    do {
        if (Comment == NULL || Buffer == NULL || *Buffer == 0)
            break;

        commentBuffer = malloc(strlen(Buffer) + 1);

        if (commentBuffer == NULL) {
            break;
        }

        strcpy(commentBuffer, Buffer);

        *Comment = commentBuffer;

        handledComment = 1;

    } while (0);

    return handledComment;
}

/*! \brief \internal Read a complete line from a file
 @@@@@
*/
static char *
read_a_complete_line(FILE * File)
{
    char * buffer = NULL;
    char * addbuffer = NULL;

    unsigned int error = 1;

    do {
        unsigned int bufferLength;

        addbuffer = cbmlibmisc_stralloc(ASSUMED_MAX_LINE_LENGTH);

        if (addbuffer == NULL) {
            break;
        }

        if (fgets(addbuffer, ASSUMED_MAX_LINE_LENGTH, File) == NULL) {

            /* error or EOF, quit */

            error = ferror(File) ? 1 : 0;
            break;
        }

        /* add the addbuffer to the buffer */

        if (buffer == NULL) {
            buffer = addbuffer;
            addbuffer = NULL;
        }
        else {
            char * tmpbuffer = cbmlibmisc_strcat(buffer, addbuffer);

            cbmlibmisc_strfree(addbuffer);
            addbuffer = NULL;

            cbmlibmisc_strfree(buffer);

            buffer = tmpbuffer;

            if (tmpbuffer == NULL) {
                break;
            }

        }

        /* If there is a newline, the line is complete */

        bufferLength = strlen(buffer);

        if ( (bufferLength > 0) && buffer[bufferLength - 1] == '\n')
        {
            buffer[bufferLength - 1] = 0;

            error = 0;
            break;
        }
    } while (1);

    if (error) {
        cbmlibmisc_strfree(buffer);
        buffer = NULL;
    }
    else {

        if (buffer == NULL) {
            buffer = cbmlibmisc_strdup("");
        }
    }

    cbmlibmisc_strfree(addbuffer);

    return buffer;
}

/*! \brief \internal Read a line of the configuration file

 Get the default filename of the configuration file.

 \param Handle
   Handle to the configuration file.

 \param Comment
   Pointer to a pointer to char. In case there is a comment
   present on the line, the comment will be placed there.
   If this pointer is NULL, comments are ignored.

 \param ConfigFile
   Pointer to an opened FILE structure for the file to be read.

 \return 
   Returns a (static) buffer which holds the current line.

 \remark
   Comment lines (beginning with a '#') and comments at the end of a line are
   ignored. Additionally, SPACEs, TABs, CR and NL at the end of the line are
   ignored, too.
*/
static char *
configuration_read_line(opencbm_configuration_handle Handle, char ** Comment, FILE * ConfigFile)
{
    char * buffer = NULL;
    char * ret = NULL;

    do {
        if (Comment) {
            *Comment = NULL;
        }

        /* If we already reached the end of file, abort here */

        if (feof(ConfigFile))
            break;

        /* If we got an error, abort here */

        if (ferror(ConfigFile))
            break;

        /* Read in a line */

        buffer = read_a_complete_line(ConfigFile);

        if (buffer == NULL) {
            break;
        }

        if (buffer[0] == '#') {
            if (configuration_read_line_handle_comment(buffer, Comment)) {
                cbmlibmisc_strfree(buffer);
                break;
            }
        }
        else {
            char *p;

            ret = buffer;

            /* search for a comment and trim it if it exists */

            p = strchr(buffer, '#');

            /* If there is no comment, begin at the end of line */

            if (p == NULL)
                p = buffer + strlen(buffer);

            while (p && (p > buffer))
            {
                /* trim any spaces from the right, if available */

                switch (*--p)
                {
                case ' ':
                case '\t':
                case 13:
                case 10:
                    break;

                default:
                    configuration_read_line_handle_comment(++p, Comment);
                    *p = 0;
                    p = NULL;
                }
            }

            if (p == buffer)
                *p = 0;

            break;
        }

    } while (0);

    return ret;
}

/*! \brief \internal Parse the configuration file

 This function parses the configuration file and records its contents
 into the internal opencbm_configuration_handle structure.

 \param Handle
   Handle to the configuration file.

 \param ConfigFile
   Pointer to an opened FILE structure for the file to be read.

 \return 
   0 if the parsing succeeded,
   1 otherwise.
*/
static int
opencbm_configuration_parse_file(opencbm_configuration_handle Handle, FILE * ConfigFile)
{
    int error = 1;

    do {
        opencbm_configuration_section_t * currentSection = NULL;
        opencbm_configuration_entry_t   * previousEntry  = NULL;
        char                            * line           = NULL;

        /* First, check if we successfully opened the configuration file */

        if (Handle == NULL)
            break;

        assert(ConfigFile != NULL);

        /* Seek to the beginning of the file */

        fseek(ConfigFile, 0, SEEK_SET);


        Handle->Sections = section_alloc_new(Handle, NULL, NULL, "");
        if (Handle->Sections == NULL) {
            break;
        }

        currentSection = Handle->Sections;

        /* Now, search section after section */

        do {
            char * comment = NULL;

            if (line) {
                cbmlibmisc_strfree(line);
            }

            line = configuration_read_line(Handle, &comment, ConfigFile);

            /* assume an error, if not cleared later */

            error = 1;

            if (line == NULL && comment == NULL) {

                /* The end of the file has been reached */

                error = 0;
                break;
            }

            /* Check if we found a new section */

            if (line && (line[0] == '['))
            {
                char * sectionName = NULL;
                char * p;

                sectionName = cbmlibmisc_strdup(&line[1]);
                if (sectionName == NULL)
                    break;

                p = strrchr(sectionName, ']');

                /* This is tricky. If the current line has no closing bracket,
                 * we will ignore this. Thus, this function will "correct"
                 * this error. Note that this correction can be performed in
                 * an incorrect way. However, changes are higher the user
                 * will recognise this change and find out that he has done
                 * something wrong.
                 */
                if (p != 0) {
                    *p = 0;
                }

                /* a new section starts */

                currentSection = section_alloc_new(Handle, currentSection, sectionName, comment);
                cbmlibmisc_strfree(sectionName);

                if (currentSection == NULL) {
                    break;
                }

                /* make sure to add the new entries to this section, not after
                 * the last entry of the previous section
                 */
                previousEntry = NULL;

                error = 0;
            }
            else {
                char * entryName = NULL;
                char * value = NULL;

                /* this line is (still) part of the current section */

                if (line) {
                    char * p;

                    /* process the entry */

                    p = strchr(line, '=');

                    if (p == NULL) {

                        /* the line is not formatted correctly. It is no real entry! */

                        value = cbmlibmisc_strdup(line);
                    }
                    else {
                        /* split the line into entry name and value */

                        *p = 0;
                        entryName = cbmlibmisc_strdup(line);
                        value = cbmlibmisc_strdup(p+1);
                    }
                }

                previousEntry = entry_alloc_new(currentSection, previousEntry,
                    entryName, value, comment);

                cbmlibmisc_strfree(entryName);
                cbmlibmisc_strfree(value);
                cbmlibmisc_strfree(comment);
                comment = NULL;

                if (previousEntry == NULL) {
                    break;
                }

                error = 0;
            }

            cbmlibmisc_strfree(comment);

        } while ( ! error);

        if (line) {
            cbmlibmisc_strfree(line);
        }

    } while (0);

    return error;
}

/*! \brief \internal Write the configuration file

 This function writes back the configuration file, generating
 the data stored in the internal opencbm_configuration_handle
 structure.

 \param Handle
   Handle to the configuration file.

 \return 
   0 if the writing succeeded,
   1 otherwise.
*/
static int
opencbm_configuration_write_file(opencbm_configuration_handle Handle)
{
    FILE * configfile = NULL;

    int error = 0;

    do {
        opencbm_configuration_section_t * currentSection;

        /* First, check if we successfully opened the configuration file */

        if (Handle == NULL)
            break;

        configfile = fopen(Handle->FileNameForWrite, "wt");

        if (configfile == NULL) {
            error = 1;
            break;
        }

        /* Seek to the beginning of the file */

        fseek(configfile, 0, SEEK_SET);

        for (currentSection = Handle->Sections; 
             (currentSection != NULL) && (error == 0); 
             currentSection = currentSection->Next) {

            opencbm_configuration_entry_t * currentEntry;

            /*
             * Process all section names but the first one.
             * The first section is special as it is no real section
             */
            if (currentSection != Handle->Sections) {
                if (fprintf(configfile, "[%s]%s\n",
                    currentSection->Name, currentSection->Comment) < 0)
                {
                    error = 1;
                }
            }

            for (currentEntry = currentSection->Entries; 
                 (currentEntry != NULL) && (error == 0);
                 currentEntry = currentEntry->Next)
            {
                if (fprintf(configfile, "%s%s%s%s\n", 
                        (currentEntry->Name ? currentEntry->Name : ""),
                        (currentEntry->Name && *(currentEntry->Name)) ? "=" : "",
                        (currentEntry->Value ? currentEntry->Value : ""),
                        currentEntry->Comment) < 0)
                {
                    error = 1;
                }
            }
        }

    } while (0);

    if (configfile) {
        fclose(configfile);
    }

    do {
        if (error != 0) {
            break;
        }

        if (Handle == NULL || Handle->FileName == NULL || Handle->FileNameForWrite == NULL) {
            error = 1;
            break;
        }

        if (arch_unlink(Handle->FileName)) {
            error = 1;
            break;
        }

        if (rename(Handle->FileNameForWrite, Handle->FileName)) {
            error = 1;
            break;
        }

    } while(0);

    return error;
}

/*! \brief Open the configuration file

 Opens the configuration file so it can be used later on with
 opencbm_configuration_get_data(). If the file does not exist,
 this function fails.

 \param Filename
   The name of the configuration file to open

 \return
   Returns a handle to the configuration file which can be used
   in subsequent calls to the other configuration file functions
   for reading. Write operations are not allowed after using
   this function.

   If the configuration file does not exist, this function
   returns NULL.
*/
opencbm_configuration_handle
opencbm_configuration_open(const char * Filename)
{
    opencbm_configuration_handle handle;
    unsigned int error = 1;

    FILE * configFile = NULL;

    do {
        handle = malloc(sizeof(*handle));

        if (!handle) {
            break;
        }

        memset(handle, 0, sizeof(*handle));

        handle->Sections = NULL;

        handle->FileName = cbmlibmisc_strdup(Filename);
        handle->FileNameForWrite = cbmlibmisc_strcat(handle->FileName, ".tmp");
        handle->Changed = 0;

        if ( (handle->FileName == NULL) || (handle->FileNameForWrite == NULL)) {
            break;
        }

        configFile = fopen(handle->FileName, "rt");

        if (configFile == NULL) {
            break;
        }

        opencbm_configuration_parse_file(handle, configFile);

        fclose(configFile);

        error = 0;

    } while (0);

    if (error && handle) {
        cbmlibmisc_strfree(handle->FileName);
        cbmlibmisc_strfree(handle->FileNameForWrite);
        free(handle);
        handle = NULL;
    }

    return handle;
}


/*! \brief Creates the configuration file for reading and writing

 Opens the configuration file so it can be used later on with
 opencbm_configuration_get_data(). If the file does not exist,
 a new, empty one is created.

 \param Filename
   The name of the configuration file to open

 \return
   Returns a handle to the configuration file which can be used
   in subsequent calls to the other configuration file functions.
*/
opencbm_configuration_handle
opencbm_configuration_create(const char * Filename)
{
    opencbm_configuration_handle handle = NULL;

    do {
        handle = opencbm_configuration_open(Filename);

        if (handle == NULL) {

            FILE * filehandle;
            filehandle = fopen(Filename, "wt");

            if (filehandle == NULL)
                break;

            fclose(filehandle);

            handle = opencbm_configuration_open(Filename);
            if (handle == NULL)
                break;
        }

    } while (0);

    return handle;
}

/*! \brief Free all of the memory occupied by the configuration file

 This function frees all of the memory occupied by the configuration
 file in processor memory.
 The file is not deleted from the permanent storage (hard disk).
*/
static void
opencbm_configuration_free_all(opencbm_configuration_handle Handle)
{
    opencbm_configuration_section_t *section;

    section = Handle->Sections;

    while (section != NULL)
    {
        section = configuration_section_free(section);
    }

    cbmlibmisc_strfree(Handle->FileName);
    cbmlibmisc_strfree(Handle->FileNameForWrite);

    free(Handle);
}

/*! \brief Flush the configuration file

 Flushes the configuration file. This is, if it
 has been changed, it is stored to permanent storage.

 \param Handle
   Handle to the opened configuration file, as obtained from
   opencbm_configuration_open()

 \return
   0 if the function succeeded,
   1 otherwise.
*/
int
opencbm_configuration_flush(opencbm_configuration_handle Handle)
{
    int error = 0;

    do {
        if (Handle == NULL) {
            break;
        }

        if (Handle->Changed) {
            error = opencbm_configuration_write_file(Handle);
        }

    } while(0);

    return error;
}

/*! \brief Close the configuration file

 Closes the configuration file after it has been used.
 If it has been changed in the mean time, it is first
 stored to permanent storage.

 \param Handle
   Handle to the opened configuration file, as obtained from
   opencbm_configuration_open()

 \return
   0 if the function succeeded,
   1 otherwise.
*/
int
opencbm_configuration_close(opencbm_configuration_handle Handle)
{
    int error = 0;

    do {
        if (Handle == NULL) {
            break;
        }

        error = opencbm_configuration_flush(Handle);

        opencbm_configuration_free_all(Handle);

    } while(0);

    return error;
}

/*! \internal \brief Find data from the configuration file

 This function searches for a specific entry in the configuration
 file and returns the value found there.

 \param Handle
   Handle to the opened configuration file, as obtained from
   opencbm_configuration_open().
 
 \param Section
   A string which holds the name of the section from where to get the data.

 \param Create
   If 0, we only try to find an existing entry. If none exists, we return
   with NULL.
   If 1, we create a new entry if no entry exists.

 \return
   Returns a pointer to the data entry. If it cannot be found, this
   function returns NULL.
*/
static opencbm_configuration_section_t *
opencbm_configuration_find_section(opencbm_configuration_handle Handle,
                                   const char Section[],
                                   unsigned int Create,
                                   opencbm_configuration_section_t ** PreviousSection)
{
    opencbm_configuration_section_t *currentSection = NULL;
    opencbm_configuration_section_t *lastSection = NULL;

    do {
        /* Check if there is a section given */

        if (Section == NULL) {
            break;
        }

        for (currentSection = Handle->Sections;
             currentSection != NULL;
             currentSection = currentSection->Next)
        {
            int foundSection = 0;

            if (currentSection->Name == NULL) {
                foundSection = Section == NULL;
            }
            else {
                if (Section) {
                    foundSection = (strcmp(currentSection->Name, Section) == 0);
                }
            }

            if (foundSection) {
                break;
            }

            lastSection = currentSection;
        }

        if (Create && currentSection == NULL) {

            /* there was no section with that name, generate a new one */
            
            currentSection = section_alloc_new(Handle, lastSection, Section, NULL);
        }

    } while(0);

    return currentSection;
}

/*! \internal \brief Find data from the configuration file

 This function searches for a specific entry in the configuration
 file and returns the value found there.

 \param Handle
   Handle to the opened configuration file, as obtained from
   opencbm_configuration_open().
 
 \param Section
   A string which holds the name of the section from where to get the data.

 \param Entry
   A string which holds the name of the entry to get.

 \param Create
   If 0, we only try to find an existing entry. If none exists, we return
   with NULL.
   If 1, we create a new entry if no entry exists.

 \param LastEntry
   Pointer to a pointer. Upon successfull return, the pointed to pointer
   will have the address last entry before the entry that was just found.
   This is useful for list manipulations.

   If this entry is the first one in this section, the pointed to pointer
   will be NULL.

 \return
   Returns a pointer to the data entry. If it cannot be found, this
   function returns NULL.
*/
static opencbm_configuration_entry_t *
opencbm_configuration_find_data_ex(opencbm_configuration_handle Handle,
                                   const char Section[], const char Entry[],
                                   unsigned int Create,
                                   opencbm_configuration_entry_t ** LastEntry,
                                   opencbm_configuration_section_t ** LastSection)
{
    opencbm_configuration_section_t *currentSection = NULL;
    opencbm_configuration_section_t *lastSection = NULL;
    opencbm_configuration_entry_t   *lastEntry = NULL;

    opencbm_configuration_entry_t * currentEntry = NULL;

    assert(LastEntry != NULL);
    assert(LastSection != NULL);

    do {
        *LastEntry = NULL;
        *LastSection = NULL;

        /* Check if there is a section and an entry given */

        if (Section == NULL || Entry == NULL) {
            break;
        }

        currentSection = opencbm_configuration_find_section(Handle, Section, Create, &lastSection);

        if (currentSection == NULL) {
            break;
        }

        *LastSection = currentSection;

        {
            for (currentEntry = currentSection->Entries;
                 currentEntry != NULL;
                 currentEntry = currentEntry->Next)
            {
                if (strcmp(currentEntry->Name, Entry) == 0) {
                    break;
                }

                /* This if() ensures that we do not add the line after
                 * some comments which most probably are meant for the next
                 * section.
                 */
                if (currentEntry->Name != NULL) {
                    lastEntry = currentEntry;
                    *LastEntry = currentEntry;
                }
            }
        }

        if (currentEntry || Create == 0) {
            break;
        }

        if (currentSection == NULL) {

            /* there was no section with that name, generate a new one */
            
            currentSection = section_alloc_new(Handle, lastSection, Section, NULL);
        }

        currentEntry = entry_alloc_new(currentSection, lastEntry, Entry, NULL, NULL);

    } while(0);

    return currentEntry;
}

/*! \internal \brief Find data from the configuration file

 This function searches for a specific entry in the configuration
 file and returns the value found there.

 \param Handle
   Handle to the opened configuration file, as obtained from
   opencbm_configuration_open().
 
 \param Section
   A string which holds the name of the section from where to get the data.

 \param Entry
   A string which holds the name of the entry to get.

 \param Create
   If 0, we only try to find an existing entry. If none exists, we return
   with NULL.
   If 1, we create a new entry if no entry exists.

 \return
   Returns a pointer to the data entry. If it cannot be found, this
   function returns NULL.
*/
static opencbm_configuration_entry_t *
opencbm_configuration_find_data(opencbm_configuration_handle Handle,
                                const char Section[], const char Entry[],
                                unsigned int Create)
{
    opencbm_configuration_entry_t * last_entry;
    opencbm_configuration_section_t * section;

    return opencbm_configuration_find_data_ex(Handle, Section, Entry, Create, &last_entry, &section);
}

/*! \brief Read data from the configuration file

 This function searches for a specific enty in the configuration
 file and returns the value found there.

 \param Handle
   Handle to the opened configuration file, as obtained from
   opencbm_configuration_open().
 
 \param Section
   A string which holds the name of the section from where to get the data.

 \param Entry
   A string which holds the name of the entry to get.

 \param ReturnBuffer
   A buffer which holds the return value on success. If the function returns
   with something different than 0, the buffer pointer to by ReturnBuffer will
   not be changed.
   Can be NULL, cf. note below.

 \return
   Returns 0 if the data entry was found. If ReturnBufferLength != 0, the
   return value is 0 only if the buffer was large enough to hold the data.

 \note
   If ReturnBuffer is NULL, this function only tests if the Entry exists
   in the given Section. In this case, this function returns 0; otherwise, it
   returns 1.
*/
int
opencbm_configuration_get_data(opencbm_configuration_handle Handle,
                               const char Section[], const char Entry[],
                               char ** ReturnBuffer)
{
    unsigned int error = 1;

    do {
        opencbm_configuration_entry_t * entry =
            opencbm_configuration_find_data(Handle, Section, Entry, 0);

        if (entry == NULL) {
            break;
        }

        /* If ReturnBufferLength is 0, we only wanted to find out if 
         * that entry existed. Thus, report "no error" and quit.
         */

        if (ReturnBuffer != 0) {

            char * p = cbmlibmisc_strdup(entry->Value);

            if (p != NULL) {
                *ReturnBuffer = p;
                error = 0;
            }
        }

    } while (0);

    return error;
}


/*! \brief Enumerate sections in the configuration file

 This function enumerates all sections in the configuration
 file. For every section name, a given callback function is called.

 \param Handle
   Handle to the opened configuration file, as obtained from
   opencbm_configuration_open().
 
 \param Callback
   The callback function to call with the section name

 \param Data
   Some data which is forwarded to the Callback function.

 \return
   Returns 0 if the data entry was found. If ReturnBufferLength != 0, the
   return value is 0 only if the buffer was large enough to hold the data.

 \note
   If ReturnBufferLength is zero, this function only tests if the Entry exists
   in the given Section. In this case, this function returns 0; otherwise, it
   returns 1.
*/
int
opencbm_configuration_enum_sections(opencbm_configuration_handle Handle,
                                    opencbm_configuration_enum_sections_callback_t Callback,
                                    void * Data)
{
    unsigned int error = 0;

    do {
        opencbm_configuration_section_t *currentSection = NULL;

        for (currentSection = Handle->Sections;
             currentSection != NULL;
             currentSection = currentSection->Next)
        {
            error = error || Callback( Handle, currentSection->Name, Data);
        }

    } while (0);

    return error;
}

/*! \brief Enumerate data in the configuration file

 This function enumerates all entries in a given section
 of the configuration file. For every entry, a given callback
 function is called.

 \param Handle
   Handle to the opened configuration file, as obtained from
   opencbm_configuration_open().
 
 \param Section
   A string which holds the name of the section from where to get the data.

 \param Callback
   The callback function to call with the section name

 \param Data
   Some data which is forwarded to the Callback function.

 \return
   Returns 0 if the data entry was found. If ReturnBufferLength != 0, the
   return value is 0 only if the buffer was large enough to hold the data.

 \note
   If ReturnBufferLength is zero, this function only tests if the Entry exists
   in the given Section. In this case, this function returns 0; otherwise, it
   returns 1.
*/
int
opencbm_configuration_enum_data(opencbm_configuration_handle Handle,
                                const char Section[],
                                opencbm_configuration_enum_data_callback_t Callback,
                                void * Data)
{
    unsigned int error = 0;

    do {
        opencbm_configuration_entry_t * currentEntry = NULL;
        opencbm_configuration_section_t * currentSection;

        currentSection = opencbm_configuration_find_section(Handle,
                                   Section, 0, NULL);

        if ( ! currentSection ) {
            error = 0;
            break;
        }

        for (currentEntry = currentSection->Entries;
             currentEntry != NULL;
             currentEntry = currentEntry->Next)
        {
            error = error || Callback( Handle, currentSection->Name, currentEntry->Name, Data);
        }

    } while (0);

    return error;
}

/*! \brief Write/Change data to/in the configuration file

 This function searches for a specific entry in the configuration
 file and changes it if it exists. If it does not exist, a new
 entry is generated.

 \param Handle
   Handle to the opened configuration file, as obtained from
   opencbm_configuration_open().
 
 \param Section
   A string which holds the name of the section where to set the data.

 \param Entry
   A string which holds the name of the entry to set.

 \param Value
   A buffer which holds the value of the entry which is to be set.

 \return
   0 if the data could be written,
   1 otherwise 
*/
int
opencbm_configuration_set_data(opencbm_configuration_handle Handle,
                               const char Section[], const char Entry[],
                               const char Value[])
{
    unsigned int error = 1;

    do {
        char * newValue = NULL;

        opencbm_configuration_entry_t * entry =
            opencbm_configuration_find_data(Handle, Section, Entry, 1);

        if (entry == NULL) {
            break;
        }

        Handle->Changed = 1;

        newValue = cbmlibmisc_strdup(Value);

        if (newValue == NULL) {
            break;
        }

        cbmlibmisc_strfree(entry->Value);
        entry->Value = newValue;

        error = 0;

    } while (0);

    return error;
}

/*! \brief Remove a complete section from the configuration file

 This function searches for a specific section in the configuration
 file and completely removes it if it exists.

 \param Handle
   Handle to the opened configuration file, as obtained from
   opencbm_configuration_open().
 
 \param Section
   A string which holds the name of the section to remove.

 \return
   0 if the data was removed.
   1 otherwise. This means the section did not exist in the first place.

 \todo
   Test opencbm_configuration_section_remove()
*/
int
opencbm_configuration_section_remove(opencbm_configuration_handle Handle,
                                     const char Section[])
{
    opencbm_configuration_section_t * section = NULL;
    opencbm_configuration_section_t * previous_section = NULL;

    int error = 1;

    do {
        section = opencbm_configuration_find_section(Handle, Section, 0, &previous_section);

        if ( ! section ) {
            break;
        }

        if (previous_section == NULL) {
            Handle->Sections = configuration_section_free(section);
        }
        else {
            assert( previous_section->Next == section );

            previous_section->Next = configuration_section_free(section);
        }

        error = 0;

    } while (0);

    return error;
}

/*! \brief Remove an entry from the configuration file

 This function searches for a specific entry in a given section
 in the configuration file and removes it if it exists.

 \param Handle
   Handle to the opened configuration file, as obtained from
   opencbm_configuration_open().
 
 \param Section
   A string which holds the name of the section from which to remove.

 \param EntryName
   A string which holds the name of the entry to remove.

 \return
   0 if the data was removed.
   1 otherwise. This means the entry did not exist in the first place.

 \todo
   Test opencbm_configuration_entry_remove()
*/
int
opencbm_configuration_entry_remove(opencbm_configuration_handle Handle,
                               const char Section[], const char EntryName[])
{
    int error = 1;

    do {
        opencbm_configuration_entry_t * entry;
        opencbm_configuration_entry_t * last_entry;
        opencbm_configuration_section_t * section;

        entry = opencbm_configuration_find_data_ex(Handle, Section, EntryName, 0,
                                                   &last_entry, &section);

        if ( ! entry ) {
            break;
        }

        assert(section->Entries == entry);

        if (last_entry == NULL) {
            /*
             * this entry is the first one in this section.
             * Thus, update the section structure
             */
            section->Entries = configuration_entry_free(entry);
        }
        else {
            /* remove the entry from the list, and free its memory */
            last_entry->Next = configuration_entry_free(entry);
        }

        error = 0;

    } while (0);

    return error;
}

/* #define OPENCBM_STANDALONE_TEST 1 */

#ifdef OPENCBM_STANDALONE_TEST

#if 0
    #ifndef NDEBUG
        #include <crtdbg.h>
    #endif
#endif

static void
EnableCrtDebug(void)
{
#if 0
#ifndef NDEBUG
    int tmpFlag;

    // Get current flag
    tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);

    // Turn on leak-checking bit
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF;
    tmpFlag |= _CRTDBG_CHECK_ALWAYS_DF;

    tmpFlag |= _CRTDBG_ALLOC_MEM_DF;

    // Set flag to the new value
    _CrtSetDbgFlag(tmpFlag);
#endif
#endif
}

static unsigned int started_an_op = 0;

static void 
OpSuccess(void)
{
    fprintf(stderr, "success.\n\n");
    fflush(stderr);
    started_an_op = 0;
}

static void 
OpFail(void)
{
    fprintf(stderr, "FAILED!\n\n");
    fflush(stderr);
    started_an_op = 0;
}

static void
OpEnd(void)
{
    if (started_an_op)
        OpSuccess();
}

static void 
OpStart(const char * const Operation)
{
    OpEnd();

    started_an_op = 1;

    fprintf(stderr, "%s() ... ", Operation);
    fflush(stderr);
}

static int enum_data_callback(opencbm_configuration_handle Handle,
                              const char Section[],
                              const char Entry[],
                              void * Data)
{
    int error = 0;

    fprintf(stderr, "\n    enum_data_callback(Handle, %s, %s, 0x%p\n", Section, Entry, Data);
    fflush(stderr);

    return error;
}

static int enum_sections_callback(opencbm_configuration_handle Handle,
                                  const char Section[],
                                  void * Data)
{
    int error = 0;

    fprintf(stderr, "\n  enum_sections_callback(Handle, %s, 0x%p\n", Section, Data);
    fflush(stderr);
    opencbm_configuration_enum_data(Handle, Section, enum_data_callback, Data);

    return error;
}

/*! \brief Simple test case for configuration

 This function implements a very simple test case for
 the configuration

 \return
   EXIT_SUCCESS on success,
   else EXIT_ERROR
*/
int ARCH_MAINDECL main(void)
{
    int errorcode = EXIT_FAILURE;

    EnableCrtDebug();

    do {
        char * buffer;

        opencbm_configuration_handle handle = NULL;

        OpStart("opencbm_configuration_create()");
        handle = opencbm_configuration_create("TestFile.inf");

        if (handle == NULL) {
            break;
        }


        OpStart("opencbm_configuration_set_data(\"SectTest\", \"EntryTest\", \"VALUE\")");
        if (opencbm_configuration_set_data(handle, "SectTest", "EntryTest", "VALUE")) {
            break;
        }

        OpStart("opencbm_configuration_set_data(\"SectTest\", \"NewTest\", \"AnotherVALUE\")");
        if (opencbm_configuration_set_data(handle, "SectTest", "NewTest", "AnotherVALUE")) {
            break;
        }


        OpStart("opencbm_configuration_get_data(handle, \"SectTest\", \"NewTest\")");
        if (opencbm_configuration_get_data(handle, "SectTest", "NewTest", &buffer) != 0) {
            break;
        }
        OpEnd();
        fprintf(stderr, "  returned: %s\n", buffer);


        OpStart("opencbm_configuration_set_data(\"NewSect\", \"AEntryTest\", \"aVALUE\")");
        if (opencbm_configuration_set_data(handle, "NewSect", "AEntryTest", "aVALUE")) {
            break;
        }

        OpStart("opencbm_configuration_set_data(\"NewSect\", \"BNewTest\", \"bAnotherVALUE\")");
        if (opencbm_configuration_set_data(handle, "NewSect", "BNewTest", "bAnotherVALUE")) {
            break;
        }


        OpStart("opencbm_configuration_set_data(\"SectTest\", \"NewTest\", \"RewrittenVALUE\")");
        if (opencbm_configuration_set_data(handle, "SectTest", "NewTest", "RewrittenVALUE")) {
            break;
        }


        OpStart("opencbm_configuration_get_data(handle, \"SectTest\", \"NewTest\")");
        if (opencbm_configuration_get_data(handle, "SectTest", "NewTest", &buffer) != 0) {
            break;
        }
        OpEnd();
        fprintf(stderr, "  returned: %s\n", buffer);

        OpStart("opencbm_configuration_enum_sections(handle, ..., NULL)");
        opencbm_configuration_enum_sections(handle, enum_sections_callback, NULL);
        OpEnd();

        OpStart("opencbm_configuration_enum_sections(handle, ..., 0x12345678)");
        opencbm_configuration_enum_sections(handle, enum_sections_callback, 0x12345678);
        OpEnd();
#if 0


int
opencbm_configuration_enum_sections(opencbm_configuration_handle Handle,
                                    opencbm_configuration_enum_sections_callback_t Callback,
                                    void * Data)
#endif

        OpStart("opencbm_configuration_close()");
        if (opencbm_configuration_close(handle) != 0) {
            break;
        }

        errorcode = EXIT_SUCCESS;

    } while(0);

    if (errorcode == EXIT_SUCCESS) {
        OpEnd();
    }
    else {
        OpFail();
    }

    return errorcode;
}

#endif /* #ifdef OPENCBM_STANDALONE_TEST */
