@echo off
cls

echo Stopping opencbm (if necessary)
net stop opencbm

echo.
echo Removing driver (if necessary)
if exist instcbm.exe instcbm --remove

echo.
echo Copying files from VMWXP
copy \\vmwxp\opencbm\*.sys
copy \\vmwxp\opencbm\*.dll
copy \\vmwxp\opencbm\*.exe
copy \\vmwxp\opencbm\*.bat
copy \\vmwxp\opencbm\*.sh

if exist setdate.bat del setdate.bat

echo.
echo DONE
