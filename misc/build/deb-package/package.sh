#!/usr/bin/bash

if [ -z $1 ]; then
    echo "You must specify a version."
    exit 1
fi

if [ -z $2 ]; then
    echo "You must specify a distribution."
    exit 1
fi

TAG=$1
VERSION=${TAG:1}
PACKAGE="xpano-$VERSION"
DISTRIBUTION=$2

git submodule update --init

# archive xpano
git archive --prefix "${PACKAGE}/" -o "${PACKAGE}.tar" HEAD

# add submodules
git submodule foreach --recursive \
  "git archive --prefix=${PACKAGE}/\$path/ --output=\$sha1.tar HEAD && 
   tar --concatenate --file=$(pwd)/${PACKAGE}.tar \$sha1.tar && 
   rm \$sha1.tar"

# create and unpack tar.gz in packages directory
mkdir packages
mv "${PACKAGE}.tar" packages && cd packages
gzip "${PACKAGE}.tar"
tar xf "${PACKAGE}.tar.gz"
cd "${PACKAGE}"

# prepare debian directory
cp -r misc/build/deb-package/debian .
jinja -D "DISTRIBUTION" $DISTRIBUTION debian/changelog.in -o debian/changelog
rm debian/changelog.in

# build without signing
debmake
debuild -i -us -uc -b
