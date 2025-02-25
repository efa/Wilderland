# Makefile: Copyright 2019-2025 Valerio Messina efa@iol.it
#
# Makefile is part of Wilderland - A Hobbit Environment
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

# Makefile to build 'Wilderland' using GCC+GNUtoolchain on Linux
# To be used on: Linux64=>bin64, Linux32=>bin32
# require $ sudo apt install libsdl2-dev libsdl2-image-dev

# To use z80emu emulator build with: $ make CPUEMUL=ez80emu   OR   $ make
# To use Z80    emulator build with: $ make CPUEMUL=eZ80
# To avoid include and link libcurl and libzip build with: $ make NODL=1
# To build for debug and sanitize=address use: $ make debug
# Note: sanitize can be also $ make SAN=thread|memory debug

PKG = LIN64
CPU = $(shell uname -m)
BIT = $(shell getconf LONG_BIT)
ifeq ($(CPU),$(filter $(CPU),i686 armv7l))
   PKG = LIN32
endif
OS = $(shell uname -o)

# to support both pkg-config and pkgconf
pkgbin = $(shell which pkg-config)
islink = $(shell file $(pkgbin) | grep symbolic)
ifneq ($(islink),)
   ifeq ($(CPU),i686)
      PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig
      PARS="--with-path=$(PKG_CONFIG_LIBDIR)"
   endif
   ifeq ($(CPU),x86_64)
      PKG_CONFIG_LIBDIR=/usr/lib/x86_64-linux-gnu/pkgconfig
      PARS="--with-path=$(PKG_CONFIG_LIBDIR)"
   endif
endif

# select the CPU emulator, CPUEMUL must be: eZ80 or ez80emu
eZ80    = 1
ez80emu = 2
ifeq ($(CPUEMUL),)   # default to ez80emu
   CPUEMUL = ez80emu
endif
ifeq ($(CPUEMUL),eZ80)
   CPUIF=Z80/Z80.h
   CPUOBJ=Z80.o
endif
ifeq ($(CPUEMUL),ez80emu)
   CPUIF=z80emu/z80emu.h
   CPUOBJ=z80emu.o
endif

CC ?= gcc
LD = $(CC)
STRIP = strip
CFLAGS = -std=c99 -Wall -O3 -DCPUEMUL=$(CPUEMUL) -fomit-frame-pointer
ifeq ($(CPU),armv7l)   # Raspberry Pi @32bit
   CFLAGS += -mfpu=neon
endif
SDLCFLAGS = $(shell pkg-config $(PARS) --cflags SDL2_image)
LDFLAGS = $(shell pkg-config $(PARS) --libs SDL2_image)
ifndef NODL
   CFLAGS += $(shell pkg-config --cflags libcurl)
   CFLAGS += $(shell pkg-config --cflags libzip)
   #LDFLAGS += -lcurl -lzip -lz
   LDFLAGS += $(shell pkg-config --libs libcurl)
   LDFLAGS += $(shell pkg-config --libs libzip)
else
   CFLAGS += -DNODL
endif

SAN?=address
CFLAGS_address = -fsanitize=address -fno-omit-frame-pointer
LDFLAGS_address = -fsanitize=address
CFLAGS_thread = -fsanitize=thread
LDFLAGS_thread = -fsanitize=thread
ifeq ($(CC),clang)
  CFLAGS_memory = -fsanitize=memory -fPIE -fno-omit-frame-pointer -fsanitize-memory-track-origins
  LDFLAGS_memory = -fsanitize=memory -fPIE -pie
endif

FILE = WL
SOURCE = $(FILE)
TARGET = $(FILE)
TAPCON = TapCon/TapCon

all: $(TARGET) $(TAPCON)

$(TARGET): $(SOURCE).o SDLTWE.o spectrum.o $(CPUOBJ)
	$(LD) $^ $(LDFLAGS) -o $@

$(SOURCE).o: $(SOURCE).c WL.h GlobalVars.h MapCoordinates.h SDLTWE.h Spectrum.h $(CPUIF)
	$(CC) $(CFLAGS) $(SDLCFLAGS) -c $< -o $@

SDLTWE.o: SDLTWE.c WL.h SDLTWE.h
	$(CC) $(CFLAGS) $(SDLCFLAGS) -c $< -o $@

spectrum.o: Spectrum.c GlobalVars.h Spectrum.h WL.h SDLTWE.h $(CPUIF)
	$(CC) $(CFLAGS) $(SDLCFLAGS) -c $< -o $@

z80emu/tables.h: z80emu/maketables.c
	$(CC) -Wall $< -o z80emu/maketables
	@echo "Generated 'maketables', now run it to output 'tables.h'"
	./z80emu/maketables > $@

z80emu.o: z80emu/z80emu.c z80emu/z80emu.h z80emu/z80config.h z80emu/z80user.h \
	z80emu/instructions.h z80emu/macros.h z80emu/tables.h Spectrum.h
	$(CC) -I. $(CFLAGS) -c $<

Z80.o: Z80/Z80.c GlobalVars.h Spectrum.h Z80/Codes.h Z80/CodesCB.h \
	Z80/CodesED.h Z80/CodesXCB.h Z80/CodesXX.h Z80/Tables.h Z80/Z80.h
	$(CC) -I. $(CFLAGS) $(SDLCFLAGS) -c $< -o $@

$(TAPCON): TapCon/TapCon.c WL.h
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $@

strip:
	$(STRIP) $(TARGET) $(TAPCON)

cleanobj:
	rm -f *.o

cleanbin:
	rm -f $(TARGET) $(TAPCON)

clean: cleanobj cleanbin

debug: CFLAGS += -O2 -g -fsanitize=undefined $(CFLAGS_$(SAN))
debug: LDFLAGS += -fsanitize=undefined $(LDFLAGS_$(SAN))
debug: clean all cleanobj

bin: all cleanobj strip

force: clean bin
	rm -f *.o

pkg:
	@WLpkg.sh $(PKG) $(BIT)

rel: force pkg
