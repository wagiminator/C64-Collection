/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999-2004 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
 *  Modifications for cbm4win and general rework Copyright 2001-2007 Spiro Trikaliotis
 *  Additions Copyright 2006,2011 Wolfgang Moser (http://d81.de)
 *  Additions Copyright 2011      Thomas Winkler
 */

#include "opencbm.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>

#include "arch.h"
#include "libmisc.h"

typedef
enum {
    PA_UNSPEC = 0,
    PA_PETSCII,
    PA_RAW
} PETSCII_RAW;

//! struct to remember the general options to the program
typedef struct {
    int argc;    //!< a (modifieable) copy of the number of arguments, as given to main()
    char **argv; //!< a (modifieable) copy of the argument list, as given to main()
    int ownargv; //!< remember: This is the original argv (=0), or this is a malloc()ed copy of it (=1)
    int error;   //!< there was an error in processing the options (=1), or not (=0)
    int help;    //!< option: the user requested help for the specified command
    int version; //!< option: print version information
    char *adapter; //!< option: an explicit adapter was specified
    PETSCII_RAW petsciiraw; //!< option: The user requested PETSCII or RAW, or nothing 
} OPTIONS;

typedef int (*mainfunc)(CBM_FILE fd, OPTIONS * const options);


static const unsigned char prog_tdchange[] = {
#include "tdchange.inc"
};


/*
 * Output version information
 */
static int do_version(CBM_FILE fd, OPTIONS * const options)
{
    printf("cbmctrl version " OPENCBM_VERSION ", built on " __DATE__ " at " __TIME__ "\n");

    return 0;
}

static int check_if_parameters_ok(OPTIONS * const options)
{
    if (options->argc != 0)
    {
        fprintf(stderr, "Extra parameter, aborting...\n");
        return 1;
    }
    return 0;
}

static int get_argument_char(OPTIONS * const options, unsigned char *where)
{
    if (options->argc > 0)
    {
        *where = arch_atoc(options->argv[0]);
        options->argv++;
        options->argc--;
        return 0;
    }
    else
    {
        fprintf(stderr, "Not enough parameters, aborting!\n");
        return 1;
    }
}

static int
get_argument_string(OPTIONS * const options, char *where[], unsigned int *len)
{
    if (options->argc > 0)
    {
        if (where)
           *where = options->argv[0];

        if (len)
            *len = strlen(*where);

        options->argv++;
        options->argc--;
        return 0;
    }
    else
    {
        fprintf(stderr, "Not enough parameters, aborting!\n");
        return 1;
    }
}

static int get_argument_file_for_write(OPTIONS * const options, FILE **f)
{
    char *filename = NULL;

    assert(f);

    if (options->argc > 0)
    {
        get_argument_string(options, &filename, NULL);

        if (filename && strcmp(filename, "-") == 0)
            filename = NULL;
    }

    if (check_if_parameters_ok(options))
        return 1;

    if(filename != NULL)
    {
        /* a filename (other than simply "-") was given, open that file */

        *f = fopen(filename, "wb");
    }
    else
    {
        /* no filename was given, open stdout in binary mode */

        *f = arch_fdopen(arch_fileno(stdout), "wb");

        /* set binary mode for output stream */

        if (*f != NULL)
            arch_setbinmode(arch_fileno(stdout));
    }

    if(*f == NULL)
    {
        arch_error(0, arch_get_errno(), "could not open output file: %s",
              (filename != 0) ? filename : "stdout");

        return 1;
    }

    return 0;
}

static int get_argument_file_for_read(OPTIONS * const options, FILE **f, char **fn)
{
    char *filename = NULL;

    assert(f);

    if (options->argc > 0)
    {
        get_argument_string(options, &filename, NULL);

        if (filename && strcmp(filename, "-") == 0)
            filename = NULL;
    }

    if (check_if_parameters_ok(options))
        return 1;

    if(filename == NULL)
    {
        filename = "(stdin)";
        *f = stdin;

        // set binary mode for input stream

        arch_setbinmode(arch_fileno(stdin));
    }
    else
    {
        off_t filesize;

        *f = fopen(filename, "rb");
        if(*f == NULL)
        {
            arch_error(0, arch_get_errno(), "could not open %s", filename);
            return 1;
        }
        if(arch_filesize(filename, &filesize))
        {
            arch_error(0, arch_get_errno(), "could not stat %s", filename);
            return 1;
        }
    }

    if (fn)
        *fn = filename;

    return 0;
}

static int
process_individual_option(OPTIONS * const options, const char short_options[], struct option long_options[])
{
    static int firstcall = 1;
    int option;

    if (firstcall)
    {
        firstcall = 0;

        optind = 0;

        --options->argv;
        ++options->argc;
    }


    option = getopt_long(options->argc, options->argv, short_options, long_options, NULL);

    if (option == EOF)
    {
        // skip the options that are already processed
        //optind --;

        options->argc -= optind;
        options->argv += optind;
    }

    return option;
}


static int
skip_options(OPTIONS * const options)
{
    if (process_individual_option(options, "+", NULL) != EOF)
        return 1;
    else
        return 0;
}

static int hex2val(const char ch)
{
    if (ch>='0' && ch<='9')
        return ch - '0';

    if (ch>='A' && ch<='F')
        return ch - 'A' + 10;

    if (ch>='a' && ch<='f')
        return ch - 'a' + 10;

    fprintf(stderr, 
        (ch==0) ? "Not enough digits for hex value given\n" 
                : "Unknown hex character '%c'.\n", ch);
    return -1;
}

static int
process_specific_byte_parameter(char *string, int stringlen, PETSCII_RAW petsciiraw)
{
    int size = 0;
    char *pread = string;
    char *pwrite = pread;

    while (*pread != 0)
    {
        char ch = *pread++;

        if (ch == '%')
        {
            ch = *pread++;

            if (ch != '%')
            {
                int val = hex2val(ch);
                int val2;

                if (val < 0)
                    return -1;

                val2 = hex2val(*pread++);

                if (val2 < 0)
                    return -1;

                ch = (val << 4) | val2;
            }

            *pwrite++ = ch;
            size++;
        }
        else
        {
            if (petsciiraw == PA_PETSCII)
                ch = cbm_ascii2petscii_c(ch);

            *pwrite++ = ch;
            size++;
        }
    }

    *pwrite = 0;

    return size;
}

static char *
string_concat(const char *string1, int len1, const char *string2, int len2)
{
    char *buffer;

    if ((string1 != NULL) && (len1 == 0))
        len1 = strlen(string1);

    if ((string2 != NULL) && (len2 == 0))
        len2 = strlen(string2);

    buffer = calloc(1, len1 + len2 + 1);

    if (buffer)
    {
        if (string1)
            memcpy(buffer, string1, len1);

        if (string2)
            memcpy(buffer + len1, string2, len2);
    }

    return buffer;
}

static int
get_extended_argument_string(int extended, 
                             OPTIONS * const options,
                             char *string[], unsigned int *stringlen)
{
    int rv;
    char *commandline = NULL;
    char *extended_commandline = NULL;
    unsigned int commandline_len = 0;
    unsigned int extended_commandline_len = 0;

    rv = get_argument_string(options, &commandline, &commandline_len);

    if (rv)
        return 1;

    if (extended)
    {
        int n = process_specific_byte_parameter(commandline, commandline_len, options->petsciiraw);

        if (n < 0)
            return 1;

        commandline_len = n;
    }
    else
    {
        // only convert ASCII -> PETSCII if we do not have extended syntax
        // (with extended syntax, this is done "on the fly" while converting
        // the % - style values into characters.)

        if (options->petsciiraw == PA_PETSCII)
            cbm_ascii2petscii(commandline);
    }

    // now, check if there are more command-line parameters

    if (options->argc > 0)
    {
        char *p;

        // get memory for the additional data

        // get 2 byte more than we have parameters, as we will append
        // \r and \0 at the end of the buffer!

        extended_commandline = malloc(options->argc + 1);

        if (extended_commandline == NULL)
        {
            fprintf(stderr, "Not enough memory for all parameters, aborting...\n");
            return 1;
        }

        // write data in the memory

        p = extended_commandline;

        while (--options->argc >= 0)
        {
            char *tail;
            long c;

            c = strtol(options->argv[0], &tail, 0);

            if(c < 0 || c > 0xff || *tail)
            {
                arch_error(0, 0, "invalid byte: %s", options->argv[0]);
                return 1;
            }
            *p++ = (char) c;
            extended_commandline_len++;

            options->argv++;
        }

        ++options->argc;

        // end the string with a \0. This is not really needed, but
        // convenient when debugging.

        *p   = 0;

    }

    *string = string_concat(commandline, commandline_len, extended_commandline, extended_commandline_len);
    *stringlen = commandline_len + extended_commandline_len;

    if (extended_commandline)
        free(extended_commandline);

    if (check_if_parameters_ok(options))
        return 1;

    return 0;
}

/*
 * Simple wrapper for lock
 */
static int do_lock(CBM_FILE fd, OPTIONS * const options)
{
    int rv = skip_options(options);
    
    rv = rv || check_if_parameters_ok(options);

    if (rv == 0)
        cbm_lock(fd);

    return rv;
}

/*
 * Simple wrapper for unlock
 */
static int do_unlock(CBM_FILE fd, OPTIONS * const options)
{
    int rv = skip_options(options);
    
    rv = rv || check_if_parameters_ok(options);

    if (rv == 0)
        cbm_unlock(fd);

    return rv;
}

/*
 * Simple wrapper for reset
 */
static int do_reset(CBM_FILE fd, OPTIONS * const options)
{
    int rv = skip_options(options);
    
    rv = rv || check_if_parameters_ok(options);

    if (rv == 0)
        rv = cbm_reset(fd);

    return rv;
}

/*
 * Simple wrapper for clk
 */
static int do_iec_clk(CBM_FILE fd, OPTIONS * const options)
{
    int rv = skip_options(options);

    rv = rv || check_if_parameters_ok(options);

    if (rv == 0)
        cbm_iec_set(fd, IEC_CLOCK);

    return rv;
}

/*
 * Simple wrapper for uclk
 */
static int do_iec_uclk(CBM_FILE fd, OPTIONS * const options)
{
    int rv = skip_options(options);

    rv = rv || check_if_parameters_ok(options);

    if (rv == 0)
        cbm_iec_release(fd, IEC_CLOCK);

    return rv;
}

/*
 * Simple wrapper for listen
 */
static int do_listen(CBM_FILE fd, OPTIONS * const options)
{
    int rv;
    unsigned char unit;
    unsigned char secondary;

    rv = skip_options(options);
    
    rv = rv || get_argument_char(options, &unit);
    rv = rv || get_argument_char(options, &secondary);

    if (rv || check_if_parameters_ok(options))
        return 1;

    return cbm_listen(fd, unit, secondary);
}

/*
 * Simple wrapper for talk
 */
static int do_talk(CBM_FILE fd, OPTIONS * const options)
{
    int rv;
    unsigned char unit;
    unsigned char secondary;

    rv = skip_options(options);
    
    rv = rv || get_argument_char(options, &unit);
    rv = rv || get_argument_char(options, &secondary);

    if (rv || check_if_parameters_ok(options))
        return 1;

    return cbm_talk(fd, unit, secondary);
}

/*
 * Simple wrapper for unlisten
 */
static int do_unlisten(CBM_FILE fd, OPTIONS * const options)
{
    int rv = skip_options(options);
    
    rv = rv || check_if_parameters_ok(options);

    if (rv == 0)
        rv = cbm_unlisten(fd);

    return rv;
}

/*
 * Simple wrapper for untalk
 */
static int do_untalk(CBM_FILE fd, OPTIONS * const options)
{
    int rv = skip_options(options);
    
    rv = rv || check_if_parameters_ok(options);

    if (rv == 0)
        rv = cbm_untalk(fd);

    return rv;
}

/*
 * Simple wrapper for open
 */
static int do_open(CBM_FILE fd, OPTIONS * const options)
{
    int rv;
    unsigned char unit;
    unsigned char secondary;
    char *filename;
    unsigned int filenamelen = 0;

    int extended = 0;
    int c;
    static const char short_options[] = "+e";
    static struct option long_options[] =
    {
        {"extended", no_argument, NULL, 'e'},
        {NULL,       no_argument, NULL, 0  }
    };

    // first of all, process the options given

    while ((c = process_individual_option(options, short_options, long_options)) != EOF)
    {
        switch (c)
        {
        case 'e':
            extended = 1;
            break;

        default:
            return 1;
        }
    }

    rv = get_argument_char(options, &unit);
    rv = rv || get_argument_char(options, &secondary);
    rv = rv || get_extended_argument_string(extended, options, &filename, &filenamelen);

    if (rv)
        return 1;

    rv = cbm_open(fd, unit, secondary, filename, filenamelen);

    free(filename);

    return rv;
}

/*
 * Simple wrapper for close
 */
static int do_close(CBM_FILE fd, OPTIONS * const options)
{
    int rv;
    unsigned char unit;
    unsigned char secondary;

    rv = skip_options(options);
    
    rv = rv || get_argument_char(options, &unit);
    rv = rv || get_argument_char(options, &secondary);

    if (rv || check_if_parameters_ok(options))
        return 1;

    return cbm_close(fd, unit, secondary);
}

/*
 * read raw data from the IEC bus
 */
static int do_read(CBM_FILE fd, OPTIONS * const options)
{
    int size, rv = 0;
    unsigned char buf[2048];
    FILE *f;

    if (skip_options(options))
        return 1;
    
    if (get_argument_file_for_write(options, &f))
        return 1;

    /* fill a buffer with up to 64k of bytes from the IEC bus */
    while(0 < (size = cbm_raw_read(fd, buf, sizeof(buf))))
    {
        // if PETSCII was recognized, convert the data before writing

        if (options->petsciiraw == PA_PETSCII)
        {
            int i;
            for (i=0; i < size; i++)
                buf[i] = cbm_petscii2ascii_c(buf[i]);
        }

        /* write that to the file */
        if(size != (int) fwrite(buf, 1, size, f))
        {
            rv=1;   /* error condition from cbm_raw_read */
            break;  /* do fclose(f) before exiting       */
        }
        /* if nobody complained, repeat filling the buffer */
    }

    if(size < 0) rv=1; /* error condition from cbm_raw_read */

    fclose(f);
    return rv;
}

/*
 * write raw data to the IEC bus
 */
static int do_write(CBM_FILE fd, OPTIONS * const options)
{
    char *fn = NULL;
    int size;
    unsigned char buf[2048];
    FILE *f;

    if (skip_options(options))
        return 1;
    
    if (get_argument_file_for_read(options, &f, &fn))
        return 1;

    /* fill a buffer with up to 64k of bytes from file/console */
    size = fread(buf, 1, sizeof(buf), f);
    /* do this test only on the very first run */
    if(size == 0 && feof(f))
    {
        arch_error(0, 0, "no data: %s", fn);
        if(f != stdin) fclose(f);
        return 1;
    }
    
    /* as long as no error occurred */
    while( ! ferror(f))
    {
        /* if requested, convert to PETSCII before writing */
        if (options->petsciiraw == PA_PETSCII)
        {
            int i;
            for (i=0; i < size; i++)
                buf[i] = cbm_ascii2petscii_c(buf[i]);
        }

        /* write that to the the IEC bus */
        if(size != cbm_raw_write(fd, buf, size))
        {
            /* exit the loop with another error condition */
            break;
        }

        /* fill a buffer with up to 64k of bytes from file/console */
        size = fread(buf, 1, sizeof(buf), f);
        if(size == 0 && feof(f))
        {
                /* nothing more to read */
            if(f != stdin) fclose(f);
            return 0;
        }
    }

    /* the loop has exited, because of an error, check, which one */
    if(ferror(f))
    {
        arch_error(0, 0, "could not read %s", fn);
    }
    /* else : size number of bytes could not be written to IEC bus */
    
    if(f != stdin) fclose(f);
    return 1;
}

/*
 * put specified data to the IEC bus
 */
static int do_put(CBM_FILE fd, OPTIONS * const options)
{
    int  rv;
    char *commandline;
    unsigned int commandlinelen = 0;


    int extended = 0;
    int c;
    static const char short_options[] = "+e";
    static struct option long_options[] =
    {
        {"extended", no_argument, NULL, 'e'},
        {NULL,       no_argument, NULL, 0  }
    };

    // first of all, process the options given

    while ((c = process_individual_option(options, short_options, long_options)) != EOF)
    {
        switch (c)
        {
        case 'e':
            extended = 1;
            break;

        default:
            return 1;
        }
    }

    rv = get_extended_argument_string(extended, options, &commandline, &commandlinelen);

    if (rv)
        return 1;

    /* if requested, convert to PETSCII before writing */
    if (options->petsciiraw == PA_PETSCII)
    {
        unsigned int i;
        for (i=0; i < commandlinelen; i++)
            commandline[i] = cbm_ascii2petscii_c(commandline[i]);
    }

    /* write that to the IEC bus */
    rv = cbm_raw_write(fd, commandline, commandlinelen);
    if(rv < 0 || commandlinelen != (unsigned int) rv)
    {
        rv = 1;
    }
    else
    {
        rv = 0;
    }

    if (commandline != NULL)
    {
        free(commandline);
    }

    return rv;
}

/*
 * display device status w/ PetSCII conversion
 */
static int do_status(CBM_FILE fd, OPTIONS * const options)
{
    char buf[40];
    unsigned char unit;
    int rv;

    rv = skip_options(options);
    
    rv = rv || get_argument_char(options, &unit);

    if (rv || check_if_parameters_ok(options))
        return 1;

    rv = cbm_device_status(fd, unit, buf, sizeof(buf));

    if (options->petsciiraw == PA_PETSCII)
        cbm_petscii2ascii(buf);

    printf("%s", buf);

    return (rv == 99) ? 1 : 0;
}


/*
 * send device command
 */
static int do_command(CBM_FILE fd, OPTIONS * const options)
{
    int  rv;
    unsigned char unit;
    char *commandline;
    unsigned int commandlinelen = 0;

    int extended = 0;
    int c;
    static const char short_options[] = "+e";
    static struct option long_options[] =
    {
        {"extended", no_argument, NULL, 'e'},
        {NULL,       no_argument, NULL, 0  }
    };

    // first of all, process the options given

    while ((c = process_individual_option(options, short_options, long_options)) != EOF)
    {
        switch (c)
        {
        case 'e':
            extended = 1;
            break;

        default:
            return 1;
        }
    }

    rv = get_argument_char(options, &unit);
    rv = rv || get_extended_argument_string(extended, options, &commandline, &commandlinelen);

    if (rv)
        return 1;

    rv = cbm_listen(fd, unit, 15);
    if(rv == 0)
    {
        if (commandlinelen > 0)
            cbm_raw_write(fd, commandline, commandlinelen);

        // make sure the buffer is ended with a '\r'; this is needed
        // only if the command ends with a '\r', to work around a bug
        // in the floppy code.

        cbm_raw_write(fd, "\r", 1);

        rv = cbm_unlisten(fd);
    }

    if (commandline != NULL)
    {
        free(commandline);
    }

    return rv;
}

/*
 * display directory
 */
static int do_dir(CBM_FILE fd, OPTIONS * const options)
{
    char c, buf[40];
    unsigned char command[] = { '$', '0' };
    int rv;
    unsigned char unit;

    rv = skip_options(options);
    
    rv = rv || get_argument_char(options, &unit);
    /* default is drive '0' */
    if (options->argc > 0)
    {
        rv = rv || get_argument_char(options, command+1);
    }

    if (rv || check_if_parameters_ok(options))
        return 1;

    rv = cbm_open(fd, unit, 0, command, sizeof(command));
    if(rv == 0)
    {
        if(cbm_device_status(fd, unit, buf, sizeof(buf)) == 0)
        {
            cbm_talk(fd, unit, 0);
            if(cbm_raw_read(fd, buf, 2) == 2)
            {
                while(cbm_raw_read(fd, buf, 2) == 2)
                {
                    if(cbm_raw_read(fd, buf, 2) == 2)
                    {
                        printf("%u ", (unsigned char)buf[0] | (unsigned char)buf[1] << 8 );
                        while((cbm_raw_read(fd, &c, 1) == 1) && c)
                        {
                            if (options->petsciiraw == PA_PETSCII)
                                putchar(cbm_petscii2ascii_c(c));
                            else
                                putchar(c);
                        }
                        putchar('\n');
                    }
                }
                cbm_untalk(fd);
                cbm_device_status(fd, unit, buf, sizeof(buf));
                printf("%s", cbm_petscii2ascii(buf));
            }
            else
            {
                cbm_untalk(fd);
            }

        }
        else
        {
            printf("%s", cbm_petscii2ascii(buf));
        }
        cbm_close(fd, unit, 0);
    }
    return rv;
}

static void show_monkey(unsigned int c)
{
    // const static char monkey[]={"¸,ø¤*º°´`°º*¤ø,¸"};     // for fast moves
    // const static char monkey[]={"\\|/-"};    // from cbmcopy
    // const static char monkey[]={"-\\|/"};    // from libtrans (reversed)
    // const static char monkey[]={"\\-/|"};    // from cbmcopy  (reversed)
    // const static char monkey[]={"-/|\\"};       // from libtrans
    // const static char monkey[]={",;:!^*Oo"};// for fast moves
    const static char monkey[]={",oO*^!:;"};// for fast moves

    c %= sizeof(monkey) - 1;
    fprintf(stderr, (c != 0) ? "\b%c" : "\b.%c" , monkey[c]);
    fflush(stderr);
}

/*
 * read device memory, dump to stdout or a file
 */
static int do_download(CBM_FILE fd, OPTIONS * const options)
{
    unsigned char unit;
    unsigned short c;
    int addr, count, rv = 0;
    char *tail, buf[256];
    FILE *f;

    char *tmpstring;

    if (skip_options(options))
        return 1;
    
    // process the drive number (unit)

    if (get_argument_char(options, &unit))
        return 1;


    // process the address

    if (get_argument_string(options, &tmpstring, NULL))
        return 1;

    addr = strtol(tmpstring, &tail, 0);
    if(addr < 0 || addr > 0xffff || *tail)
    {
        arch_error(0, 0, "invalid address: %s", tmpstring);
        return 1;
    }


    // process the count of bytes

    if (get_argument_string(options, &tmpstring, NULL))
        return 1;

    count = strtol(tmpstring, &tail, 0);
    if((count + addr) > 0x10000 || *tail)
    {
        arch_error(0, arch_get_errno(), "invalid byte count %s", tmpstring);
        return 1;
    }


    // process the filename, if any

    if (get_argument_file_for_write(options, &f))
        return 1;


    // download in chunks of sizeof(buf) (currently: 256) bytes
    while(count > 0)
    {
        show_monkey(count / sizeof(buf));

        c = (count > sizeof(buf)) ? sizeof(buf) : count;

        if (c + (addr & 0xFF) > 0x100) {
            c = 0x100 - (addr & 0xFF);
        }

        if(c != cbm_download(fd, unit, addr, buf, c))
        {
            rv = 1;
            fprintf(stderr, "A transfer error occurred!\n");
            break;
        }

        // If the user wants to convert them from PETSCII, do this
        // (I find it hard to believe someone would want to do this,
        // but who knows?)

        if (options->petsciiraw == PA_PETSCII)
        {
            int i;
            for (i = 0; i < c; i++)
                buf[i] = cbm_petscii2ascii_c(buf[i]);
        }

        fwrite(buf, 1, c, f);

        addr  += c;
        count -= c;
    }

    fclose(f);
    return rv;
}

/*
 * load binary data from file into device memory
 */
static int do_upload(CBM_FILE fd, OPTIONS * const options)
{
    unsigned char unit;
    int addr;
    int rv;
    size_t size;
    char *tail, *fn;
    char *tmpstring;
    unsigned char addr_buf[2];
    unsigned int buflen = 65537;
    unsigned char *buf;
    FILE *f;

    if (skip_options(options))
        return 1;
    
    // process the drive number (unit)

    if (get_argument_char(options, &unit))
        return 1;


    // process the address

    if (get_argument_string(options, &tmpstring, NULL))
        return 1;

    addr = strtol(tmpstring, &tail, 0);
    if(addr < -1 || addr > 0xffff || *tail)
    {
        arch_error(0, 0, "invalid address: %s", tmpstring);
        return 1;
    }

    if (get_argument_file_for_read(options, &f, &fn))
        return 1;


    // allocate memory for the transfer

    buf = malloc(buflen);
    if (!buf)
    {
        fprintf(stderr, "Not enough memory for buffer.\n");
        return 1;
    }

    if(addr == -1)
    {
        /* read address from file */
        if(fread(addr_buf, 2, 1, f) != 1)
        {
            arch_error(0, arch_get_errno(), "could not read %s", fn);
            if(f != stdin) fclose(f);
            free(buf);
            return 1;
        }

        /* don't assume a particular endianess, although the cbm4linux
         * package is only i386 for now  */
        addr = addr_buf[0] | (addr_buf[1] << 8);
    }

    size = fread(buf, 1, buflen, f);
    if(ferror(f))
    {
        arch_error(0, 0, "could not read %s", fn);
        if(f != stdin) fclose(f);
        free(buf);
        return 1;
    }
    else if(size == 0 && feof(f))
    {
        arch_error(0, 0, "no data: %s", fn);
        if(f != stdin) fclose(f);
        free(buf);
        return 1;
    }

    if(addr + size > 0x10000)
    {
        arch_error(0, 0, "program too big: %s", fn);
        if (f != stdin) fclose(f);
        free(buf);
        return 1;
    }

    if(f != stdin) fclose(f);

    // If the user wants to convert them from PETSCII, do this
    // (I find it hard to believe someone would want to do this,
    // but who knows?)

    if (options->petsciiraw == PA_PETSCII)
    {
        unsigned int i;
        for (i = 0; i < size; i++)
            buf[i] = cbm_ascii2petscii_c(buf[i]);
    }

    rv = (cbm_upload(fd, unit, addr, buf, size) == (int)size) ? 0 : 1;

    if ( rv != 0 ) {
        fprintf(stderr, "A transfer error occurred!\n");
    }

    free(buf);

    return rv;
}

/*
 * identify connected devices
 */
static int do_detect(CBM_FILE fd, OPTIONS * const options)
{
    unsigned int num_devices;
    unsigned char device;
    const char *type_str;

    if (skip_options(options))
        return 1;
    
    if (check_if_parameters_ok(options))
        return 1;

    num_devices = 0;

    for( device = 8; device < 16; device++ )
    {
        enum cbm_device_type_e device_type;
        if( cbm_identify( fd, device, &device_type, &type_str ) == 0 )
        {
            enum cbm_cable_type_e cable_type;
            const char *cable_str = "(cannot determine cable type)";
 
            num_devices++;

            if ( cbm_identify_xp1541( fd, device, &device_type, &cable_type ) == 0 )
            {
                switch (cable_type)
                {
                case cbm_ct_none:
                    cable_str = "";
                    break;

                case cbm_ct_xp1541:
                    cable_str = "(XP1541)";
                    break;

                case cbm_ct_unknown:
                default:
                    break;
                }
            }
            printf( "%2d: %s %s\n", device, type_str, cable_str );
        }
    }
    arch_set_errno(0);
    return num_devices > 0 ? 0 : 1;
}

/*
 * wait until user changes the disk
 */
static int do_change(CBM_FILE fd, OPTIONS * const options)
{
    unsigned char unit;
    int rv;

    rv = skip_options(options);
    
    rv = rv || get_argument_char(options, &unit);

    if (rv || check_if_parameters_ok(options))
        return 1;

    do
    {
        /*
         * Determine if we have a supported drive type.
         * Note: As we do not recognize all drives reliably,
         *       we only block a 1581. If we cannot determine
         *       the drive type, just allow using it!
         * \todo: Fix this!
         */

        enum cbm_device_type_e device_type;

        if (cbm_identify(fd, unit, &device_type, NULL) == 0)
        {
            if (device_type == cbm_dt_cbm1581)
            {
                fprintf(stderr, "Drive %u is a 1581, which is not supported (yet).\n", unit);
                rv = 1;
                break;
            }
        }

        /*
         * Make sure the drive is on track 18
         */
        if (cbm_exec_command(fd, unit, "I0:", 0) != 0)
        {
            /*
             * The drive did not react; most probably, there is none,
             * thus quit.
             */
            rv = 1;
            break;
        }

        if (cbm_upload(fd, unit, 0x500, prog_tdchange, sizeof(prog_tdchange)) != sizeof(prog_tdchange))
        {
            /*
             * The drive code wasn't uploaded fully, thus quit.
             */
            rv = 1;
            break;
        }

        cbm_exec_command(fd, unit, "U3:", 0);
        cbm_iec_release(fd, IEC_ATN | IEC_DATA | IEC_CLOCK | IEC_RESET);

        /*
         * Now, wait for the drive routine to signal its starting
         */
        cbm_iec_wait(fd, IEC_DATA, 1);

        /*
         * Now, wait until CLOCK is high, too, which tells us that
         * a new disk has been successfully read.
         */
        cbm_iec_wait(fd, IEC_CLOCK, 1);

        /*
         * Signal: We recognized this
         */
        cbm_iec_set(fd, IEC_ATN);

        /*
         * Wait for routine ending.
         */
        cbm_iec_wait(fd, IEC_CLOCK, 0);

        /*
         * Release ATN again
         */
        cbm_iec_release(fd, IEC_ATN);

    } while (0);

    return rv;
}

struct prog
{
    int      need_driver;
    char    *name;
    PETSCII_RAW petsciiraw;
    mainfunc prog;
    char    *arglist;
    char    *shorthelp_text;
    char    *help_text;
};

static struct prog prog_table[] =
{
    {1, "lock"    , PA_UNSPEC,  do_lock    , "",
        "Lock the parallel port for the use by cbm4win/cbm4linux.",
        "This command locks the parallel port for the use by cbm4win/cbm4linux,\n"
        "so that sequences of e.g. talk/read/untalk commands are not broken by\n"
        "concurrent processes wanting to access the parallel port." },

    {1, "unlock"  , PA_UNSPEC,  do_unlock  , "",
        "Unlock the parallel port for the use by cbm4win/cbm4linux.",
        "This command unlocks the parallel port again so that other processes\n"
        "get a chance to access the parallel port." },

    {1, "listen"  , PA_UNSPEC,  do_listen  , "<device> <secadr>",
        "perform a listen on the IEC bus",
        "Output a listen command on the IEC bus.\n"
        "<device> is the device number,\n"
        "<secadr> the secondary address to use for this.\n\n"
        "This has to be undone later with an unlisten command." },

    {1, "talk"    , PA_UNSPEC,  do_talk    , "<device> <secadr>",
        "perform a talk on the IEC bus",
        "Output a talk command on the IEC bus.\n"
        "<device> is the device number,\n"
        "<secadr> the secondary address to use for this.\n\n"
        "This has to be undone later with an untalk command." },

    {1, "unlisten", PA_UNSPEC,  do_unlisten, "",
        "perform an unlisten on the IEC bus",
        "Undo one or more previous listen commands.\n"
        "This affects all drives." },

    {1, "untalk"  , PA_UNSPEC,  do_untalk  , "",
        "perform an untalk on the IEC bus",
        "Undo one or more previous talk commands.\n"
        "This affects all drives." },

    {1, "open"    , PA_RAW,     do_open    , "[-e|--extended] <device> <secadr> <filename> [<file1>...<fileN>]",
        "perform an open on the IEC bus",
        "Output an open command on the IEC bus.\n"
        "<device> is the device number,\n"
        "<secadr> the secondary address to use for this.\n"
        "<filename> is the name of the file to be opened.\n"
        "           It must be given, but may be empty by specifying it as \"\".\n"
        "<file1..N> are additional bytes to append to the filename  <filename>.\n"
        "           Single bytes can be given as decimal, octal (0 prefix) or\n"
        "           sedecimal (0x prefix).\n\n"
        "If the option -e or --extended is given, an extended format is used:\n"
        "  You can specify extra characters by given their ASCII value in hex,\n"
        "  prepended with '%', that is: '%20' => SPACE, '%41' => 'A', '%35' => '5',\n"
        "  and-so-on. A '%' is given by giving its hex ASCII: '%25' => '%'.\n\n"
        "Opening a file has to be undone later with a close command.\n\n"
        "NOTES:\n"
        "- You cannot do an open without a filename.\n"
        "  Although a CBM machine (i.e., a C64) allows this, it is an internal operation\n"
        "  to the computer only.\n"
        "- If used with the global --petscii option, this action is equivalent\n"
        "  to the deprecated command-line 'cbmctrl popen'.\n"
        "- If using both --petscii and --extended option, the bytes given via the\n"
        "  '%' meta-character or as <file1> .. <fileN> are *not* converted to petscii." },

    {1, "popen"   , PA_PETSCII, do_open    , "<device> <secadr> <filename>",
        "deprecated; use 'cbmctrl --petscii open' instead!",
        "This command is deprecated; use 'cbmctrl -p open' instead!" },

    {1, "close"   , PA_UNSPEC,  do_close   , "<device> <secadr>",
        "perform a close on the IEC bus",
        "Undo a previous open command." },

    {1, "read"    , PA_RAW,     do_read    , "[<file>]",
        "read raw data from the IEC bus",
        "With this command, you can read raw data from the IEC bus.\n"
        "<file>   (optional) file name of a file to write the contents to.\n"
        "         If this name is not given or it is a dash ('-'), the\n"
        "         contents will be written to stdout, normally the console." },

    {1, "write"   , PA_RAW,     do_write   , "[<file>]",
        "write raw data to the IEC bus",
        "With this command, you can write raw data to the IEC bus.\n"
        "<file>   (optional) file name of a file to read the values from.\n"
        "         If this name is not given or it is a dash ('-'), the\n"
        "         contents will be read from stdin, normally the console." },

    {1, "put"     , PA_RAW,     do_put     , "[-e|--extended] <datastr> [<data1> ... <dataN>]",
        "put specified data to the IEC bus",
        "With this command, you can write raw data to the IEC bus.\n"
        "<datastr> is the string to output to the IEC bus.\n"
        "          It must be given, but may be empty by specifying it as \"\".\n\n"
        "<data1..N> are additional bytes to append to the string <datastr>.\n"
        "           Single bytes can be given as decimal, octal (0 prefix) or\n"
        "           sedecimal (0x prefix).\n\n"
        "If the option -e or --extended is given, an extended format is used:\n"
        "  You can specify extra characters by given their ASCII value in hex,\n"
        "  prepended with '%', that is: '%20' => SPACE, '%41' => 'A', '%35' => '5',\n"
        "  and-so-on. A '%' is given by giving its hex ASCII: '%25' => '%'.\n\n"
        "NOTES:\n"
        "- If using both --pestscii and --extended option, the bytes given via the\n"
        "  '%' meta-character or as <data1> .. <dataN> are *not* converted to petscii." },

    {1, "status"  , PA_PETSCII, do_status  , "<device>",
        "give the status of the specified drive",
        "This command gets the status (the so-called 'error channel')"
        "of the given drive and outputs it on the screen.\n"
        "<device> is the device number of the drive." },

    {1, "command" , PA_RAW,     do_command , "[-e|--extended] <device> <cmdstr> [<cmd1> ... <cmdN>]",
        "issue a command to the specified drive",
        "This command issues a command to a specific drive.\n"
        "This command is a command that you normally give to\n"
        "channel 15 (i.e., N: to format a drive, V: to validate, etc.).\n\n"
        "<device> is the device number of the drive.\n\n"
        "<cmdstr> is the command to execute in the drive.\n"
        "         It must be given, but may be empty by specifying it as \"\".\n\n"
        "<cmd1..N> are additional bytes to append to the command string <cmdstr>.\n"
        "          Single bytes can be given as decimal, octal (0 prefix) or\n"
        "          sedecimal (0x prefix).\n\n"
        "If the option -e or --extended is given, an extended format is used:\n"
        "  You can specify extra characters by given their ASCII value in hex,\n"
        "  prepended with '%', that is: '%20' => SPACE, '%41' => 'A', '%35' => '5',\n"
        "  and-so-on. A '%' is given by giving its hex ASCII: '%25' => '%'.\n\n"
        "Example: Write the bytes $29, $49 into memory locations $0077 and $0078\n"
        "         of drive 8 (which gives the drive the address 9):\n"
        "         cbmctrl -p command -e 8 m-w 119 0 2 41 73\n"
        "      or cbmctrl -p command -e 8 m-w%77%00 2 0x29 0x49\n\n"
        "NOTES:\n"
        "- If --raw is used (default), you have to give the commands in uppercase\n"
        "  letters, lowercase will NOT work!\n"
        "  If --petscii is used, you must give the commands in lowercase.\n"
        "- If used with the global --petscii option, this action is equivalent\n"
        "  to the deprecated command-line 'cbmctrl pcommand'.\n"
        "- If using both --petscii and --extended option, the bytes given via the\n"
        "  '%' meta-character or as <cmd1> .. <cmdN> are *not* converted to petscii." },

    {1, "pcommand", PA_PETSCII, do_command , "[-e|--extended] <device> <cmdstr>",
        "deprecated; use 'cbmctrl --petscii command' instead!",
        "This command is deprecated; use 'cbmctrl -p command' instead!\n\n"
        "NOTE: You have to give the commands in lower-case letters.\n"
        "      Upper case will NOT work!\n" },

    {1, "dir"     , PA_PETSCII, do_dir     , "<device> [<drive>]",
        "output the directory of the disk in the specified drive",
        "This command gets the directory of a disk in the drive.\n\n"
        "<device> is the device number of the drive (bus ID).\n" 
        "<drive> is the drive number of a dual drive (LUN), default is 0." },

    {1, "download", PA_RAW,     do_download, "<device> <adr> <count> [<file>]",
        "download memory contents from the floppy drive",
        "With this command, you can get data from the floppy drive memory.\n"
        "<device> is the device number of the drive.\n"
        "<adr>    is the starting address of the memory region to get.\n"
        "         it can be given in decimal or in hex (with a 0x prefix).\n"
        "<count>  is the number of bytes to read.\n"
        "         it can be given in decimal or in hex (with a 0x prefix).\n"
        "<file>   (optional) file name of a file to write the contents to.\n"
        "         If this name is not given or it is a dash ('-'), the\n"
        "         contents will be written to stdout, normally the console.\n\n" 
        "Example:\n"
        " cbmctrl download 8 0xc000 0x4000 1541ROM.BIN\n"
        " * reads the 1541 ROM (from $C000 to $FFFF) from drive 8 into 1541ROM.BIN" },

    {1, "upload"  , PA_RAW,     do_upload  , "<device> <adr> [<file>]",
        "upload memory contents to the floppy drive",
        "With this command, you can write data to the floppy drive memory.\n"
        "<device> is the device number of the drive.\n"
        "<adr>    is the starting address of the memory region to write to.\n"
        "         it can be given in decimal or in hex (with a 0x prefix).\n"
        "         If this is -1, the first two bytes from the file are\n"
        "         considered as start address.\n"
        "<file>   (optional) file name of a file to read the values from.\n"
        "         If this name is not given or it is a dash ('-'), the\n"
        "         contents will be read from stdin, normally the console."
        "Example:\n"
        " cbmctrl upload 8 0x500 BUFFER2.BIN\n"
        " * writes the file BUFFER2.BIN to drive 8, address $500." },

    {1, "clk"     , PA_UNSPEC,  do_iec_clk  , "",
        "Set the clk line on the IEC bus.",
        "This command unconditionally sets the CLK line on the IEC bus." },

    {1, "uclk"    , PA_UNSPEC,  do_iec_uclk , "",
        "Unset the clk line on the IEC bus.",
        "This command unconditionally unsets the CLK line on the IEC bus." },

    {1, "reset"   , PA_UNSPEC,  do_reset   , "",
        "reset all drives on the IEC bus",
        "This command performs a (physical) reset of all drives on the IEC bus." },

    {1, "detect"  , PA_UNSPEC,  do_detect  , "",
        "detect all drives on the IEC bus",
        "This command tries to detect all drives on the IEC bus.\n"
        "For this, this command accesses all possible drives and tries to read\n"
        "some bytes from their memory. If a drive is detected, its name is output.\n"
        "Additionally, this routine determines if the drive is connected via a\n"
        "parallel cable (XP1541 companion cable)." },

    {1, "change"  , PA_UNSPEC,  do_change  , "<device>",
        "wait for a disk to be changed in the specified drive",
        "This command waits for a disk to be changed in the specified drive.\n\n"
        "For this, it makes the following assumptions:\n\n"
        "* there is already a disk in the drive.\n"
        "* that disk will be removed and replaced by another disk.\n"
        "* we do not want to return from this command until the disk is completely\n"
        "  inserted and ready to be read/written.\n\n"
        "Because of this, just opening the drive and closing it again (without\n"
        "actually removing the disk) will not work in most cases." },

    {0, NULL, PA_UNSPEC, NULL, NULL, NULL}
};

static struct prog *
process_cmdline_find_command(OPTIONS *options)
{
    if (options->argc >= 1)
    {
        const char * const name = options->argv[0];
        int i;

        for (i=0; prog_table[i].name; i++)
        {
            if (strcmp(name, prog_table[i].name) == 0)
            {
                // advance to the next command-line argument
                options->argc -= 1;
                options->argv += 1;

                // return: We found the command
                return &prog_table[i];
            }
        }
    }

    return NULL;
}

/*
 * Output a help screen
 */
static int do_help(CBM_FILE fd, OPTIONS * const options)
{
    int i;

    do_version(fd, options);

    printf("\n");

    if (options->argc == 0)
    {
        printf("control serial CBM devices\n"
            "\n"
            " Synopsis:  cbmctrl  [global_options] [action] [action_opt] [--] [action_args]\n"
            "\n"
            " Global options:\n"
            "\n"
            "   -h, --help:    Output this help screen if specified without an action.\n"
            "                  Outputs some help information about a specific action\n"
            "                  if an action is specified.\n"
            "   -V, --version: Output version information\n"
            "   -@, --adapter: Tell OpenCBM which backend plugin and bus to use. This option\n"
            "                  requires an argument of the form <plugin>[:<bus>].\n"
            "                  <plugin> is the backend plugin's name to use (e.g.: xa1541)\n"
            "                  <bus>    is a bus unit identifier, if supported by the backend;\n"
            "                           look up the backend's documentation for the supported\n"
            "                           bus unit identifier(s) and the format for <bus>\n"
            "   -p, --petscii: Convert input or output parameter from CBM format (PETSCII)\n"
            "                  into PC format (ASCII). Default with all actions but 'open'\n"
            "                  and 'command'\n"
            "   -r, --raw:     Do not convert data between CBM and PC format.\n"
            "                  Default with 'open' and 'command'.\n"
            "   --             Delimiter between action_opt and action_args; if any of the\n"
            "                  arguments in action_args starts with a '-', make sure to set\n"
            "                  the '--' so the argument is not treated as an option,\n"
            "                  accidentially.\n"
            "\n"
            " action: one of:\n"
            "\n");

        for(i=0; prog_table[i].prog; i++)
        {
            printf("  %-9s %s\n", prog_table[i].name, prog_table[i].shorthelp_text);
        }

        printf("\nFor more information on a specific action, try --help <action>.\n");

    }
    else
    {
        struct prog *pprog;

        pprog = process_cmdline_find_command(options);

        if (pprog)
        {
            printf(" cbmctrl %s %s\n\n  %s\n\n%s\n",
                pprog->name, pprog->arglist, pprog->shorthelp_text, pprog->help_text);
        }
        else
        {
            printf(" Nothing known about \"cbmctrl %s\".\n", options->argv[0]);
        }
    }

    return 0;
}

static int
set_option(int *Where, int NewValue, int OldValue, const char * const Description)
{
    int error = 0;

    if (*Where != OldValue)
    {
        error = 1;
        fprintf(stderr, "Specified option '%s', but there is a conflicting option,\n"
            "or it is specified twice!\n", Description);
    }

    *Where = NewValue;

    return error;
}

static int
set_option_petsciiraw(PETSCII_RAW *Where, PETSCII_RAW NewValue, int IgnoreError)
{
    int error = 0;

    if (!IgnoreError && (*Where != PA_UNSPEC))
    {
        error = 1;
        fprintf(stderr, "Specified option '--petscii' and '--raw' in one command!\n");
    }

    *Where = NewValue;

    return error;
}

static int
process_cmdline_common_options(int argc, char **argv, OPTIONS *options)
{
    unsigned int use_rc = 1;
    int option_index;
    int option;

    static const char short_options[] = "+fhVpr@:";
    static struct option long_options[] =
    {
        { "adapter" , required_argument, NULL, '@' },
        { "forget"  , no_argument,       NULL, 'f' },
        { "help"    , no_argument,       NULL, 'h' },
        { "version" , no_argument,       NULL, 'V' },
        { "petscii" , no_argument,       NULL, 'p' },
        { "raw"     , no_argument,       NULL, 'r' },
        { NULL      , no_argument,       NULL, 0   }
    };

    // clear all options
    memset(options, 0, sizeof(*options));
    options->adapter = 0;

    // remember argc and argv, so they can be used later
    options->argc = argc;
    options->argv = argv;

    // first of all, parse the command-line (before the command):
    // is -f specified (thus, we should ignore .opencbmrc)?
    //
    optind = 0;

    while ((option = getopt_long(options->argc, options->argv, short_options, long_options, &option_index)) != EOF)
    {
        if (option == 'f')
            use_rc = 0;
    }

    // if we should read .opencbmrc, do it now:
    if (use_rc)
        ; // @TODO: TBD

    // now, we start the "real" scanning
    optind = 0;

    while ((option = getopt_long(options->argc, options->argv, short_options, long_options, &option_index)) != EOF)
    {
        switch (option)
        {
        case '?':
            fprintf(stderr, "unknown option %s specified!\n",
                options->argv[optind-1]);
            options->error = 1;
            break;

        case 'f':
            // this option has already been processed,
            // ignore it
            break;

        case 'h':
            set_option(&options->help, 1, 0, "--help");
            break;

        case '@':
            if (options->adapter != NULL)
                options->error = 1;
            else
                options->adapter = cbmlibmisc_strdup(optarg);
            break;

        case 'V':
            set_option(&options->version, 1, 0, "--version");
            break;

        case 'p':
            options->error |= set_option_petsciiraw(&options->petsciiraw, PA_PETSCII, 0);
            break;

        case 'r':
            options->error |= set_option_petsciiraw(&options->petsciiraw, PA_RAW, 0);
            break;
        };
    }

    // skip the options that are already processed
    options->argc -= optind;
    options->argv += optind;

    return options->error;
}

static void
free_options(OPTIONS * const options)
{
    cbmlibmisc_strfree(options->adapter);
}

static int
process_version_help(OPTIONS * const options)
{
    int processed = 0;

    if (options->help)
    {
        do_help(0, options);
        processed = 1;
    }

    if (options->version)
    {
        do_version(0, NULL);
        processed = 1;
    }

    if (processed && options->argc > 0)
        fprintf(stderr, "Extra parameters on command-line are ignored.\n");

    return processed;
}

int ARCH_MAINDECL main(int argc, char *argv[])
{
    struct prog *pprog;

    OPTIONS options;

    int rv = 0;

    do {
        CBM_FILE fd;

        // first of all, process the options which affect all commands

        rv = process_cmdline_common_options(argc, argv, &options);

        // check if we have --version or --help specified:

        if (process_version_help(&options))
            break;

        pprog = process_cmdline_find_command(&options);

        if (pprog == NULL)
        {
            printf("invalid command. For info on possible commands, try the --help parameter.\n\n");
            rv = 2;
            break;
        }

        // if neither PETSCII or RAW was specified, use default for that command
        if (options.petsciiraw == PA_UNSPEC)
            options.petsciiraw = pprog->petsciiraw;

        if (pprog->need_driver)
            rv = cbm_driver_open_ex(&fd, options.adapter);

        if (rv != 0)
        {
            if (arch_get_errno())
            {
                arch_error(0, arch_get_errno(), "%s", cbm_get_driver_name_ex(options.adapter));
            }
            else
            {
                fprintf(stderr, "An error occurred opening OpenCBM, aborting...\n");
            }
            break;
        }

        rv = pprog->prog(fd, &options) != 0;
        if (rv && arch_get_errno())
        {
            arch_error(0, arch_get_errno(), "%s", pprog->name);
        }

        if (pprog->need_driver)
            cbm_driver_close(fd);

    } while (0);

    free_options(&options);

    return rv;
}
