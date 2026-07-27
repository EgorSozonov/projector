#! /usr/bin/bash
app=$1
vers=$2

if [[ -z "$app" || -z "$vers" ]]; then
   echo "Must set application name and version! Example: VERSION=1.0.0 make package"
   exit 1
fi

export APP=$1
export VERSION=$2
origPath=$(pwd)
export DEST="$HOME/toys/$APP"

export TEMP=$(mktemp -d)
cleanup() {
   rm -rf "$TEMP"
}
mkdir -p $TEMP
trap cleanup EXIT


#Make a dir with the source tarball and the debian/ subdir
cp -r $origPath/debian $TEMP
if [[ -z "$PREFIX" ]]; then
    export PREFIX="usr"
fi
echo $PREFIX
export DESTDIR="$TEMP/debian/$app"

echo $TEMP/debian/rules


#Actually build the artifact
artifact="${APP}_${VERSION}_amd64"
BIN="$TEMP" DH_OPTIONS="--destdir=$DESTDIR" \
dpkg-buildpackage -b -us -uc \
   --buildinfo-file="$TEMP/$artifact.buildinfo"  --buildinfo-option=-u"$TEMP" \
   --changes-option=-u"$TEMP" --changes-file="$TEMP/$artifact.changes" 
   
#Actually build the artifact
#BIN=. DH_OPTIONS="--destdir=$TEMP" \
#dpkg-buildpackage -b -us -uc \
#   --buildinfo-file="$TEMP/$artifact.buildinfo"  --buildinfo-option=-u"$TEMP" \
#   --changes-option=-u"$TEMP" --changes-file="$TEMP/$artifact.changes" 
   

#Copy the artifact to end directory
mkdir -p $DEST
cp $TEMP/$artifact.deb $DEST/$artifact.deb

