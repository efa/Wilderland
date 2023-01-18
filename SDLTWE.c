/****************************************************************************\
*                                                                            *
*                                  SDLTWE.c                                  *
*                                                                            *
* SDL Text Windows Engine                                                    *
*                                                                            *
* (c) 2012-2019 by CH, Copyright 2019-2022 Valerio Messina                   *
*                                                                            *
* V 2.09 - 20220907                                                          *
*                                                                            *
*  SDLTWE.c is part of Wilderland - A Hobbit Environment                     *
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


/*** INCLUDES ***************************************************************/
#include <stdio.h>

#include "WL.h"
#include "SDLTWE.h"
#include "GlobalVars.h"


byte firstEditable = 0; // column by SDLTWE_PrintCharTextWindow


/****************************************************************************\
* SDLTWE_DrawTextWindowFrame                                                 *
*                                                                            *
* Draw a frame around a text window.                                         *
* The caller is responsible for sensible values!                             *
\****************************************************************************/
void SDLTWE_DrawTextWindowFrame(struct TextWindowStruct *TW, int BorderWidth, color_t Color) {
   uint8_t r;
   uint8_t g;
   uint8_t b;
   uint8_t a;
   SDL_Rect rect;

   SDL_GetRenderDrawColor(renPtr, &r, &g, &b, &a); // save prev color
   SDL_SetRenderDrawColor(renPtr, components(Color));
   for (int i=1; i<=BorderWidth; i++) { // 1,2,3,4
      rect.x = TW->rect.x - i; // 7,6,5,4
      rect.y = TW->rect.y - i;
      rect.w = TW->rect.w + 2*i; // 2,4,6,8
      rect.h = TW->rect.h + 2*i;
      SDL_RenderDrawRect(renPtr, &rect);
   }
   SDL_SetRenderDrawColor(renPtr, r, g, b, a); // restore prev color
}


/****************************************************************************\
* SDLTWE_Setpixel                                                            *
*                                                                            *
\****************************************************************************/
void SDLTWE_SetPixel(struct TextWindowStruct *TW, int x, int y, color_t color) {
   if (x>=TW->rect.w) return;
   if (y>=TW->rect.h) return;
   TW->framePtr[y*(TW->rect.w) + x] = color;
}


/****************************************************************************\
* SDLTWE_VerticalScrollUpOneLine                                             *
*                                                                            *
\****************************************************************************/
void SDLTWE_VerticalScrollUpOneLine(struct TextWindowStruct *TW, struct CharSetStruct *CS, color_t paper) {
   int x, y;

   //size_t plines = TW->rect.h - CS->Height; // pixel lines number - 1 char line
   //memmove (&TW->framePtr[0], &TW->framePtr[CS->Height * TW->rect.w], plines * TW->rect.w * BYTESPERPIXEL); // move up Frame
   int sp, dp;
   for (uint8_t l=1; l<TW->rect.h/CS->Height; l++) { // lines 1 to 63
      sp= l    * CS->Height * TW->rect.w;
      dp=(l-1) * CS->Height * TW->rect.w;
      //printf("l:%02u sp:%06d, dp:%06d sz:%04u\n", l, sp, dp, CS->Height * TW->pitch);
      memcpy (&TW->framePtr[dp], &TW->framePtr[sp], CS->Height * TW->pitch); // move up one char line
   }

   for (y = 0; y < CS->Height; y++) // fill last char line with paper
      for (x = 0; x < TW->rect.w; x++)
         SDLTWE_SetPixel(TW, x, TW->rect.h - CS->Height + y, paper);
}


/****************************************************************************\
* SDLTWE_PrintCharTextWindow                                                 *
*                                                                            *
* Prints a character to a text window. The print coordinates are updated and *
* a vertical scroll is performed if necessary.                               *
* The following control characters are processed:                            *
*   \n \r (both used as Newline, ie CR/LF)                                   *
*   \f    (used as formfeed / CLS)                                           *
*   \b    (used as backspace)                                                *
\****************************************************************************/
void SDLTWE_PrintCharTextWindow(struct TextWindowStruct *TW, char a, struct CharSetStruct *CS, color_t ink, color_t paper) {
   int i, j;
   int CharIndex;
   byte mask;
   uint32_t pixm;
   byte bs=0;

   if (WL_DEBUG) {
      if ((CS->Width != 8) || (CS->Height != 8)) {
         fprintf(stderr, "SDLTWE: ERROR in PrintCharTextWindow. Only 8x8 pixel charsets are supported.\n");
         return;
      }
   }

   if (a == '\f') { // formfeed / CLS
      for (i = 0; i < (TW->rect.h / CS->Height); i++) { // # of char lines
         SDLTWE_VerticalScrollUpOneLine(TW, CS, paper);
         SDL_UpdateTexture(TW->texPtr, NULL, TW->framePtr, TW->pitch); // copy Frame to Texture
         SDL_RenderCopy(renPtr, TW->texPtr, NULL, &TW->rect); // Texture to renderer
         SDL_RenderPresent(renPtr);
         SDL_Delay(12); // at least 12 ms to avoid flickering
      }
      TW->CurrentPrintPosX = 0;
      TW->CurrentPrintPosY = 0;
      return;
   }

   if (TW->CurrentPrintPosY >= TW->rect.h) { // Scroll
      SDLTWE_VerticalScrollUpOneLine(TW, CS, paper);
      TW->CurrentPrintPosX = 0;
      TW->CurrentPrintPosY -= CS->Height;
   }

   if (a == '\r' || a == '\n') { // 0x0D or 0x0A
      TW->CurrentPrintPosX = 0;
      TW->CurrentPrintPosY += CS->Height;
      return;
   }

   if (a == '\b') { // Backspace 0x08
      if (TW->CurrentPrintPosX > firstEditable*CS->Width) {
         TW->CurrentPrintPosX -= CS->Width;
         a=0; // will be substituted
      } else return;
      bs=1;
   }

   // if required substitute characters not contained in the character set
   if ((a < CS->CharMin) || (a > CS->CharMax)) {
      if (CS->CharSubstitute)
         a = CS->CharSubstitute;
      else
         return;
   }

   for (i = 0; i <= 7; i++) { // print char from CharSetStruct
      mask = 0x80;
      CharIndex = (a - CS->CharMin) * CS->Height;
      for (j = 0; j <= 7; j++, mask >>= 1) {
         if (CS->Bitmap[CharIndex + i] & mask)
            pixm = ink;
         else
            pixm = paper;
         TW->framePtr[(TW->CurrentPrintPosY + i) * TW->rect.w + TW->CurrentPrintPosX + j] = pixm;
      }
   }

   if (bs==0) TW->CurrentPrintPosX += CS->Width;
   if (TW->CurrentPrintPosX >= TW->rect.w) {
      TW->CurrentPrintPosX = 0;
      TW->CurrentPrintPosY += CS->Height;
   }
}


/****************************************************************************\
* SDLTWE_PrintString                                                         *
*                                                                            *
\****************************************************************************/
void SDLTWE_PrintString(struct TextWindowStruct *TW, char *ps, struct CharSetStruct *CS, color_t ink, color_t paper) {
   while (*ps)
      SDLTWE_PrintCharTextWindow(TW, *ps++, CS, ink, paper);
}


/****************************************************************************\
* SDLTWE_ReadCharSet                                                         *
*                                                                            *
* Reads a binary character set.                                              *
*                                                                            *
* INPUT:  CharSetFilename = Filename (incl. path) of data to load            *
*         CharSet = a place to store the data                                *
* RETURN: pointer to data read OR NULL if error occured                      *
\****************************************************************************/
byte *SDLTWE_ReadCharSet(char *CharSetFilename, byte *CharSet) {
   FILE *fp;
   long int FileLength;
   size_t BytesRead;

   fp = fopen(CharSetFilename, "rb");
   if (!fp) {
      fprintf(stderr, "ERROR in SDLWTE.c: can't open character set file '%s'\n", CharSetFilename);
      return (NULL);
   }

   fseek(fp, 0, SEEK_END);
   FileLength = ftell(fp);
   fseek(fp, 0, SEEK_SET);

   if (WL_DEBUG) {
      printf("SDLWTE: Reading character set with %li byte from %s ... ", FileLength, CharSetFilename);
   }

   BytesRead = fread(CharSet, 1, FileLength, fp);

   fclose(fp);

   if (FileLength != BytesRead) {
      if (WL_DEBUG)
         printf("ERROR!\n");
      return (NULL);
   } else {
      if (WL_DEBUG)
         printf("read!\n");
      return (CharSet);
   }
}
