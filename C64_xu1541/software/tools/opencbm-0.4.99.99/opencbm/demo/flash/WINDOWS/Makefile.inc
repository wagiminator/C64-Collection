a65:

..\flash.c: ..\flash.inc

..\flash.inc: ..\flash.a65

.SUFFIXES: .a65

{..\}.a65{..\}.inc:
    ..\..\..\WINDOWS\buildoneinc ..\..\.. $?
