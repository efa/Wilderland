/****************************************************************************\
*                                                                            *
*                              SPECTRUM.h                                    *
*                                                                            *
* Spectrum specific subroutines for the WL project                           *
*                                                                            *
* (c) 2012-2019 by CH, Copyright 2019-2021 Valerio Messina                   *
*                                                                            *
* V 1.08 - 20210905                                                          *
*                                                                            *
\****************************************************************************/


#ifndef SPECTRUM_H
#define SPECTRUM_H

#include <SDL2/SDL.h>

#include "Z80/Z80.h"


// My data types
#ifndef BYTE_TYPE_DEFINED
#define BYTE_TYPE_DEFINED
typedef Uint8 byte;
#endif /* BYTE_TYPE_DEFINED */

#ifndef WORD_TYPE_DEFINED
#define WORD_TYPE_DEFINED
typedef Uint16 word;
#endif /* WORD_TYPE_DEFINED */

#ifndef COLOR_T_TYPE_DEFINED
#define COLOR_T_TYPE_DEFINED
typedef Uint32 color_t;
#endif /* COLOR_T_TYPE_DEFINED */

#include "SDLTWE.h"


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

// Spectrum Colors in ARGB888 format
#define SC_BLACK          0x00000000ul
#define SC_BLUE           0x000000C0ul
#define SC_RED            0x00C00000ul
#define SC_MAGENTA        0x00C000C0ul
#define SC_GREEN          0x0000C000ul
#define SC_CYAN           0x0000C0C0ul
#define SC_YELLOW         0x00C0C000ul
#define SC_WHITE          0x00C0C0C0ul
#define SC_BRBLACK        0x00000000ul
#define SC_BRBLUE         0x000000FFul
#define SC_BRRED          0x00FF0000ul
#define SC_BRMAGENTA      0x00FF00FFul
#define SC_BRGREEN        0x0000FF00ul
#define SC_BRCYAN         0x0000FFFFul
#define SC_BRYELLOW       0x00FFFF00ul
#define SC_BRWHITE        0x00FFFFFFul


// Hardware stuff
#define NUMBER_OF_IN_READS   1
typedef enum SHR {
    SHR_cV = 0,       // keyboard half rows
    SHR_AG,
    SHR_QT,
    SHR_15,
    SHR_06,
    SHR_PY,
    SHR_eH,
    SHR_sB
} SHR;
typedef enum SHRP {
    SHRP_cV = 0xFE,   // keyboard half row port addresses
    SHRP_AG = 0xFD,
    SHRP_QT = 0xFB,
    SHRP_15 = 0xF7,
    SHRP_06 = 0xEF,
    SHRP_PY = 0xDF,
    SHRP_eH = 0xBF,
    SHRP_sB = 0x7F
} SHRP;

// Prototypes
void SetQuadPixel(struct TextWindowStruct *TW, int x, int y, Uint32 color);
void WriteScreenByte(word address, byte v);
void WriteAttributeByte(word address, byte v);
byte RdZ80(word A);
void WrZ80(word A, byte v);
byte InZ80(word P);
void OutZ80(word P, byte v);
word LoopZ80(register Z80 *R);
void PatchZ80(register Z80 *R);

#endif /* SPECTRUM_H */
