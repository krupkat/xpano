export BUILD_TYPE='Release'
export SDL_VERSION='release-2.28.5'
# export OPENCV_VERSION='4.8.1'
export CATCH_VERSION='v3.5.0'
export SPDLOG_VERSION='v1.12.0'
export EXIV2_VERSION='v0.28.1'
# export GENERATOR='Ninja Multi-Config'

git submodule update --init

# sudo apt-get update
# sudo apt-get install -y libopencv-dev libgtk-3-dev build-essential
# pip install cmake

git clone https://github.com/catchorg/Catch2.git catch --depth 1 --branch $CATCH_VERSION
cd catch
cmake -B build \
  -DCMAKE_INSTALL_PREFIX=install \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DBUILD_TESTING=OFF
cmake --build build --target install -j $(nproc)
cd ..

git clone https://github.com/Exiv2/exiv2.git --depth 1 --branch $EXIV2_VERSION
cd exiv2
cmake -B build \
  -DCMAKE_INSTALL_PREFIX=install \
  `cat ../misc/build/exiv2-minimal-flags.txt`
cmake --build build --target install -j $(nproc)
cd ..

git clone https://github.com/gabime/spdlog.git --depth 1 --branch $SPDLOG_VERSION
cd spdlog
cmake -B build \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_INSTALL_PREFIX=build/install
cmake --build build --target install -j $(nproc)
cd ..

git clone https://github.com/libsdl-org/SDL.git --depth 1 --branch $SDL_VERSION
cd SDL
cmake -B build \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_INSTALL_PREFIX=install
cmake --build build --target install -j $(nproc)
cd ..

cmake -B build \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_INSTALL_PREFIX=install \
  -DBUILD_TESTING=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCatch2_DIR=../catch/install/lib/cmake/Catch2 \
  -Dexiv2_ROOT=exiv2/install \
  -Dspdlog_DIR=`pwd`/spdlog/build/install/lib/cmake/spdlog \
  -DSDL2_DIR=SDL/install/lib/cmake/SDL2

cmake --build build -j $(nproc) --target install
cd build
ctest --output-on-failure
cd ..
