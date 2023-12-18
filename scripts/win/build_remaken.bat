@echo off
setlocal

REM default parameter value
set QTVERSION=6.2.2
set PROJECTROOT=../..
set QMAKEPATH=C:\Qt\%QTVERSION%\msvc2019_64\bin

REM check whether user had supplied -h or --help . If yes display usage 
for %%A in ("--help" "-h") do if "%1"==%%A (call:display_usage %1 & exit /b 0)

REM default win walues
if NOT [%1]==[] set PROJECTROOT=%1
if NOT [%2]==[] set QTVERSION=%2
set QMAKEPATH=C:\Qt\%QTVERSION%\msvc2019_64\bin
if NOT [%3]==[] set QMAKEPATH=%3

if not exist %PROJECTROOT% (echo "Remaken project root path '%PROJECTROOT%' doesn't exist" & exit /b 2)
echo "Remaken project root path used is : %PROJECTROOT%"
if not exist %QMAKEPATH% (echo "qmake.exe path 'QMAKEPATH' doesn't exist" & exit /b 2)
echo "qmake path used is : %QMAKEPATH%"

call %PROJECTROOT%/scripts/win/build_remaken_project.bat remaken shared %PROJECTROOT% %QTVERSION% %QMAKEPATH%

endlocal
goto:eof

::--------------------------------------------------------
::-- Function display_usage starts below here
::--------------------------------------------------------

:display_usage

echo This script builds Remaken in shared mode.
echo It can receive three optional argument. 
echo.
echo Usage: param [path to remaken project root - default='%PROJECTROOT%'] [Qt kit version to use - default='%QTVERSION%'] [path to qmake.exe - default='%QMAKEPATH%']
exit /b 0

:end
