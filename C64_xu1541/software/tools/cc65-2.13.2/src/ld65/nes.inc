static const char CfgNES [] = 
"MEMORY {\n"
"\n"
"ZP:  start = $02, size = $1A, type = rw, define = yes;\n"
"\n"
"\n"    /* INES Cartridge Header */
"HEADER: start = $0, size = $10, file = %O ,fill = yes;\n"
"\n"
"\n"    /* 2 16K ROM Banks */
"\n"    /* - startup */
"\n"    /* - code */
"\n"    /* - rodata */
"\n"    /* - data (load) */
"ROM0: start = $8000, size = $7ff4, file = %O ,fill = yes, define = yes;\n"
"\n"
"\n"    /* Hardware Vectors at End of 2nd 8K ROM */
"ROMV: start = $fff6, size = $c, file = %O, fill = yes;\n"
"\n"
"\n"    /* 1 8k CHR Bank */
"ROM2: start = $0000, size = $2000, file = %O, fill = yes;\n"
"\n"
"\n"    /* standard 2k SRAM (-zeropage) */
"\n"    /* $0100-$0200 cpu stack */
"\n"    /* $0200-$0500 3 pages for ppu memory write buffer */
"\n"    /* $0500-$0800 3 pages for cc65 parameter stack */
"SRAM: start = $0500, size = $0300, define = yes;\n"
"\n"
"\n"    /* additional 8K SRAM Bank */
"\n"    /* - data (run) */
"\n"    /* - bss */
"\n"    /* - heap */
"RAM: start = $6000, size = $2000, define = yes;\n"
"\n"
"}\n"
"\n"
"SEGMENTS {\n"
"HEADER:   load = HEADER,          type = ro;\n"
"STARTUP:  load = ROM0,            type = ro,  define = yes;\n"
"LOWCODE:  load = ROM0,            type = ro,                optional = yes;\n"
"INIT:     load = ROM0,            type = ro,  define = yes, optional = yes;\n"
"CODE:     load = ROM0,            type = ro,  define = yes;\n"
"RODATA:   load = ROM0,            type = ro,  define = yes;\n"
"DATA:     load = ROM0, run = RAM, type = rw,  define = yes;\n"
"VECTORS:  load = ROMV,            type = rw;\n"
"CHARS:    load = ROM2,            type = rw;\n"
"BSS:      load = RAM,             type = bss, define = yes;\n"
"HEAP:     load = RAM,             type = bss, optional = yes;\n"
"ZEROPAGE: load = ZP,              type = zp;\n"
"}\n"
"\n"
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
"\n"
"SYMBOLS {\n"
"__STACKSIZE__ = $0300;\n"  	/* 3 pages stack */
"}\n"
;
