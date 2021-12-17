@echo off
cls

echo Checking without having installed the driver
instcbm --check

:a
echo.
echo Installing driver with --nocopy
instcbm --nocopy
echo.
echo Checking driver 1
instcbm --check
echo.
echo Checking driver 2
instcbm --check
echo.
echo Checking driver 3
instcbm --check
echo.
echo Removing driver
instcbm --remove
echo.
goto a
