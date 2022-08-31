#!/usr/bin/bash

if [ -z $1 ]; then
    echo "You must specify a version."
    exit 1
fi

PACKAGE="xpano-$1"

git submodule update --init

# archive xpano
git archive --prefix "${PACKAGE}/" -o "${PACKAGE}.tar" HEAD

# add submodules
git submodule foreach --recursive \
  "git archive --prefix=${PACKAGE}/\$path/ --output=\$sha1.tar HEAD && 
   tar --concatenate --file=$(pwd)/${PACKAGE}.tar \$sha1.tar && 
   rm \$sha1.tar"

mkdir packages
mv "${PACKAGE}.tar" packages && cd packages
gzip "${PACKAGE}.tar"
tar xf "${PACKAGE}.tar.gz"
cd "${PACKAGE}"

debmake
debuild
