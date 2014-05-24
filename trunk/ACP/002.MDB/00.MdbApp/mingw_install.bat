echo off
call ..\..\..\mingw_env.bat
make install
if errorlevel 1 pause
echo on
