# DO NOT MODIFY: Auto-generated by the gen_installer.py script from the .github/workflows/ubuntu.yml Github Action

export BUILD_TYPE='Release'
export CATCH_VERSION='v3.1.0'
export OPENCV_VERSION='4.6.0'
export SDL_VERSION='release-2.24.0'

git submodule update --init


git clone https://github.com/catchorg/Catch2.git catch --depth 1 --branch $CATCH_VERSION
cd catch
cmake -B build -DCMAKE_INSTALL_PREFIX=install -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_TESTING=OFF
cmake --build build -j $(nproc) --target install
cd ..

cmake -B build \
  -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
  -DCMAKE_INSTALL_PREFIX=install \
  -DBUILD_TESTING=ON \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCatch2_DIR=../catch/install/lib/cmake/Catch2

cmake --build build -j $(nproc) --target install
cd build
ctest
cd ..
