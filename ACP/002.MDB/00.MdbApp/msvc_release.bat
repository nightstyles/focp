echo off
call ..\..\..\msvc_env.bat
make release
if errorlevel 1 pause
echo on
