echo off

echo you must start this script from the Visual Studio Command Line Window
echo find this under the start menu at:
echo     All Programs / Microsoft Visual Studio 2012 / Visual Studio Tools / Developer Command Prompt 
echo for the 64-bit build, use the 64-bit prompt:
echo     All Programs / Microsoft Visual Studio 2012 / Visual Studio Tools / VS2012 x64 Cross Tools Command Prompt
echo.
echo the script also assumes you have cloned the Fritzing git repository and are launching this script from within that repository
echo.
echo for a full release, run the script twice, once for a 64-built build, once for a 32-bit build
echo you may need to change QTBIN to point to your Qt folder (once for 64-bit, once for 32-bit)
echo.

IF .%1 == . (
	echo first parameter--release version--is missing, should be something like 0.8.6b
	EXIT /B
)

IF .%2 == . (
	echo second parameter--target architecture--is missing, should be either "32" for a 32-bit build or "64" for a 64-bit build
	EXIT /B
)

echo set the path to the qt sdk bin folder
IF %2==64 (
	set QTBIN=C:\Qt\Qt5.2.1\5.2.1\msvc2012_64\bin
	set arch=""QMAKE_TARGET.arch=x86_64""
) ELSE (
	IF %2==32 (
		set QTBIN=C:\Qt\Qt5.2.1-32bit\5.2.1\msvc2012\bin
		set arch=.
	) ELSE (
		echo second parameter--target architecture--should be either "32" for a 32-bit build or "64" for a 64-bit build
		EXIT /B	
	)
)

set QMAKE=%QTBIN%\qmake.exe

if not exist %QMAKE% echo '%QMAKE%' not found--please change the path to Qt\bin
if not exist %QMAKE% EXIT /B n

echo found qmake.exe
echo.

cd /d %~dp0
cd ..

rem set environment variable for qmake phoenix.pro
set RELEASE_SCRIPT="release_script"	


%QMAKE% -o Makefile phoenix.pro %arch%
echo building fritzing
nmake release

set DESTDIR=..\..\release%2
set RELEASE_NAME=%DESTDIR%\forzip\fritzing.%1.%2.pc

echo setting up deploy folder. ignore any "The system cannot find ..." messages
rmdir %DESTDIR%\deploy /s /q
rmdir %DESTDIR%\forzip /s /q
if exist %RELEASE_NAME%.zip (
	del %RELEASE_NAME%.zip
)
mkdir %DESTDIR%\deploy
mkdir %DESTDIR%\forzip
mkdir %DESTDIR%\deploy\platforms
mkdir %DESTDIR%\deploy\lib
mkdir  %DESTDIR%\deploy\lib\imageformats
mkdir  %DESTDIR%\deploy\lib\sqldrivers
mkdir  %DESTDIR%\deploy\lib\printsupport
echo deploy folder ready.  any further "The system cannot find ..." messages represent significant problems with the script
echo.

echo copy qt libraries
copy %QTBIN%\libEGL.dll %DESTDIR%\deploy\libEGL.dll
copy %QTBIN%\libGLESv2.dll %DESTDIR%\deploy\libGLESv2.dll
copy %QTBIN%\Qt5Core.dll %DESTDIR%\deploy\Qt5Core.dll
copy %QTBIN%\Qt5Gui.dll %DESTDIR%\deploy\Qt5Gui.dll
copy %QTBIN%\Qt5Network.dll %DESTDIR%\deploy\Qt5Network.dll
copy %QTBIN%\Qt5PrintSupport.dll %DESTDIR%\deploy\Qt5PrintSupport.dll
copy %QTBIN%\Qt5Sql.dll %DESTDIR%\deploy\Qt5Sql.dll
copy %QTBIN%\Qt5Svg.dll %DESTDIR%\deploy\Qt5Svg.dll
copy %QTBIN%\Qt5Widgets.dll %DESTDIR%\deploy\Qt5Widgets.dll
copy %QTBIN%\Qt5Xml.dll %DESTDIR%\deploy\Qt5Xml.dll

copy %QTBIN%\icudt51.dll %DESTDIR%\deploy\icudt51.dll
copy %QTBIN%\icuin51.dll %DESTDIR%\deploy\icuin51.dll
copy %QTBIN%\icuuc51.dll %DESTDIR%\deploy\icuuc51.dll

copy %QTBIN%\..\plugins\imageformats\qjpeg.dll %DESTDIR%\deploy\lib\imageformats\qjpeg.dll
copy %QTBIN%\..\plugins\sqldrivers\qsqlite.dll %DESTDIR%\deploy\lib\sqldrivers\qsqlite.dll
copy %QTBIN%\..\plugins\platforms\qwindows.dll %DESTDIR%\deploy\platforms\qwindows.dll
copy %QTBIN%\..\plugins\printsupport\windowsprintersupport.dll  %DESTDIR%\deploy\lib\windowsprintersupport.dll 


echo copying bins, parts, sketches, translations, pdb, help
echo.

copy  %DESTDIR%\Fritzing.exe %DESTDIR%\deploy\Fritzing.exe

xcopy /q .\translations %DESTDIR%\deploy\translations /E  /I

xcopy /q .\bins %DESTDIR%\deploy\bins /E  /I

xcopy /q .\sketches %DESTDIR%\deploy\sketches /E  /I

xcopy /q .\parts %DESTDIR%\deploy\parts /E  /I

xcopy /q .\pdb %DESTDIR%\deploy\pdb /E  /I
xcopy /q .\help %DESTDIR%\deploy\help /E  /I
copy .\README.txt %DESTDIR%\deploy\README.txt
copy .\LICENSE.GPL2 %DESTDIR%\deploy\LICENSE.GPL2
copy .\LICENSE.GPL3 %DESTDIR%\deploy\LICENSE.GPL3
copy .\LICENSE.CC-BY-SA %DESTDIR%\deploy\LICENSE.CC-BY-SA

echo removing empty translation files
echo.

del %DESTDIR%\deploy\translations\*.ts

set CURRENTDIR=%cd%
cd %DESTDIR%
cd deploy
del/s placeholder.txt
cd translations
for /f "usebackq delims=;" %%A in (`dir /b *.qm`) do If %%~zA LSS 1024 del "%%A"
cd %CURRENTDIR%

IF %2==32 (
	echo make the executable compatible with windows xp
	"%VCINSTALLDIR%bin\editbin.exe" %DESTDIR%\deploy\Fritzing.exe /SUBSYSTEM:WINDOWS,5.01 /OSVERSION:5.1
)

echo copying vc redist files
IF %2==32 (
	set XFOLDER=x86
) ELSE (
	set XFOLDER=x64
)

copy  "%VCINSTALLDIR%redist\%XFOLDER%\Microsoft.VC110.CRT\msvcp110.dll" %DESTDIR%\deploy\msvcp110.dll
copy  "%VCINSTALLDIR%redist\%XFOLDER%\Microsoft.VC110.CRT\msvcr110.dll" %DESTDIR%\deploy\msvcr110.dll

move %DESTDIR%\deploy %RELEASE_NAME%

echo create zip file
FOR /F %%i IN ("%DESTDIR%\forzip") DO SET SRC=%%~fi
FOR /F %%i IN ("%DESTDIR%\fritzing.%1.%2.pc.zip") DO SET DEST=%%~fi
CScript .\tools\zip.vbs %SRC% %DEST%