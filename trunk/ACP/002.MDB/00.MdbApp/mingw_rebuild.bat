echo off
call ..\..\..\mingw_env.bat
make rebuild
if errorlevel 1 pause
echo on
