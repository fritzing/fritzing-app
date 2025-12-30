@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cd /d c:\00_fritzing\fritzing-app
echo ========================================
echo Testing full compilation (Release)
echo ========================================
nmake -f Makefile.Release clean 2>&1 | findstr /V "^[[:space:]]*$"
echo.
echo Starting compilation...
nmake -f Makefile.Release 2>&1
set COMPILE_ERRORLEVEL=%ERRORLEVEL%
echo.
echo ========================================
echo Compilation finished with errorlevel %COMPILE_ERRORLEVEL%
echo ========================================
if %COMPILE_ERRORLEVEL% NEQ 0 (
    echo COMPILATION FAILED
) else (
    echo COMPILATION SUCCEEDED
)

