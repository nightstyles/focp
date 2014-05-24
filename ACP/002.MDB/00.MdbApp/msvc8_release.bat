echo off
call ..\..\..\msvc8_env.bat
make release
if errorlevel 1 pause
echo on
