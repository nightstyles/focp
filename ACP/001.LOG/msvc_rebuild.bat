echo off
call ..\..\msvc_env.bat
make rebuild
if errorlevel 1 pause
echo on