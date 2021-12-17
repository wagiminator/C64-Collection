/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2004 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Modifications for openCBM Copyright 2011-2011 Thomas Winkler
*/

#include "imgcopy_int.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arch.h"

static imgcopy_settings *fs_settings;

static FILE *the_file;
static char *error_map;
static int block_count;



/* always use maximum size for error map */
//#define ERROR_MAP_LENGTH D82_BLOCKS


static int block_offset(int tr, int se)
{
    int sectors = 0, i;
    for(i = 1; i < tr; i++)
    {
        sectors += imgcopy_sector_count(fs_settings, i);
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

static int open_disk(CBM_FILE fd, imgcopy_settings *settings,
                     const void *arg, int for_writing,
                     turbo_start start, imgcopy_message_cb message_cb)
{
    off_t filesize;
    int stat_ok, is_image, error_info;
    int tr = 0;
    char *name = (char*)arg;

    //printf("open imagefile ...\n");

    the_file = NULL;
    fs_settings = settings;
    //block_count = 0;

    stat_ok = arch_filesize(name, &filesize) == 0;
    is_image = error_info = 0;

    block_count = settings->block_count;

    if(stat_ok)
    {
        tr = settings->max_tracks;

        if(filesize == block_count  * BLOCKSIZE)
        {
            is_image = 1;
        }
        else if(filesize == block_count * (BLOCKSIZE + 1))
        {
            is_image = 1;
            error_info = 1;
        }
        else
        {
            printf("filesize=%d, blockcount=%d, calc1=%d, calc2=%d\n", filesize, block_count, block_count * (BLOCKSIZE), block_count * (BLOCKSIZE + 1));
            /*   D64 sonderformate

                 for( tr = D82_TRACKS; !is_image && tr <= D82_TRACKS; )
                 {
                 is_image = filesize == block_count * BLOCKSIZE;
                 if(!is_image)
                 {
                 error_info = is_image =
                 filesize == block_count * (BLOCKSIZE + 1);
                 }
                 if(!is_image)
                 {
                 block_count += imgcopy_sector_count( 0, tr++ );
                 }
                 }
                 if( is_image && tr != D80_TRACKS )
                 {
                 message_cb(1, "non-standard number or tracks: %d", tr);
                 }
                 */
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
                message_cb(0, "image file is not compatible: %s", name);
            }
        }
        else
        {
            message_cb(0, "could not access imagefile: %s", name);
        }
    }
    else
    {
        the_file = fopen(name, is_image ? "r+b" : "wb");
        if(the_file)
        {
            /* check whether we must resize or create an image file */
            int new_tr;

            new_tr = settings->max_tracks;

            /* always use maximum size for error map */
            error_map = calloc(block_count, 1);
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
    message_cb(2, "open imagefile ok. %s", name);
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
