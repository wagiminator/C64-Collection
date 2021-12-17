@echo off
set LOOPCOUNT=1000
REM set LOOPCOUNT=2

cbmctrl reset

	REM automatically selected a device which got a parallel cable
	REM connected to, otherwise select the device with the highest ID
for /f "tokens=1,3 delims=():" %%f in ('cbmctrl detect') do (
	set DRIVE=%%f
	if "%%g"=="XP1541" goto FOR_BREAK
	)
:FOR_BREAK

	REM explicitly select an IEC device
REM set DRIVE=10

echo This test requires an empty disk to be inserted that gets overwritten.
echo Please remove a currently inserted disk and (re-) insert the work disk:
echo.
echo Waiting for disk change in IEC device %DRIVE%
cbmctrl change %DRIVE%

cbmforng -v -s %DRIVE% "FULL DIR TRACK,FT"
d64copy -ta -w RELFILES.D64 %DRIVE%

for /l %%i in (1,1,%LOOPCOUNT%) do (
        echo.
        echo Run number %%i:
        cbmctrl status %DRIVE%
        cbmctrl dir %DRIVE%
)
