ca65 -l -t c64 fastformat.a65
ld65 -t c64 -o fastformat.bin fastformat.o

ca65 -l -t c64 fastread.a65
ld65 -t c64 -o fastread.bin fastread.o

ca65 -l -t c64 fastwrite.a65
ld65 -t c64 -o fastwrite.bin fastwrite.o

cc65 cross development package for 65(C)02 systems: https://github.com/cc65/cc65
fastformat adapted from cbmformat: https://github.com/OpenCBM/OpenCBM
