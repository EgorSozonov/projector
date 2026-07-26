#! /usr/bin/bash
app=$1
vers=$2
tarballDir=$HOME/toys/temp

#if [[ -z "$app" || -z "$vers" ]]; then
#   echo "Must set application name and version! Example: VERSION=1.0.0 make package"
#   exit 1
#fi

dest=$(realpath "$tarballDir/${app}-$vers")

rm -rf $dest
mkdir -p $dest


DH_OPTIONS="--destdir=$dest" dpkg-buildpackage -b -us -uc --buildinfo-option=-O"$dest/projector_1.0-1_amd64.buildinfo"  --buildinfo-option=-u"$dest" --changes-option=-u"$dest" --changes-option=-O"$dest/projector_1.0-1_amd64.changes" 

