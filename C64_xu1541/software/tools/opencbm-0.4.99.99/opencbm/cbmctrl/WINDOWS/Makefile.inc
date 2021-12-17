a65:

..\cbmctrl.c: ..\tdchange.inc

..\tdchange.inc: ..\tdchange.a65 ..\common.i65

.SUFFIXES: .a65

{..\}.a65{..\}.inc:
    ..\..\WINDOWS\buildoneinc ..\.. $?
