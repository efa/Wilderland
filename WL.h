/****************************************************************************\
*                                                                            *
*                              WL.h                                          *
*                                                                            *
* Wilderland - A Hobbit Environment                                          *
*                                                                            *
* (c) 2012-2019 by CH, Copyright 2019-2021 Valerio Messina                   *
*                                                                            *
* V 1.08 - 20210905                                                          *
*                                                                            *
\****************************************************************************/


#ifndef WL_H
#define WL_H

#include "SPECTRUM.h"


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


#define WL_DEBUG 0

// Versions (The chronological order seems to be V10-OWN-V12)
#define OWN 0
#define V10 1
#define V12 2
#define FN_OWN          "HOB_OWN.bin"
#define FN_V10          "HOB_V10.bin"
#define FN_V12          "HOB_V12.bin"
#define STARTADR_OWN            0x6000
#define STARTADR_V10            0x6000
#define STARTADR_V12            0x6000
#define LENGTH_OWN              40000
#define LENGTH_V10              37888
#define LENGTH_V12              40000
#define DICTIONARY_BASE_OWN     0x6000
#define DICTIONARY_BASE_V10     0x6000
#define DICTIONARY_BASE_V12     0x6000
#define DICTIONARY_BEGIN_OFFSET 0x0040
#define ROOMS_INDEX_OWN         0xB984
#define ROOMS_INDEX_V10         0xB8D0
#define ROOMS_INDEX_V12         0xB9E0
#define ROOMS_OWN               0xBA2E
#define ROOMS_V10               0xB97A
#define ROOMS_V12               0xBA8A
#define ROOMS_NROF_OWN          0x50
#define ROOMS_NROF_V10          0x50
#define ROOMS_NROF_V12          0x50
#define ROOMS_NROF_MAX          0x50
#define ROOMS_NR_MAX            0x4F
#define OBJECTS_INDEX_OWN       0xC007
#define OBJECTS_INDEX_V10       0xBF53
#define OBJECTS_INDEX_V12       0xC063
#define OBJECTS_OWN             0xC0BF
#define OBJECTS_V10             0xC00B
#define OBJECTS_V12             0xC11B
#define OBJECTS_NROF_OWN        0x3D
#define OBJECTS_NROF_V10        0x3D
#define OBJECTS_NROF_V12        0x3D
#define OBJECTS_NROF_MAX        0x3D
#define OBJECTS_NR_MAX          0x4C
#define L_START_OWN             0x6C00
#define L_START_V10             0x6C00
#define L_START_V12             0x6C00
#define L_PRINT_CHAR_OWN        0x7589
#define L_PRINT_CHAR_V10        0x7580
#define L_PRINT_CHAR_V12        0x858B
#define L_PRINT_UPPERWINDOW_OWN 0x769D
#define L_PRINT_UPPERWINDOW_V10 0x7694
#define L_PRINT_UPPERWINDOW_V12 0x86A1
#define L_SELECT_PRINTWIN_OWN   0xB6A5
#define L_SELECT_PRINTWIN_V10   0xB5F2
#define L_SELECT_PRINTWIN_V12   0xB701
#define L_ACTING_ANIMAL_OWN     0xB68E
#define L_ACTING_ANIMAL_V10     0xB5DB
#define L_ACTING_ANIMAL_V12     0xB6EA
#define L_YOU_OBJ_POSITION_OWN  0xC0CF
#define L_YOU_OBJ_POSITION_V10  0xC01B
#define L_YOU_OBJ_POSITION_V12  0xC12B

// Spectrum Screen Pixels in PC screen pixels
#define SS_WIDTH_SPIXELS  (SP_WIDTH*2)
#define SS_HEIGHT_SPIXELS (SP_HEIGHT*2)

// Main Window
#define MAINWINWIDTH  1280
#define MAINWINHEIGHT 1024
#define COLORDEPTH    32
#define BYTESPERPIXEL (COLORDEPTH/8) // 4
#define BORDERGRAY    0xFF404040ul // Gray in ARGB888 format with SDL_ALPHA_OPAQUE set
#define BORDERWIDTH   4

// Log Window (the rigth and left frame overdraws game and object window)
#define LOGWINPOSX    (2*BORDERWIDTH) // 8
#define LOGWINPOSY    (2*BORDERWIDTH) // 8
#define LOGWINWIDTH   (MAINWINWIDTH - OBJWINWIDTH - GAMEWINWIDTH - 10*BORDERWIDTH) // 280
#define LOGWINHEIGHT  (OBJWINHEIGHT) // 512: GAMEWINHEIGHT + HELPWINHEIGHT + 4*BORDERWIDTH

// Game Window
#define GAMEWINPOSX   (LOGWINWIDTH + 5*BORDERWIDTH) // 300
#define GAMEWINPOSY   (2*BORDERWIDTH) // 8
#define GAMEWINWIDTH  SS_WIDTH_SPIXELS  // 512
#define GAMEWINHEIGHT SS_HEIGHT_SPIXELS // 384

// Object Window
#define OBJWINPOSX    (MAINWINWIDTH - OBJWINWIDTH - 2*BORDERWIDTH) // 824
#define OBJWINPOSY    (2*BORDERWIDTH) // 8
#define OBJWINWIDTH   (56*8) // 448: 58 column, 8 pixel/char
#define OBJWINHEIGHT  (64*8) // 512: 64 lines, 8 pixel/char

// Help Window
#define HELPWINPOSX   (LOGWINWIDTH + 5*BORDERWIDTH)   // 300
#define HELPWINPOSY   (GAMEWINHEIGHT + 6*BORDERWIDTH) // 408
#define HELPWINWIDTH  (GAMEWINWIDTH) // 512
#define HELPWINHEIGHT (LOGWINHEIGHT - GAMEWINHEIGHT - 4*BORDERWIDTH) // 112

// Game Map Window
#define MAPWINPOSX    0
#define MAPWINPOSY    (LOGWINHEIGHT + 4*BORDERWIDTH) // 528
#define MAPWINWIDTH   MAINWINWIDTH // 1280
#define MAPWINHEIGHT  (MAINWINHEIGHT - LOGWINHEIGHT - 4*BORDERWIDTH) // 496
#define INDICATOROFFSET 2


#define SDLTWE_CHARSETFILEPATH  "CharSetSpectrum8x8.bin"
#define SDLTWE_CHARSETLENGTH    (0x300)


// Object Number Animals
#define OBJNR_YOU              0x00
#define OBJNR_DRAGON           0x3C
#define OBJNR_NASTYGOBLIN      0x3D
#define OBJNR_GANDALF          0x3E
#define OBJNR_THORIN           0x3F
#define OBJNR_ELF              0x40
#define OBJNR_ELROND           0x41
#define OBJNR_BUTLER           0x42
#define OBJNR_WARG             0x43
#define OBJNR_GOLLUM           0x44
#define OBJNR_HIDEOUSGOBLIN    0x45
#define OBJNR_BARD             0x46
#define OBJNR_HIDEOUSTROLL     0x47
#define OBJNR_VICIOUSTROLL     0x48
#define OBJNR_HORRIBLEGOBLIN   0x49
#define OBJNR_MEANGOBLIN       0x4A
#define OBJNR_VICIOUSGOBLIN    0x4B
#define OBJNR_DISGUSTINGGOBLIN 0x4C


// Object Properties Offsets
#define OCC_OFF          0x00
#define MO_OFF           0x01
#define VOLUME_OFF       0x02
#define MASS_OFF         0x03
#define A4_OFF           0x04
#define A5_OFF           0x05
#define A6_OFF           0x06
#define ATTRIBUTE_OFF    0x07
#define FIRST_OCCURRENCE 0x10

// Object Attributes
#define ATTR_LOCKED      0x01
#define ATTR_LIQUID      0x02
#define ATTR_FULL        0x04
#define ATTR_BROKEN      0x08
#define ATTR_GIVESLIGHT  0x10
#define ATTR_OPEN        0x20
#define ATTR_ANIMAL      0x40
#define ATTR_VISIBLE     0x80

struct GameKeyboardInputStruct {
    int B;
    int E;
    char buffer[100];
    int length;
};

#endif /* WL_H */
