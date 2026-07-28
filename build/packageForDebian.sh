#! /usr/bin/bash
#0. Validations
app=$1
vers=$2
if [[ -z "$app" || -z "$vers" ]]; then
   echo "Must set application name and version! Example: VERSION=1.0.0 make package"
   exit 1
fi


#1. Path parameters
export APP=$1
export VERSION=$2
origPath=$(pwd)
if [[ -z "$DEST" ]]; then
   export DEST=$(realpath "../.b/$APP")
else
   export DEST=$(realpath $DEST)
fi
if [[ -z "$PREFIX" ]]; then
    export PREFIX="usr"
fi


#2. Temporary directory in memory
export TEMP=$(mktemp -d)
cleanup() {
   rm -rf "$TEMP"
}
mkdir -p $TEMP/a
trap cleanup EXIT


#3. Make a copy of the source at this particular version and move the debian/ subdir to top
git -c core.abbrev=no -C "$origPath" archive --format tar "$vers" \
   > $TEMP/${APP}_$VERSION.orig.tar.gz
cd $TEMP
tar -x -f $TEMP/${APP}_$VERSION.orig.tar.gz --directory a
cd a
mv build/debian .


#4. Actually build the artifact
artifact="${APP}_${VERSION}_amd64"
BIN="." DEB_BUILD_OPTIONS=noautodbgsym \
   dpkg-buildpackage -b -us -uc --buildinfo-option=-O


#5. Copy the artifact to destination directory
mkdir -p $DEST
cp $TEMP/$artifact.deb $DEST/$artifact.deb

