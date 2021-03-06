/****************************************************************************\
*                                                                            *
*                                SDLTWE.c                                    *
*                                                                            *
* SDL Text Windows Engine                                                    *
*                                                                            *
* (c) 2012-2019 by CH, Copyright 2019 Valerio Messina                        *
*                                                                            *
* V 1.08 - 20191005                                                          *
*                                                                            *
\****************************************************************************/


#include <stdio.h>
#include <SDL2/SDL.h>

#include "WL.h"
#include "SDLTWE.h"


/****************************************************************************\
* SDLTWE_DrawTextWindowFrame                                                 *
*                                                                            *
* Draw a frame around a text window.                                         *
* The caller is responsible for sensible values!                             *
\****************************************************************************/
void SDLTWE_DrawTextWindowFrame(struct TextWindowStruct *TW, int BorderWidth, color_t Color)
{
    Uint8 r;
    Uint8 g;
    Uint8 b;
    Uint8 a;
    SDL_Rect rect;
    
    SDL_GetRenderDrawColor(renPtr, &r, &g, &b, &a); // save prev color
    SDL_SetRenderDrawColor(renPtr, components(Color));
    for (int i=1; i<=BorderWidth; i++) {
       rect.x = TW->rect.x - i;
       rect.y = TW->rect.y - i;
       rect.w = TW->rect.w + 2*i;
       rect.h = TW->rect.h + 2*i;
       SDL_RenderDrawRect(renPtr, &rect);
    }
    SDL_SetRenderDrawColor(renPtr, r, g, b, a); // restore prev color
}


/****************************************************************************\
* SDLTWE_Setpixel                                                            *
*                                                                            *
\****************************************************************************/
void SDLTWE_SetPixel(struct TextWindowStruct *TW, int x, int y, color_t color)
{
   if (x>=TW->rect.w) return;
   if (y>=TW->rect.h) return;
   TW->framePtr[y*(TW->rect.w) + x] = color;
}


/****************************************************************************\
* SDLTWE_VerticalScrollUpOneLine                                             *
*                                                                            *
\****************************************************************************/
void SDLTWE_VerticalScrollUpOneLine(struct TextWindowStruct *TW, struct CharSetStruct *CS, color_t paper)
{
    int x, y;

    //size_t plines = TW->rect.h - CS->Height; // pixel lines number - 1 char line
    //memmove (&TW->framePtr[0], &TW->framePtr[CS->Height * TW->rect.w], plines * TW->rect.w * BYTESPERPIXEL); // move up Frame
    int sp, dp;
    for (Uint8 l=1; l<TW->rect.h/CS->Height; l++) { // lines 1 to 63
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
*   \f    (used as CLS)                                                      *
\****************************************************************************/
void SDLTWE_PrintCharTextWindow(struct TextWindowStruct *TW, char a, struct CharSetStruct *CS, color_t ink, color_t paper)
{
    int i, j;
    int CharIndex;
    byte mask;
    Uint32 pixm;

    if (WL_DEBUG)
    {
        if ((CS->Width != 8) || (CS->Height != 8))
        {
            fprintf(stderr, "SDLTWE: ERROR in PrintCharTextWindow. Only 8x8 pixel charsets are supported.\n");
            return;
        }
    }

    if (a == '\f')
    {
        for (i = 0; i < (TW->rect.h / CS->Height); i++) { // # of char lines
            SDLTWE_VerticalScrollUpOneLine(TW, CS, paper);
            SDL_UpdateTexture (TW->texPtr, NULL, TW->framePtr, TW->pitch); // copy Frame to Texture
            SDL_RenderCopy (renPtr, TW->texPtr, NULL, &TW->rect); // Texture to renderer
            SDL_RenderPresent (renPtr);
            SDL_Delay (12); // at least 12 ms to avoid flickering
        }
        TW->CurrentPrintPosX = 0;
        TW->CurrentPrintPosY = 0;
        return;
    }


    if (TW->CurrentPrintPosY >= TW->rect.h)
    {
        SDLTWE_VerticalScrollUpOneLine(TW, CS, paper);
        TW->CurrentPrintPosX = 0;
        TW->CurrentPrintPosY -= CS->Height;
    }


    if (a == '\n' || a == '\r') // 0x0A or 0x0D
    {
        TW->CurrentPrintPosX = 0;
        TW->CurrentPrintPosY += CS->Height;
        return;
    }


    // if required substitute characters not contained in the character set
    if ((a < CS->CharMin) || (a > CS->CharMax))
    {
        if (CS->CharSubstitute)
            a = CS->CharSubstitute;
        else
            return;
    }

    for (i = 0; i <= 7; i++)
    {
        mask = 0x80;
        CharIndex = (a - CS->CharMin) * CS->Height;
        for (j = 0; j <= 7; j++, mask >>= 1)
        {
            if (CS->Bitmap[CharIndex + i] & mask)
                pixm = ink;
            else
                pixm = paper;
            TW->framePtr[(TW->CurrentPrintPosY + i) * TW->rect.w + TW->CurrentPrintPosX + j] = pixm;
        }
    }

    TW->CurrentPrintPosX += CS->Width;
    if (TW->CurrentPrintPosX >= TW->rect.w)
    {
        TW->CurrentPrintPosX = 0;
        TW->CurrentPrintPosY += CS->Height;
    }
}


/****************************************************************************\
* SDLTWE_PrintString                                                         *
*                                                                            *
\****************************************************************************/
void SDLTWE_PrintString(struct TextWindowStruct *TW, char *ps, struct CharSetStruct *CS, color_t ink, color_t paper)
{
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
byte *SDLTWE_ReadCharSet(char *CharSetFilename, byte *CharSet)
{
    FILE *fp;
    long int FileLength;
    size_t BytesRead;

    fp = fopen(CharSetFilename, "rb");
    if (!fp)
    {
        fprintf(stderr, "ERROR in SDLWTE.c: can't open character set file '%s'\n", CharSetFilename);
        return (NULL);
    }

    fseek(fp, 0, SEEK_END);
    FileLength = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (WL_DEBUG)
    {
        printf("SDLWTE: Reading character set with %li byte from %s ... ", FileLength, CharSetFilename);
    }

    BytesRead = fread(CharSet, 1, FileLength, fp);

    fclose(fp);

    if (FileLength != BytesRead)
    {
        if (WL_DEBUG)
            printf("ERROR!\n");
        return (NULL);
    }
    else
    {
        if (WL_DEBUG)
            printf("read!\n");
        return (CharSet);
    }
}
