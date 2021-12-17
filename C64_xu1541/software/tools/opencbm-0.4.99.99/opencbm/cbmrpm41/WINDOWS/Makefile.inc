a65:

..\$(TARGETNAME).c: ..\$(TARGETNAME).inc ..\$(TARGETNAME).h ..\$(TARGETNAME).idh

..\$(TARGETNAME).inc: ..\$(TARGETNAME).a65

..\$(TARGETNAME).a65: ..\$(TARGETNAME).idh

.SUFFIXES: .a65

{..\}.a65{..\}.inc:
    ..\..\WINDOWS\buildoneinc ..\.. $?
