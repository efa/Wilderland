/****************************************************************************\
*                                                                            *
*                                 Spectrum.h                                 *
*                                                                            *
* Spectrum specific subroutines for the WL project                           *
*                                                                            *
* (c) 2012-2019 by CH, Copyright 2019-2025 Valerio Messina                   *
*                                                                            *
* V 2.10 - 20250106                                                          *
*                                                                            *
*  Spectrum.h is part of Wilderland - A Hobbit Environment                   *
*  Wilderland is free software: you can redistribute it and/or modify        *
*  it under the terms of the GNU General Public License as published by      *
*  the Free Software Foundation, either version 2 of the License, or         *
*  (at your option) any later version.                                       *
*                                                                            *
*  Wilderland is distributed in the hope that it will be useful,             *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              *
*  GNU General Public License for more details.                              *
*                                                                            *
*  You should have received a copy of the GNU General Public License         *
*  along with Wilderland. If not, see <http://www.gnu.org/licenses/>.        *
*                                                                            *
\****************************************************************************/


#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <stdint.h>

// select the CPU emulator, CPUEMUL must be: eZ80 or ez80emu, see Makefile
#define eZ80    1
#define ez80emu 2

#if CPUEMUL == eZ80
   #include "Z80/Z80.h"
#endif
#if CPUEMUL == ez80emu
   #include "z80emu/z80emu.h"
#endif


// Custom data types, use C99 fixed size as basis
#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef uint8_t byte;
#endif /* BYTE_TYPE_DEFINED */

#ifndef WORD_TYPE_DEFINED
#define WORD_TYPE_DEFINED
typedef uint16_t word;
#endif /* WORD_TYPE_DEFINED */

#ifndef COLOR_T_TYPE_DEFINED
#define COLOR_T_TYPE_DEFINED
typedef uint32_t color_t;
#endif /* COLOR_T_TYPE_DEFINED */


// Clock
#define CLOCK_HZ 3500000 // 3.5 MHz Z80 Spectrum clock
#define FPS 25
#define TSTATES_PER_LOOP ((int)(CLOCK_HZ / FPS)) // 140 kperiods/frame
#define DELAY_MS ((int)(1000/FPS)) // 40 ms/frame

// Spectrum Screen pixels and raw/col
#define SP_WIDTH  256
#define SP_HEIGHT 192
#define SP_PIXELPERCHAR 8
#define SP_COL (SP_WIDTH/SP_PIXELPERCHAR)  // 32
#define SP_ROW (SP_HEIGHT/SP_PIXELPERCHAR) // 24

// Spectrum Labels
#define SL_CHARSET         0x3D00
#define SL_ROMEND          0x3FFF
#define SL_SCREENSTART     0x4000 // 16384
#define SL_SCREENEND       0X57FF // 22527:(SCREENSTART + WIDTH * HEIGHT / 8 - 1)
#define SL_ATTRIBUTESTART  0x5800 // 22528:(SCREENSTART + WIDTH * HEIGHT / 8)
#define SL_ATTRIBUTEEND    0x5AFF // 23295:(ATTRIBUTESTART + SP_COL * SP_ROW - 1)
#define SL_RAMSTART        0x5B00 // 23296
#define SL_RAMEND          0xFFFF // 65335
#define SL_48K             0x10000 // 65336

// Spectrum Colors in ARGB888 format
#define SC_BLACK          0x00000000u
#define SC_BLUE           0x000000C0u
#define SC_RED            0x00C00000u
#define SC_MAGENTA        0x00C000C0u
#define SC_GREEN          0x0000C000u
#define SC_CYAN           0x0000C0C0u
#define SC_YELLOW         0x00C0C000u
#define SC_WHITE          0x00C0C0C0u
#define SC_BRBLACK        0x00000000u
#define SC_BRBLUE         0x000000FFu
#define SC_BRRED          0x00FF0000u
#define SC_BRMAGENTA      0x00FF00FFu
#define SC_BRGREEN        0x0000FF00u
#define SC_BRCYAN         0x0000FFFFu
#define SC_BRYELLOW       0x00FFFF00u
#define SC_BRWHITE        0x00FFFFFFu


// Hardware stuff
typedef enum SHRP { // keyboard half row port addresses
   //  108421 : keys bit weight hex
   //   43210 : keys bit number
   SHRP_VCXZc = 0xFE, // c=CAPS SHIFT
   SHRP_GFDSA = 0xFD,
   SHRP_TREWQ = 0xFB,
   SHRP_54321 = 0xF7,
   SHRP_67890 = 0xEF,
   SHRP_YUIOP = 0xDF,
   SHRP_HJKLe = 0xBF, // e=ENTER
   SHRP_BNMas = 0x7F  // a=SYMBOL SHIFT, s=SPACE
} SHRP;

// Prototypes
//void SetQuadPixel(struct TextWindowStruct *TW, int x, int y, uint32_t color);
void WriteScreenByte(word address, byte v);
void WriteAttributeByte(word address, byte v);
byte RdZ80(word A);
void WrZ80(word A, byte v);
word RdwZ80(word a);
void WrwZ80(word a, word w);
byte InZ80(word P);
void OutZ80(word P, byte v);
//word LoopZ80(register Z80 *R);
//void PatchZ80(register Z80 *R);
void JumpZ80(word PC);
#if CPUEMUL == ez80emu
   void SystemCall (void* context);
#endif

#endif /* SPECTRUM_H */
