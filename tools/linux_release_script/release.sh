#!/bin/bash
set -euE -o functrace
failure() {
  local lineno=$1
  local msg=$2
  echo "Failed at $lineno: $msg"
  exit
}
trap 'failure ${LINENO} "$BASH_COMMAND"' ERR

#TODO Add debug-parameter to temporarily switch this of.
if [[ -n $(git status -s) ]]; then
  echo "Build directory is not clean. Check git status."
  git status -s
  exit -1
fi

#TODO Add debug-parameter to temporarily switch this of.
if [[ -n $(git clean -xdn) ]]; then
  echo "Build directory is not clean. Check git clean -xdn."
  git clean -xfn
  exit -1
fi

arch_aux=`uname -m`

echo
script_path="$(readlink -f "$0")"
script_folder=$(dirname ${script_path})
echo "{script_folder} ${script_folder}"

current_dir=$(pwd)

if [ "$arch_aux" == 'x86_64' ] ; then
  arch='AMD64'
else
  arch='i386'
fi

if [ -z "${1:-}"] ; then
  echo "Usage: $0 <need a version string such as '0.6.4b' (without the quotes)>"
  exit
else
  relname=$1  #`date +%Y.%m.%d`
fi

quazip='QUAZIP_LIB'
echo "using src/lib/quazip"


app_folder=$(dirname ${script_folder})
app_folder=$(dirname ${app_folder})
cd $app_folder
echo "appfolder ${app_folder}"

echo "Compiling."
qmake CONFIG+=release DEFINES+=$quazip
make -j16

release_name=fritzing-${relname}.linux.${arch}
release_folder="${current_dir}/${release_name}"

# Archive this for evaluation of crash reports
cp Fritzing Fritzing_${release_name}
# Outcomment this if you want a debug release
strip Fritzing

echo "making release folder: ${release_folder}"
mkdir ${release_folder}

echo "copying release files"
cp -rf sketches/ help/ translations/ Fritzing.sh Fritzing.1 fritzing.desktop fritzing.rc fritzing.appdata.xml install_fritzing.sh readme.md LICENSE.CC-BY-SA LICENSE.GPL2 LICENSE.GPL3 $release_folder/
mkdir ${release_folder}/icons
cp resources/system_icons/linux/* $release_folder/icons/
mv Fritzing ${release_folder}/
chmod +x ${release_folder}/install_fritzing.sh

cd ${release_folder}

echo "cleaning translations"
rm ./translations/*.ts  			# remove translation xml files, since we only need the binaries in the release
find ./translations -name "*.qm" -size -128c -delete   # delete empty translation binaries

git clone --branch master --single-branch https://github.com/fritzing/fritzing-parts.git

echo "making library folders"
mkdir lib
mkdir lib/imageformats
mkdir lib/sqldrivers
mkdir lib/platforms

cd lib
# echo "copying qt libraries"
# cp -d $QT_HOME/lib/libicudata.so* $QT_HOME/lib/libicui18n.so* $QT_HOME/lib/libicuuc.so.* $QT_HOME/lib/libQt5Concurrent.so* $QT_HOME/lib/libQt5Core.so* $QT_HOME/lib/libQt5DBus.so* $QT_HOME/lib/libQt5Gui.so* $QT_HOME/lib/libQt5Network.so* $QT_HOME/lib/libQt5SerialPort.so* $QT_HOME/lib/libQt5PrintSupport.so* $QT_HOME/lib/libQt5Sql.so* $QT_HOME/lib/libQt5Svg.so* $QT_HOME/lib/libQt5Xml.so* $QT_HOME/lib/libQt5Widgets.so* $QT_HOME/lib/libQt5XmlPatterns.so* $QT_HOME/lib/libQt5XcbQpa.so* .

# echo "copying qt plugins"
# cp $QT_HOME/plugins/imageformats/libqjpeg.so imageformats
# cp $QT_HOME/plugins/sqldrivers/libqsqlite.so sqldrivers
# cp $QT_HOME/plugins/platforms/libqxcb.so platforms

mv ../Fritzing .  				# hide the executable in the lib folder
mv ../Fritzing.sh ../Fritzing   		# rename Fritzing.sh to Fritzing
chmod +x ../Fritzing

LD_LIBRARY_PATH=$(pwd)
export LD_LIBRARY_PATH
./Fritzing -db "${release_folder}/fritzing-parts/parts.db" -pp "${release_folder}/fritzing-parts" -f "${release_folder}"

cd ${current_dir}

echo "compressing...."
tar -cjf  ./${release_name}.tar.bz2 ${release_name}

echo "cleaning up"
rm -rf ${release_folder}

echo "done!"
