#!/bin/sh
# macports based - adapt library path to libSDL when using brew

appname=Xpano
appfolder=$appname.app
macosfolder=$appfolder/Contents/MacOS
libfolder=$appfolder/Contents/Resources/lib
plistfile=$appfolder/Contents/Info.plist
appfile=Xpano

PkgInfoContents="APPL????"

#
if [ -e $appfolder ]
then
  echo "$appfolder already exists"
  rm -rf $appfolder
fi

echo "Creating $appfolder..."
mkdir -p $macosfolder $libfolder

# Copy App
cp ./install/bin/$appfile $macosfolder/$appfile

# Copy the resource files to the correct place
cp -r ./install/share $appfolder/Contents/

cp /opt/local/lib/libSDL2-2.0.0.dylib $libfolder
cp ./exiv2/build/lib/libexiv2.28.dylib $libfolder
cp ./opencv/build/lib/libopencv_imgcodecs.407.dylib $libfolder
cp ./opencv/build/lib/libopencv_calib3d.407.dylib $libfolder
cp ./opencv/build/lib/libopencv_core.407.dylib $libfolder
cp ./opencv/build/lib/libopencv_features2d.407.dylib $libfolder
cp ./opencv/build/lib/libopencv_flann.407.dylib $libfolder
cp ./opencv/build/lib/libopencv_imgproc.407.dylib $libfolder
cp ./opencv/build/lib/libopencv_photo.407.dylib $libfolder
cp ./opencv/build/lib/libopencv_stitching.407.dylib $libfolder

cp ./misc/build/macos/$appfile.icns $appfolder/Contents/Resources/

#inametool
install_name_tool -change @rpath/libexiv2.28.dylib @executable_path/../Resources/lib/libexiv2.28.dylib $macosfolder/$appfile
install_name_tool -change @rpath/libopencv_imgcodecs.407.dylib @executable_path/../Resources/lib/libopencv_imgcodecs.407.dylib $macosfolder/$appfile
install_name_tool -change @rpath/libopencv_calib3d.407.dylib @executable_path/../Resources/lib/libopencv_calib3d.407.dylib $macosfolder/$appfile
install_name_tool -change @rpath/libopencv_core.407.dylib @executable_path/../Resources/lib/libopencv_core.407.dylib $macosfolder/$appfile
install_name_tool -change @rpath/libopencv_features2d.407.dylib @executable_path/../Resources/lib/libopencv_features2d.407.dylib $macosfolder/$appfile
install_name_tool -change @rpath/libopencv_flann.407.dylib @executable_path/../Resources/lib/libopencv_flann.407.dylib $macosfolder/$appfile
install_name_tool -change @rpath/libopencv_imgproc.407.dylib @executable_path/../Resources/lib/libopencv_imgproc.407.dylib $macosfolder/$appfile
install_name_tool -change @rpath/libopencv_photo.407.dylib @executable_path/../Resources/lib/libopencv_photo.407.dylib $macosfolder/$appfile
install_name_tool -change @rpath/libopencv_stitching.407.dylib @executable_path/../Resources/lib/libopencv_stitching.407.dylib $macosfolder/$appfile
install_name_tool -change /opt/local/lib/libSDL2-2.0.0.dylib @executable_path/../Resources/lib/libSDL2-2.0.0.dylib $macosfolder/$appfile

#
# Create PkgInfo file.
  echo $PkgInfoContents >$appfolder/Contents/PkgInfo
#
# Create information property list file (Info.plist).
  echo '<?xml version="1.0" encoding="UTF-8"?>' >$plistfile
  echo '<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">' >>$plistfile
  echo '<plist version="1.0">' >>$plistfile
  echo '<dict>' >>$plistfile
  echo '  <key>CFBundleDevelopmentRegion</key>' >>$plistfile
  echo '  <string>English</string>' >>$plistfile
  echo '  <key>CFBundleExecutable</key>' >>$plistfile
  echo '  <string>'$appfile'</string>' >>$plistfile
  echo '  <key>CFBundleIconFile</key>' >>$plistfile
  echo '  <string>'$appfile.icns'</string>' >>$plistfile
  echo '  <key>CFBundleIdentifier</key>' >>$plistfile
  echo '  <string>cz.krupkat.'$appname'</string>' >>$plistfile
  echo '  <key>CFBundleInfoDictionaryVersion</key>' >>$plistfile
  echo '  <string>6.0</string>' >>$plistfile
  echo '  <key>CFBundlePackageType</key>' >>$plistfile
  echo '  <string>APPL</string>' >>$plistfile
  echo '  <key>CFBundleSignature</key>' >>$plistfile
  echo '  <string>????</string>' >>$plistfile
  echo '  <key>CFBundleVersion</key>' >>$plistfile
  echo '  <string>1.0</string>' >>$plistfile
  echo '  <key>NSHighResolutionCapable</key>' >>$plistfile
  echo '  <string>True</string>' >>$plistfile
  echo '</dict>' >>$plistfile
  echo '</plist>' >>$plistfile
