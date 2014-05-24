echo off
call ..\msvc8_env.bat
make rebuild copy
if errorlevel 1 pause
echo on