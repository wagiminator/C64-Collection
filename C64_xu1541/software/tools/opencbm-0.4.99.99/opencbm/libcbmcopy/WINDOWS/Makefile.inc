a65:

..\cbmcopy.c: ..\turboread1541.inc ..\turbowrite1541.inc ..\turboread1571.inc ..\turbowrite1571.inc ..\turboread1581.inc ..\turbowrite1581.inc

..\pp.c: ..\ppr-1541.inc ..\ppr-1571.inc ..\ppw-1541.inc ..\ppw-1571.inc
..\s1.c: ..\s1r.inc ..\s1w.inc ..\s1r-1581.inc ..\s1w-1581.inc
..\s2.c: ..\s2r.inc ..\s2w.inc ..\s2r-1581.inc ..\s2w-1581.inc

..\ppr-1541.inc: ..\ppr-1541.a65
..\ppw-1541.inc: ..\ppw-1541.a65
..\ppr-1571.inc: ..\ppr-1571.a65
..\ppw-1571.inc: ..\ppw-1571.a65
..\s1r.inc: ..\s1r.a65
..\s1w.inc: ..\s1w.a65
..\s1r-1581.inc: ..\s1r-1581.a65
..\s1w-1581.inc: ..\s1w-1581.a65
..\s2r.inc: ..\s2r.a65
..\s2w.inc: ..\s2w.a65
..\s2r-1581.inc: ..\s2r-1581.a65
..\s2w-1581.inc: ..\s2w-1581.a65

..\turboread1541.inc: ..\turboread1541.a65
..\turbowrite1541.inc: ..\turbowrite1541.a65
..\turboread1571.inc: ..\turboread1571.a65
..\turbowrite1571.inc: ..\turbowrite1571.a65
..\turboread1581.inc: ..\turboread1581.a65
..\turbowrite1581.inc: ..\turbowrite1581.a65


.SUFFIXES: .a65

{..\}.a65{..\}.inc:
    ..\..\WINDOWS\buildoneinc ..\.. $?
