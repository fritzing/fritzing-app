cd /d %~dp0
cd ..
rem C:\Qt\qt-everywhere-opensource-src-4.8.5\bin\qmake.exe -t vcapp -spec win32-msvc2012 phoenix.pro 
C:\Qt\qt-everywhere-opensource-src-5.2.1-32\qtbase\bin\qmake.exe -t vcapp -spec win32-msvc2012 phoenix.pro -o Fritzing32.vcxproj
C:\Qt\qt-everywhere-opensource-src-5.2.1-64\qtbase\bin\qmake.exe "QMAKE_TARGET.arch=x86_64" -t vcapp -spec win32-msvc2012 phoenix.pro -o Fritzing64.vcxproj 