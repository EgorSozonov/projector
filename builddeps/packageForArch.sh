#! /usr/bin/bash
app=$1
vers=$2
tarballDir=$3

if [[ -z "$app" || -z "$vers" ]]; then
   echo "Must set application name and version! Example: VERSION=1.0.0 make package"
   exit 1
fi

original="$(pwd)"
dest="$tarballDir/${app}-$vers"

rm -rf $dest
mkdir -p $dest

theTarball=$dest/tarball.tar
git -c core.abbrev=no -C "$pwd" archive --format tar "$vers" > $theTarball

checksum=$(sha256sum $theTarball | awk '{print $1}')

#AWK programs to replace checksum etc in the PKGBUILD
read -r -d '' localSourceSubst <<EOF
{ 
gsub(/source=\([^)]+\)/, "source=(tarball.tar)"); 
gsub(/pkgver=_/, "pkgver=$vers"); 
gsub(/sha256sums=\([^)]+\)/, "sha256sums=('$checksum')"); 
}1
EOF

read -r -d '' globalSourceSubst <<EOF
{ 
gsub(/pkgver=_/, "pkgver=$vers"); 
gsub(/sha256sums=\([^)]+\)/, "sha256sums=('$checksum')"); 
}1
EOF

awk -v RS='^$' -v ORS='' "$localSourceSubst" builddeps/PKGBUILD > $dest/PKGBUILD

(cd $dest && makepkg \
   && /usr/bin/rm tarball.tar \
   && awk -v RS='^$' -v ORS='' "$globalSourceSubst" $original/builddeps/PKGBUILD > $(pwd)/PKGBUILD \
   && echo "Version $vers Arch package built in $(pwd)" \
   || echo "ERROR")
