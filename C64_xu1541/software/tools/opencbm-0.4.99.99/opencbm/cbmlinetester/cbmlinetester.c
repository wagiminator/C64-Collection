/*
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Copyright 1999 Michael Klein <michael(dot)klein(at)puffin(dot)lb(dot)shuttle(dot)de>
*/

#include "opencbm.h"

#include <ctype.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_NCURSES
# include <ncurses.h>
# define arch_getch() getch()
#endif

#include "arch.h"
#include "libmisc.h"


#ifdef WIN32
# include <conio.h>
# define HAVE_NCURSES
# define printw printf
# define refresh()
# define initscr()
# define nodelay(_a, _b)
# define noecho()
# define scrollok(_a, _b)
# define cbreak()
# define endwin()

static int move(SHORT _y, SHORT _x)
{
    COORD coordinates;
    coordinates.X = _x;
    coordinates.Y = _y;
    return !SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coordinates);
}

static int arch_getch(void)
{
    int ch = -1;
    
    if (_kbhit()) {
        ch = _getch();
    }
    else {
        Sleep(50);
    }

    return ch;
}

#endif

static void help()
{
    printf(
        "Usage: cbmlinetester [OPTION]\n"
        "Set the IEC lines to specific values\n"
        "\n"
        "  -h, --help                 display this help and exit\n"
        "  -V, --version              display version information and exit\n"
        "  -@, --adapter=plugin:bus   tell OpenCBM which backend plugin and bus to use\n"
        "\n"
        "  -i, --interactive          Use the tool interactively (only with ncurses)\n"
        "  -p, --poll                 Poll the values of the lines\n"
        "\n"
        "  -r, --reset                SET the RESET line\n"
        "  -R, --RESET                RELEASE the RESET line\n"
        "  -a, --atn                  SET the ATN line\n"
        "  -A, --ATN                  RELEASE the ATN line\n"
        "  -c, --clock                SET the CLOCK line\n"
        "  -C, --CLOCK                RELEASE the CLOCK line\n"
        "  -d, --data                 SET the DATA line\n"
        "  -D, --DATA                 RELEASE the DATA line\n"
        "\n"
        );
}

static void hint(char *s)
{
    fprintf(stderr, "Try `%s' -h for more information.\n", s);
}

static const char * getLineStatus(int status)
{
    static char strstatus[25];

    arch_snprintf(strstatus, sizeof strstatus, "%s %s %s %s",
        (status & IEC_RESET) ? "RESET" : "     ",
        (status & IEC_ATN  ) ? "ATN  " : "     ",
        (status & IEC_DATA ) ? "DATA " : "     ",
        (status & IEC_CLOCK) ? "CLOCK" : "     ");

    return strstatus;
}

static void printpoll(CBM_FILE fd)
{
    /* read the line status */
    int poll_status;

    poll_status = cbm_iec_poll(fd);

    printf("line status: %s\n", getLineStatus(poll_status));
}

#ifdef HAVE_NCURSES
static void printpollw(CBM_FILE fd)
{
    /* read the line status */
    int poll_status;

    poll_status = cbm_iec_poll(fd);

    printw("line status: %s\n", getLineStatus(poll_status));
    refresh();
}

static void interactive(CBM_FILE fd, int ownmask)
{
    int quit = 0;

    initscr();
    noecho();
    cbreak();
    nodelay(stdscr, TRUE);
    scrollok(stdscr, TRUE);

    do {
        int setmask = 0;
        int releasemask = 0;
        int ch;

        move(0, 0);
        printw("own:         %s\n", getLineStatus(ownmask));
        printpollw(fd);

        ch = arch_getch();

        switch (ch) {
            case 'r': setmask     = IEC_RESET; break;
            case 'R': releasemask = IEC_RESET; break;
            case 'a': setmask     = IEC_ATN;   break;
            case 'A': releasemask = IEC_ATN;   break;
            case 'c': setmask     = IEC_CLOCK; break;
            case 'C': releasemask = IEC_CLOCK; break;
            case 'd': setmask     = IEC_DATA;  break;
            case 'D': releasemask = IEC_DATA;  break;
            case 'q': case 'Q': case 'x': case 'X': quit = 1; break;
            default:  break;
        }

        if (!quit) {
                cbm_iec_setrelease(fd, setmask, releasemask);
                ownmask |= setmask;
                ownmask &= ~releasemask;
        }

    } while (!quit);

    endwin();

    printpoll(fd);
}
#endif // #ifdef HAVE_NCURSES

int ARCH_MAINDECL main(int argc, char *argv[])
{
    CBM_FILE fd;
    int fd_is_init = 0;

    int error_return = 1;
    int setmask = 0;
    int releasemask = 0;

    int do_poll = 0;
#ifdef HAVE_NCURSES
    int do_interactive = 0;
#endif // #ifdef HAVE_NCURSES

    char *adapter = NULL;
    int option;

    static const struct option longopts[] =
    {
        { "help"       , no_argument      , NULL, 'h' },
        { "version"    , no_argument      , NULL, 'V' },
        { "adapter"    , required_argument, NULL, '@' },
#ifdef HAVE_NCURSES
        { "interactive", no_argument      , NULL, 'i' },
#endif // #ifdef HAVE_NCURSES
        { "poll"       , no_argument      , NULL, 'p' },
        { "reset"      , no_argument      , NULL, 'r' },
        { "atn"        , no_argument      , NULL, 'a' },
        { "clock"      , no_argument      , NULL, 'c' },
        { "data"       , no_argument      , NULL, 'd' },
        { "RESET"      , no_argument      , NULL, 'R' },
        { "ATN"        , no_argument      , NULL, 'A' },
        { "CLOCK"      , no_argument      , NULL, 'C' },
        { "DATA"       , no_argument      , NULL, 'D' },
        { NULL         , 0                , NULL, 0   }
    };

    static const char shortopts[] ="hV@:pracdRACD"
#ifdef HAVE_NCURSES
                                   "i"
#endif // #ifdef HAVE_NCURSES
                                   ;
 
    while ((option = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1)
    {
        switch(option)
        {
            case 'h': help();
                      return 0;
            case 'V': printf("cbmlinetester %s\n", OPENCBM_VERSION);
                      return 0;
            case '@': if (adapter == NULL)
                          adapter = cbmlibmisc_strdup(optarg);
                      else
                      {
                          fprintf(stderr, "--adapter/-@ given more than once.");
                          hint(argv[0]);
                          return 1;
                      }
                      break;
#ifdef HAVE_NCURSES
            case 'i': do_interactive = 1;
                      break;
#endif // #ifdef HAVE_NCURSES
            case 'p': do_poll = 1;
                      break;
            case 'r': setmask |= IEC_RESET;
                      break;
            case 'a': setmask |= IEC_ATN;
                      break;
            case 'c': setmask |= IEC_CLOCK;
                      break;
            case 'd': setmask |= IEC_DATA;
                      break;
            case 'R': releasemask |= IEC_RESET;
                      break;
            case 'A': releasemask |= IEC_ATN;
                      break;
            case 'C': releasemask |= IEC_CLOCK;
                      break;
            case 'D': releasemask |= IEC_DATA;
                      break;
            default : hint(argv[0]);
                      return 1;
        }
    }

    do {
        if (setmask & releasemask) {
            /* some line(s) has/ave been specified for setting and resetting --> abort */

            int doubled_mask = setmask & releasemask;

            fprintf(stderr, "You have specified to SET and RELEASE the line(s)\n");
            if (doubled_mask & IEC_RESET) {
                fprintf(stderr, "\tRESET");
                    if ((doubled_mask &= ~IEC_RESET) != 0) fprintf(stderr, ",\n");
            }
            if (doubled_mask & IEC_ATN) {
                fprintf(stderr, "\tATN");
                    if ((doubled_mask &= ~IEC_ATN) != 0) fprintf(stderr, ",\n");
            }
            if (doubled_mask & IEC_DATA) {
                fprintf(stderr, "\tDATA");
                    if ((doubled_mask &= ~IEC_DATA) != 0) fprintf(stderr, ",\n");
            }
            if (doubled_mask & IEC_CLOCK) {
                fprintf(stderr, "\tCLOCK");
                    if ((doubled_mask &= ~IEC_CLOCK) != 0) fprintf(stderr, ",\n");
            }
            fprintf(stderr, "\nat the same time.\n\nThis cannot work, thus, aborting.\n");
            break;
        }

        if (cbm_driver_open_ex(&fd, adapter)) {
            arch_error(0, arch_get_errno(), "%s", cbm_get_driver_name_ex(adapter));
            cbmlibmisc_strfree(adapter);
            break;
        }

        fd_is_init = 1;

        cbm_iec_setrelease(fd, setmask, releasemask);

#ifdef HAVE_NCURSES
        if (do_interactive) {
            interactive(fd, setmask);
        }
        else
#endif // #ifdef HAVE_NCURSES
        if (do_poll) {
            printpoll(fd);
        }

        error_return = 0;

    } while (0);


    if (fd_is_init) {
        cbm_driver_close(fd);
    }

    cbmlibmisc_strfree(adapter);

    return error_return;
}
