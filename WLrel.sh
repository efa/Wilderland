#!/bin/bash
# create the Build release packages for all target platforms
echo "Build release packages for all target platforms"
read -p "Proceed? A key to continue"
echo ""
os=`uname -o`

if [[ "$os" = "GNU/Linux" ]]; then
   echo "Build release for Linux64 ..."
   make rel
   echo "-----------------------------------"
   echo ""
   echo "Build release for Linux32 ..."
   make -f Makefile32 rel
   echo "-----------------------------------"
   echo ""
   echo "Cross-Build release for Win64 ..."
   make -f MakefileM64 rel
   echo "-----------------------------------"
   echo ""
   echo "Cross-Build release for Win32 ..."
   make -f MakefileM32 rel
   echo "-----------------------------------"
   echo ""
   echo "Cross-Build release for macOS64 ..."
   make -f MakefileO64 rel
   echo "-----------------------------------"
   echo ""
fi

if [[ "$os" = "GNU/Linux" || "$os" = "Msys" ]]; then
   echo "Note: To Build natively on MinGw/MSYS2/Win use:"
   echo "MinGW64 $ make -f MakefileG64 rel"
   echo "MinGW32 $ make -f MakefileG32 rel"
   echo ""
fi

if [[ "$MSYSTEM" = "MINGW64" ]]; then
   echo "Build release for Win64 ..."
   make -f MakefileG64 rel
   echo ""
fi
if [[ "$MSYSTEM" = "MINGW32" ]]; then
   echo "Build release for Win32 ..."
   make -f MakefileG32 rel
   echo ""
fi
