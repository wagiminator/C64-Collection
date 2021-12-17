echo off
:a
if exist test2.bin del test2.bin
cbmctrl download 8 0xc000 0x0001 test2.bin
set RETCODE=%ERRORLEVEL%
if exist test2.bin (
	if not "%RETCODE%" == "0" (
		echo ERROR-Condition: Unsuccessful command created a file of size:
		dir | find "test2.bin"
		) else (
		echo single MP test was completely successful
		)
	) else (
	if "%RETCODE%" == "0" (
		echo ERROR-Condition: Successful command did not create a file
		) else (
		echo unsuccessful single MP test did not create a file, which is correct
		)
	)
goto :a
