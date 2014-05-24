set FOCP_VOB_DIR=%~dp0%
set FOCP_VOB_DIR=%FOCP_VOB_DIR:~0,-1%
set PATH=%FOCP_VOB_DIR%\MakeOption\cygwin\usr\local\bin;%FOCP_VOB_DIR%\MakeOption\cygwin\bin
for /f %%i in ('%FOCP_VOB_DIR%\MakeOption\cygwin\bin\cygpath -a -u %FOCP_VOB_DIR%') do set FOCP_VOB_DIR=%%i
set FOCP_APP_NAME=rdb
set HOME=/cygdrive/c