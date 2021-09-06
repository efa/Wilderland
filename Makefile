# Makefile: Copyright 2019-2021 Valerio Messina efa@iol.it
# Makefile is part of Wilderland - A Hobbit Environment
# Wilderland is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Wilderland is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Wilderland. If not, see <http://www.gnu.org/licenses/>.
# Special exception/limitation to GPL: not for commercial use

# Makefile to generate 'Wilderland' using GCC+GNUtoolchain
# used on: Linux64=>bin64, Linux32=>bin32, Mingw64=>bin64, Mingw32=>bin32
# require $ sudo apt-get install libsdl2-dev libsdl2-image-dev

CC = gcc
LD = $(CC)
STRIP = strip
CFLAGS = -std=c99 -Wall -O3 -D__USE_MINGW_ANSI_STDIO=1
SDLCFLAGS= $(shell pkg-config --cflags SDL2_image)
ifeq ($(OS),Windows_NT) # on MINGW + SDL force console for debug
SDLMINGWLDFLAGS = -mconsole
endif
LDFLAGS = $(shell pkg-config --libs SDL2_image) $(SDLMINGWLDFLAGS)
FILE = WL
SOURCE = $(FILE)
TARGET = $(FILE)
TAPCON = TapCon/TapCon
CPU = $(uname -m)
ifeq ($(MSYSTEM),MINGW64) # strip and rm require .exe in MSYS2/MINGW
	CPU = x86_64
	TARGET = $(FILE).exe
	TAPCON = TapCon/TapCon.exe
endif
ifeq ($(MSYSTEM),MINGW32) # strip and rm require .exe in MSYS2/MINGW
	CPU = i686
	TARGET = $(FILE)32.exe
	TAPCON = TapCon/TapCon32.exe
endif
OS = $(uname -o)

all: $(TARGET) $(TAPCON)

$(TARGET): $(SOURCE).o SDLTWE.o spectrum.o Z80.o
	$(LD) $^ $(LDFLAGS) -o $@

$(SOURCE).o: $(SOURCE).c WL.h GLOBAL_VARS.h MapCoordinates.h SDLTWE.h SPECTRUM.h Z80/Z80.h
	$(CC) $(CFLAGS) $(SDLCFLAGS) -c $< -o $@

SDLTWE.o: SDLTWE.c WL.h SDLTWE.h
	$(CC) $(CFLAGS) $(SDLCFLAGS) -c $< -o $@

spectrum.o: Spectrum.c GLOBAL_VARS.h SPECTRUM.h WL.h SDLTWE.h Z80/Z80.h
	$(CC) $(CFLAGS) $(SDLCFLAGS) -c $< -o $@

Z80.o: Z80/Z80.c GLOBAL_VARS.h SPECTRUM.h Z80/Codes.h Z80/CodesCB.h Z80/CodesED.h Z80/CodesXCB.h Z80/CodesXX.h Z80/Tables.h Z80/Z80.h
	$(CC) -I. $(CFLAGS) $(SDLCFLAGS) -c $< -o $@

$(TAPCON): TapCon/TapCon.c
	$(CC) $(CFLAGS) $^ -o $@
	@chmod +x $@ # required on Linux

strip:
	$(STRIP) $(TARGET) $(TAPCON)

cleanobj:
	rm -f *.o

cleanexe:
	rm -f $(TARGET) $(TAPCON)

clean: cleanobj cleanexe

bin: all cleanobj strip

pkg: bin
	@WLpkg
