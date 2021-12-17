static const char CfgLunix [] = 
"\n"/* ld65 Linker-configuration for LUnix, Next Generation. */
"MEMORY {\n"
"ZP:	 start = $0080, size = $0040;\n"        /* userzp buffer */
"RAM: start = %S,  size = $7600;\n"
"}\n"
"SEGMENTS {\n"
"ZEROPAGE: load = ZP,  type = zp,  define = yes;\n" /* Pseudo-registers */
"STARTUP:  load = RAM, type = ro;\n"  	/* First initiation code */
"LOWCODE:  load = RAM, type = ro, optional = yes;\n" /* Legacy from other platforms */
"INIT:     load = RAM, type = ro, define = yes, optional = yes;\n"
"CODE:     load = RAM, type = ro;\n"  	/* Program */
"RODATA:   load = RAM, type = ro;\n"  	/* Literals, constants */
"DATA:     load = RAM, type = rw;\n"  	/* Initialized variables */
"BSS:      load = RAM, type = bss, define = yes;\n" /* Uninitialized variables */
"}\n"
"FEATURES {\n"
"CONDES: segment = INIT,\n"
"type = constructor,\n"
"label = __CONSTRUCTOR_TABLE__,\n"
"count = __CONSTRUCTOR_COUNT__;\n"
"CONDES: segment = RODATA,\n"
"type = destructor,\n"
"label = __DESTRUCTOR_TABLE__,\n"
"count = __DESTRUCTOR_COUNT__;\n"
"CONDES: type = interruptor,\n"
"segment = RODATA,\n"
"label = __INTERRUPTOR_TABLE__,\n"
"count = __INTERRUPTOR_COUNT__;\n"
"}\n"
"FILES {\n"
"%O: format = o65;\n"
"}\n"
"FORMATS {\n"
"o65: os = lunix, type = small,\n"
"export = main,\n"	       	/* Program entry-point */
"import = LUNIXKERNEL;\n"	/* Kernel entry-points */
"}\n"
"SYMBOLS {\n"
"__HEAPSIZE__  = $2000;\n"      /* 8k heap [temporary, until LUnix malloc() exists] */
"__STACKSIZE__ = $0400;\n"      /* 1k stack (do typical LUnix apps. need 2k?) */
"}\n"
;
