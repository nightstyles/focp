echo off
:LOOP
IF [%1]==[] GOTO END
make -C %1
if errorlevel 1 exit
SHIFT
GOTO LOOP
:END
