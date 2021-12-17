/*
 * Minimalistic overlay demo program.
 *
 * 2009-10-02, Oliver Schmidt (ol.sc@web.de)
 *
 */



#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>


extern void _OVL1CODE_LOAD__, _OVL1CODE_SIZE__;
extern void _OVL2CODE_LOAD__, _OVL2CODE_SIZE__;
extern void _OVL3CODE_LOAD__, _OVL3CODE_SIZE__;


void log (char *msg)
{
    printf ("Log: %s\n", msg);
}


#pragma codeseg (push, "OVL1CODE");

void foo (void)
{
    log ("Calling main from overlay 1");
}

#pragma codeseg(pop);


#pragma codeseg (push, "OVL2CODE");

void bar (void)
{
    log ("Calling main from overlay 2");
}

#pragma codeseg(pop);


#pragma codeseg (push, "OVL3CODE");

void foobar (void)
{
    log ("Calling main from overlay 3");
}

#pragma codeseg(pop);


unsigned char loadfile (char *name, void *addr, void *size)
{
    int file = open (name, O_RDONLY);
    if (file == -1) {
        log ("Opening overlay file failed");
        return 0;
    }

    read (file, addr, (unsigned) size);
    close (file);
    return 1;
}


void main (void)
{
    log ("Calling overlay 1 from main");
    if (loadfile ("overlaydemo.1", &_OVL1CODE_LOAD__, &_OVL1CODE_SIZE__)) {
        foo ();
    }

    log ("Calling overlay 2 from main");
    if (loadfile ("overlaydemo.2", &_OVL2CODE_LOAD__, &_OVL2CODE_SIZE__)) {
        bar ();
    }

    log ("Calling overlay 3 from main");
    if (loadfile ("overlaydemo.3", &_OVL3CODE_LOAD__, &_OVL3CODE_SIZE__)) {
        foobar ();
    }
}
