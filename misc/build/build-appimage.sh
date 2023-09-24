./misc/build/build-ubuntu-20.sh

mkdir licenses

cp SDL/LICENSE.txt licenses/sdl-license.txt
cp spdlog/LICENSE licenses/spdlog-license.txt
cp exiv2/COPYING licenses/exiv2-license.txt
cp opencv/install/share/licenses/opencv4/* licenses
cp opencv/LICENSE licenses/opencv-license.txt

cmake -B build \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DXPANO_EXTRA_LICENSES=licenses \
  -DNFD_PORTAL=ON

DESTDIR=AppDir cmake --build build -j $(nproc) --target install

export LD_LIBRARY_PATH=SDL/install/lib:exiv2/install/lib:opencv/install/lib

linuxdeploy-x86_64.AppImage --appdir build/AppDir --output appimage
