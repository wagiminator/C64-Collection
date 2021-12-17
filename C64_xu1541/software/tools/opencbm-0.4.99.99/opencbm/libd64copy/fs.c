/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2004 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
*/

#include "d64copy_int.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch.h"

static d64copy_settings *fs_settings;

static FILE *the_file;
static char *error_map;
static int block_count;

/* always use maximum size for error map */
#define ERROR_MAP_LENGTH D71_BLOCKS

static int block_offset(int tr, int se)
{
    int sectors = 0, i;
    for(i = 1; i < tr; i++)
    {
        sectors += d64copy_sector_count(fs_settings->two_sided, i);
    }
    return (sectors + se) * BLOCKSIZE;
}

static int read_block(unsigned char tr, unsigned char se, unsigned char *block)
{
    if(fseek(the_file, block_offset(tr, se), SEEK_SET) == 0)
    {
        return fread(block, BLOCKSIZE, 1, the_file) != 1;
    }
    return 1;
}

/*
 * Variables to make sure writing the block is an atomary process
 */
static int atom_execute = 0;
static unsigned char atom_tr;
static unsigned char atom_se;
static const unsigned char *atom_blk;
static int atom_size;
static int atom_read_status;

static int write_block(unsigned char tr, unsigned char se, const unsigned char *blk, int size, int read_status)
{
    long ofs;
    int ret;

    atom_tr = tr;
    atom_se = se;
    atom_blk = blk;
    atom_size = size;
    atom_read_status = read_status;

    atom_execute = 1;

    ofs = block_offset(tr, se);
    if(fseek(the_file, ofs, SEEK_SET) == 0)
    {
        error_map[ofs / BLOCKSIZE] = (char) ((read_status == 0) ? 1 : read_status);
        ret = fwrite(blk, size, 1, the_file) != 1;
    }
    else
    {
        ret = 1;
    }

    atom_execute = 0;

    return ret;
}

static int open_disk(CBM_FILE fd, d64copy_settings *settings,
                     const void *arg, int for_writing,
                     turbo_start start, d64copy_message_cb message_cb)
{
    off_t filesize;
    int stat_ok, is_image, error_info;
    int tr = 0;
    char *name = (char*)arg;

    the_file = NULL;
    fs_settings = settings;
    block_count = 0;

    stat_ok = arch_filesize(name, &filesize) == 0;
    is_image = error_info = 0;

    if(stat_ok)
    {
        if(filesize == D71_BLOCKS * BLOCKSIZE)
        {
            is_image = 1;
            block_count = D71_BLOCKS;
            tr = D71_TRACKS;
        }
        else if(filesize == D71_BLOCKS * (BLOCKSIZE + 1))
        {
            is_image = 1;
            error_info = 1;
            block_count = D71_BLOCKS;
            tr = D71_TRACKS;
        }
        else
        {
            block_count = STD_BLOCKS;
            for( tr = STD_TRACKS; !is_image && tr <= TOT_TRACKS; )
            {
                is_image = filesize == block_count * BLOCKSIZE;
                if(!is_image)
                {
                    error_info = is_image =
                        filesize == block_count * (BLOCKSIZE + 1);
                }
                if(!is_image)
                {
                    block_count += d64copy_sector_count( 0, tr++ );
                }
            }
            if( is_image && tr != STD_TRACKS )
            {
                message_cb(1, "non-standard number or tracks: %d", tr);
            }
        }
    }

    if(!for_writing)
    {
        if(stat_ok)
        {
            if(is_image)
            {
                the_file = fopen(name, "rb");
                if(the_file == NULL)
                {
                    message_cb(0, "could not open %s", name);
                }
                if(error_info)
                {
                    message_cb(1, "image contains error information");
                }
                if(settings->end_track == -1)
                {
                    settings->end_track = tr;
                }
                else if(settings->end_track > tr)
                {
                    message_cb(1, "resetting end track to %d", tr);
                    settings->end_track = tr;
                }
                /* FIXME: error map */
            }
            else
            {
                message_cb(0, "neither a .d64 not .d71 file: %s", name);
            }
        }
        else
        {
            message_cb(0, "could not stat %s", name);
        }
    }
    else
    {
        the_file = fopen(name, is_image ? "r+b" : "wb");
        if(the_file)
        {
            /* check whether we must resize or create an image file */
            int new_tr;
            if(settings->two_sided)
            {
                new_tr = D71_TRACKS;
            }
            else if(settings->end_track <= STD_TRACKS)
            {
                new_tr = STD_TRACKS;
            }
            else if(settings->end_track <= EXT_TRACKS)
            {
                new_tr = EXT_TRACKS;
            }
            else
            {
                new_tr = TOT_TRACKS;
            }

            /* always use maximum size for error map */
            error_map = calloc(ERROR_MAP_LENGTH, 1);
            if(!error_map)
            {
                message_cb(0, "no memory for error map");
                fclose(the_file);
                if(!is_image)
                {
                    arch_unlink(name);
                }
                return 1;
            }

            if(is_image)
            {
                if(error_info)
                {
                    if(fseek(the_file, block_count * BLOCKSIZE, SEEK_SET) != 0 ||
                       fread(error_map, block_count, 1, the_file) != 1)
                    {
                        message_cb(0, "%s: could not read error map", name);
                        fclose(the_file);
                        return 1;
                    }
                }
                if(fseek(the_file, block_count * BLOCKSIZE, SEEK_SET) != 0)
                {
                    message_cb(0, "%s: could not seek to end of file", name);
                    fclose(the_file);
                    return 1;
                }
            }

            if(new_tr > tr)
            {
                /* grow image */
                while(tr < new_tr)
                {
                    block_count += d64copy_sector_count(settings->two_sided, ++tr);
                }

                message_cb(1, "growing image file to %d blocks", block_count);

                if (arch_ftruncate(arch_fileno(the_file), block_count * BLOCKSIZE) != 0)
                {
                    message_cb(0, "%s: could not extend image file", name);
                    fclose(the_file);
                    if(!is_image)
                        arch_unlink(name);
                    return 1;
                }
            }
        }
        else
        {
            message_cb(0, "could not open %s", name);
        }
    }
    return the_file == NULL;
}

static void close_disk(void)
{
    int i, has_errors = 0;

    /* if writing the block was interrupted, make sure it is
     * redone before closing the disk 
     */

    if (the_file && atom_execute)
    {
        atom_execute = 0;
        write_block(atom_tr, atom_se, atom_blk, atom_size, atom_read_status);
    }

    if (fs_settings)
    {
        switch(fs_settings->error_mode)
        {
            case em_always:
                has_errors = 1;
                break;
            case em_never:
                has_errors = 0;
                break;
            default:
                if(error_map)
                {
                    for(i = 0; !has_errors && i < block_count; i++)
                    {
                        has_errors = error_map[i] != 1;
                    }
                }
                break;
        }
    }

    if(the_file)
    {
        if(has_errors)
        {
            if(fseek(the_file, block_count * BLOCKSIZE, SEEK_SET) == 0)
            {
                fwrite(error_map, block_count, 1, the_file);
            }
        } 
        else
        {
            arch_ftruncate(arch_fileno(the_file), block_count * BLOCKSIZE);
        }
    }

    if(error_map)
    {
        free(error_map);
        error_map = NULL;
    }
    if(the_file)
    {
        fclose(the_file);
        the_file = NULL;
    }
}

DECLARE_TRANSFER_FUNCS(fs_transfer, 0, 0);
