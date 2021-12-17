a65:

..\morse.c: ..\morse.inc

..\morse.inc: ..\morse.a65

.SUFFIXES: .a65

{..\}.a65{..\}.inc:
    ..\..\..\WINDOWS\buildoneinc ..\..\.. $?
