@echo off
set USETASM=1

if "%USETASM%"=="1" GOTO USE_TASM

masm -Mx -t -W1 -Ic:\winddk\3790\inc\ddk\w2k sample.asm;
link sample;
exe2bin sample.exe sample.com
goto end

:USE_TASM
TASM  sample.asm
TLINK /t sample.obj

:end
