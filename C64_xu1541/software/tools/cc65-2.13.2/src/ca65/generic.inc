static char MacGeneric[] = 
/* add - Add without carry */
".macro  add     Arg1, Arg2\n"
"clc\n"
".if .paramcount = 2\n"
"adc     Arg1, Arg2\n"
".else\n"
"adc     Arg1\n"
".endif\n"
".endmacro\n"
/* sub - subtract without borrow */
".macro  sub     Arg1, Arg2\n"
"sec\n"
".if .paramcount = 2\n"
"sbc     Arg1, Arg2\n"
".else\n"
"sbc     Arg1\n"
".endif\n"
".endmacro\n"
;
