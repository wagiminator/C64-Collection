ca65 -l -t c64 fastformat.asm
ld65 -t c64 -o fastformat.bin fastformat.o

ca65 -l -t c64 fastread.asm
ld65 -t c64 -o fastread.bin fastread.o

ca65 -l -t c64 fastwrite.asm
ld65 -t c64 -o fastwrite.bin fastwrite.o

ca65 -l -t c64 fastload.asm
ld65 -t c64 -o fastload.bin fastload.o

ca65 -l -t c64 fastsave.asm
ld65 -t c64 -o fastsave.bin fastread.o

cc65 cross development package for 65(C)02 systems: https://github.com/cc65/cc65
fastformat adapted from cbmformat: https://github.com/OpenCBM/OpenCBM
