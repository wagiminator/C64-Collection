@ECHO OFF

REM transfer mode can be set to `auto' since some versions now
SET TRANSFEROPT=-ta

REM SET TRANSFEROPT=-tp		REM explicitly set parallel transfer mode
REM SET TRANSFEROPT=-ts2	REM explicitly set serial2 transfer mode
REM SET TRANSFEROPT=-ts1	REM explicitly set serial1 transfer mode

REM number of loops with formatting and transferring
SET LOOPCOUNT=100

REM the test image file (35 tracks)
SET SOURCEFILE=filleddk.d64

REM the read back image
SET DESTFILE=destfile.d64

REM logging of the script
SET SCRDEBUGLOG=cbmf_script.log

REM explicit next generation format debug output log file
SET CBMFNGLOG=cbmf_ng_debug.log


if "%1"=="LOGGING" goto LOGGINGENABLED
del %CBMFNGLOG% 2> NUL
del %SCRDEBUGLOG% 2> NUL

ECHO J | CALL %0 LOGGING > %SCRDEBUGLOG%
REM CALL %0 LOGGING | tee %SCRDEBUGLOG%
GOTO EXIT
:LOGGINGENABLED


@ECHO ON

cbmctrl reset

	REM automatically selected a device which got a parallel cable
	REM connected to, otherwise select the device with the highest ID
for /f "tokens=1,3 delims=():" %%f in ('cbmctrl detect') do (
	set DRIVENO=%%f
	if "%%g"=="XP1541" goto FOR_BREAK
	)
:FOR_BREAK

echo This test requires an empty disk to be inserted that gets overwritten. 1>&2
echo Please remove a currently inserted disk and (re-) insert the work disk: 1>&2
echo. 1>&2
echo Waiting for disk change in IEC device %DRIVENO% 1>&2
cbmctrl change %DRIVENO%

cbmctrl command %DRIVENO% "N0:originalformat,of"
cbmctrl status %DRIVENO% 1>&2

d64copy -v -w %TRANSFEROPT% %SOURCEFILE% %DRIVENO% 1>&2
d64copy -v -w %TRANSFEROPT% %DRIVENO% %DESTFILE% 1>&2
fc /b %SOURCEFILE% %DESTFILE% | find /v "000" 1>&2

for /l %%i in (1,1,%LOOPCOUNT%) do (
    echo Doing next generation formatter test loop No. %%i 1>&2
    echo Doing next generation formatter test loop No. %%i
	cbmctrl reset
	cbmctrl status %DRIVENO%
    echo "Formatting (with verify)..." 1>&2
	cbmforng -o -v -s %DRIVENO% "cbm-ng-format,nf" >> %CBMFNGLOG%
    echo Exporting... 1>&2
	d64copy -v -w %TRANSFEROPT% %SOURCEFILE% %DRIVENO% 1>&2
    echo Importing... 1>&2
	d64copy -v -w %TRANSFEROPT% %DRIVENO% %DESTFILE% 1>&2
    echo Comparing... 1>&2
	fc /b %SOURCEFILE% %DESTFILE% | find /v "000" 1>&2
	)

:EXIT
