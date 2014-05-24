echo off
call ..\mingw_env.bat
make rebuild copy
if errorlevel 1 pause
echo on