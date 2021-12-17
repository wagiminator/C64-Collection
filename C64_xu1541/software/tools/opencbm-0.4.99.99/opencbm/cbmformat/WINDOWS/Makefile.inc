a65:

..\cbmformat.c: ..\cbmformat.inc

..\cbmformat.inc: ..\cbmformat.a65

.SUFFIXES: .a65

{..\}.a65{..\}.inc:
    ..\..\WINDOWS\buildoneinc ..\.. $?
