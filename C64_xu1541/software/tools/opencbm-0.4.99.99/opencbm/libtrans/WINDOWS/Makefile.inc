a65: ..\turbomain.inc ..\s1.inc ..\s2.inc ..\pp1541.inc ..\pp1571.inc

.SUFFIXES: .a65

{..\}.a65{..\}.inc:
    ..\..\WINDOWS\buildoneinc ..\.. $?
