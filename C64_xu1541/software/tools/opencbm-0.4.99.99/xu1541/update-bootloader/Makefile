include ../bootloader/Makefile.common
LDFLAGS += -Wl,$(COMMON_SECTIONS)

DEFINES += -DF_CPU=12000000
COMPILE = avr-gcc -Wall -O2 -Iusbdrv -I. -mmcu=atmega8 $(DEFINES) -I../include/

OBJECTS = flasher.o spm.o firmwaretable.o

TARGET=flash-firmware

all:	$(TARGET).hex

.INTERMEDIATE: $(OBJECTS)

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

.c.s:
	$(COMPILE) -S $< -o $@

$(TARGET).bin:	$(OBJECTS)
	$(COMPILE) -o $(TARGET).bin $(OBJECTS) ${LDFLAGS}

$(TARGET).hex:	$(TARGET).bin
	rm -f $(TARGET).hex $(TARGET).eep.hex
	avr-objcopy -j .text -j .data -j .textfirmwaretable -O ihex $(TARGET).bin $(TARGET).hex
	./checksize $(TARGET).bin 8192 960

disasm:	$(TARGET).bin
	avr-objdump -d $(TARGET).bin

cpp:
	$(COMPILE) -E flasher.c

#program: program-avrusb
program: $(TARGET).hex
	../update_tool/xu1541_update $(TARGET).hex

program-avrusb: $(TARGET).hex
	../update_tool/xu1541_update $(TARGET).hex -o=0x1000 ../bootloader/bootldr-avrusb.hex

program-usbtiny: $(TARGET).hex
	../update_tool/xu1541_update $(TARGET).hex -o=0x1000 ../bootloader/bootldr-usbtiny.hex

terminal:
	avrdude -c stk200 -p atmega8 -t

dump:
	echo "dump flash 0x1700 0x900" > 123
	echo "dump flash 0x0 0x1700" >> 123
	echo "quit" >> 123
	avrdude -c stk200 -p atmega8 -t < 123
	rm 123
