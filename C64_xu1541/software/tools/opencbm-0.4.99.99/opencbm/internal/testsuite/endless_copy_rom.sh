#! /bin/sh
#
# set -x

clear

FC=`which fc`

ROMSTART=0x8000
ROMLEN=0x8000
DRIVE=8

[ -f COMPARE.ROM.TEST ] && rm COMPARE.ROM.TEST

echo -e "\nDOWNLOADING initial ROM (for compare reasons)"
date
./cbmctrl download $DRIVE $ROMSTART $ROMLEN COMPARE.ROM.TEST
date

while true; do
	[ -f read.rom.test ] && rm read.rom.test
	echo -e "\nDOWNLOADING"
	date
	./cbmctrl download $DRIVE $ROMSTART $ROMLEN read.rom.test
	date
	$FC /b COMPARE.ROM.TEST read.rom.test
	echo -e "\nUPLOADING (does not make sense, but anyway...)"
	date
	./cbmctrl upload $DRIVE $ROMSTART COMPARE.ROM.TEST
	date
done
