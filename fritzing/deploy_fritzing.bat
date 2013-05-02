cd /d %~dp0

set svndir="C:\Program Files\SlikSvn\bin\"
%svndir%svn export https://fritzing.googlecode.com/svn/trunk/fritzing/translations ./release/translations

%svndir%svn export https://fritzing.googlecode.com/svn/trunk/fritzing/bins ./release/bins

%svndir%svn export https://fritzing.googlecode.com/svn/trunk/fritzing/sketches ./release/sketches

%svndir%svn export https://fritzing.googlecode.com/svn/trunk/fritzing/parts ./release/parts

%svndir%svn export https://fritzing.googlecode.com/svn/trunk/fritzing/pdb ./release/pdb
%svndir%svn export https://fritzing.googlecode.com/svn/trunk/fritzing/help ./release/help
%svndir%svn export https://fritzing.googlecode.com/svn/trunk/fritzing/README.txt ./release/README.txt
%svndir%svn export https://fritzing.googlecode.com/svn/trunk/fritzing/LICENSE.GPL2 ./release/LICENSE.GPL2
%svndir%svn export https://fritzing.googlecode.com/svn/trunk/fritzing/LICENSE.GPL3 ./release/LICENSE.GPL3
%svndir%svn export https://fritzing.googlecode.com/svn/trunk/fritzing/LICENSE.CC-BY-SA ./release/LICENSE.CC-BY-SA



del .\release\translations\*.ts
cd release
cd translations
for /f "usebackq delims=;" %%A in (`dir /b *.qm`) do If %%~zA LSS 1024 del "%%A"
cd ..
cd ..

del .\release\bins\more\sparkfun-*.fzb
del .\release\pdb\user\*.fzp
del .\release\parts\svg\user\breadboard\*.svg
del .\release\parts\svg\user\icon\*.svg
del .\release\parts\svg\user\pcb\*.svg
del .\release\parts\svg\user\schematic\*.svg
del ".\release\parts\svg\user\new schematic\*.svg"
rmdir ".\release\parts\svg\user\new schematic"
