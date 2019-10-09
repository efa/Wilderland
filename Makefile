# Makefile to generate 'Wilderland' using gcc+GNUtoolchain
# Copyright 2019 Valerio Messina efa@iol.it
# Makefile is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Makefile is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Makefile. If not, see <http://www.gnu.org/licenses/>.

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

all: TapCon/TapCon $(TARGET)

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

TapCon/TapCon: TapCon/TapCon.c
	$(CC) $(CFLAGS) $^ -o TapCon/TapCon

strip:
ifeq ($(OS),Windows_NT) # on MINGW strip need .exe
	$(STRIP) $(TARGET).exe TapCon/TapCon.exe
else
	$(STRIP) $(TARGET) TapCon/TapCon
endif

cleanobj:
	rm -f *.o

cleanexe:
	rm -f $(TARGET) TapCon/TapCon

clean:
	rm -f *.o && rm -f $(TARGET) TapCon/TapCon
