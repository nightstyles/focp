echo off
call ..\..\..\mingw_env.bat
make release
if errorlevel 1 pause
echo on
