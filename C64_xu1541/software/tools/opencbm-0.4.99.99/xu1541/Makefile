.PHONY: all mrproper distclean clean all-linux firmware bootloader bootloader-avrusb bootloader-usbtiny update-bootloader misc update_tool program-avrusb program-usbtiny update-avrusb update-usbtiny update-bios-avrusb update-bios-usbtiny update-all-avrusb update-all-usbtiny diff terminal version xu1541lib xmingw exe cygwin

all:	all-linux

mrproper: clean
	rm -f firmware/*.hex bootloader/*.hex update-bootloader/*.hex misc/read_event_log.exe misc/usb_echo_test.exe update_tool/xu1541_update.exe

distclean: mrproper

clean:
	cvspurge

all-linux: xu1541lib misc update_tool firmware bootloader update-bootloader

firmware:
	make -C firmware

bootloader: bootloader-avrusb bootloader-usbtiny

bootloader-avrusb:
	make -C bootloader -f Makefile-avrusb

bootloader-usbtiny:
	make -C bootloader -f Makefile-usbtiny

update-bootloader:
	make -C update-bootloader

misc:
	make -C misc/

update_tool:
	make -C update_tool/src/

program-avrusb: bootloader-avrusb
	make -C bootloader -f Makefile-avrusb program

program-usbtiny: bootloader-usbtiny
	make -C bootloader -f Makefile-usbtiny program

update-firmware: firmware update_tool
	./update_tool/xu1541_update ./firmware/firmware.hex

update-bios-avrusb: update-avrusb
update-avrusb: bootloader-avrusb update_tool update-bootloader
	make -C update-bootloader program-avrusb

update-bios-usbtiny: update-usbtiny
update-usbtiny: bootloader-usbtiny update_tool update-bootloader
	make -C update-bootloader program-usbtiny

update-all-avrusb: bootloader-avrusb update_tool update-bootloader firmware
	./update_tool/xu1541_update ./update-bootloader/flash-firmware.hex -o=0x1000 ./bootloader/bootldr-avrusb.hex -R ./firmware/firmware.hex

update-all-usbtiny: bootloader-usbtiny update_tool update-bootloader firmware
	./update_tool/xu1541_update ./update-bootloader/flash-firmware.hex -o=0x1000 ./bootloader/bootldr-usbtiny.hex -R ./firmware/firmware.hex

diff:
	make distclean
	cvs diff|view -

terminal:
	make -C update-bootloader terminal

version: misc
	./misc/usb_echo_test

xu1541lib:
	make -C lib

xmingw:
	(export MINGW=1; make -C lib/)
	(export MINGW=1; make -C misc/)
	(export MINGW=1; make -C update_tool/src/)

exe:	misc update_tool

cygwin:	exe
