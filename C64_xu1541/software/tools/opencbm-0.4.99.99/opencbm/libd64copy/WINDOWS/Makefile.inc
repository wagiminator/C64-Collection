a65:

..\d64copy.c: ..\turboread1541.inc ..\turbowrite1541.inc ..\turboread1571.inc ..\turbowrite1571.inc ..\warpread1541.inc ..\warpwrite1541.inc ..\warpread1571.inc ..\warpwrite1571.inc

..\pp.c: ..\pp1541.inc ..\pp1571.inc
..\s1.c: ..\s1.inc
..\s2.c: ..\s2.inc

..\pp1541.inc: ..\pp1541.a65
..\pp1541.inc: ..\pp1571.a65
..\s1.inc: ..\s1.a65
..\s1.inc: ..\s2.a65

..\turboread1541.inc: ..\turboread1541.a65
..\turbowrite1541.inc: ..\turbowrite1541.a65
..\turboread1571.inc: ..\turboread1571.a65
..\turbowrite1571.inc: ..\turbowrite1571.a65

..\warpread1541.inc: ..\warpread1541.a65
..\warpwrite1541.inc: ..\warpwrite1541.a65
..\warpread1571.inc: ..\warpread1571.a65
..\warpwrite1571.inc: ..\warpwrite1571.a65


.SUFFIXES: .a65

{..\}.a65{..\}.inc:
    ..\..\WINDOWS\buildoneinc ..\.. $?
