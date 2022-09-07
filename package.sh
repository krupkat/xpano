#!/usr/bin/bash

if [ -z $1 ]; then
    echo "You must specify a version."
    exit 1
fi

if [ -z $2 ]; then
    echo "You must specify a distribution."
    exit 1
fi

VERSION=$1
PACKAGE="xpano-$VERSION"
DISTRIBUTION=$2

# create and unpack tar.gz in packages/distribution directory
cd packages
mkdir -p ${DISTRIBUTION} && cd ${DISTRIBUTION}
cp "../${PACKAGE}.tar.gz" .
tar xf "${PACKAGE}.tar.gz"
cd "${PACKAGE}"

# prepare debian directory
cp -r ../../../debian .
jinja -D "dist" $DISTRIBUTION debian/changelog.in -o debian/changelog
rm debian/changelog.in

# build source package
debmake
debuild -S
