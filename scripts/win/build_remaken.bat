@echo off
setlocal

set PROJECTNAME=remaken

REM default parameter value
set QTVERSION=6.2.2
set PROJECTROOT=../..
set QMAKEPATH=C:\Qt\%QTVERSION%\msvc2019_64\bin
set JOMPATH=c:\Qt\Tools\QtCreator\bin\jom

REM check whether user had supplied -h or --help . If yes display usage 
for %%A in ("--help" "-h") do if "%1"==%%A (call:display_usage %1 & exit /b 0)

REM default win walues
if NOT [%1]==[] set PROJECTROOT=%1
if NOT [%2]==[] set QTVERSION=%2
if NOT [%3]==[] set QMAKEPATH=%3
if NOT [%4]==[] set JOMPATH=%4

if not exist %PROJECTROOT% (echo "\"%PROJECTNAME%\" project root path '%PROJECTROOT%' doesn't exist" & exit /b 2)
echo "%PROJECTNAME% project root path used is : %PROJECTROOT%"
if not exist %QMAKEPATH% (echo "qmake.exe path 'QMAKEPATH' doesn't exist" & exit /b 2)
echo "qmake path used is : %QMAKEPATH%"
if not exist %JOMPATH% (echo "jom.exe path 'JOMPATH' doesn't exist" & exit /b 2)
echo "jom path used is : %JOMPATH%"

call %PROJECTROOT%/scripts/win/build_remaken_project.bat %PROJECTNAME% shared %PROJECTROOT% %QTVERSION% %QMAKEPATH% %JOMPATH%

endlocal
goto:eof

::--------------------------------------------------------
::-- Function display_usage starts below here
::--------------------------------------------------------

:display_usage

echo This script builds \"%PROJECTNAME%\" in shared mode.
echo It can receive four optional argument. 
echo.
echo Usage: param [path to %PROJECTNAME% project root - default='%PROJECTROOT%'] [Qt kit version to use - default='%QTVERSION%'] [path to qmake.exe - default='%QMAKEPATH%'] [path to jom.exe='%JOMPATH%']
exit /b 0

:end
