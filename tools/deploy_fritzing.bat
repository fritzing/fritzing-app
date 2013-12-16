cd /d %~dp0
cd ..

rmdir .\release\deploy /s /q
mkdir .\release\deploy

xcopy .\translations .\release\deploy\translations /E  /I

xcopy .\bins .\release\deploy\bins /E  /I

xcopy .\sketches .\release\deploy\sketches /E  /I

xcopy .\parts .\release\deploy\parts /E  /I

xcopy .\pdb .\release\deploy\pdb /E  /I
xcopy .\help .\release\deploy\help /E  /I
copy .\README.txt .\release\deploy\README.txt
copy .\LICENSE.GPL2 .\release\deploy\LICENSE.GPL2
copy .\LICENSE.GPL3 .\release\deploy\LICENSE.GPL3
copy .\LICENSE.CC-BY-SA .\release\deploy\LICENSE.CC-BY-SA
copy .\release\fritzing.exe .\release\deploy\fritzing.exe
xcopy .\release\*.dll .\release\deploy


del .\release\deploy\translations\*.ts
cd release
cd deploy
del/s placeholder.txt
cd translations
for /f "usebackq delims=;" %%A in (`dir /b *.qm`) do If %%~zA LSS 1024 del "%%A"
cd ..
cd ..
cd ..


echo don't forget about the msvc files
echo don't forget about the lib folder
echo C:\Program Files (x86)\Microsoft Visual Studio 11.0>editbin.exe C:\Users\jonathan\fritzing\fritzing\fritzing\release\Fritzing.exe /SUBSYSTEM:WINDOWS,5.01 /OSVERSION:5.1
