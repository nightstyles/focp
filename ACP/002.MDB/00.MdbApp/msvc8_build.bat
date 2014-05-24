echo off
call ..\..\..\msvc8_env.bat
make 
if errorlevel 1 pause
echo on
