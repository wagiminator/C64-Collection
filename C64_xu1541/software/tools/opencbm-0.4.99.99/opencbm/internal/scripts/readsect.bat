@echo off
if "%3" == "" (
	echo Read sectors of a CBM diskette.
	echo Usage: readsect DRIVE TRACK SECT
	goto end
)

set DRIVE=%1
set TRACK=%2
set SECT=%3
del drive.bin
cbmctrl open %DRIVE% 2 #
cbmctrl pcommand %DRIVE% "u1: 2 0 %TRACK% %SECT%"
cbmctrl talk %DRIVE% 2
cbmctrl read drive.bin
cbmctrl untalk
cbmctrl close %DRIVE% 2
cbmctrl status %DRIVE%
debug drive.bin

:end
