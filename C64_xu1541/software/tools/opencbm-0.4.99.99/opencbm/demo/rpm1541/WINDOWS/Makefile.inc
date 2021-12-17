a65:

..\rpm1541.c: ..\rpm1541.inc

..\rpm1541.inc: ..\rpm1541.a65

.SUFFIXES: .a65

{..\}.a65{..\}.inc:
    ..\..\..\WINDOWS\buildoneinc ..\..\.. $?
