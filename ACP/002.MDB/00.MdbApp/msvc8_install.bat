echo off
call ..\..\..\msvc8_env.bat
make install
if errorlevel 1 pause
echo on
