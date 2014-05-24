echo off
call ..\..\..\mingw_env.bat
make 
if errorlevel 1 pause
echo on
