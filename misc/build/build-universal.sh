export BUILD_TYPE='Release'
export SDL_VERSION='release-2.30.10'
export OPENCV_VERSION='4.10.0'
export CATCH_VERSION='v3.7.1'
export SPDLOG_VERSION='v1.15.0'
export EXIV2_VERSION='v0.28.3'
export GENERATOR='Ninja Multi-Config'
export C_COMPILER='gcc'
export CXX_COMPILER='g++'

git submodule update --init


git clone https://github.com/catchorg/Catch2.git catch --depth 1 --branch $CATCH_VERSION
cd catch
cmake -B build \
  -DCMAKE_C_COMPILER=$C_COMPILER \
  -DCMAKE_CXX_COMPILER=$CXX_COMPILER \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_INSTALL_PREFIX=install \
  -DBUILD_TESTING=OFF
cmake --build build --target install -j $(nproc)
cd ..


git clone https://github.com/opencv/opencv.git --depth 1 --branch $OPENCV_VERSION
cd opencv
cmake -B build \
  -DCMAKE_C_COMPILER=$C_COMPILER \
  -DCMAKE_CXX_COMPILER=$CXX_COMPILER \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_INSTALL_PREFIX=install \
  `cat ../misc/build/opencv-minimal-flags.txt`
cmake --build build --target install -j $(nproc)
cd ..


git clone https://github.com/libsdl-org/SDL.git --depth 1 --branch $SDL_VERSION
cd SDL
cmake -B build \
  -DCMAKE_C_COMPILER=$C_COMPILER \
  -DCMAKE_CXX_COMPILER=$CXX_COMPILER \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_INSTALL_PREFIX=install
cmake --build build --target install -j $(nproc)
cd ..


git clone https://github.com/gabime/spdlog.git --depth 1 --branch $SPDLOG_VERSION
cd spdlog
cmake -B build \
  -DCMAKE_C_COMPILER=$C_COMPILER \
  -DCMAKE_CXX_COMPILER=$CXX_COMPILER \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_INSTALL_PREFIX=build/install
cmake --build build --target install -j $(nproc)
cd ..


git clone https://github.com/Exiv2/exiv2.git --depth 1 --branch $EXIV2_VERSION
cd exiv2
cmake -B build \
  -DCMAKE_C_COMPILER=$C_COMPILER \
  -DCMAKE_CXX_COMPILER=$CXX_COMPILER \
  -DCMAKE_INSTALL_PREFIX=install \
  `cat ../misc/build/exiv2-minimal-flags.txt`
cmake --build build --target install -j $(nproc)
cd ..

cmake -B build \
  -DCMAKE_C_COMPILER=$C_COMPILER \
  -DCMAKE_CXX_COMPILER=$CXX_COMPILER \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_INSTALL_PREFIX=install \
  -DBUILD_TESTING=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCatch2_ROOT=`pwd`/catch/install \
  -DOpenCV_ROOT=`pwd`/opencv/install \
  -DSDL2_ROOT=`pwd`/SDL/install \
  -Dexiv2_ROOT=`pwd`/exiv2/install \
  -Dspdlog_ROOT=`pwd`/spdlog/build/install

cmake --build build -j $(nproc) --target install
cd build
ctest --output-on-failure
cd ..
