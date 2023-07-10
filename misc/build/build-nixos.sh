#!/usr/bin/env nix-shell 
#! nix-shell -i bash
#! nix-shell -p git cmake ninja pkg-config gcc13 opencv SDL2 gtk3 catch2_3 spdlog 

git submodule update --init

export BUILD_TYPE='Release'
export EXIV2_VERSION='v0.28.0'

git clone https://github.com/Exiv2/exiv2.git --depth 1 --branch $EXIV2_VERSION
cd exiv2
cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_INSTALL_PREFIX=install \
  `cat ../misc/build/exiv2-minimal-flags.txt`
cmake --build build --target install
cd ..

cmake -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_INSTALL_PREFIX=install \
  -DBUILD_TESTING=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -Dexiv2_DIR=exiv2/install/lib64/cmake/exiv2

cmake --build build --target install

cd build
ctest --output-on-failure
cd ..
