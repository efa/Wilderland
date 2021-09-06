/****************************************************************************\
*                                                                            *
*                              SDLTWE.h                                      *
*                                                                            *
* SDL Text Windows Engine                                                    *
*                                                                            *
* (c) 2012-2019 by CH, Copyright 2019-2021 Valerio Messina                   *
*                                                                            *
* V 1.08 - 20210905                                                          *
*                                                                            *
\****************************************************************************/


#ifndef SDLTWE_H
#define SDLTWE_H

#include <SDL2/SDL.h>

#include "GLOBAL_VARS.h"


// input as ARGB8888, generate comma separated RGBA components for SetRenderDrawColor:
#define compA(color) ((color>>24)&0xFF)
#define compR(color) ((color>>16)&0xFF)
#define compG(color) ((color>>8)&0xFF)
#define compB(color) ((color>>0)&0xFF)
#define components(color) compR(color),compG(color),compB(color),compA(color)

struct TextWindowStruct {
    SDL_Texture* texPtr;
    Uint32* framePtr;
    int frameSize;
    SDL_Rect rect;
    int pitch;
    int CurrentPrintPosX;
    int CurrentPrintPosY;
};

struct CharSetStruct {
    int Width;
    int Height;
    byte * Bitmap;
    char CharMin;
    char CharMax;
    char CharSubstitute;
};

void SDLTWE_SetPixel(struct TextWindowStruct *TW, int x, int y, Uint32 color);
void SDLTWE_VerticalScrollUpOneLine(struct TextWindowStruct *TW, struct CharSetStruct *CS, color_t paper);
void SDLTWE_DrawTextWindowFrame(struct TextWindowStruct *TW, int BorderWidth, color_t Color);
void SDLTWE_PrintCharTextWindow(struct TextWindowStruct *TW, char a, struct CharSetStruct *CS, Uint32 ink, Uint32 paper);
void SDLTWE_PrintString (struct TextWindowStruct *TW, char *ps, struct CharSetStruct *CS, Uint32 ink, Uint32 paper);
byte *SDLTWE_ReadCharSet(char *CharSetFilename, byte *CharSet);

#endif /* SDLTWE_H */
