@echo off
cls
setlocal

for /f "delims=" %%D in ('dir /a:d /b build-*') do (
    echo "Removing %%~fD folder"
    rmdir /S /Q %%~FD
)

for /f "delims=" %%D in ('dir /a:d /b test-*') do (
    echo "Removing %%~fD folder"
    rmdir /S /Q %%~FD
)

rm out.txt

endlocal
goto:eof

:end


