@ECHO OFF
SET PION_LIBS=C:\Atomic Labs\pion-libraries
SETLOCAL EnableDelayedExpansion
SET PION_HOME=C:\Atomic Labs\pion-platform
SET INCLUDE=%PION_LIBS%\boost-1.37.0;%PION_LIBS%\bzip2-1.0.5\include;%PION_LIBS%\dssl-1.6.8\src;%PION_LIBS%\iconv-1.9.2\include;%PION_LIBS%\libxml2-2.6.30\include;%PION_LIBS%\log4cplus-1.0.3\include;%PION_LIBS%\openssl-0.9.8l\inc32;%PION_LIBS%\sqlapi-3.7.24;%PION_LIBS%\yajl-1.0.5\include;%PION_LIBS%\zlib-1.2.3\include;%PION_LIBS%\WpdPack-4.0.2\Include;%INCLUDE%
SET LIB=%PION_LIBS%\boost-1.37.0\lib;%PION_LIBS%\bzip2-1.0.5\lib;%PION_LIBS%\dssl-1.6.8\release;%PION_LIBS%\iconv-1.9.2\lib;%PION_LIBS%\libxml2-2.6.30\lib;%PION_LIBS%\log4cplus-1.0.3\bin;%PION_LIBS%\openssl-0.9.8l\bin;%PION_LIBS%\sqlapi-3.7.24\lib;%PION_LIBS%\yajl-1.0.5\bin;%PION_LIBS%\zlib-1.2.3\lib;%PION_LIBS%\WpdPack-4.0.2\Lib;%LIB%
SET PATH=%PION_LIBS%\boost-1.37.0\lib;%PION_LIBS%\bzip2-1.0.5\bin;%PION_LIBS%\iconv-1.9.2\bin;%PION_LIBS%\libxml2-2.6.30\bin;%PION_LIBS%\log4cplus-1.0.3\bin;%PION_LIBS%\openssl-0.9.8l\bin;%PION_LIBS%\sqlapi-3.7.24\bin;%PION_LIBS%\yajl-1.0.5\bin;%PION_LIBS%\zlib-1.2.3\bin;%PION_LIBS%\WpdPack-4.0.2\Bin;%PATH%
SET opts=/nologo /platform:Win32 /logcommands /nohtmllog /M1 /useenv
SET conf=Release_DLL_full
FOR %%I IN (%*) DO (SET opt=%%~I
  IF "!opt:~0,1!"=="/" SET opts=!opts! %%I
  IF "!opt!"=="debug" SET conf=Debug_DLL_full
  IF "!opt!"=="release" SET conf=Release_DLL_full)
FOR %%S in (*.sln) DO vcbuild %opts% "/logfile:%%~nS[%conf%].log" %%S "%conf%|Win32"
