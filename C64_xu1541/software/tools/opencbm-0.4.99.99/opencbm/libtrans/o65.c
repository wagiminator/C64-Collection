/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 2006, 2009 Spiro Trikaliotis
*/

/*! Mark: We are in user-space (for debug.h) */
#define DBG_USERMODE

/*! The name of the executable */
#define DBG_PROGNAME "LIBTRANS.DLL"

#include "debug.h"

#include "arch.h"

#include "o65.h"
#include "o65_int.h"

#if 1
 #define DBG_O65_SHOW(_x_)              DBG_PRINT(_x_)
 #define DBG_O65_MEMDUMP(_x_, _y_, _z_) DBG_MEMDUMP(_x_, _y_, _z_)
#else
 #define DBG_O65_SHOW(_x_)
 #define DBG_O65_MEMDUMP(_x_, _y_, _z_)
#endif

/*-----------------------------------------------------------*/
/* functions for implementing linked lists                   */

typedef
struct linkedlist_node_s
{
    struct linkedlist_node_s *next;
    struct linkedlist_node_s *item;
} linkedlist_node_t;

typedef
struct linkedlist_list_s
{
    linkedlist_node_t *first;

} linkedlist_list_t;

static void
linkedlist_list_init(linkedlist_node_t * const ListHead)
{
    FUNC_ENTER();

    DBG_ASSERT(ListHead != NULL);

    ListHead->next = (linkedlist_node_t *) &(ListHead->item);
    ListHead->item = (linkedlist_node_t *) &(ListHead->item);

    FUNC_LEAVE();
}

static int
linkedlist_is_last(const linkedlist_node_t * const Node)
{
    int isLast;

    FUNC_ENTER();

    isLast = Node->next == Node;

    FUNC_LEAVE_INT(isLast);
}

static linkedlist_node_t *
linkedlist_insertafter(linkedlist_node_t * const Previous, void * const Item)
{
    linkedlist_node_t *newNode;

    FUNC_ENTER();

    DBG_ASSERT(Previous != NULL);
    DBG_ASSERT(Item != NULL);

    newNode = malloc(sizeof(linkedlist_node_t));

    if (!newNode)
    {
        DBG_ERROR((DBG_PREFIX
            "Not enough memory for allocating new linked list node."));
        exit(1);
    }

    newNode->next = Previous->next;
    newNode->item = Item;
    Previous->next = newNode;

    FUNC_LEAVE_PTR(newNode, linkedlist_node_t *);
}

static void *
linkedlist_removeafter(linkedlist_node_t * const Previous)
{
    linkedlist_node_t *currentNode;
    void *currentItem;

    FUNC_ENTER();

    DBG_ASSERT(Previous != NULL);
    DBG_ASSERT(linkedlist_is_last(Previous->next) == 0);

    currentNode = Previous->next;
    currentItem = currentNode->item;

    Previous->next = currentNode->next;

    DBGDO(currentNode->next = NULL);
    DBGDO(currentNode->item = NULL);

    free(currentNode);

    FUNC_LEAVE_PTR(currentItem, void*);
}

/*-----------------------------------------------------------*/
/* functions for implementing the symbol table of the loader */

typedef
struct o65_symboltable_entry
{
    unsigned char *module;  /* name of the module which contains this symbol */
    unsigned char *name;    /* name of the symbol */
    uint16         address; /* address to where this symbol is located */
} o65_symbol;

#define O65_SYMBOLTABLE_MAX 1000

static o65_symbol o65_symboltable[O65_SYMBOLTABLE_MAX];
static int        o65_symboltable_count = 0;


static char *
stralloc(const char * const String)
{
    char *p;

    FUNC_ENTER();

    p = malloc(strlen(String)+1);

    DBG_ASSERT(p != NULL);

    if (p)
    {
        strcpy(p, String);
    }

    FUNC_LEAVE_STRING(p);
}

static int
o65_symbol_search(const char * const Name)
{
    int i;
    int found = -1;

    FUNC_ENTER();

    for (i = 0; i < o65_symboltable_count; i++)
    {
        if (strcmp(o65_symboltable[i].name, Name) == 0)
        {
            found = i;
            break;
        }
    }

    FUNC_LEAVE_INT(found);
}

static int
o65_symbol_add(const char * const Name, uint16 Address, const char * const Module)
{
    int entry;

    FUNC_ENTER();

    DBG_O65_SHOW((DBG_PREFIX "Adding symbol '%s' at $%04X, module '%s'.",
        Name, Address, Module));

    /* check if the symbol already exists */

    entry = o65_symbol_search(Name);

    if (entry >= 0)
    {
        DBG_ERROR((DBG_PREFIX "Trying to add symbol %s which already exists!",
            Name));

        entry = -1;
    }
    else
    {
        /* advance the number of symbols in the table */
        o65_symboltable[o65_symboltable_count].module = stralloc(Module);
        o65_symboltable[o65_symboltable_count].name = stralloc(Name);
        o65_symboltable[o65_symboltable_count].address = Address;

        o65_symboltable_count++;

        DBG_ASSERT(o65_symboltable_count < O65_SYMBOLTABLE_MAX);
    }

    FUNC_LEAVE_INT(entry);
}

static int
o65_symbol_delete(int Entry)
{
    FUNC_ENTER();

    DBG_ASSERT(o65_symboltable_count > 0);
    DBG_ASSERT(Entry < o65_symboltable_count);
    DBG_ASSERT(Entry >= 0);

    DBG_ASSERT(o65_symboltable[Entry].module != NULL);
    DBG_ASSERT(o65_symboltable[Entry].name   != NULL);

    DBG_O65_SHOW((DBG_PREFIX "Deleting symbol '%s'.",
        o65_symboltable[Entry].name));

    /* free the allocated memory */
    free(o65_symboltable[Entry].module);
    free(o65_symboltable[Entry].name);

    --o65_symboltable_count;

    /* now, copy the last item over the just removed item */
    o65_symboltable[Entry].module  = o65_symboltable[o65_symboltable_count].module;
    o65_symboltable[Entry].name    = o65_symboltable[o65_symboltable_count].name;
    o65_symboltable[Entry].address = o65_symboltable[o65_symboltable_count].address;

    /* clear the last entry */
    DBGDO(o65_symboltable[o65_symboltable_count].module  = NULL);
    DBGDO(o65_symboltable[o65_symboltable_count].name    = NULL);
    DBGDO(o65_symboltable[o65_symboltable_count].address = 0);

    FUNC_LEAVE_INT(0);
}

static int
o65_symbol_delete_module(const char * const ModuleName)
{
    int i;

    FUNC_ENTER();

    DBG_O65_SHOW((DBG_PREFIX "Deleting symbols for module '%s'.", ModuleName));

    for (i=0; i < o65_symboltable_count; i++)
    {
        if (strcmp(o65_symboltable[i].module, ModuleName) == 0)
        {
            DBG_O65_SHOW((DBG_PREFIX "Deleting symbol '%s'.",
                o65_symboltable[i].module));

            o65_symbol_delete(i);
            --i;
        }
    }

    FUNC_LEAVE_INT(0);
}


/*-----------------------------------------*/
/* functions for interpreting the o65 file */

typedef int o65version_type;

#define O65VERSION_CALC(_x, _y) ((_x)*10+(_y))
#define O65VERSION(_x, _y)      { \
    o65version_type tmpversion = O65VERSION_CALC((_x), (_y)); \
    O65file->o65version = O65file->o65version > tmpversion ? O65file->o65version : tmpversion; \
    }

#define O65VERSION_MAJOR(_x)    ((int)((_x) / 10))
#define O65VERSION_MINOR(_x)    ((int)((_x) % 10))

typedef
struct o65_file_references_s
{
    char *name;
} o65_file_references_t;

typedef
struct o65_file_globals_s
{
    char   *name;
    uint8   segmentid;
    uint32  value;
} o65_file_globals_t;

typedef
struct o65_file_relocation_entry_s
{
    uint32 relocAddress;
    uint8  reference;
    uint8  segment;
    uint8  type;
    uint8  additional;

} o65_file_relocation_entry_t;

typedef
struct o65_file_s
{
    char                       *raw_buffer;
    o65version_type             o65version;
    o65_file_header_common_t    header;
    o65_file_header_32_t        header_32;
    linkedlist_node_t           options_list;
    o65_file_references_t      *references;
    uint32                      references_count;
    o65_file_globals_t         *globals;
    uint32                      globals_count;
    unsigned char              *ptext;
    unsigned char              *pdata;
    linkedlist_node_t           text_relocation_list;
    linkedlist_node_t           data_relocation_list;

} o65_file_t;

static int
o65_read_byte(uint8 *InBuffer, unsigned Length, unsigned *Ptr,
              const char * const What, void *OutBuffer, unsigned Count)
{
    int error = O65ERR_NO_ERROR;

    FUNC_ENTER();

    DBG_ASSERT(InBuffer != NULL);
    DBG_ASSERT(Length > 0);
    DBG_ASSERT(OutBuffer != NULL);

    if (*Ptr + Count <= Length)
    {
        memcpy(OutBuffer, &InBuffer[*Ptr], Count);
        *Ptr += Count;
    }
    else
    {
        error = O65ERR_UNEXPECTED_END_OF_FILE;

        DBG_ERROR((DBG_PREFIX "%s has size %u, but I could "
            "not read more than %u byte", What, Count, Length - *Ptr));
    }

    FUNC_LEAVE_INT(error);
}

static char *
o65_read_string_zt(uint8 *InBuffer, unsigned Length, unsigned *Ptr)
{
    unsigned int i;
    char *result = NULL;
    char *p;
    int error = O65ERR_NO_ERROR;

    FUNC_ENTER();

    DBG_ASSERT(InBuffer != NULL);
    DBG_ASSERT(Length > 0);


    /* determine the length of the string */

    for (i = 0, p = &InBuffer[*Ptr]; (*p != 0) && (*Ptr < Length); p++, i++, (*Ptr)++)
    {
    }

    if (*p != 0)
    {
        if ((*Ptr) == Length)
        {
            DBG_ERROR((DBG_PREFIX
                "End of file while searching for end of string."));
            error = O65ERR_UNEXPECTED_END_OF_FILE;
        }
        else
        {
            DBG_ERROR((DBG_PREFIX "String is too long, aborting..."));
            error = O65ERR_STRING_TOO_LONG;
        }
    }

    if (!error)
    {
        result = malloc(i+1);

        if (result)
        {
            strcpy(result, p - i);
        }

        ++(*Ptr);
    }

    FUNC_LEAVE_PTR(result, char *);
}

static int
o65_file_load_header(uint8 *Buffer, unsigned Length, unsigned *Ptr, o65_file_t *O65file)
{
    int error = O65ERR_NO_ERROR;

    FUNC_ENTER();

    DBG_ASSERT(Buffer != NULL);
    DBG_ASSERT(Length != 0);
    DBG_ASSERT(O65file != NULL);

    error = o65_read_byte(Buffer, Length, Ptr, "o65 file header", 
        &O65file->header, sizeof(O65file->header));

    FUNC_LEAVE_INT(error);
}

static int
o65_file_load_header_dump(o65_file_t *O65file)
{
    int error = O65ERR_NO_ERROR;

    FUNC_ENTER();

    DBG_ASSERT(O65file != NULL);

    /* now, interpret the contents */

    DBG_O65_SHOW((DBG_PREFIX "O65 file:"));
    DBG_O65_SHOW((DBG_PREFIX "- marker = $%02X, $%02X.",
        O65file->header.marker[0], O65file->header.marker[1]));

    if ((O65file->header.marker[0] != 0x01) || (O65file->header.marker[1] != 0x00))
    {
        error = O65ERR_NO_O65_FILE;
    }

    DBG_O65_SHOW((DBG_PREFIX "- o65 mark = $%02X $%02X $%02X.",
        O65file->header.o65[0], O65file->header.o65[1], O65file->header.o65[2]));

    if (   (O65file->header.o65[0] != 'o') 
        || (O65file->header.o65[1] != '6')
        || (O65file->header.o65[2] != '5'))
    {
        error = error ? error : O65ERR_NO_O65_FILE;
    }

    DBG_O65_SHOW((DBG_PREFIX "- version = $%02X.",
        O65file->header.version));

    if (O65file->header.version != 0)
    {
        error = error ? error : O65ERR_UNKNOWN_VERSION;
    }

    DBG_O65_SHOW((DBG_PREFIX "- mode = $%02X.",
        O65file->header.mode));

    if (O65file->header.mode & O65_FILE_HEADER_MODE_65816)
    {
        DBG_O65_SHOW((DBG_PREFIX "  * 65816"));
        error = error ? error : O65ERR_65816_NOT_ALLOWED;
    }
    else
    {
        DBG_O65_SHOW((DBG_PREFIX "  * 6502"));
    }

    if (O65file->header.mode & O65_FILE_HEADER_MODE_PAGERELOC)
    {
        DBG_O65_SHOW((DBG_PREFIX "  * page-wise relocation only"));
    }
    else
    {
        DBG_O65_SHOW((DBG_PREFIX "  * byte-wise relocation allowed, "
            "depending on other flags"));
    }

    if (O65file->header.mode & O65_FILE_HEADER_MODE_SIZE32)
    {
        DBG_O65_SHOW((DBG_PREFIX "  * size is 32 bit"));
        error = error ? error : O65ERR_65816_NOT_ALLOWED;
    }
    else
    {
        DBG_O65_SHOW((DBG_PREFIX "  * size is 16 bit"));
    }

    if (O65file->header.mode & O65_FILE_HEADER_MODE_OBJFILE)
    {
        DBG_O65_SHOW((DBG_PREFIX "  * this is an object file"));
    }
    else
    {
        DBG_O65_SHOW((DBG_PREFIX "  * this is an executable"));
    }

    if (O65file->header.mode & O65_FILE_HEADER_MODE_SIMPLE)
    {
        DBG_O65_SHOW((DBG_PREFIX "  * this is a simple file, that is, "
            "the segmets are following each other. (v1.3)"));
        DBG_O65_SHOW((DBG_PREFIX "    dbase = tbase + tlen, and "
            "bbase = dbase + dlen"));
        O65VERSION(1,3);
    }
    else
    {
        DBG_O65_SHOW((DBG_PREFIX "  * this is not a simple file, "
            "that is, "
            "the segmets do not necessarily follow each other"
            "directly"));
    }

    if (O65file->header.mode & O65_FILE_HEADER_MODE_CHAIN)
    {
        DBG_O65_SHOW((DBG_PREFIX "  * this is a chained o65 file (v1.3)"));
        O65VERSION(1,3);
    }
    else
    {
        DBG_O65_SHOW((DBG_PREFIX "  * this is not a chained o65 file"));
    }

    if (O65file->header.mode & O65_FILE_HEADER_MODE_BSSZERO)
    {
        DBG_O65_SHOW((DBG_PREFIX "  * zero out BSS segment (v1.3)"));
        O65VERSION(1,3);
    }
    else
    {
        DBG_O65_SHOW((DBG_PREFIX "  * do not zero out BSS segment"));
    }

    if (O65file->header.mode & O65_FILE_HEADER_MODE_UNUSED)
    {
        DBG_O65_SHOW((DBG_PREFIX "  * !!! UNUSED Bits are set: $%04X",
            O65file->header.mode & O65_FILE_HEADER_MODE_UNUSED));
        error = error ? error : O65ERR_UNKNOWN_VERSION;
    }

    switch (O65file->header.mode & O65_FILE_HEADER_MODE_CPU_MASK)
    {
    case O65_FILE_HEADER_MODE_CPU_6502:
        DBG_O65_SHOW((DBG_PREFIX "  * CPU is a 6502 core, no illegal opcodes"));
        break;

    case O65_FILE_HEADER_MODE_CPU_65C02:
        DBG_O65_SHOW((DBG_PREFIX "  * CPU is a 65C02 core w/ some bugfix, no illegal opcodes (v1.3)"));
        O65VERSION(1,3);
        break;

    case O65_FILE_HEADER_MODE_CPU_65SC02:
        DBG_O65_SHOW((DBG_PREFIX "  * CPU is a 65SC02 core (enhanced 65C02), some new opcodes (v1.3)"));
        O65VERSION(1,3);
        break;

    case O65_FILE_HEADER_MODE_CPU_65CE02:
        DBG_O65_SHOW((DBG_PREFIX "  * CPU is a 65CE02 core some 16 bit ops/branches, Z register modifiable (v1.3)"));
        O65VERSION(1,3);
        break;

    case O65_FILE_HEADER_MODE_CPU_NMOS6502:
        DBG_O65_SHOW((DBG_PREFIX "  * CPU is an NMOS 6502 core (including undocumented opcodes) (v1.3)"));
        O65VERSION(1,3);
        break;

    case O65_FILE_HEADER_MODE_CPU_65816:
        DBG_O65_SHOW((DBG_PREFIX "  * CPU is a 65816 in 6502 emulation mode (v1.3)"));
        O65VERSION(1,3);
        break;

    default:
        DBG_O65_SHOW((DBG_PREFIX "  * UNKNOWN CPU specification!"));
        error = O65ERR_UNKNOWN_CPU_SPECIFICATION;
        break;
    }


    switch (O65file->header.mode & O65_FILE_HEADER_MODE_ALIGN_MASK)
    {
    case O65_FILE_HEADER_MODE_ALIGN_BYTE:
        DBG_O65_SHOW((DBG_PREFIX "  * byte-wise relocation"));
        break;

    case O65_FILE_HEADER_MODE_ALIGN_WORD:
        DBG_O65_SHOW((DBG_PREFIX "  * word-wise relocation"));
        break;

    case O65_FILE_HEADER_MODE_ALIGN_QUAD:
        DBG_O65_SHOW((DBG_PREFIX "  * quad-wise relocation"));
        break;

    case O65_FILE_HEADER_MODE_ALIGN_PAGE:
        DBG_O65_SHOW((DBG_PREFIX "  * page-wise relocation"));
        break;
    }

    FUNC_LEAVE_INT(error);
}

static int
o65_file_load_header32(uint8 *Buffer, unsigned Length, unsigned *Ptr, o65_file_t *O65file)
{
    int error = O65ERR_NO_ERROR;

    FUNC_ENTER();

    DBG_ASSERT(Buffer != NULL);
    DBG_ASSERT(Length > 0);
    DBG_ASSERT(O65file != NULL);

    if (O65file->header.mode & O65_FILE_HEADER_MODE_SIZE32)
    {
        error = o65_read_byte(Buffer, Length, Ptr, "o65 file header (32)",
            &O65file->header_32, sizeof(O65file->header_32));
    }
    else
    {
        o65_file_header_16_t o65_file_header_16;

        error = o65_read_byte(Buffer, Length, Ptr, "o65 file header (16)",
            &o65_file_header_16, sizeof(o65_file_header_16));

        if (!error)
        {
            /* copy the contents over to the 32 bit structure */

            O65file->header_32.tbase = o65_file_header_16.tbase;
            O65file->header_32.tlen  = o65_file_header_16.tlen;
            O65file->header_32.dbase = o65_file_header_16.dbase;
            O65file->header_32.dlen  = o65_file_header_16.dlen;
            O65file->header_32.bbase = o65_file_header_16.bbase;
            O65file->header_32.blen  = o65_file_header_16.blen;
            O65file->header_32.zbase = o65_file_header_16.zbase;
            O65file->header_32.zlen  = o65_file_header_16.zlen;
            O65file->header_32.stack = o65_file_header_16.stack;
        }
    }

    FUNC_LEAVE_INT(error);
}

static int
o65_file_load_header_32_dump(o65_file_t *O65file)
{
    FUNC_ENTER();

    DBG_ASSERT(O65file != NULL);

    DBG_O65_SHOW((DBG_PREFIX "- tbase = $%04X, tlen = $%04X.",
        O65file->header_32.tbase, O65file->header_32.tlen));
    DBG_O65_SHOW((DBG_PREFIX "- dbase = $%04X, dlen = $%04X.",
        O65file->header_32.dbase, O65file->header_32.dlen));
    DBG_O65_SHOW((DBG_PREFIX "- bbase = $%04X, blen = $%04X.",
        O65file->header_32.bbase, O65file->header_32.blen));
    DBG_O65_SHOW((DBG_PREFIX "- zbase = $%04X, zlen = $%04X.",
        O65file->header_32.zbase, O65file->header_32.zlen));
    DBG_O65_SHOW((DBG_PREFIX "- stack = $%04X.",
        O65file->header_32.stack));

    FUNC_LEAVE_INT(O65ERR_NO_ERROR);
}

static int
o65_file_load_oheader_dump(o65_file_t *O65file, o65_file_header_oheader_t *po65_file_header_oheader)
{
    const char *stringType = NULL;
    int error = O65ERR_NO_ERROR;

    FUNC_ENTER();

    DBG_ASSERT(po65_file_header_oheader != NULL);

    switch (po65_file_header_oheader->optiontype)
    {
    case O65_FILE_HEADER_OHEADER_TYPE_OS_SYSTEM:
        if (po65_file_header_oheader->optionlength < sizeof(*po65_file_header_oheader))
        {
            DBG_ERROR((DBG_PREFIX 
                "- optional OS SYSTEM HEADER too short!"));
            error = O65ERR_OPTIONAL_SYSTEM_TOO_SHORT;
        }
        else
        {
            switch (po65_file_header_oheader->data[0])
            {
            case O65_FILE_HEADER_OHEADER_TYPE_OS_OSA65:
                DBG_O65_SHOW((DBG_PREFIX "- OSA/65 header supplement"));
                break;

            case O65_FILE_HEADER_OHEADER_TYPE_OS_LUNIX:
                DBG_O65_SHOW((DBG_PREFIX "- LUNIX module"));
                break;

            case O65_FILE_HEADER_OHEADER_TYPE_OS_CC65:
                DBG_O65_SHOW((DBG_PREFIX "- CC65 generic module (v1.3)"));
                O65VERSION(1,3);
                break;

            case O65_FILE_HEADER_OHEADER_TYPE_OS_OPENCBM:
                DBG_O65_SHOW((DBG_PREFIX "- OpenCBM floppy module (v1.3)"));
                O65VERSION(1,3);
                break;

            default:
                DBG_ERROR((DBG_PREFIX "- unknown OS %u", po65_file_header_oheader->data[0]));
                error = O65ERR_UNKNOWN_SYSTEM;
                break;
            }
            DBG_O65_SHOW((DBG_PREFIX "  * rest of %u bytes not recognized.",
                po65_file_header_oheader->optionlength - sizeof(*po65_file_header_oheader)));

            DBG_O65_MEMDUMP("    optional header (dump)",
                (unsigned char *) po65_file_header_oheader, po65_file_header_oheader->optionlength);
        }
        break;

    case O65_FILE_HEADER_OHEADER_TYPE_FILENAME:
        if (!stringType) stringType = "filename";
        /* FALL THROUGH */

    case O65_FILE_HEADER_OHEADER_TYPE_ASSEMBLER:
        if (!stringType) stringType = "assembler";
        /* FALL THROUGH */

    case O65_FILE_HEADER_OHEADER_TYPE_AUTHOR:
        if (!stringType) stringType = "author";
        /* FALL THROUGH */

    case O65_FILE_HEADER_OHEADER_TYPE_CREATION:
        if (!stringType) stringType = "creation date";

        if (po65_file_header_oheader->optionlength < sizeof(*po65_file_header_oheader))
        {
            DBG_ERROR((DBG_PREFIX 
                "- optional %s header too short!", stringType));
            error = O65ERR_OPTIONAL_HEADER_TOO_SHORT;
        }

        if (!error)
        {
            /* as we made sure we have a trailing zero
               (see above), we can output this without
               problems. */
            DBG_O65_SHOW((DBG_PREFIX
                "- %s: %s", 
                stringType, po65_file_header_oheader->data));
        }

        if (!error)
        {
            if (po65_file_header_oheader->data[po65_file_header_oheader->optionlength 
                - sizeof(*po65_file_header_oheader)]
                != 0)
            {
                DBG_ERROR((DBG_PREFIX
                    "- optional %s header not correctly terminated "
                    "with a zero.", stringType));
                error = O65ERR_OPTIONAL_HEADER_NOT_TERMINATED;
            }
        }
        break;

    default:
        DBG_WARN((DBG_PREFIX "Unknown header option %u "
            "encountered.", po65_file_header_oheader->optiontype));
        error = O65ERR_UNKNOWN_HEADER_OPTION;
        break;
    }

    FUNC_LEAVE_INT(error);
}

static int
o65_file_load_oheader(uint8 *Buffer, unsigned Length, unsigned *Ptr, o65_file_t *O65file)
{
    uint8 length;
    int error = O65ERR_NO_ERROR;

    FUNC_ENTER();

    do {
        o65_file_header_oheader_t *po65_file_header_oheader = NULL;

        error = o65_read_byte(Buffer, Length, Ptr,
            "O65 option (length)", &length, sizeof(length));

        if (!error && length != 0)
        {
            po65_file_header_oheader = malloc(length+1);
    
            if (!po65_file_header_oheader)
            {
                DBG_ERROR((DBG_PREFIX "Not enough memory for malloc() "
                    "of optional header, length %u.", length));
                error = O65ERR_OUT_OF_MEMORY;
            }

            if (!error)
            {
                /* initialize the whole structure to zero. This
                   way, there is a trailing zero at the end. */

                memset(po65_file_header_oheader, 0, length + 1);

                po65_file_header_oheader->optionlength = length;

                error = o65_read_byte(Buffer, Length, Ptr, "O65 option (remaining)",
                    ((unsigned char*)po65_file_header_oheader)
                      + sizeof(po65_file_header_oheader->optionlength),
                    length 
                      - sizeof(po65_file_header_oheader->optionlength));
            }

            if (!error)
            {
                error = o65_file_load_oheader_dump(O65file, po65_file_header_oheader);
            }
        }

        if (po65_file_header_oheader)
        {
            linkedlist_insertafter(&O65file->options_list, po65_file_header_oheader);
        }

    } while (!error && length != 0);

    FUNC_LEAVE_INT(0);
}

static int
o65_file_load_readtext(uint8 *InBuffer, unsigned Length, unsigned *Ptr,
                       o65_file_t *O65file, const char * const What,
                       unsigned char **OutBuffer, unsigned int Count)
{
    int error = O65ERR_NO_ERROR;

    FUNC_ENTER();

    DBG_ASSERT(InBuffer != NULL);
    DBG_ASSERT(O65file != NULL);
    DBG_ASSERT(OutBuffer != NULL);
    DBG_ASSERT(*OutBuffer == NULL);

    if (Count != 0)
    {
        *OutBuffer = malloc(Count);

        if (!*OutBuffer)
        {
            error = O65ERR_OUT_OF_MEMORY;
            DBG_ERROR((DBG_PREFIX "Error while malloc()ing %u byte for %s.",
                Count, What));
        }
        else
        {
            error = o65_read_byte(InBuffer, Length, Ptr, What, *OutBuffer, Count);
            DBG_O65_MEMDUMP(What, *OutBuffer, Count);
        }
    }

    FUNC_LEAVE_INT(error);
}

static int
o65_file_read_size(uint8 *Buffer, unsigned Length, unsigned *Ptr,
                   const char * const What, o65_file_t *O65file, uint32 *Result)
{
    int error = O65ERR_NO_ERROR;

    FUNC_ENTER();

    if (O65file->header.mode & O65_FILE_HEADER_MODE_SIZE32)
    {
        error = o65_read_byte(Buffer, Length, Ptr, What,
            Result, sizeof(*Result));
    }
    else
    {
        uint16 result16;

        error = o65_read_byte(Buffer, Length, Ptr, What,
            &result16, sizeof(result16));

        *Result = result16;
    }

    FUNC_LEAVE_INT(error);
}

static int
o65_file_load_references(uint8 *Buffer, unsigned Length, unsigned *Ptr,
                         o65_file_t *O65file)
{
    int error = O65ERR_NO_ERROR;
    unsigned int i;

    FUNC_ENTER();

    error = o65_file_read_size(Buffer, Length, Ptr,
        "References count", O65file, &O65file->references_count);

    /* allocate memory to hold all references values */

    if (!error)
    {
        O65file->references = malloc(sizeof(o65_file_references_t) * O65file->references_count);

        if (!O65file->references)
        {
            DBG_ERROR((DBG_PREFIX "Could not allocate memory to "
                "hold all %u references.",
                O65file->references_count));
            error = O65ERR_OUT_OF_MEMORY;
        }
        else
        {
            memset(O65file->references, 0,
                sizeof(o65_file_references_t) * O65file->references_count);
        }
    }

    DBG_O65_SHOW((DBG_PREFIX "  * read undefined references:"));
    if (!error)
    {
        for (i = 0; i < O65file->references_count; i++)
        {
            O65file->references[i].name = o65_read_string_zt(Buffer, Length, Ptr);
            if (!O65file->references[i].name)
            {
                error = O65ERR_STRING_ERROR_OR_MEMORY;
                break;
            }
            else
            {
                DBG_O65_SHOW((DBG_PREFIX "    - '%s'", O65file->references[i].name));
            }
        }
    }

    FUNC_LEAVE_INT(error);
}

static int
o65_file_load_globals(uint8 *Buffer, unsigned Length, unsigned *Ptr,
                      o65_file_t *O65file)
{
    int error = O65ERR_NO_ERROR;
    int i;

    FUNC_ENTER();

    error = o65_file_read_size(Buffer, Length, Ptr,
        "Globals count", O65file, &O65file->globals_count);

    /* allocate memory to hold all globals values */

    if (!error)
    {
        O65file->globals = malloc(sizeof(o65_file_globals_t) * O65file->globals_count);

        if (!O65file->globals)
        {
            DBG_ERROR((DBG_PREFIX "Could not allocate memory to "
                "hold all %u globals.",
                O65file->globals_count));
            error = O65ERR_OUT_OF_MEMORY;
        }
        else
        {
            memset(O65file->globals, 0,
                sizeof(o65_file_globals_t) * O65file->globals_count);
        }
    }

    DBG_O65_SHOW((DBG_PREFIX "  * read exported globals:"));
    if (!error)
    {
        for (i = O65file->globals_count - 1; i >= 0; i--)
        {
            O65file->globals[i].name = o65_read_string_zt(Buffer, Length, Ptr);
            if (!O65file->globals[i].name)
            {
                error = O65ERR_STRING_ERROR_OR_MEMORY;
                break;
            }
            else
            {
                error = o65_read_byte(Buffer, Length, Ptr, "Exported global, segmentid",
                    &O65file->globals[i].segmentid, 1);

                if (!error)
                {
                    error = o65_file_read_size(Buffer, Length, Ptr, "Exported global, value",
                        O65file, &O65file->globals[i].value);
                }

                DBG_O65_SHOW((DBG_PREFIX "    - '%s', segment = $%02X, value = $%04X",
                    O65file->globals[i].name,
                    O65file->globals[i].segmentid,
                    O65file->globals[i].value));
            }
        }
    }

    FUNC_LEAVE_INT(error);
}

static int
o65_file_load_reloc(uint8 *Buffer, unsigned Length, unsigned *Ptr,
                    o65_file_t *O65file, const char * const What, linkedlist_node_t *List)
{
    uint8 buffer[10];

    int error = O65ERR_NO_ERROR;
    int quit = 0;
    uint32 relocAddress;
    uint8 *p;

    FUNC_ENTER();

    DBG_ASSERT(O65file != NULL);

    relocAddress = -1;

    for (p = buffer; !quit && !error; p = buffer)
    {
        o65_file_relocation_entry_t *po65_relocation_entry = NULL;

        if ((error = o65_read_byte(Buffer, Length, Ptr, "byte from reloc table, 1", p, 1)) != 0)
            break;

        if (*p == 0)
        {
            quit = 1;
            break;
        }

        po65_relocation_entry = malloc(sizeof(*po65_relocation_entry));
        if (!po65_relocation_entry)
        {
            DBG_ERROR((DBG_PREFIX "Could not allocate memory for relocation entry."));
            error = O65ERR_OUT_OF_MEMORY;
            break;
        }
        memset(po65_relocation_entry, 0, sizeof(*po65_relocation_entry));

        while (*p == 0xFF)
        {
            relocAddress += 0xFE;
            p++;
            if ((error = o65_read_byte(Buffer, Length, Ptr, "byte from reloc table, 2", p, 1)) != 0)
            {
                break;
            }
        }
        relocAddress += *p++;
        if ((error = o65_read_byte(Buffer, Length, Ptr, "byte from reloc table, 3", p, 1)) != 0)
        {
            break;
        }

        DBG_O65_SHOW((DBG_PREFIX "    - Relocation address $%04X", relocAddress));

        po65_relocation_entry->relocAddress = relocAddress;

        if (*p & O65_FILE_RELOC_SEGMTYPEBYTE_UNDEF_MASK)
        {
            DBG_ERROR((DBG_PREFIX
                "Relocation SEGMENT/ID byte contains invalid bits: $%02X",
                *p & O65_FILE_RELOC_SEGMTYPEBYTE_UNDEF_MASK));
        }

        po65_relocation_entry->segment = *p & O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_MASK;

        switch (po65_relocation_entry->segment)
        {
        case O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_UNDEF:
            DBG_O65_SHOW((DBG_PREFIX "    - Segment undef"));
            break;

        case O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_ABS:
            DBG_O65_SHOW((DBG_PREFIX "    - Segment abs"));
            break;

        case O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_TEXT:
            DBG_O65_SHOW((DBG_PREFIX "    - Segment text"));
            break;

        case O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_DATA:
            DBG_O65_SHOW((DBG_PREFIX "    - Segment data"));
            break;

        case O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_BSS:
            DBG_O65_SHOW((DBG_PREFIX "    - Segment bss"));
            break;

        case O65_FILE_RELOC_SEGMTYPEBYTE_SEGM_ZERO:
            DBG_O65_SHOW((DBG_PREFIX "    - Segment zero"));
            break;

        default:
            DBG_ERROR((DBG_PREFIX "Unknown segment in SEGMENT/TYPE byte: $%02X",
                po65_relocation_entry->segment));
            break;
        }


        po65_relocation_entry->type = *p & O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_MASK;

        switch (po65_relocation_entry->type)
        {
        case O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_WORD:
            if ((error = o65_read_byte(Buffer, Length, Ptr, "2 bytes from reloc table, 1", p+1, 2)) == 0)
            {
                po65_relocation_entry->reference = p[1];
                po65_relocation_entry->additional = p[2];

                DBG_O65_SHOW((DBG_PREFIX
                    "    - Type WORD, %s(%u), additional data: $%04X",
                    O65file->references[p[1]], p[1], p[2]));
                p += 2;
            }
            break;

        case O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_HIGH:
            if ((error = o65_read_byte(Buffer, Length, Ptr, "2 bytes from reloc table, 2", p+1, 2)) == 0)
            {
                po65_relocation_entry->reference = p[1];
                po65_relocation_entry->additional = p[2];

                DBG_O65_SHOW((DBG_PREFIX
                    "    - Type HIGH, %s, additional data: $%04X",
                    O65file->references[p[1]], p[2]));
                p += 2;
            }
            break;

        case O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_LOW:
            if ((error = o65_read_byte(Buffer, Length, Ptr, "2 bytes from reloc table, 3", p+1, 2)) == 0)
            {
                po65_relocation_entry->reference = p[1];
                po65_relocation_entry->additional = p[2];

                DBG_O65_SHOW((DBG_PREFIX
                    "    - Type LOW, %s, additional data: $%04X",
                    O65file->references[p[1]], p[2]));
                p += 2;
            }
            break;

        case O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_SEGADR:
            DBG_ASSERT(po65_relocation_entry->type != O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_SEGADR);
            error = O65ERR_65816_NOT_ALLOWED;
            DBG_ERROR((DBG_PREFIX
                "    - Type SEGADR"));
            break;

        case O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_SEG:
            DBG_ASSERT(po65_relocation_entry->type != O65_FILE_RELOC_SEGMTYPEBYTE_TYPE_SEG);
            error = O65ERR_65816_NOT_ALLOWED;
            DBG_ERROR((DBG_PREFIX
                "    - Type SEG"));
            break;

        default:
            DBG_ERROR((DBG_PREFIX "Unknown type in SEGMENT/TYPE byte: $%02X",
                po65_relocation_entry->type));
            break;
        }

        if (po65_relocation_entry)
        {
            if (po65_relocation_entry->reference >= O65file->references_count)
            {
                DBG_ERROR((DBG_PREFIX "references illegal reference %u",
                    po65_relocation_entry->reference));
                error = O65ERR_UNDEFINED_REFERENCE;
            }

            if (!error)
            {
                linkedlist_insertafter(List, po65_relocation_entry);
            }
            else
            {
                free(po65_relocation_entry);
            }
        }
    }

    FUNC_LEAVE_INT(error);
}

static o65_file_t *
o65_file_alloc(char *Buffer)
{
    o65_file_t *o65file = NULL;

    FUNC_ENTER();

    o65file = malloc(sizeof(*o65file));

    if (!o65file)
    {
        DBG_ERROR((DBG_PREFIX "Not enough memory for malloc() "
            "of o65file structure, length %u.", sizeof(*o65file)));
    }
    else
    {
        memset(o65file, 0, sizeof(*o65file));

        linkedlist_list_init(&o65file->options_list);
        linkedlist_list_init(&o65file->text_relocation_list);
        linkedlist_list_init(&o65file->data_relocation_list);

        o65file->raw_buffer = Buffer;

        o65file->o65version = O65VERSION_CALC(1, 2); /* assume: version 1.2 of o65 file */
    }

    FUNC_LEAVE_PTR(o65file, o65_file_t *);
}

void
o65_file_delete(o65_file_t *O65file)
{
    FUNC_ENTER();

    DBG_ASSERT(O65file != NULL);

    if (O65file)
    {
        free(O65file->ptext);
        free(O65file->pdata);

        while (!linkedlist_is_last(O65file->options_list.next))
            free(linkedlist_removeafter(&O65file->options_list));

        while (!linkedlist_is_last(O65file->text_relocation_list.next))
            free(linkedlist_removeafter(&O65file->text_relocation_list));

        while (!linkedlist_is_last(O65file->data_relocation_list.next))
            free(linkedlist_removeafter(&O65file->data_relocation_list));

        free(O65file->raw_buffer);
        free(O65file);
    }

    FUNC_LEAVE();
}

int
o65_file_process(char *Buffer, unsigned Length, o65_file_t **PO65file)
{
    o65_file_t *o65file = NULL;
    unsigned ptr = 0;
    int error = O65ERR_UNSPECIFIED;

    FUNC_ENTER();

    DBG_ASSERT(PO65file != NULL);
    DBG_ASSERT(Buffer != NULL);
    DBG_ASSERT(Length > 0);

    do {
        if (!Buffer || Length == 0) {
            error = O65ERR_NO_DATA;
            break;
        }

        o65file = o65_file_alloc(Buffer);
        if (!o65file) {
            error = O65ERR_OUT_OF_MEMORY;
            break;
        }

        if ( O65ERR_NO_ERROR != (error = o65_file_load_header(Buffer, Length, &ptr, o65file) ) ) {
            break;
        }

        if ( O65ERR_NO_ERROR != (error = o65_file_load_header_dump(o65file) ) ) {
            break;
        }

        if ( O65ERR_NO_ERROR != (error = o65_file_load_header32(Buffer, Length, &ptr, o65file) ) ) {
            break;
        }

        if ( O65ERR_NO_ERROR != (error = o65_file_load_header_32_dump(o65file) ) ) {
            break;
        }

        if ( O65ERR_NO_ERROR != (error = o65_file_load_oheader(Buffer, Length, &ptr, o65file) ) ) {
            break;
        }

        /* read the text segment */

        if ( O65ERR_NO_ERROR != (error = o65_file_load_readtext(Buffer, Length, &ptr,
                                o65file, "text segment",
                                &o65file->ptext, o65file->header_32.tlen) ) ) {
            break;
        }

        /* read the data segment */

        if ( O65ERR_NO_ERROR != (error = o65_file_load_readtext(Buffer, Length, &ptr,
                                o65file, "data segment",
                                &o65file->pdata, o65file->header_32.dlen) ) ) {
            break;
        }

        if ( O65ERR_NO_ERROR != (error = o65_file_load_references(Buffer, Length, &ptr, o65file) ) ) {
            break;
        }

        if ( O65ERR_NO_ERROR != (error = o65_file_load_reloc(Buffer, Length, &ptr,
                                o65file, "text relocation",
                                &o65file->text_relocation_list) ) ) {
            break;
        }

        if ( O65ERR_NO_ERROR != (error = o65_file_load_reloc(Buffer, Length, &ptr,
                                o65file, "data relocation",
                                &o65file->data_relocation_list) ) ) {
            break;
        }

        if ( O65ERR_NO_ERROR != (error = o65_file_load_globals(Buffer, Length, &ptr,
                                o65file) ) ) {
            break;
        }

        DBG_O65_SHOW((DBG_PREFIX "This O65 file is a version %u.%u file.",
           O65VERSION_MAJOR(o65file->o65version),
           O65VERSION_MINOR(o65file->o65version)));


        *PO65file = o65file;
        error = O65ERR_NO_ERROR; /* redundant, but we do it anyway */

    } while (0);

    if ( error ) {
        o65_file_delete(o65file);
    }

    FUNC_LEAVE_INT(error);
}

int
o65_file_load(const char * const Filename, o65_file_t **PO65file)
{
    FILE *f = NULL;
    char *buffer = NULL;
    int error = O65ERR_UNSPECIFIED;
    int fileSize;
    int fileSizeSeek;

    FUNC_ENTER();

    DBG_ASSERT(Filename != NULL);
    DBG_ASSERT(PO65file != NULL);

    DBG_O65_SHOW((DBG_PREFIX "Reading O65 file '%s'", Filename));

    do {
        f = fopen(Filename, "rb");
        if (!f) {
            DBG_ERROR((DBG_PREFIX "could not open file '%s'", Filename));
            error = O65ERR_NO_FILE;
            break;
        }

        if (fseek(f, 0, SEEK_END) != 0) {
            error = O65ERR_FILE_HANDLING_ERROR;
            break;
        }

        fileSizeSeek = ftell(f);
        if (fileSizeSeek == 0) {
            error = O65ERR_FILE_EMPTY;
            break;
        }

        if (fseek(f, 0, SEEK_SET) != 0) {
            error = O65ERR_FILE_HANDLING_ERROR;
            break;
        }

        buffer = malloc(fileSizeSeek);
        if (!buffer) {
            error = O65ERR_OUT_OF_MEMORY;
            break;
        }

        fileSize = fread(buffer, 1, fileSizeSeek + 1, f);
        if (fileSize == 0) {
            error = O65ERR_FILE_EMPTY;
            break;
        }

        if (fileSize == fileSizeSeek && !feof(f)) {
            error = O65ERR_FILE_HANDLING_ERROR;
            break;
        }

        error = o65_file_process(buffer, fileSize, PO65file);
        if (error) {
            break;
        }

    } while (0);

    if (f != NULL) {
        fclose(f);
    }

    if (error) {
        free(buffer);
    }

    FUNC_LEAVE_INT(error);
}

int
o65_file_reloc(o65_file_t *O65file, unsigned int Address)
{
    int error = O65ERR_NO_ERROR;

    FUNC_ENTER();

    FUNC_LEAVE_INT(error);
}
