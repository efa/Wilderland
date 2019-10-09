/****************************************************************************\
*                                                                            *
*                                Spectrum.c                                  *
*                                                                            *
* Spectrum specific subroutines for the WL project.                          *
*                                                                            *
* (c) 2012-2019 by CH, Copyright 2019 Valerio Messina                        *
*                                                                            *
* V 1.08 - 20191005                                                          *
*                                                                            *
\****************************************************************************/


/*** INCLUDES ***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>

#include "SPECTRUM.h"
#include "WL.h"
#include "SDLTWE.h"

/*** GLOBAL VARIABLES *******************************************************/
#include "GLOBAL_VARS.h"


color_t ColorTable[] =
{ SC_BLACK, SC_BLUE, SC_RED, SC_MAGENTA, SC_GREEN, SC_CYAN, SC_YELLOW, SC_WHITE,
    SC_BRBLACK, SC_BRBLUE, SC_BRRED, SC_BRMAGENTA, SC_BRGREEN, SC_BRCYAN, SC_BRYELLOW, SC_BRWHITE
};

SHRP shrp[] =  // Spectrum half row port addresses
{ SHRP_cV, SHRP_AG, SHRP_QT, SHRP_15, SHRP_06, SHRP_PY, SHRP_eH, SHRP_sB };


/****************************************************************************\
* SetQuadPixel                                                               *
*                                                                            *
\****************************************************************************/
void SetQuadPixel(struct TextWindowStruct *TW, int x, int y, color_t color)
{
    Uint32 *pixmem32;

                      // TW->framePtr[y*(TW->rect.w) + x] = color;
    pixmem32 = (Uint32 *)TW->framePtr + 2 * (y * TW->rect.w) + 2 * x;
    *pixmem32++ = color;
    *pixmem32 = color;

    if (NoScanLines)
    {
        pixmem32 += TW->rect.w; //pitch / BYTESPERPIXEL : below 1 pixel line
        *pixmem32-- = color;
        *pixmem32 = color;
    }
//#if 0
    SDLTWE_SetPixel(TW, 2*x, 2*y, color);
    SDLTWE_SetPixel(TW, 2*x+1, 2*y, color);
    if (NoScanLines) {
       SDLTWE_SetPixel(TW, 2*x, 2*y+1, color);
       SDLTWE_SetPixel(TW, 2*x+1, 2*y+1, color);
    }
//#endif
}


/****************************************************************************\
* WriteScreenByte update Game Texture with Spectrum frame buffer             *
*                                                                            *
\****************************************************************************/
void WriteScreenByte(word address, byte v)
{
    int i;
    byte xx, yy, mask, attr, BRIGHT;
    color_t ColI, ColP;

    if (address < SL_SCREENSTART || address > SL_SCREENEND)
        return;

    xx = (byte) (address & 0x001F);
    yy = (byte) (((address & 0x1800) >> 5) | ((address & 0x0700) >> 8) | ((address & 0x00E0) >> 2));
    attr = ZXmem[SL_ATTRIBUTESTART + xx + ((yy & 0xF8) << 2)];
    BRIGHT = (attr & 0x40) >> 3;
    ColI = ColorTable[(attr & 0x07) + BRIGHT];
    ColP = ColorTable[((attr & 0x38) >> 3) + BRIGHT];

    mask = 0x80;
    for (i = 0; i <= 7; i++, mask >>= 1)
    {
        //SetQuadPixel(&GameWin, GAMEWINPOSX / 2 + (xx << 3) + i, GAMEWINPOSY / 2 + yy, (v & mask) ? ColI : ColP);
        SetQuadPixel(&GameWin, (xx << 3) + i, yy, (v & mask) ? ColI : ColP);
    }
}


/****************************************************************************\
* WriteAttributeByte                                                         *
*                                                                            *
\****************************************************************************/
void WriteAttributeByte(word address, byte v)
{
    word i, base_adr;
    word x, y;

    if (address < SL_ATTRIBUTESTART || address > SL_ATTRIBUTEEND)
        return;

    y = ((address - SL_ATTRIBUTESTART) / 32) * 8;
    x = (address - SL_ATTRIBUTESTART) % 32;
    base_adr = SL_SCREENSTART + (((y & 0xC0) << 5) | ((y & 0x38) << 2) | ((y & 0x03) << 8));
    base_adr = (base_adr + x) & 0xF8FF;

    for (i = 0; i <= 0x700; i += 0x100)
        WriteScreenByte(base_adr + i, ZXmem[base_adr + i]);
}


/****************************************************************************\
* WrZ80                                                                      *
*                                                                            *
\****************************************************************************/
void WrZ80(word A, byte v)
{
    if (A <= SL_ROMEND) // skip write to the 16 k ROM
        return;
    ZXmem[A] = v; // update Spectrum memory
    if (A <= SL_SCREENEND) // write to the frame buffer
    {
        WriteScreenByte(A, v); // update SDL screen
        return;
    }
    if (A <= SL_ATTRIBUTEEND) // write to the attrib buffer
    {
        WriteAttributeByte(A, v); // update SDL screen
        return;
    }
}


/****************************************************************************\
* RdZ80                                                                      *
*                                                                            *
\****************************************************************************/
byte RdZ80(word A)
{
    return (ZXmem[A]);
}


#ifdef __LCC__
#pragma optimize(none)
#endif
/****************************************************************************\
* InZ80                                                                      *
*                                                                            *
* Simulates a basic keyboard interface.                                      *
*                                                                            *
* In Pelles C 6.50.8 optimiziation 'time' causes a hard to detect optimi-    *
* zation-error. Therefore optimizition is turned off for this function with  *
* the private #pragma optimize(none) instruction!                            *
\****************************************************************************/
byte InZ80(word P)
{
    if ((P & 0xFF) != 0xFE) // we only support the keyboard port 0xFE
    {
        printf("Spectrum.c: InZ80() tried to read from unsupported port %04X.\n", P);
        return (0xFF);
    }


    if (P == 0x00FE)    // all keyboard rows at once (the upper 8 bit select the half row)
    {
        if (CurrentPressedKey)
        {
            switch (CurrentPressedKey)
            {
                case 'a':
                case 'q':
                case '1':
                case '0':
                case 'p':
                case SDLK_RETURN:
                case ' ':
                    return (0xFF - 0x01);
                case 'z':
                case 's':
                case 'w':
                case '2':
                case '9':
                case 'o':
                case 'l':
                    return (0xFF - 0x02);
                case 'x':
                case 'd':
                case 'e':
                case '3':
                case '8':
                case 'i':
                case 'k':
                case 'm':
                    return (0xFF - 0x04);
                case 'c':
                case 'f':
                case 'r':
                case '4':
                case '7':
                case 'u':
                case 'j':
                case 'n':
                    return (0xFF - 0x08);
                case 'v':
                case 'g':
                case 't':
                case '5':
                case '6':
                case 'y':
                case 'h':
                case 'b':
                    return (0xFF - 0x10);

                default:
                    return (0xFF);
            }
        }

        else

        {
            return (0xFF);  //no key pressed
        }
    }


    P >>= 8;

    switch (CurrentPressedKey)
    {
        case 'z':
            if (P == SHRP_cV)
                return (0xFF - 0x02);
            break;
        case 'x':
            if (P == SHRP_cV)
                return (0xFF - 0x04);
            break;
        case 'c':
            if (P == SHRP_cV)
                return (0xFF - 0x08);
            break;
        case 'v':
            if (P == SHRP_cV)
                return (0xFF - 0x10);
            break;

        case 'a':
            if (P == SHRP_AG)
                return (0xFF - 0x01);
            break;
        case 's':
            if (P == SHRP_AG)
                return (0xFF - 0x02);
            break;
        case 'd':
            if (P == SHRP_AG)
                return (0xFF - 0x04);
            break;
        case 'f':
            if (P == SHRP_AG)
                return (0xFF - 0x08);
            break;
        case 'g':
            if (P == SHRP_AG)
                return (0xFF - 0x10);
            break;

        case 'q':
            if (P == SHRP_QT)
                return (0xFF - 0x01);
            break;
        case 'w':
            if (P == SHRP_QT)
                return (0xFF - 0x02);
            break;
        case 'e':
            if (P == SHRP_QT)
                return (0xFF - 0x04);
            break;
        case 'r':
            if (P == SHRP_QT)
                return (0xFF - 0x08);
            break;
        case 't':
            if (P == SHRP_QT)
                return (0xFF - 0x10);
            break;

        case '1':
            if (P == SHRP_15)
                return (0xFF - 0x01);
            break;
        case '2':
            if (P == SHRP_15)
                return (0xFF - 0x02);
            break;
        case '@':
            if (P == SHRP_15)
                return (0xFF - 0x02);
            if (P == SHRP_sB)
                return (0xFF - 0x02);
            break;
        case '3':
            if (P == SHRP_15)
                return (0xFF - 0x04);
            break;
        case '4':
            if (P == SHRP_15)
                return (0xFF - 0x08);
            break;
        case '5':
            if (P == SHRP_15)
                return (0xFF - 0x10);
            break;

        case '0':
            if (P == SHRP_06)
                return (0xFF - 0x01);
            break;
        case '9':
            if (P == SHRP_06)
                return (0xFF - 0x02);
            break;
        case '8':
            if (P == SHRP_06)
                return (0xFF - 0x04);
            break;
        case '7':
            if (P == SHRP_06)
                return (0xFF - 0x08);
            break;
        case '6':
            if (P == SHRP_06)
                return (0xFF - 0x10);
            break;

        case 'p':
            if (P == SHRP_PY)
                return (0xFF - 0x01);
            break;
        case '"':
            if (P == SHRP_PY)
                return (0xFF - 0x01);
            if (P == SHRP_sB)
                return (0xFF - 0x02);
            break;
        case 'o':
            if (P == SHRP_PY)
                return (0xFF - 0x02);
            break;
        case 'i':
            if (P == SHRP_PY)
                return (0xFF - 0x04);
            break;
        case 'u':
            if (P == SHRP_PY)
                return (0xFF - 0x08);
            break;
        case 'y':
            if (P == SHRP_PY)
                return (0xFF - 0x10);
            break;

        case SDLK_RETURN:
            if (P == SHRP_eH)
                return (0xFF - 0x01);
            break;
        case 'l':
            if (P == SHRP_eH)
                return (0xFF - 0x02);
            break;
        case 'k':
            if (P == SHRP_eH)
                return (0xFF - 0x04);
            break;
        case 'j':
            if (P == SHRP_eH)
                return (0xFF - 0x08);
            break;
        case 'h':
            if (P == SHRP_eH)
                return (0xFF - 0x10);
            break;

        case ' ':
            if (P == SHRP_sB)
                return (0xFF - 0x01);
            break;
        case 'm':
            if (P == SHRP_sB)
                return (0xFF - 0x04);
            break;
        case '.':
            if (P == SHRP_sB)
                return (0xFF - 0x04 - 0x02);
            break;
        case 'n':
            if (P == SHRP_sB)
                return (0xFF - 0x08);
            break;
        case ',':
            if (P == SHRP_sB)
                return (0xFF - 0x08 - 0x02);
            break;
        case 'b':
            if (P == SHRP_sB)
                return (0xFF - 0x10);
            break;

        default:
            return (0xFF);
    }

    return (0xFF);
}
#ifdef __LCC__
#pragma optimize(time)
#endif


/****************************************************************************\
* OutZ80                                                                     *
*                                                                            *
\****************************************************************************/
void OutZ80(word P, byte v)
{
    return;
}


/****************************************************************************\
* LoopZ80                                                                    *
*                                                                            *
\****************************************************************************/
word LoopZ80(register Z80 *R)
{
    return 0;
}


/****************************************************************************\
* PatchZ80                                                                   *
*                                                                            *
\****************************************************************************/
void PatchZ80(register Z80 *R)
{
    return;
}


/****************************************************************************\
* PrintObjNrActingAnimal                                                     *
*                                                                            *
\****************************************************************************/
void PrintObjNrActingAnimal (byte onaa)
{
char aa[20];

    strcpy(aa, "[");
    sprintf(aa+1, "%02X", onaa);
    strcat(aa,"]:");

    SDLTWE_PrintString(&LogWin, aa, &CharSet, SC_BLACK, SC_WHITE);
}


/****************************************************************************\
* JumpZ80                                                                    *
*                                                                            *
\****************************************************************************/
void JumpZ80(word PC)
{
    static char LastChar = 0;

    switch (HV) // to make this fast...
    {

        case OWN:
            if (PC == L_PRINT_CHAR_OWN)
            {
                if (LastChar == 0x0D)
                {
                    LastChar = 0;
                    PrintObjNrActingAnimal(ZXmem[L_ACTING_ANIMAL_OWN]);
                }

                if (!ZXmem[L_SELECT_PRINTWIN_OWN])    // only upper window
                {
                    SDLTWE_PrintCharTextWindow(&LogWin, z80.AF.B.h, &CharSet, SC_BLACK, SC_WHITE);
                    LastChar = z80.AF.B.h;
                }
            }
            break;

        case V10:
            if (PC == L_PRINT_CHAR_V10)
            {
                if (LastChar == 0x0D)
                {
                    LastChar = 0;
                    PrintObjNrActingAnimal(ZXmem[L_ACTING_ANIMAL_V10]);
                }

                if (!ZXmem[L_SELECT_PRINTWIN_V10])    // only upper window
                {
                    SDLTWE_PrintCharTextWindow(&LogWin, z80.AF.B.h, &CharSet, SC_BLACK, SC_WHITE);
                    LastChar = z80.AF.B.h;
                }
            }
            break;

        case V12:
            if (PC == L_PRINT_CHAR_V12)
            {
                if (LastChar == 0x0D)
                {
                    LastChar = 0;
                    PrintObjNrActingAnimal(ZXmem[L_ACTING_ANIMAL_V12]);
                }

                if (!ZXmem[L_SELECT_PRINTWIN_V12])    // only upper window
                {
                    SDLTWE_PrintCharTextWindow(&LogWin, z80.AF.B.h, &CharSet, SC_BLACK, SC_WHITE);
                    LastChar = z80.AF.B.h;
                }
            }
            break;
    }
}


