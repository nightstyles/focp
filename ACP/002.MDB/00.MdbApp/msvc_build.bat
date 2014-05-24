echo off
call ..\..\..\msvc_env.bat
make 
if errorlevel 1 pause
echo on
