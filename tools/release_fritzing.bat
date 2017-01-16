echo off

echo .
echo you must start this script from the Visual Studio Command Line Window
echo find this under the start menu at (depending on your version of Visual Studio):
echo     All Programs / Microsoft Visual Studio 2012 / Visual Studio Tools / Developer Command Prompt
echo for the 64-bit build, use the 64-bit prompt:
echo     All Programs / Microsoft Visual Studio 2012 / Visual Studio Tools / VS2012 x64 Cross Tools Command Prompt
echo.
echo the script also assumes you have cloned the Fritzing git repository and are launching this script from within that repository

echo.
echo for a full release, run the script twice, once for a 64-built build, once for a 32-bit build
echo you may need to change the script variable QTBIN to point to your Qt folder (once for 64-bit, once for 32-bit)
echo.
echo you may need to set the PATH in the script to your git for windows installation
echo.
echo you may need to change the script variable LIBGIT2 to find git2.dll
echo.


IF .%1 == . (
	echo first parameter--release version--is missing, should be something like 0.8.6b
	EXIT /B
)

IF .%2 == . (
	echo second parameter--target architecture--is missing, should be either "32" for a 32-bit build or "64" for a 64-bit build
	EXIT /B
)

IF .%3 == . (
	echo third parameter--visual studio version--is missing, should be "2012", "2013", "2015"
	EXIT /B
)

echo add the path for your git installation if it's not already there
set PATH=%PATH%;"C:\Program Files (x86)\Git\bin";

echo set the path to the qt sdk bin folder
IF %2==64 (
	IF %3==2012 (
    	set QTBIN=C:\Qt\5.6\msvc2012_64\bin
    ) ELSE IF %3==2013 (
    	set QTBIN=C:\Qt\5.6\msvc2013_64\bin
    ) ELSE IF %3==2015 (
    	set QTBIN=C:\Qt\5.6\msvc2015_64\bin
    )
	set arch=""QMAKE_TARGET.arch=x86_64""
) ELSE (
	IF %2==32 (
		IF %3==2012 (
	    	set QTBIN=C:\Qt\5.6\msvc2012\bin
	    ) ELSE IF %3==2013 (
	    	set QTBIN=C:\Qt\5.6\msvc2013\bin
	    ) ELSE IF %3==2015 (
	    	set QTBIN=C:\Qt\5.6\msvc2015\bin
	    )
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

set LIBGIT2=%~dp0..\..\libgit2\build%2

rem set environment variable for qmake phoenix.pro
set RELEASE_SCRIPT="release_script"


%QMAKE% -o Makefile phoenix.pro %arch%

echo building fritzing
nmake release

set DESTDIR=..\release%2
rem get the absolute path of DESTDIR
pushd %DESTDIR%
set DESTDIR=%CD%
popd

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
mkdir %DESTDIR%\deploy\lib\imageformats
mkdir %DESTDIR%\deploy\lib\sqldrivers
mkdir %DESTDIR%\deploy\lib\printsupport
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
copy %QTBIN%\Qt5SerialPort.dll %DESTDIR%\deploy\Qt5SerialPort.dll

xcopy /q %QTBIN%\icu*.dll %DESTDIR%\deploy /E  /I

copy %QTBIN%\..\plugins\imageformats\qjpeg.dll %DESTDIR%\deploy\lib\imageformats\qjpeg.dll
copy %QTBIN%\..\plugins\sqldrivers\qsqlite.dll %DESTDIR%\deploy\lib\sqldrivers\qsqlite.dll
copy %QTBIN%\..\plugins\platforms\qwindows.dll %DESTDIR%\deploy\platforms\qwindows.dll
copy %QTBIN%\..\plugins\printsupport\windowsprintersupport.dll  %DESTDIR%\deploy\lib\printsupport\windowsprintersupport.dll

echo copying git2.dll from %LIBGIT2%
copy %LIBGIT2%\git2.dll  %DESTDIR%\deploy\git2.dll

echo copying sketches, translations, help, README, LICENSE
echo.

copy  %DESTDIR%\Fritzing.exe %DESTDIR%\deploy\Fritzing.exe

xcopy /q .\translations %DESTDIR%\deploy\translations /E  /I

xcopy /q .\sketches %DESTDIR%\deploy\sketches /E  /I

xcopy /q .\help %DESTDIR%\deploy\help /E  /I

copy .\readme.md %DESTDIR%\deploy\readme.md
copy .\LICENSE.GPL2 %DESTDIR%\deploy\LICENSE.GPL2
copy .\LICENSE.GPL3 %DESTDIR%\deploy\LICENSE.GPL3
copy .\LICENSE.CC-BY-SA %DESTDIR%\deploy\LICENSE.CC-BY-SA

echo removing empty translation files
echo.
del %DESTDIR%\deploy\translations\*.ts

set CURRENTDIR=%cd%
cd %DESTDIR%
cd deploy

git clone --branch master --single-branch https://github.com/fritzing/fritzing-parts.git

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

IF %3==2012 (
    copy  "%VCINSTALLDIR%redist\%XFOLDER%\Microsoft.VC110.CRT\msvcp110.dll" %DESTDIR%\deploy\msvcp110.dll
    copy  "%VCINSTALLDIR%redist\%XFOLDER%\Microsoft.VC110.CRT\msvcr110.dll" %DESTDIR%\deploy\msvcr110.dll
) ELSE IF %3==2013 (
    copy  "%VCINSTALLDIR%redist\%XFOLDER%\Microsoft.VC120.CRT\msvcp120.dll" %DESTDIR%\deploy\msvcp120.dll
    copy  "%VCINSTALLDIR%redist\%XFOLDER%\Microsoft.VC120.CRT\msvcr120.dll" %DESTDIR%\deploy\msvcr120.dll
) ELSE IF %3==2015 (
    copy  "%VCINSTALLDIR%redist\%XFOLDER%\Microsoft.VC140.CRT\msvcp140.dll" %DESTDIR%\deploy\msvcp140.dll
	copy  "%VCINSTALLDIR%redist\%XFOLDER%\Microsoft.VC140.CRT\vcruntime140.dll" %DESTDIR%\deploy\vcruntime140.dll
)

echo run fritzing to create parts.db
%DESTDIR%\deploy\Fritzing.exe -pp %DESTDIR%\deploy\fritzing-parts -db %DESTDIR%\deploy\fritzing-parts\parts.db

echo moving deploy to %RELEASE_NAME%
move %DESTDIR%\deploy %RELEASE_NAME%

echo create zip file
FOR /F %%i IN ("%DESTDIR%\forzip") DO SET SRC=%%~fi
FOR /F %%i IN ("%DESTDIR%\fritzing.%1.%2.pc.zip") DO SET DEST=%%~fi
CScript .\tools\zip.vbs %SRC% %DEST%

echo done
