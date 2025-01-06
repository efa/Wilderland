#!/bin/bash
# WLpkg: Copyright 2019-2025 Valerio Messina efa@iol.it
# WLpkg is part of Wilderland - A Hobbit Environment
# Wilderland is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# Wilderland is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Wilderland. If not, see <http://www.gnu.org/licenses/>.
#
# Script to generate a Linux|Mingw|MXE|OSX package of 'Wilderland'
# used on: Linux=>bin64, Linux=>bin32, Linux=>macOS64
#          MinGw64=>bin64, MinGw32=>bin32, MXE64=>bin64, MXE32=>bin32
#
# Syntax: $ WLpkg.sh LIN32|LIN64|MGW32|MGW64|MXE32|MXE64|OSX64

makever=2023-12-03

DEPSPATHMGW64="/mingw64/bin" # path of DLLs needed to generate the Mingw64 package
DEPSPATHMGW32="/mingw32/bin" # path of DLLs needed to generate the Mingw32 package
DEPSLISTMGW="libdeflate.dll libjbig-0.dll libjpeg-8.dll libjxl.dll libpng16-16.dll libtiff-5.dll libwebp-7.dll SDL2.dll SDL2_image.dll zlib1.dll libgcc_s_seh-1.dll libwinpthread-1.dll libstdc++-6.dll libbrotlidec.dll libbrotlienc.dll libLerc.dll liblzma-5.dll libzstd.dll libbrotlicommon.dll"

DSTPATH=".." # path where create the Linux|Mingw/MXE|OSX package directory

echo "WLpkg: create a Linux|MinGW|MXE|OSX package for Wilderland ..."

# check for external dependency compliance
flag=0
for extCmd in 7z cp cut date grep mkdir mv pwd rm tar uname ; do
   exist=`which $extCmd 2> /dev/null`
   if (test "" = "$exist") then
      echo "Required external dependency: "\"$extCmd\"" unsatisfied!"
      flag=1
   fi
done
if [[ "$flag" = 1 ]]; then
   echo "ERROR: Install the required packages and retry. Exit"
   exit
fi

if [[ "$1" = "" || "$1" != "LIN32" && "$1" != "LIN64" && "$1" != "MGW32" && "$1" != "MGW64" && "$1" != "MXE32" && "$1" != "MXE64" && "$1" != "OSX64" ]]; then
   echo "ERROR: miss/unsupported package type"
   echo "Syntax: $ WLpkg.sh LIN32|LIN64|MGW32|MGW64|MXE32|MXE64|OSX64"
   exit
fi

exist=`which osxcross-dmg 2> /dev/null`
if (test "$1" = "OSX64" && test "" = "$exist") then
   echo "ERROR: WLpkg depend on 'osxcross-dmg' to generate for macOS. Exit"
   exit
fi

PKG="$1"
CPU=`uname -m` # i686 or x86_64
BIT=64
if [[ "$PKG" = "LIN32" || "$PKG" = "MGW32" || "$PKG" = "MXE32" ]]; then
   BIT=32
fi
if [[ "$BIT" = "32" && "$CPU" = "x86_64" ]]; then
   CPU=i686
fi

OS=`uname -o`  # Msys or GNU/Linux
VER=`grep WLVER WL.h | cut -d'"' -f2` # M.mm
DATE=`date -I`
SRC=`pwd`
TGT=WinMxe
if [[ "$OS" = "Msys" ]]; then
   TGT=WinMgw
   if [[ "$BIT" = "64" ]]; then
      DEPSRC=$DEPSPATHMGW64
   fi
   if [[ "$BIT" = "32" ]]; then
      DEPSRC=$DEPSPATHMGW32
   fi
fi
UPP="" # Uncompressed Package Path
if [[ "$PKG" = "LIN32" || "$PKG" = "LIN64" ]]; then
   TGT=Linux
   UPP="usr/bin"
fi
if [[ "$PKG" = "OSX64" ]]; then
   TGT=Osx
   UPP="DiskImage/Wilderland.app/Contents/MacOS"
fi
DST="WL${VER}_${DATE}_${TGT}_${CPU}_${BIT}bit"

if [[ "$OS" != "Msys" && "$OS" != "GNU/Linux" ]]; then
   echo "ERROR: work in MinGW|MXE(Linux) only"
   exit
fi
if [[ "$OS" = "Msys" && "$PKG" != "MGW32" && "$PKG" != "MGW64" ]]; then
   echo "ERROR WLpkg: Unsupported target package:$PKG on MinGW/MSYS2"
   exit
fi

if [[ "$OS" = "GNU/Linux" && "$PKG" != "LIN32" && "$PKG" != "LIN64" && "$PKG" != "MXE32" && "$PKG" != "MXE64" && "$PKG" != "OSX64" ]]; then
   echo "ERROR WLpkg: Unsupported target package:$PKG on Linux"
   exit
fi

echo "PKG : $PKG"
echo "CPU : $CPU"
echo "BIT : $BIT"
echo "OS  : $OS"
echo "VER : $VER"
echo "DATE: $DATE"
echo "SRC : $SRC"
echo "TGT : $TGT"
echo "DEP : $DEPSRC"
echo "UPP : $UPP"
echo "DST : $DSTPATH/$DST"
read -p "Proceed? A key to continue"
echo ""

echo "Creating Wilderland $VER package for $TGT$BIT.$CPU ..."
if [[ -d "AppDir" ]]; then
   echo "Removing old AppDir ..."
   rm -rf "AppDir"
fi
if [[ -d $DST ]]; then
   echo "Removing old DST ..."
   rm -rf "$DST"
fi
mkdir -p "AppDir/$UPP"
mkdir -p "AppDir/$UPP/Design"
mkdir -p "AppDir/$UPP/TapCon"
mkdir -p "AppDir/$UPP/z80emu"
mkdir -p "AppDir/$UPP/Z80"
mkdir -p "AppDir/$UPP/wls"
echo "Copying sources ..."
cp -a *c *.h AppDir/$UPP
cp -a TapCon/TapCon.c AppDir/$UPP/TapCon
cp -a z80emu/* AppDir/$UPP/z80emu 2> /dev/null
cp -a Z80/* AppDir/$UPP/Z80
cp -a Makefile* AppDir/$UPP
cp -a WLpkg.sh AppDir/$UPP
cp -a WLrel.sh AppDir/$UPP
echo "Copying assets ..."
cp -a CharSetSpectrum8x8.bin AppDir/$UPP
cp -a GameMap.png AppDir/$UPP
cp -a GameMapHi.png AppDir/$UPP
cp -a HOBBIT.SCR AppDir/$UPP
cp -a HOBBIT12.SCR AppDir/$UPP
cp -a Wilderland.png AppDir/$UPP
cp -a Wilderland.desktop AppDir/$UPP
echo "Copying docs ..."
cp -a Changelog.txt AppDir/$UPP
cp -a COPYING.txt AppDir/$UPP
cp -a dict_v12.txt AppDir/$UPP
cp -a README.txt AppDir/$UPP
cp -a rooms_v12.txt AppDir/$UPP
cp -a WLpackage.txt AppDir/$UPP
cp -a WLconfig.txt  AppDir/$UPP
cp -a Design/Map_V6_gross.png AppDir/$UPP/Design
cp -a Design/Map_V6_W1000.png AppDir/$UPP/Design
cp -a Design/WL.odg AppDir/$UPP/Design
cp -a Design/WL.pdf AppDir/$UPP/Design
echo "Copying binary ..."
if [[ "$PKG" = "LIN64" ]]; then
   cp -a WL AppDir/$UPP
   cp -a TapCon/TapCon AppDir/$UPP/TapCon
fi
if [[ "$PKG" = "LIN32" ]]; then
   cp -a WL32 AppDir/$UPP
   cp -a TapCon/TapCon32 AppDir/$UPP/TapCon
fi
if [[ "$PKG" = "MGW64" ]]; then
   cp -a WLmgw.exe AppDir/$UPP
   cp -a TapCon/TapConMgw.exe AppDir/$UPP/TapCon
   echo "Copying deps ..."
   for DLL in $DEPSLISTMGW ; do
      cp -a $DEPSPATHMGW64/$DLL AppDir/$UPP
   done
fi
if [[ "$PKG" = "MGW32" ]]; then
   cp -a WLmgw32.exe AppDir/$UPP
   cp -a TapCon/TapConMgw32.exe AppDir/$UPP/TapCon
   echo "Copying deps ..."
   for DLL in $DEPSLISTMGW ; do
      if [[ "$DLL" = "libgcc_s_seh-1.dll" ]]; then continue ; fi
      cp -a $DEPSPATHMGW32/$DLL AppDir/$UPP
   done
   cp -a $DEPSPATHMGW32/libgcc_s_dw2-1.dll AppDir/$UPP
fi
if [[ "$PKG" = "MXE64" ]]; then
   cp -a WLmxe.exe AppDir/$UPP
   cp -a TapCon/TapConMxe.exe AppDir/$UPP/TapCon
fi
if [[ "$PKG" = "MXE32" ]]; then
   cp -a WLmxe32.exe AppDir/$UPP
   cp -a TapCon/TapConMxe32.exe AppDir/$UPP/TapCon
fi
if [[ "$PKG" = "OSX64" ]]; then
   cp -a TapCon/TapConOsx AppDir/$UPP/TapCon
   cp -a WLosx Wilderland.icns AppDir/$UPP
fi

# remove unwanted files
rm "AppDir/$UPP/HOB*.bin" 2> /dev/null
rm "AppDir/$UPP/HOB*.TAP" 2> /dev/null
rm "AppDir/$UPP/The*.tzx" 2> /dev/null
rm "AppDir/$UPP/Hob*.zip" 2> /dev/null
rm "AppDir/$UPP/TapCon/HOB*.bin" 2> /dev/null
rm "AppDir/$UPP/TapCon/HOB*.TAP" 2> /dev/null
rm "AppDir/$UPP/TapCon/The*.tzx" 2> /dev/null
rm "AppDir/$UPP/TapCon/Hob*.zip" 2> /dev/null

# make AppImage
if [[ ("$PKG" = "LIN64" || "$PKG" = "LIN32") && ("$CPU" = "x86_64" || "$CPU" = "i686")]]; then
   echo "Generating the AppImage (about 30\") ..."
   if (test -f logWget$date.txt) then { rm logWget$date.txt ; } fi
   if (test "$BIT" = "64") then
      if (! test -x linuxdeploy-x86_64.AppImage) then
         #wget -nv "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" 2>> logWget$date.txt
         wget -nv "https://github.com/linuxdeploy/linuxdeploy/releases/download/1-alpha-20231026-1/linuxdeploy-x86_64.AppImage" 2>> logWget$date.txt
         chmod +x linuxdeploy-x86_64.AppImage
      fi
      linuxdeploy-x86_64.AppImage -e WL --appdir AppDir -i Wilderland.png -d Wilderland.desktop --output appimage > logLinuxdeploy$date.txt
      file=Wilderland${VER}_${DATE}_Linux_${CPU}_${BIT}bit.AppImage
      mv Wilderland-x86_64.AppImage $file
      cp -a $file ..
      echo "AppImage created: $file"
   fi
   if (test "$BIT" = "32") then
      echo "As now skip AppImage at 32 bit"
   fi
fi

echo "Compressing package ..."
if [[ "$PKG" = "OSX64" ]]; then
   cp -a WLosx Wilderland.icns AppDir
   cd AppDir
   osxcross-dmg -rw WLosx Wilderland $VER
   rm uncompressed.dmg WLosx Wilderland.icns
   mv Wilderland$VER.dmg ..
   cd ..
   mv Wilderland$VER.dmg $DSTPATH/Wilderland$VER.dmg
   echo "Release file: '$DSTPATH/Wilderland$VER.dmg'"
   echo Done
   exit
fi
mkdir $DST
cp -Ra AppDir/$UPP/* $DST/
if [[ -f $DST.tgz ]]; then
   rm $DST.tgz
fi
if [[ -f $DST.7z ]]; then
   rm $DST.7z
fi
if [[ "$PKG" = "LIN64" || "$PKG" = "LIN32" ]]; then
   #echo "tar cvaf $DST.tgz $DST"
   tar cvaf $DST.tgz $DST > /dev/null
   file=$DST.tgz
fi
if [[ "$OS" = "Msys" ]]; then
   #echo "7z a -mx=9 -r $DST.7z $DST"
   7z a -mx=9 -r $DST.7z $DST > /dev/null
   file=$DST.7z
fi
if [[ "$PKG" = "MXE64" || "$PKG" = "MXE32" ]]; then
   #echo "7z a -m0=lzma -mx=9 -r $DST.7z $DST"
   7z a -m0=lzma -mx=9 -r $DST.7z $DST > /dev/null
   file=$DST.7z
fi
rm -rf $DST
mv $file $DSTPATH
echo "Release file: '$DSTPATH/$file'"
echo Done
