@echo off
cls

echo Checking OPENCBM driver without having it installed
instcbm --check

echo.
echo Installing OPENCBM driver
instcbm --nocopy

echo.
echo Checking OPENCBM driver
instcbm --check

rem echo.
rem echo Starting OPENCBM driver
rem net start opencbm

echo.
echo Performing CBMCTRL RESET
cbmctrl reset

echo.
echo Stopping OPENCBM driver
net stop opencbm

echo.
echo Uninstalling OPENCBM driver
instcbm --remove
