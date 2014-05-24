echo off
call ..\msvc_env.bat
make rebuild copy
if errorlevel 1 pause
echo on