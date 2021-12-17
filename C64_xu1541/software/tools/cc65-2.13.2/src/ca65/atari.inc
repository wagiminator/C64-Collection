static char MacAtari[] = 
/* Convert characters to screen codes */
/* Helper macro that converts and outputs one character */
".macro _scrcode char\n"
".if (char >= 0) .and (char <= 31)\n"
".byte   (char + 64)\n"
".elseif (char >= 32) .and (char <= 95)\n"
".byte   (char - 32)\n"
".elseif (char >= 96) .and (char <= 127)\n"
".byte   char\n"
".elseif (char >= 128) .and (char <= 159)\n"
".byte   (char + 64)\n"
".elseif (char >= 160) .and (char <= 223)\n"
".byte   (char - 32)\n"
".elseif (char >= 224) .and (char <= 255)\n"
".byte   char\n"
".else\n"
".error  \"scrcode: Character constant out of range\"\n"
".endif\n"
".endmacro\n"
".macro  scrcode arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9\n"
/* Bail out if next argument is empty */
".if     .blank (arg1)\n"
".exitmacro\n"
".endif\n"
/* Check for a string */
".if     .match ({arg1}, \"\")\n"
/* Walk over all string chars */
".repeat .strlen (arg1), i\n"
"_scrcode        {.strat (arg1, i)}\n"
".endrepeat\n"
/* Check for a number */
".elseif .match (.left (1, {arg1}), 0)\n"
/* Just output the number */
"_scrcode        arg1\n"
/* Check for a character */
".elseif .match (.left (1, {arg1}), \'a\')\n"
/* Just output the character */
"_scrcode        arg1\n"
/* Anything else is an error */
".else\n"
".error  \"scrcode: invalid argument type\"\n"
".endif\n"
/* Call the macro recursively with the remaining args */
"scrcode arg2, arg3, arg4, arg5, arg6, arg7, arg8\n"
".endmacro\n"
;
