@echo off
setlocal

REM default parameter value
set QTVERSION=6.2.2
set PROJECTROOT=../..
set MODE=shared
set QMAKEMODE=

REM check whether user had supplied -h or --help . If yes display usage 
for %%A in ("--help" "-h") do if "%1"==%%A (call:display_usage %1 & exit /b 0)

REM default win walues
if not "%1" == "" goto args_count_ok
:args_count_ko
call:display_usage
exit /b 1
:args_count_ok

set PROJECTNAME=%1
if NOT [%2]==[] set MODE=%2
if NOT [%3]==[] set PROJECTROOT=%3
if NOT [%4]==[] set QTVERSION=%4

set JOM_PATH=c:\Qt\Tools\QtCreator\bin\jom
set QMAKE_PATH=C:\Qt\%QTVERSION%\msvc2019_64\bin

if not %MODE%==shared (if not %MODE%==static (echo "mode must be either shared or static - %MODE% is an unsupported value" & exit /b 2) )
if %MODE%==static (
	set QMAKEMODE="CONFIG+=staticlib"
	if %MODE%==static (echo "%MODE% build enabled: building with "CONFIG+=staticlib" option")
)

if not exist %QMAKE_PATH% (echo "Qt path '%QMAKE_PATH%' doesn't exist : check your Qt installation and kits" & exit /b 2)

if not exist %PROJECTROOT% (echo "Project root path '%PROJECTROOT%' doesn't exist" & exit /b 2)

if not exist %PROJECTROOT%/%PROJECTNAME%.pro (
    echo "Project path '%PROJECTROOT%/%PROJECTNAME%.pro' doesn't exist"
    echo "Trying  %PROJECTROOT%/%PROJECTNAME%/%PROJECTNAME%.pro"
    if exist %PROJECTROOT%/%PROJECTNAME%/%PROJECTNAME%.pro (set PROJECTROOT=%PROJECTROOT%/%PROJECTNAME%) else echo ("Project path '%PROJECTROOT%/%PROJECTNAME%/%PROJECTNAME%.pro' doesn't exist" & exit /b /2)
)

REM check path is relative or absolute
if not %PROJECTROOT:~1,1%==: (set PROJECTROOT=../../../%PROJECTROOT%)
echo "Project root path used is : %PROJECTROOT%"
echo "Project path used is : %PROJECTROOT%/%PROJECTNAME%.pro"

REM call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\VC\Auxiliary\Build\vcvars64.bat"
REM call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
@REM setup Visual Studio environment
set output=setup_script.txt
call init_compiler_env_script.bat --year 2019 --output %output%
set /p setup_script=<"%output%"
call "%setup_script%"
del %output%

set BUILDROOTFOLDER=build-%PROJECTNAME%

if exist %BUILDROOTFOLDER%\%MODE% rmdir /S /Q %BUILDROOTFOLDER%\%MODE%
if not exist %BUILDROOTFOLDER%\%MODE%\debug mkdir %BUILDROOTFOLDER%\%MODE%\debug
if not exist %BUILDROOTFOLDER%\%MODE%\release mkdir %BUILDROOTFOLDER%\%MODE%\release

echo "===========> building %PROJECTNAME% static <==========="
pushd %BUILDROOTFOLDER%\%MODE%\debug
%QMAKE_PATH%\qmake.exe %PROJECTROOT%/%PROJECTNAME%.pro -spec win32-msvc CONFIG+=debug %QMAKEMODE%
if not %errorlevel%==0 ( exit /b %errorlevel% )
echo "====> qmake ok"
%JOM_PATH%\jom.exe
if not %errorlevel%==0 ( exit /b %errorlevel% )
echo "====> jom ok"
%JOM_PATH%\jom.exe create_setup
if not %errorlevel%==0 ( exit /b %errorlevel% )
echo "====> jom create_setup ok"
popd

popd
pushd %BUILDROOTFOLDER%\\%MODE%\release
%QMAKE_PATH%\qmake.exe %PROJECTROOT%/%PROJECTNAME%.pro -spec win32-msvc %QMAKEMODE%
if not %errorlevel%==0 ( exit /b %errorlevel% )
echo "====> qmake ok"
%JOM_PATH%\jom.exe
if not %errorlevel%==0 ( exit /b %errorlevel% )
echo "====> jom ok"
%JOM_PATH%\jom.exe create_setup
if not %errorlevel%==0 ( exit /b %errorlevel% )
echo "====> jom create_setup ok"
popd

endlocal
goto:eof

::--------------------------------------------------------
::-- Function display_usage starts below here
::--------------------------------------------------------

:display_usage

echo This script builds a remaken project.
echo It takes the project name as first argument and can receive threes optional arguments.
echo.
echo "Usage: [project name] [ build mode {shared|static} | default='%MODE%' ] [path to  project root - default='%PROJECTROOT%'] [Qt kit version to use - default='%QTVERSION%']"
exit /b 0

:end
