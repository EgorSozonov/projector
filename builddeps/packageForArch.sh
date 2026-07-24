#! /usr/bin/bash
app=$1
vers=$2
tarballDir=$3

if [[ -z "$app" || -z "$vers" ]]; then
   echo "Must set application name and version! Example: VERSION=1.0.0 make package"
   exit 1
fi

#git checkout "$vers" || { echo "Cannot checkout version $vers!"; exit 1; }

dest=$tarballDir/$app

#program to replace "source" and "sha256sums" values in the PKGBUILD
read -r -d '' awkProgram <<EOF
{ 
gsub(/source=\([^)]+\)/, "source=(\"leRepo::git+file://$(pwd)#tag=$vers\")"); 
gsub(/pkgver=_/, "pkgver=$vers"); 
}1
EOF

awk -v RS='^$' -v ORS='' "$awkProgram" builddeps/PKGBUILD > $dest/PKGBUILD

(cd $dest && makepkg && echo "Version $vers Arch package built in dir $dest" \
   || echo "ERROR")
#   && /usr/bin/rm $dest/PKGBUILD \
