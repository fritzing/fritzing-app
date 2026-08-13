#! /bin/sh

# Prerequisite: doxygen must be installed

# targeet folder relative to where this script is stored
REL_DOX_PATH="../../../fritzing-doxygen"
REL_SCRIPT_PATH="`dirname \"$0\"`"

TO_DOX="$REL_SCRIPT_PATH/$REL_DOX_PATH"
mkdir "$TO_DOX"
ABS_DOX="`( cd \"$TO_DOX\" && pwd )`"
if [ -z "$ABS_DOX" ] ; then
  printf "ERROR: The fritzing-doxygen path is not accessible\n"
  # to the script (e.g. permissions re-evaled after suid)
  exit 1  # fail
fi

cp "$REL_SCRIPT_PATH/Doxyfile" "$TO_DOX/"
cd "$TO_DOX"
printf "\nGenerating / Updating doxygen information in $ABS_DOX\n\n… please wait …\n"

doxygen

printf "Generation complete\n\nPlease see warning log file $ABS_DOX/Fritzing-warn.log\n"
printf "The generated html output is in folder $ABS_DOX/html\n\nUse\n\n"
printf "xdg-open \"$ABS_DOX/html/index.html\""
printf "\n\nto open it in your default browser\n"
