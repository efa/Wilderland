/****************************************************************************\
*                                                                            *
*                                  SDLTWE.h                                  *
*                                                                            *
* SDL Text Windows Engine                                                    *
*                                                                            *
* (c) 2012-2019 by CH, Copyright 2019-2022 Valerio Messina                   *
*                                                                            *
* V 2.09 - 20220907                                                          *
*                                                                            *
*  SDLTWE.h is part of Wilderland - A Hobbit Environment                     *
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


#ifndef SDLTWE_H
#define SDLTWE_H

#include <SDL.h> // SDL2


// input as ARGB8888, generate comma separated RGBA components for SetRenderDrawColor:
#define compA(color) ((color>>24)&0xFF)
#define compR(color) ((color>>16)&0xFF)
#define compG(color) ((color>>8)&0xFF)
#define compB(color) ((color>>0)&0xFF)
#define components(color) compR(color),compG(color),compB(color),compA(color)


struct TextWindowStruct {
   SDL_Texture* texPtr;
   uint32_t* framePtr;
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

extern byte firstEditable; // column by SDLTWE_PrintCharTextWindow

void SDLTWE_SetPixel(struct TextWindowStruct *TW, int x, int y, uint32_t color);
void SDLTWE_VerticalScrollUpOneLine(struct TextWindowStruct *TW, struct CharSetStruct *CS, color_t paper);
void SDLTWE_DrawTextWindowFrame(struct TextWindowStruct *TW, int BorderWidth, color_t Color);
void SDLTWE_PrintCharTextWindow(struct TextWindowStruct *TW, char a, struct CharSetStruct *CS, uint32_t ink, uint32_t paper);
void SDLTWE_PrintString (struct TextWindowStruct *TW, char *ps, struct CharSetStruct *CS, uint32_t ink, uint32_t paper);
byte *SDLTWE_ReadCharSet(char *CharSetFilename, byte *CharSet);

#endif /* SDLTWE_H */
