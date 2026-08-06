#!/usr/bin/env bash

git submodule update --init

export BUILD_TYPE='Release'

cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_INSTALL_PREFIX=install \
  -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
  -DBUILD_TESTING=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DNFD_PORTAL=ON

cmake --build build --target install

cd build
ctest --output-on-failure
cd ..
