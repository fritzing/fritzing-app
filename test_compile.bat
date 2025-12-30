@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
cd /d c:\00_fritzing\fritzing-app
echo Testing compilation...
nmake -nologo -f Makefile.Release first 2>&1
echo Compilation test finished with errorlevel %ERRORLEVEL%

