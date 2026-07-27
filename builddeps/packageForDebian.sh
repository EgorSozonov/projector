#! /usr/bin/bash
app=$1
vers=$2

if [[ -z "$app" || -z "$vers" ]]; then
   echo "Must set application name and version! Example: VERSION=1.0.0 make package"
   exit 1
fi

origPath=$(pwd)
dest=$(realpath "$HOME/toys/$app")

temp=$(mktemp -d)
cleanup() {
   rm -rf "$temp"
}
mkdir -p $temp
#trap cleanup EXIT


#Make a dir with the source tarball and the debian/ subdir
cp -r $origPath/debian $temp
pref=${PREFIX:-"usr"}
sed -i "s|usr/|$app $pref/|" $temp/debian/install
cd $temp
dpkg-source -b "$origPath" 

artifact="${app}_${vers}_amd64"

#Actually build the artifact
BIN=. DH_OPTIONS="--destdir=$temp" \
dpkg-buildpackage -b -us -uc \
   --buildinfo-option=-O"$temp/$artifact.buildinfo"  --buildinfo-option=-u"$temp" \
   --changes-option=-u"$temp" --changes-option=-O"$temp/$artifact.changes" 

#Copy the artifact to end directory
mkdir -p $dest
cp $temp/$artifact.deb $dest/$artifact.deb

