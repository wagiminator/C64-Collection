@echo off
cls

echo Performing RESET
cbmctrl reset

:repeat
echo.
echo Performing STATUS 8
cbmctrl status 8
goto repeat
