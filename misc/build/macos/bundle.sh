#!/bin/sh
# macports based - adapt library path to libSDL an libiconv when using brew

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
echo "App copied..."
# Copy the resource files to the correct place
cp -r ./install/share $appfolder/Contents/
echo "share copied..."

#get local dependencies
rpathdependencies=$(otool -L ./install/bin/Xpano| awk 'NR>1{print $1}' | grep @rpath | sed 's/@rpath\///')

echo "rpath dependencies..."

for dep in $rpathdependencies; do 
  case ${dep:0:5} in 
   'libop' ) 
   libfile='./opencv/install/lib/'$dep;;
   'libex' ) 
   libfile='./exiv2/install/lib/'$dep;;
  esac
 echo "handling dep" $dep
  
  #copy lib
  cp $libfile $libfolder
  #inametool
  install_name_tool -change @rpath/$dep @executable_path/../Resources/lib/$dep $macosfolder/$appfile
  
  #detect and copy second level macports dependencies
  submacportsdependencies=$(otool -L $libfile | awk 'NR>1{print $1}' | grep opt)
  for subdep in $submacportsdependencies; do 
   echo "handlingsubdep " $subdep
   #copy lib
   cp $subdep $libfolder
   #inametool
   install_name_tool -change $subdep @rpath/$(basename $subdep) $libfolder/$(basename $dep)  
  done 
  
done 

#get macports dependencies
macportsdependencies=$(otool -L ./install/bin/Xpano| awk 'NR>1{print $1}' | grep opt)

for dep in $macportsdependencies; do 
 echo "handling dep" $dep
 #copy lib
 cp $dep $libfolder
 #inametool
 install_name_tool -change $dep @executable_path/../Resources/lib/$(basename $dep) $macosfolder/$appfile
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
  echo '  <key>NSHighResolutionCapable</key>' >>$plistfile
  echo '  <string>True</string>' >>$plistfile
  echo '</dict>' >>$plistfile
  echo '</plist>' >>$plistfile
