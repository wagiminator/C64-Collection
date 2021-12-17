a65:

..\imgcopy.c: ..\turboread1541.inc ..\turbowrite1541.inc ..\turboread1571.inc ..\turbowrite1571.inc ..\turboread1581.inc ..\turbowrite1581.inc

..\pp.c: ..\pp1541.inc ..\pp1571.inc
..\s1.c: ..\s1.inc ..\s1-1581.inc
..\s2.c: ..\s2.inc ..\s2-1581.inc
..\s3.c: ..\s3.inc ..\s3-1581.inc

..\pp1541.inc: ..\pp1541.a65
..\pp1571.inc: ..\pp1571.a65
..\s1.inc: ..\s1.a65
..\s1-1581.inc: ..\s1-1581.a65
..\s2.inc: ..\s2.a65
..\s2-1581.inc: ..\s2-1581.a65
..\s3.inc: ..\s3.a65
..\s3-1581.inc: ..\s3-1581.a65

..\turboread1541.inc: ..\turboread1541.a65
..\turbowrite1541.inc: ..\turbowrite1541.a65
..\turboread1571.inc: ..\turboread1571.a65
..\turbowrite1571.inc: ..\turbowrite1571.a65
..\turboread1581.inc: ..\turboread1581.a65
..\turbowrite1581.inc: ..\turbowrite1581.a65


.SUFFIXES: .a65

{..\}.a65{..\}.inc:
    ..\..\WINDOWS\buildoneinc ..\.. $?
