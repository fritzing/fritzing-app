#!/bin/bash
set -euE -o functrace
failure() {
  local lineno=$1
  local msg=$2
  echo "Failed at $lineno: $msg"
  exit
}
trap 'failure ${LINENO} "$BASH_COMMAND"' ERR

if [ -z "${1:-}" ] ; then
  echo "Usage: $0 <need a version string such as '0.6.4b' (without the quotes)>"
  exit
else
  relname=$1  #`date +%Y.%m.%d`
fi

echo "Building release: ${relname}..."

if [[ ${relname} == *"debug"* ]]; then
  echo "Building a debug release"
  export QT_HASH_SEED=123
  export QT_DEBUG_PLUGINS=0  # usually to verbose
  export QML_IMPORT_TRACE=0
  target="debug"
else
  if [[ -n $(git status -s) ]]; then
    echo "Build directory is not clean. Check git status."
    git status -s
    exit -1
  fi

  if [[ -n $(git clean -xdn) ]]; then
    echo "Build directory is not clean. Check git clean -xdn."
    git clean -xfn
    exit -1
  fi
  target="release"
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

quazip='QUAZIP_LIB'
echo "using src/lib/quazip"


app_folder=$(dirname ${script_folder})
app_folder=$(dirname ${app_folder})
cd $app_folder
echo "appfolder ${app_folder}"

echo "Compiling."
qmake CONFIG+=${target} DEFINES+=$quazip
make -j16

release_name=fritzing-${relname}.linux.${arch}
release_folder="${current_dir}/${release_name}"

if [[ ${relname} != *"debug"* ]] ; then
  # Archive this for evaluation of crash reports
  cp Fritzing Fritzing_${release_name}
  strip Fritzing
fi

echo "making release folder: ${release_folder}"
mkdir -p ${release_folder}

echo "copying release files"
cp -rf sketches/ help/ translations/ Fritzing.sh Fritzing.1 fritzing.desktop fritzing.rc org.fritzing.Fritzing.appdata.xml install_fritzing.sh README.md LICENSE.CC-BY-SA LICENSE.GPL2 LICENSE.GPL3 $release_folder/
mkdir -p ${release_folder}/icons
cp resources/system_icons/linux/* $release_folder/icons/
mv Fritzing ${release_folder}/
chmod +x ${release_folder}/install_fritzing.sh

cd ${release_folder}

echo "cleaning translations"
rm ./translations/*.ts  			# remove translation xml files, since we only need the binaries in the release
find ./translations -name "*.qm" -size -128c -delete   # delete empty translation binaries

if [[ ${relname} == *"debug"* ]] ; then
  git clone --branch nightlyParts --single-branch https://github.com/fritzing/fritzing-parts.git || echo -e "\n   ####   \033[1;31m Ignoring git error for debug build!  \033[0m ####\n"
else
  git clone --branch master --single-branch https://github.com/fritzing/fritzing-parts.git
fi

echo "making library folders"

mkdir -p lib/imageformats
mkdir -p lib/sqldrivers
mkdir -p lib/platforms

mv Fritzing lib  				# hide the executable in the lib folder
mv Fritzing.sh Fritzing   		# rename Fritzing.sh to Fritzing
chmod +x Fritzing

./Fritzing -db "${release_folder}/fritzing-parts/parts.db" -pp "${release_folder}/fritzing-parts" -f "${release_folder}"

cd ${current_dir}

if [[ ${relname} != *"debug"* ]]; then
  echo "compressing...."
  tar -cjf  ./${release_name}.tar.bz2 ${release_name}

  echo "cleaning up"
  rm -rf ${release_folder}
fi

echo "done!"
