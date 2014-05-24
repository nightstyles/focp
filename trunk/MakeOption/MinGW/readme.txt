修改系统变量 PATH,增加 %MINGW_PATH%\bin;%MINGW_PATH%\libexec\gcc\mingw32\3.4.5;

增加变量：

MINGW_PATH=D:\MinGW5.1.3

C_INCLUDE_PATH=%MINGW_PATH%\include;%MINGW_PATH%\lib\gcc\mingw32\3.4.5\include

CPLUS_INCLUDE_PATH=%MINGW_PATH%\include\c++\3.4.5;%MINGW_PATH%\include\c++\3.4.5\mingw32;%MINGW_PATH%\include\c++\3.4.5\backward;%C_INCLUDE_PATH%

LIBRARY_PATH=%MINGW_PATH%\lib;%MINGW_PATH%\lib\gcc\mingw32\3.4.5

将%MINGW_PATH%\bin下的mingw32-make.exe复制并改名为make.exe,因为CDT缺省的是使用make