@echo off
cd WINDOWS
call ddkbuild_start        chk -cefZ
call ddkbuild_start -amd64 chk -cefZ
call ddkbuild_start -ia64  chk -cefZ
cd ..\bin
move i386  i386checked
move ia64  ia64checked
move amd64 amd64checked
cd ..\windows
call ddkbuild_start        fre -cefZ
call ddkbuild_start -amd64 fre -cefZ
call ddkbuild_start -ia64  fre -cefZ
