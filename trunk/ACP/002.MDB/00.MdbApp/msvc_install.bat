echo off
call ..\..\..\msvc_env.bat
make install
if errorlevel 1 pause
echo on
