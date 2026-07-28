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
export DEST="../.b/$APP"


export TEMP=$(mktemp -d)
cleanup() {
   rm -rf "$TEMP"
}
mkdir -p $TEMP
#trap cleanup EXIT


git -c core.abbrev=no -C "$origPath" archive --format tar "$vers" > $TEMP/${APP}_$VERSION.orig.tar.gz
cd $TEMP
tar -x -f $TEMP/${APP}_$VERSION.orig.tar.gz



#Make a dir with the source tarball and the debian/ subdir
#cp -r $origPath/debian $TEMP
#if [[ -z "$PREFIX" ]]; then
#    export PREFIX="usr"
#fi
#export DESTDIR="$TEMP/debian/$app"


#Actually build the artifact
#artifact="${APP}_${VERSION}_amd64"
#BIN="$TEMP" DH_OPTIONS="--destdir=$DESTDIR" \
#dpkg-buildpackage -b -us -uc \
#   --buildinfo-file="$TEMP/$artifact.buildinfo"  --buildinfo-option=-u"$TEMP" \
#   --changes-option=-u"$TEMP" --changes-file="$TEMP/$artifact.changes" 
#   
#
##Copy the artifact to destination directory
#mkdir -p $DEST
#cp $TEMP/$artifact.deb $DEST/$artifact.deb

