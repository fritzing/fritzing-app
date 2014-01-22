echo off

echo you must start this script from the Visual Studio Command Line Window
echo find this under the start menu at:
echo     All Programs / Microsoft Visual Studio 2012 / Visual Studio Tools / Developer Command Prompt 
echo.
echo the script also assumes you have cloned the Fritzing git repository and are launching this script from within that repository
echo.

IF .%1 == . (
	echo release version parameter is missing, should be something like 0.8.6b
	EXIT /B
)

rem set the path to the qt sdk bin folder
set QTBIN=C:\Qt\qt-everywhere-opensource-src-4.8.5\bin

set QMAKE=%QTBIN%\qmake.exe

if not exist %QMAKE% echo qmake not found--please change the path to Qt\bin
if not exist %QMAKE% EXIT /B n

echo found qmake.exe
echo.

cd /d %~dp0
cd ..

rem set environment variable for qmake phoenix.pro
set RELEASE_SCRIPT="release_script"	

%QMAKE% -o Makefile phoenix.pro
echo building fritzing
nmake release

set RELEASE_NAME=.\release\forzip\fritzing.%1.pc

rem set up deployment folder
rmdir .\release\deploy /s /q
rmdir .\release\forzip /s /q
del %RELEASE_NAME%.zip
mkdir .\release\deploy
mkdir .\release\forzip
mkdir .\release\deploy\lib
mkdir  .\release\deploy\lib\imageformats
mkdir  .\release\deploy\lib\sqldrivers

echo copy qt libraries
copy %QTBIN%\QtCore4.dll .\release\deploy\QtCore4.dll
copy %QTBIN%\QtGui4.dll .\release\deploy\QtGui4.dll
copy %QTBIN%\QtNetwork4.dll .\release\deploy\QtNetwork4.dll
copy %QTBIN%\QtSql4.dll .\release\deploy\QtSql4.dll
copy %QTBIN%\QtSvg4.dll .\release\deploy\QtSvg4.dll
copy %QTBIN%\QtXml4.dll .\release\deploy\QtXml4.dll
copy %QTBIN%\..\plugins\imageformats\qjpeg4.dll .\release\deploy\lib\imageformats\qjpeg4.dll
copy %QTBIN%\..\plugins\sqldrivers\qsqlite4.dll .\release\deploy\lib\sqldrivers\qsqlite4.dll

echo copying bins, parts, sketches, translations, pdb, help
echo.

copy  .\release\Fritzing.exe .\release\deploy\Fritzing.exe

xcopy /q .\translations .\release\deploy\translations /E  /I

xcopy /q .\bins .\release\deploy\bins /E  /I

xcopy /q .\sketches .\release\deploy\sketches /E  /I

xcopy /q .\parts .\release\deploy\parts /E  /I

xcopy /q .\pdb .\release\deploy\pdb /E  /I
xcopy /q .\help .\release\deploy\help /E  /I
copy .\README.txt .\release\deploy\README.txt
copy .\LICENSE.GPL2 .\release\deploy\LICENSE.GPL2
copy .\LICENSE.GPL3 .\release\deploy\LICENSE.GPL3
copy .\LICENSE.CC-BY-SA .\release\deploy\LICENSE.CC-BY-SA

echo removing empty translation files
echo.

del .\release\deploy\translations\*.ts
cd release
cd deploy
del/s placeholder.txt
cd translations
for /f "usebackq delims=;" %%A in (`dir /b *.qm`) do If %%~zA LSS 1024 del "%%A"
cd ..
cd ..
cd ..

rem make the executable compatible with windows xp
"%VCINSTALLDIR%bin\editbin.exe" .\release\deploy\Fritzing.exe /SUBSYSTEM:WINDOWS,5.01 /OSVERSION:5.1

echo copying vc redist files
copy  "%VCINSTALLDIR%redist\x86\Microsoft.VC110.CRT\msvcp110.dll" .\release\deploy\msvcp110.dll
copy  "%VCINSTALLDIR%redist\x86\Microsoft.VC110.CRT\msvcr110.dll" .\release\deploy\msvcr110.dll

move .\release\deploy %RELEASE_NAME%

echo create zip file
FOR /F %%i IN (".\release\forzip") DO SET SRC=%%~fi
FOR /F %%i IN (".\release\fritzing.%1.pc.zip") DO SET DEST=%%~fi
CScript .\tools\zip.vbs %SRC% %DEST%