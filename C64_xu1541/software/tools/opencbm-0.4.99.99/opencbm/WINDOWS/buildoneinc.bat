@echo off
rem start buildoneinc

rem set default local settings (not controlled by CVS)
if exist %1\..\DDKBUILD_LOCAL.BAT call %1\..\DDKBUILD_LOCAL.BAT
if exist %1\..\..\DDKBUILD_LOCAL.BAT call %1\..\..\DDKBUILD_LOCAL.BAT


if not defined BASH set BASH=bash

rem set CYGWIN=c:\cygwin
rem set CYGWINBIN=%CYGWIN%\BIN\
rem set BASH=%CYGWINBIN%bash

%BASH% %1/WINDOWS/buildoneinc %1/WINDOWS %2
