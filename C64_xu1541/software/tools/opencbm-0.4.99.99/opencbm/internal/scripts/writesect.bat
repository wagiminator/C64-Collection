@echo off
if "%3" == "" (
	echo Write sectors to a CBM diskette.
	echo Usage: writesect DRIVE TRACK SECT
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

cbmctrl status %DRIVE%
debug drive.bin
cbmctrl close %DRIVE% 2

echo You can abort by pressing CTRL+C or CTRL+BREAK
pause
cbmctrl open %DRIVE% 2 #
cbmctrl pcommand %DRIVE% "b-p 2 0"
cbmctrl listen %DRIVE% 2
cbmctrl write drive.bin
cbmctrl unlisten

cbmctrl pcommand %DRIVE% "u2: 2 0 %TRACK% %SECT%"

cbmctrl close %DRIVE% 2

:end
