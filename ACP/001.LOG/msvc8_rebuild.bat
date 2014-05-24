echo off
call ..\..\msvc8_env.bat
make rebuild
if errorlevel 1 pause
echo on