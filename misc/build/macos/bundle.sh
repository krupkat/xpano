#!/bin/sh

set -e

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
  echo "$appfolder already exists, deleting"
  rm -rf $appfolder
fi

echo "Creating $appfolder..."
mkdir -p $macosfolder $libfolder

# Copy App
cp ./install/bin/$appfile $macosfolder/$appfile
# Resolve @rpath references to bundled libraries.
install_name_tool -add_rpath @executable_path/../Resources/lib "$macosfolder/$appfile"
echo "App copied..."
# Copy the resource files to the correct place
cp -r ./install/share $appfolder/Contents/
echo "share copied..."

#get local dependencies
rpathdependencies=$(otool -L ./install/bin/Xpano| awk 'NR>1{print $1}' | grep @rpath | sed 's/@rpath\///') || true

echo "rpath dependencies..."

# for debugging
otool -L ./install/bin/Xpano

for dep in $rpathdependencies; do 
  case $dep in
   libopencv* )
   libfile='./opencv/install/lib/'$dep;;
   libexiv2* )
   libfile='./exiv2/install/lib/'$dep;;
   libSDL3* )
   libfile='./SDL/install/lib/'$dep;;
   * )
   echo "Unknown rpath dependency: $dep" >&2
   exit 1;;
  esac
 echo "handling dep" $dep
  
  #copy lib
  cp $libfile $libfolder
  
  # for debugging
  otool -L $libfile
done

#handle icon
cp ./misc/build/macos/$appfile.icns $appfolder/Contents/Resources/

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
if [ -n "${MACOSX_DEPLOYMENT_TARGET-}" ]; then
  echo '  <key>LSMinimumSystemVersion</key>' >>$plistfile
  echo "  <string>$MACOSX_DEPLOYMENT_TARGET</string>" >>$plistfile
fi
  echo '  <key>NSHighResolutionCapable</key>' >>$plistfile
  echo '  <true/>' >>$plistfile
  echo '</dict>' >>$plistfile
  echo '</plist>' >>$plistfile

# Report build versions and reject Homebrew and MacPorts dependencies.
for binary in "$macosfolder/$appfile" "$libfolder"/*.dylib; do
  build_info=$(otool -l "$binary")
  printf '%s\n' "$build_info" | awk -v binary="$binary" '$1 == "minos" {print binary ": minos " $2}'
  dependencies=$(otool -L "$binary")
  if printf '%s\n' "$dependencies" | awk 'NR > 1 {print $1}' | grep -E '^(/opt/homebrew|/opt/local|/usr/local)/'; then
    echo "Homebrew or MacPorts dependency found in $binary" >&2
    exit 1
  fi
done

# Verify the bundled executable loads its libraries.
"$macosfolder/$appfile" --help
