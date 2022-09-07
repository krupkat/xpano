#!/usr/bin/bash

# use git archive to create a tar.gz with the project sources including submodules

if [ -z $1 ]; then
    echo "You must specify a version"
    exit 1
fi

VERSION=$1
PACKAGE="xpano-$VERSION"

git submodule update --init

# archive xpano
git archive --prefix "${PACKAGE}/" -o "${PACKAGE}.tar" HEAD

# add submodules
git submodule foreach --recursive \
  "git archive --prefix=${PACKAGE}/\$path/ --output=\$sha1.tar HEAD && 
   tar --concatenate --file=$(pwd)/${PACKAGE}.tar \$sha1.tar && 
   rm \$sha1.tar"

# create tar.gz in packages directory
mkdir -p packages
mv "${PACKAGE}.tar" packages && cd packages
gzip "${PACKAGE}.tar"
