a65:

..\frm_analyzer.c: ..\frm_analyzer.inc

..\frm_analyzer.inc: ..\frm_analyzer.a65

.SUFFIXES: .a65

{..\}.a65{..\}.inc:
    ..\..\..\WINDOWS\buildoneinc ..\..\.. $?
