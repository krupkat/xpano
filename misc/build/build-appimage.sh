./misc/build/build-ubuntu-22.sh

mkdir licenses

cp exiv2/COPYING licenses/exiv2-license.txt

cmake -B build \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DXPANO_EXTRA_LICENSES=licenses \
  -DNFD_PORTAL=ON

DESTDIR=AppDir cmake --build build -j $(nproc) --target install

export LD_LIBRARY_PATH=exiv2/install/lib

linuxdeploy-x86_64.AppImage --appdir build/AppDir --output appimage
