@echo off
cls

set OC_DESTINATION=%ProgramFiles%\opencbm

set OC_VARIANT_DISPLAY_DEFAULT=ZoomFloppy
set OC_VARIANT_DEFAULT=xum1541
set OC_VARIANT_DEFAULT_INSTALL_DRIVER=OC_INSTALL_DRIVER_ZOOMFLOPPY

rem ---------------------------------------

setlocal enabledelayedexpansion

@cd /d "%~dp0"

set OC_VERSION=0.4.99.99
set OC_INSTALLED_SIZE_IN_KB=1125

set OC_VARIANT_DISPLAY=
set OC_VARIANT=

if [%PROCESSOR_ARCHITECTURE%] == [AMD64] (
	set OC_BINDIR_LOCAL=amd64
) else if [%PROCESSOR_ARCHITECTURE%] == [x86] (
	set OC_BINDIR_LOCAL=i386
) else (
	echo unknown architecture, exiting...
	exit
)

set OC_INSTALL_DRIVER_ZOOMFLOPPY=0
set OC_INSTALL_DRIVER_XUM1541=0
set OC_INSTALL_DRIVER_XU1541=0
 
rem Process command line parameter

for /d %%p in (%*) do (
	if [%%~p] == [zoomfloppy] (
		set OC_VARIANT_DISPLAY=!OC_VARIANT_DISPLAY! ZoomFloppy
		set OC_VARIANT=!OC_VARIANT! xum1541
		set OC_INSTALL_DRIVER_ZOOMFLOPPY=1
	) else if [%%~p] == [xum1541] (
		set OC_VARIANT_DISPLAY=!OC_VARIANT_DISPLAY! xum1541
		set OC_VARIANT=!OC_VARIANT! xum1541
		set OC_INSTALL_DRIVER_XUM1541=1
	) else if [%%~p] == [xu1541] (
		set OC_VARIANT_DISPLAY=!OC_VARIANT_DISPLAY! xu1541
		set OC_VARIANT=!OC_VARIANT! xu1541
		set OC_INSTALL_DRIVER_XU1541=1
	) else if [%%~p] == [xa1541] (
		set OC_VARIANT_DISPLAY=!OC_VARIANT_DISPLAY! xa1541
		set OC_VARIANT=!OC_VARIANT! xa1541
	) else (
		echo Unknown parameter %%~p, ignoring ...
		pause
	)
)

rem If no variant was given, use default

if "%OC_VARIANT_DISPLAY%" == "" (
	set OC_VARIANT_DISPLAY=%OC_VARIANT_DISPLAY_DEFAULT%
	set OC_VARIANT=%OC_VARIANT_DEFAULT%
	set %OC_VARIANT_DEFAULT_INSTALL_DRIVER%=1
)

rem Install by copying everything in place

rem The current version should not exhibit the opencbm.conf problem anymore.
rem (https://sourceforge.net/p/opencbm/bugs/60/)
rem
rem echo Copying opencbm.conf...
rem xcopy opencbm.conf %SystemRoot%\System32\ /i /q

xcopy %OC_BINDIR_LOCAL%\*.exe "%OC_DESTINATION%\"     /q /i
xcopy %OC_BINDIR_LOCAL%\*.dll "%OC_DESTINATION%\"     /q /i
xcopy                   *.pdf "%OC_DESTINATION%\doc\" /q /i

pushd "%OC_DESTINATION%"
instcbm.exe %OC_VARIANT%
popd

echo @echo off> "%OC_DESTINATION%\uninstall.cmd"
echo @cd /d "%OC_DESTINATION%">> "%OC_DESTINATION%\uninstall.cmd"
echo echo Removing %OC_VARIANT%>> "%OC_DESTINATION%\uninstall.cmd"
echo instcbm --remove %OC_VARIANT%>> "%OC_DESTINATION%\uninstall.cmd"
echo del "%SystemRoot%\System32\opencbm.conf">> "%OC_DESTINATION%\uninstall.cmd"
echo cd ..>> "%OC_DESTINATION%\uninstall.cmd"
echo %%SYSTEMROOT%%\System32\reg delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenCBM" /f>> "%OC_DESTINATION%\uninstall.cmd"
echo rd /s /q "%OC_DESTINATION%">> "%OC_DESTINATION%\uninstall.cmd"

set day=%date:~0,2%
set month=%date:~3,2%
set year=%date:~6%

%SYSTEMROOT%\System32\reg delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenCBM" /f > NUL 2> NUL

%SYSTEMROOT%\System32\reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenCBM" /t REG_SZ    /v "UninstallString" /d "%SYSTEMROOT%\System32\cmd.exe /c \"%OC_DESTINATION%\uninstall.cmd\"" > NUL
%SYSTEMROOT%\System32\reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenCBM" /t REG_SZ    /v "Comments"        /d "OpenCBM v%OC_VERSION% %OC_VARIANT_DISPLAY%" > NUL
%SYSTEMROOT%\System32\reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenCBM" /t REG_SZ    /v "DisplayName"     /d "OpenCBM v%OC_VERSION%" > NUL
%SYSTEMROOT%\System32\reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenCBM" /t REG_SZ    /v "DisplayVersion"  /d "%OC_VERSION%" > NUL
%SYSTEMROOT%\System32\reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenCBM" /t REG_SZ    /v "InstallDate"     /d "%year%%month%%day%" > NUL 
%SYSTEMROOT%\System32\reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenCBM" /t REG_SZ    /v "Publisher"       /d "The OpenCBM team" > NUL
%SYSTEMROOT%\System32\reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenCBM" /t REG_SZ    /v "HelpLink"        /d "http://opencbm.sourceforge.net/" > NUL
%SYSTEMROOT%\System32\reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenCBM" /t REG_DWORD /v "NoModify"        /d 1 > NUL
%SYSTEMROOT%\System32\reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenCBM" /t REG_DWORD /v "NoRepair"        /d 1 > NUL
%SYSTEMROOT%\System32\reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\OpenCBM" /t REG_DWORD /v "EstimatedSize"   /d %OC_INSTALLED_SIZE_IN_KB% > NUL


echo.
echo =================================================
echo === OpenCBM v%OC_VERSION% for %OC_VARIANT_DISPLAY%
echo === installation done!
echo ===
echo === Add %OC_DESTINATION% to your PATH to use the command line tools there.
echo =================================================
echo.

if not "%OC_INSTALL_DRIVER_ZOOMFLOPPY% %OC_INSTALL_DRIVER_XUM1541% %OC_INSTALL_DRIVER_XU1541%" == "0 0 0" (
	cd "%~dp0"

	If exist zadic.exe (

		echo.
		echo I could install the necessary USB drivers if you like.
		echo If you want this, then please
		echo 1. attach your device, and then
		echo 2. answer the following question with "y", without parenthesis:
		echo.
		set OC_INSTALL_USB=
		set /p OC_INSTALL_USB="Do you want me to install the necessary USB drivers for you? (y/n) "

		set OC_INSTALL_USB_RESULT=
		if [!OC_INSTALL_USB!] == [y] set OC_INSTALL_USB_RESULT=1
		if [!OC_INSTALL_USB!] == [Y] set OC_INSTALL_USB_RESULT=1

		if [!OC_INSTALL_USB_RESULT!] == [1] (
			echo You answered YES.
			echo.
			if [%OC_INSTALL_DRIVER_ZOOMFLOPPY%] == [1] (
				echo INSTALL ZoomFloppy driver
				zadic.exe --vid=0x16D0 --pid=0x0504 --usealldevices
			)

			if [%OC_INSTALL_DRIVER_XUM1541%] == [1] (
				echo INSTALL xum1541 driver
				zadic.exe --vid=0x16D0 --pid=0x0504 --usealldevices
			)

			if [%OC_INSTALL_DRIVER_XU1541%] == [1] (
				echo INSTALL xu1541 driver
				zadic.exe --vid=0x0403 --pid=0xC632 --usealldevices
			)
		)
		else (
			echo You answered NO.
		)
	)
)

echo That's it, I am finished.
pause
