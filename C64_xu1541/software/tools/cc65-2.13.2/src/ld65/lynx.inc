static const char CfgLynx [] = 
"MEMORY {\n"
"ZP:     start = $0000, size = $0100, type = rw, define = yes;\n"
"HEADER: start = $0000, size = $000A, file = %O;\n"
"RAM:    start = $0400, size = $BC38, define = yes, file = %O;\n"
"}\n"
"SEGMENTS {\n"
"EXEHDR:   load = HEADER, type = ro;\n"
"STARTUP:  load = RAM,    type = ro;\n"
"LOWCODE:  load = RAM,    type = ro,               optional = yes;\n"
"INIT:     load = RAM,    type = ro, define = yes, optional = yes;\n"
"CODE:     load = RAM,    type = ro;\n"
"RODATA:   load = RAM,    type = ro;\n"
"DATA:     load = RAM,    type = rw;\n"
"BSS:      load = RAM,    type = bss, define = yes;\n"
"HEAP:     load = RAM,    type = bss,              optional = yes;\n" /* must sit just below stack */
"ZEROPAGE: load = ZP,     type = zp;\n"
"EXTZP:    load = ZP,     type = zp,               optional = yes;\n"
"APPZP:    load = ZP,     type = zp,               optional = yes;\n"
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
"CONDES: segment = RODATA,\n"
"type = interruptor,\n"
"label = __INTERRUPTOR_TABLE__,\n"
"count = __INTERRUPTOR_COUNT__;\n"
"}\n"
"SYMBOLS {\n"
"__STACKSIZE__ = $800;\n"       /* 2K stack */
"}\n"
;