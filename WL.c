/****************************************************************************\
*                                                                            *
*                                   WL.c                                     *
*                                                                            *
* Wilderland - A Hobbit Environment                                          *
*                                                                            *
* (c) 2012-2019 by CH, Contact: wilderland@aon.at                            *
* Copyright 2019-2021 Valerio Messina efa@iol.it                             *
*                                                                            *
* Simple Direct Media Layer library (SDL 2.0) from www.libsdl.org (LGPL)     *
* Z80 emulator based on Marat Fayzullin's work from 2007 (fms.komkon.org)    *
* 8x8 character set from ZX Spectrum ROM (c) by Amstrad, PD for emulators    *
*                                                                            *
* Compiler: Pelles C for Windows 6.5.80 with 'Microsoft Extensions' enabled, *
*           GCC, MinGW/Msys2, Clang/LLVM                                     *
*                                                                            *
* V 1.08 - 20210905                                                          *
*                                                                            *
\****************************************************************************/


/*** INCLUDES ***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "WL.h"
#include "SDLTWE.h"
#include "SPECTRUM.h"
#include "Z80/Z80.h"
#include "MapCoordinates.h"

/*** GLOBAL VARIABLES *******************************************************/
#include "GLOBAL_VARS.h"

#define WLVER "1.08"

int HV;      //Hobbit version

word DictionaryBaseAddress;
word ObjectsIndexAddress, ObjectsAddress;

SDL_Window*   winPtr;
SDL_Renderer* renPtr;
SDL_Surface*  GameMapSfcPtr;

int NoScanLines;
int LockLevel;
struct CharSetStruct CharSet;

struct TextWindowStruct GameWin;  // dummy to draw frame
struct TextWindowStruct LogWin;
struct TextWindowStruct ObjWin;
struct TextWindowStruct HelpWin;
struct TextWindowStruct MapWin;

Z80 z80;                   // my Z80 processor
byte ZXmem[0x10000];       // Spectrum 64 kB memory

Uint16 CurrentPressedKey;  // is used in InZ80()
Uint16 CurrentPressedMod;  // used by InZ80()

//extern color_t ColorTable[];    //defined in Spectrum.c
int delay=22; // min 22 ms to avoid flickering


/****************************************************************************\
* LockScreen                                                                 *
*                                                                            *
\****************************************************************************/
void LockScreen(SDL_Window *LSscreen)
{
    if (LockLevel)
        return;

    LockLevel++;
#if 0
    if (SDL_MUSTLOCK(LSscreen))
    {
        if (SDL_LockSurface(LSscreen) < 0)
            fprintf(stderr, "WL: ERROR in LockScreen().\n");
    }
#endif
}


/****************************************************************************\
* UnLockScreen                                                               *
*                                                                            *
\****************************************************************************/
void UnLockScreen(SDL_Window *ULSscreen)
{

    if (LockLevel-- >= 2)
    {
        //SDL_RenderPresent (renPtr);
        return;
    }

    if (LockLevel != 1)
    {
        //SDL_RenderPresent (renPtr);
        return;
    }

#if 0
    if (SDL_MUSTLOCK(ULSscreen))
        SDL_UnlockSurface(ULSscreen);
#endif

    //SDL_RenderPresent (renPtr);
}


/****************************************************************************\
* Setpixel on map                                                            *
*                                                                            *
\****************************************************************************/
void SetPixel(Uint32* framePtr, int x, int y, color_t color)
{
    if (x>=MAPWINWIDTH) return;
    if (y>=MAPWINHEIGHT) return;
    //pixmem32 = (Uint32*) y * / BYTESPERPIXEL + x;
    // *pixmem32 = color;
    framePtr[y*MAPWINWIDTH + x] = color;
}


/****************************************************************************\
* DrawLineRelative: x,y start; dx,dy offset to end; rx,ry thick; color       *
*                                                                            *
\****************************************************************************/
void DrawLineRelative(Uint32* framePtr, int x, int y, int dx, int dy, int rx, int ry, color_t color)
{
    int x1, y1, sx, sy;

    if (rx == 0 || ry == 0)
        return;

    sx = (dx > 0) ? 1 : -1;
    dx *= sx;

    sy = (dy > 0) ? 1 : -1;
    dy *= sy;

    while (rx > 1 || ry > 1)
    {
        if (rx > 1)
            rx--;
        if (ry > 1)
            ry--;
        if (dx > dy)
            for (x1 = 0; x1 < dx; x1++)
                SetPixel(framePtr, x + sx * (x1 + (rx - 1)), sy * (y + ((int)(dy * x1 / dx)) + (ry - 1)), color);
        else
            for (y1 = 0; y1 < dy; y1++)
                SetPixel(framePtr, x + sx * (((int)(dx * y1 / dy)) + (rx - 1)), y + sy * (y1 + (ry - 1)), color);
    }
}


/****************************************************************************\
* PrintChar                                                                  *
*                                                                            *
\****************************************************************************/
void PrintChar(Uint32* framePtr, struct CharSetStruct *cs, char a, int x, int y, color_t ink, color_t paper)
{
    int i, j;
    int CharIndex;
    byte mask;
    int ymul;
    Uint32 *pixm;

    if (WL_DEBUG)
    {
        if ((cs->Width != 8) || (cs->Height != 8))
        {
            fprintf(stderr, "WL: ERROR in PrintChar. Only 8x8 pixel charsets are supported.\n");
            return;
        }
    }


    ymul = MAPWINWIDTH; // scr->pitch / BYTESPERPIXEL;
    for (i = 0; i <= 7; i++)
    {
        mask = 0x80;
        CharIndex = (a - cs->CharMin) * cs->Height;
        for (j = 0; j <= 7; j++, mask >>= 1)
        {
            pixm = (Uint32 *) framePtr + (y + i) * ymul + x + j;
            if (cs->Bitmap[CharIndex + i] & mask)
                *pixm = ink;
            else
                *pixm = paper;
        }
    }
}


/****************************************************************************\
* PrintString                                                                *
*                                                                            *
\****************************************************************************/
void PrintString(Uint32* framePtr, struct CharSetStruct *cs, char *ps, int x, int y, color_t ink, color_t paper)
{
    while (*ps)
    {
        PrintChar(framePtr, cs, *ps++, x, y, ink, paper);
        x += cs->Width;
    }
}


/****************************************************************************\
* SearchByteWordTable                                                        *
*                                                                            *
* Returns pointer value or 0 if not found.                                   *
\****************************************************************************/
word SearchByteWordTable(byte a, word address)
{
    while (ZXmem[address] != 0xFF)
    {
        if (ZXmem[address] == a)
            return (ZXmem[address + 1] + 0x100 * ZXmem[address + 2]);
        else
            address += 3;
    }
    return (0);
}


/****************************************************************************\
* GetObjectAttributePointer                                                  *
*                                                                            *
* Returns pointer value or 0 if not found.                                   *
\****************************************************************************/
word GetObjectAttributePointer(byte a, byte attributeoffset)
{
    word OAP;

    OAP = SearchByteWordTable(a, ObjectsIndexAddress);
    if (OAP)
        return (OAP + attributeoffset);
    else
        return (0);
}


/****************************************************************************\
* DrawAnimalPositon                                                          *
*                                                                            *
\****************************************************************************/
void DrawAnimalPosition(Uint32* framePtr, struct CharSetStruct *cs, byte animalnr, word x, word y, byte objectoffset)
{
    char initials[10];

    DrawLineRelative(framePtr, x, y + objectoffset*15, 10, -30, 4, 1, SC_BRRED);

    strcpy(initials, AnimalInitials[animalnr]);
    if (ZXmem[GetObjectAttributePointer(animalnr, ATTRIBUTE_OFF)] & ATTR_BROKEN)
        strcat(initials, "+");
    if (animalnr == OBJNR_YOU || animalnr == OBJNR_GANDALF || animalnr == OBJNR_THORIN)
        PrintString(framePtr, cs, initials, x + 13, y + objectoffset*15 - 33, SC_BRRED, 0x00000000ul);
    else
        PrintString(framePtr, cs, initials, x + 13, y + objectoffset*15 - 33, SC_BRGREEN, 0x00000000ul);
}


/****************************************************************************\
* PrepareOneAnimalPositon                                                    *
*                                                                            *
\****************************************************************************/
void PrepareOneAnimalPosition(Uint32* framePtr, struct CharSetStruct *cs, byte AnimalNr, byte *objectsperroom)
{
    byte CurrentAnimalRoom;
    int i;

    CurrentAnimalRoom = ZXmem[GetObjectAttributePointer(AnimalNr, FIRST_OCCURRENCE)];

    for(i = 0; i<ROOMS_NROF_MAX; i++)
    {
        if (MapCoordinates[i].RoomNumber == CurrentAnimalRoom)
        {
            DrawAnimalPosition(framePtr, cs, AnimalNr, MapCoordinates[i].X - INDICATOROFFSET,
              //MapCoordinates[i].Y + (MAINWINHEIGHT - GAMEMAPHEIGHT) + INDICATOROFFSET,
                MapCoordinates[i].Y + INDICATOROFFSET,
                objectsperroom[CurrentAnimalRoom]);
            objectsperroom[CurrentAnimalRoom]++;
        }
    }
}


/****************************************************************************\
* PrepareAnimalPositions                                                     *
*                                                                            *
\****************************************************************************/
void PrepareAnimalPositions(Uint32* framePtr, struct CharSetStruct *cs, byte *objectsperroom)
{
    byte AnimalNr;

    PrepareOneAnimalPosition(framePtr, cs, OBJNR_YOU, objectsperroom);

    for(AnimalNr = OBJNR_DRAGON; AnimalNr <= OBJNR_DISGUSTINGGOBLIN; AnimalNr++)
            PrepareOneAnimalPosition(framePtr, cs, AnimalNr, objectsperroom);
}


/****************************************************************************\
* ShowTextWindow                                                             *
*                                                                            *
\****************************************************************************/
void ShowTextWindow (struct TextWindowStruct* TW) {
   SDL_UpdateTexture (TW->texPtr, NULL, TW->framePtr, TW->pitch); // Frame to Texture
   SDL_RenderCopy (renPtr, TW->texPtr, NULL, &TW->rect); // Texture to Renderer
   SDL_RenderPresent (renPtr); // to screen
   SDL_Delay (14); // at least 14 ms to avoid flickering
}


/****************************************************************************\
* GetHexByte                                                                 *
*                                                                            *
\****************************************************************************/
void GetHexByte(byte *b, struct TextWindowStruct *TW, struct CharSetStruct *CS, color_t ink, color_t paper)
{
    Uint16 a;
    SDL_Event event;
    int nibble_count = 0;

    *b = 0;

    while (1)
    {
        if (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_KEYDOWN:
                    a = event.key.keysym.sym;
                    if (a >= '0' && a <= '9')
                    {
                        if (nibble_count >= 2) break;
                        *b <<= 4;
                        *b += (a - '0');
                        nibble_count++;
                        LockScreen(winPtr);
                        SDLTWE_PrintCharTextWindow(TW, a, CS, ink, paper);
                        ShowTextWindow (TW);
                        UnLockScreen(winPtr);
                        break;
                    }
                    if ((a >= 'a' && a <= 'f') || (a >= 'A' && a <= 'F'))
                    {
                        if (nibble_count >= 2) break;
                        a |= 0x20;
                        *b <<= 4;
                        *b += (a - 'a' + 0x0a);
                        nibble_count++;
                        LockScreen(winPtr);
                        SDLTWE_PrintCharTextWindow(TW, a, CS, ink, paper);
                        ShowTextWindow (TW);
                        UnLockScreen(winPtr);
                        break;
                    }
                    if (a == SDLK_BACKSPACE)
                        break;
                    if (a == SDLK_RETURN)
                    {
                        return;
                    }
                case SDL_KEYUP:
                    a = 0;
                    break;
            }
        }
    }
}


/****************************************************************************\
* GetFileName                                                                *
*                                                                            *
\****************************************************************************/
void GetFileName(char *fnstr, struct TextWindowStruct *TW, struct CharSetStruct *CS, color_t ink, color_t paper)
{
    Uint16 a;
    SDL_Event event;

    LockScreen(winPtr);
    SDLTWE_PrintString(TW, "\n\nFilename: ", CS, ink, paper);
    ShowTextWindow (TW);
    UnLockScreen(winPtr);

    while (1)
    {
        if (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_KEYDOWN:
                    a = event.key.keysym.sym;
                    if ((a >= 'a' && a <= 'z') || (a >= '0' && a <= '9') || a == '.' || a == '_')
                    {
                        *fnstr = a;
                        fnstr++;
                        LockScreen(winPtr);
                        SDLTWE_PrintCharTextWindow(TW, a, CS, ink, paper);
                        ShowTextWindow (TW);
                        UnLockScreen(winPtr);
                        break;
                    }
                    if (a == SDLK_BACKSPACE)
                        break;
                    if (a == SDLK_RETURN)
                    {
                        *fnstr = 0;
                        return;
                    }
                case SDL_KEYUP:
                    a = 0;
                    break;
            }
        }
    }
}


/****************************************************************************\
* SaveGame                                                                   *
*                                                                            *
\****************************************************************************/
void SaveGame(struct TextWindowStruct *TW, struct CharSetStruct *CS, int hv, color_t ink, color_t paper)
{
    char fn[100];
    FILE *f;
    int i;

    GetFileName(fn, TW, CS, ink, paper);

    f = fopen(fn, "wb");
    if (!f)
    {
        fprintf(stderr, "WL: ERROR - Can't open file '%s' for saving game.\n", fn);
        return;
    }

    fprintf(f, "GAMEVERSION=%i\r\n", hv);
    fprintf(f, "FILEVERSION=1\r\n");
    fprintf(f, "SIZE(Z80)=%02zX\r\n", sizeof(z80));
    fprintf(f, "PC=%04X\r\n", z80.PC.W);

    fprintf(f, "***Z80***\n");
    for (i = 0; i < sizeof(z80); i++)
        fprintf(f, "%c", *((byte *) & z80 + i));

    fprintf(f, "\n");
    fprintf(f, "***MEMORY***\n");
    for (i = 0; i <= 0xFFFF; i++)
        fprintf(f, "%c", ZXmem[i]);

    fclose(f);

    LockScreen(winPtr);
    SDLTWE_PrintString(TW, " saved.\n\n", CS, ink, paper);
    ShowTextWindow (TW);
    UnLockScreen(winPtr);
}


/****************************************************************************\
* LoadGame                                                                   *
*                                                                            *
\****************************************************************************/
void LoadGame(struct TextWindowStruct *TW, struct CharSetStruct *CS, int hv, color_t ink, color_t paper)
{
    char fn[100];
    FILE *f;
    int i, gameversion, fileversion, SizeOfZ80;
    char* temp;

    GetFileName(fn, TW, CS, ink, paper);

    f = fopen(fn, "rb");
    if (!f)
    {
        fprintf(stderr, "WL: ERROR - Can't open file '%s' for loading game.\n", fn);
        LockScreen(winPtr);
        SDLTWE_PrintString(TW, " not found!\n\n", CS, ink, paper);
        ShowTextWindow (TW);
        UnLockScreen(winPtr);
        return;
    }

    if (fscanf(f, "GAMEVERSION=%i\r\n", &gameversion) != 1)
    {
        LockScreen(winPtr);
        SDLTWE_PrintString(TW, "\nError. Data does not match format.\n\n", CS, ink, paper);
        ShowTextWindow (TW);
        UnLockScreen(winPtr);
        return;
    }

    if (gameversion != hv)
    {
        LockScreen(winPtr);
        SDLTWE_PrintString(TW, "\nWrong game version.\n\n", CS, ink, paper);
        ShowTextWindow (TW);
        UnLockScreen(winPtr);
        return;
    }

    if (fscanf(f, "FILEVERSION=%i\r\n", &fileversion) != 1)
    {
        LockScreen(winPtr);
        SDLTWE_PrintString(TW, "\nError. File version does not match format.\n\n", CS, ink, paper);
        ShowTextWindow (TW);
        UnLockScreen(winPtr);
        return;
    }

    if (fscanf(f, "SIZE(Z80)=%02X\r\n", &SizeOfZ80) != 1)
    {
        LockScreen(winPtr);
        SDLTWE_PrintString(TW, "\nError. SIZE(Z80) does not match format.\n\n", CS, ink, paper);
        ShowTextWindow (TW);
        UnLockScreen(winPtr);
        return;
    }

    if (SizeOfZ80 != sizeof(z80))
    {
        LockScreen(winPtr);
        SDLTWE_PrintString(TW, "\nZ80-structure sizes do not match.\n\n", CS, ink, paper);
        ShowTextWindow (TW);
        UnLockScreen(winPtr);
        return;
    }

    if (fileversion == 1)
    {
        if (fscanf(f, "PC=%04hX\r\n", &z80.PC.W) != 1)
        {
           LockScreen(winPtr);
           SDLTWE_PrintString(TW, "\nError. Fileversion does not match format.\n\n", CS, ink, paper);
           ShowTextWindow (TW);
           UnLockScreen(winPtr);
           return;
        }
        if (fscanf(f, "%ms\n", &temp) != 1) // should be "***Z80***"
        {
           LockScreen(winPtr);
           SDLTWE_PrintString(TW, "\nError. ***Z80*** does not match format.\n\n", CS, ink, paper);
           ShowTextWindow (TW);
           UnLockScreen(winPtr);
           return;
        }
        //printf ("'%s'\n", temp);
        if (strcmp (temp, "***Z80***") != 0) {
           LockScreen(winPtr);
           SDLTWE_PrintString(TW, "\nError. ***Z80*** missing\n\n", CS, ink, paper);
           ShowTextWindow (TW);
           UnLockScreen(winPtr);
           return;
        }
        free (temp);
        for (i = 0; i < sizeof(z80); i++)
            if (fscanf(f, "%c", ((byte *) & z80 + i)) != 1)
            {
               LockScreen(winPtr);
               SDLTWE_PrintString(TW, "\nError. %%c does not match format.\n\n", CS, ink, paper);
               ShowTextWindow (TW);
               UnLockScreen(winPtr);
               return;
            }
        if (fscanf(f, "%ms\n", &temp) != 1) // should be "***MEMORY***"
        {
           LockScreen(winPtr);
           SDLTWE_PrintString(TW, "\nError. ***MEMORY*** does not match format.\n\n", CS, ink, paper);
           ShowTextWindow (TW);
           UnLockScreen(winPtr);
           return;
        }
        //printf ("'%s'\n", temp);
        if (strcmp (temp, "***MEMORY***") != 0) {
           LockScreen(winPtr);
           SDLTWE_PrintString(TW, "\nError. ***MEMORY*** missing\n\n", CS, ink, paper);
           ShowTextWindow (TW);
           UnLockScreen(winPtr);
           return;
        }
        free (temp);
        for (i = 0; i <= 0xFFFF; i++)
            if (fscanf(f, "%c", ZXmem + i) != 1)
            {
               LockScreen(winPtr);
               SDLTWE_PrintString(TW, "\nError. %%c does not match format.\n\n", CS, ink, paper);
               ShowTextWindow (TW);
               UnLockScreen(winPtr);
               return;
            }
        fclose(f);
    }
    else
    {
        LockScreen(winPtr);
        SDLTWE_PrintString(TW, "\nThis game file version is not supported!\n\n", CS, ink, paper);
        ShowTextWindow (TW);
        UnLockScreen(winPtr);
        return;
    }

    LockScreen(winPtr);
    SDLTWE_PrintString(TW, " loaded.\n\n", CS, ink, paper);
    ShowTextWindow (TW);
    UnLockScreen(winPtr);

    for (i = SL_SCREENSTART; i <= SL_ATTRIBUTEEND; i++)
        WrZ80(i, ZXmem[i]);
}


/****************************************************************************\
* HelpInfo                                                                   *
*                                                                            *
\****************************************************************************/
void HelpInfo(struct TextWindowStruct *TW, struct CharSetStruct *CS)
{
    LockScreen(winPtr);

#if 0
    SDLTWE_PrintString(TW, "\n                         WILDERLAND V 1.08\n\n", CS, SC_BRWHITE, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, "                       A Hobbit Environment\n\n", CS, SC_BRWHITE, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, "                  (c) 2012-2019 by CH, Copyright 2019 V.Messina\n\n\n", CS, SC_WHITE, SC_BLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, " a-z.,@0[BS] -> play         ATTRIBUTES\n", CS, SC_BRMAGENTA, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, " S-ave   H-elp   G-o         v_isible  A_nimal  o_pen   *_light\n", CS, SC_BRCYAN, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, " L-oad   I-nfo               b_roken   f_ull    F_luid  l_ocked\n", CS, SC_BRCYAN, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, " Q-uit\n\n", CS, SC_BRWHITE, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, " Press 'n' as first key to play without graphics (not game 1.0).\n", CS, SC_RED, SC_BLACK);
    ShowTextWindow (TW);
#endif
    //                     "1234567890123456789012345678901234567890123456789012345678901234"
    SDLTWE_PrintString(TW, "            WILDERLAND V"WLVER" - A Hobbit Environment             ", CS, SC_BRWHITE, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, "       (c) 2012-2019 by CH, Copyright 2019-2021 V.Messina       ", CS, SC_WHITE, SC_BLACK);
    ShowTextWindow (TW);
    if (HV == OWN) SDLTWE_PrintString(TW, "                  Using The Hobbit binary VOWN                  \n\n", CS, SC_BRWHITE, SC_BRBLACK);
    if (HV == V10) SDLTWE_PrintString(TW, "                  Using The Hobbit binary V1.0                  \n\n", CS, SC_BRWHITE, SC_BRBLACK);
    if (HV == V12) SDLTWE_PrintString(TW, "                  Using The Hobbit binary V1.2                  \n\n", CS, SC_BRWHITE, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, " INFO: ", CS, SC_BRMAGENTA, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW,        "1L=place #=qty MO=parent Vo=volume Ma=mass St=strength   \n", CS, SC_BRCYAN, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, " a-z.,@0[BS] -> game         ATTRIBUTES                         ", CS, SC_BRMAGENTA, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, " S-ave   H-elp   G-o         v_isible  A_nimal  o_pen   *_light ", CS, SC_BRCYAN, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, " L-oad   I-nfo               b_roken   f_ull    F_luid  l_ocked ", CS, SC_BRCYAN, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, " Q-uit                                                          \n", CS, SC_BRWHITE, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, " Press 'n' as first key to play without graphics (not game 1.0).\n", CS, SC_RED, SC_BLACK);

    UnLockScreen(winPtr);
}


/****************************************************************************\
* Info                                                                       *
*                                                                            *
\****************************************************************************/
void Info(struct TextWindowStruct *TW, struct CharSetStruct *CS)
{
    LockScreen(winPtr);

    SDLTWE_PrintString(TW, "\nWILDERLAND - A Hobbit Environment (c) 2012-2019 by CH, 2019 VM\n\n", CS, SC_BRWHITE, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, "\"The Hobbit\" (c) Melbourne House, 1982. Written by Philip\n", CS, SC_BRMAGENTA, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, "Mitchell and Veronika Megler.\n\n", CS, SC_BRMAGENTA, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, "Simple Direct Media Layer library (SDL 2.0) from www.libsdl.org\n", CS, SC_WHITE, SC_BLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, "Z80 emulator based on Marat Fayzullin's work (fms.komkon.org)\n", CS, SC_WHITE, SC_BLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, "8x8 charset from ZX Spectrum ROM (c) by Amstrad,PD for emulators\n\n", CS, SC_WHITE, SC_BLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, "You are not allowed to distribute 'WILDERLAND' with Hobbit\nbinaries included!\n\n", CS, SC_BRWHITE, SC_BRBLACK);
    ShowTextWindow (TW);
    SDLTWE_PrintString(TW, "Contact: wilderland@aon.at\n", CS, SC_WHITE, SC_BLACK);
    ShowTextWindow (TW);

    UnLockScreen(winPtr);
}


/****************************************************************************\
* Go                                                                         *
*                                                                            *
\****************************************************************************/
void Go(struct TextWindowStruct *TW, struct CharSetStruct *CS, int hv, color_t ink, color_t paper)
{
    byte rn;

    LockScreen(winPtr);
    SDLTWE_PrintString(TW, "\n\nEnter room number 0x", CS, ink, paper);
    ShowTextWindow (TW);
    UnLockScreen(winPtr);

    GetHexByte(&rn, TW, CS, ink, paper);

    if (rn < 1 || rn > ROOMS_NR_MAX)
    {
        LockScreen(winPtr);
        SDLTWE_PrintString(TW, ". ERROR, invalid room number.\n", CS, ink, paper);
        ShowTextWindow (TW);
        UnLockScreen(winPtr);
        return;
    }

    switch (hv)
    {
        case OWN:
            ZXmem[L_YOU_OBJ_POSITION_OWN] = rn;
            break;
        case V10:
            ZXmem[L_YOU_OBJ_POSITION_V10] = rn;
            break;
        case V12:
            ZXmem[L_YOU_OBJ_POSITION_V12] = rn;
            break;
        default:
            fprintf(stderr, "ERROR in WL/Go: unknown game version.\n");
            return;
    }

    LockScreen(winPtr);
    SDLTWE_PrintString(TW, ". OK, you are now in this room.\n", CS, ink, paper);
    ShowTextWindow (TW);
    UnLockScreen(winPtr);
}


/****************************************************************************\
* sbinprintf                                                                 *
*                                                                            *
\****************************************************************************/
void sbinprintf(char *buf, byte b)
{
    int i;
    byte mask = 0x80;

    for (i = 0; i < 8; i++, mask >>= 1)
    {
        *buf++ = '0' + ((b & mask) != 0);
        if (i == 3)
            *buf++ = '.';
    }
    *buf = 0;
}


/****************************************************************************\
* GetDictWord                                                                *
*                                                                            *
\****************************************************************************/
void GetDictWord(word a, char *buf)
{
    word i = 1; // char count

    a += DictionaryBaseAddress;

    // the following line relies on evaluation from left to right
    while (!(ZXmem[a] & 0x80) || (i == 2) || ((i == 3) && (ZXmem[a - 1] & 0x80)))
    {
        if (ZXmem[a] & 0x1F)
        {
            *buf++ = (ZXmem[a] & 0x1F) + 'A' - 1;
        }
        a++;
        i++;
    }
    if (ZXmem[a] & 0x1F)
        *buf++ = (ZXmem[a] & 0x1F) + 'A' - 1;
    *buf = 0;
}


/****************************************************************************\
* GetObjectFullName                                                          *
*                                                                            *
* +0A/0B: ADJECTIVE1, +0C/0D: ADJECTIVE2, +08/09: NOUN                       *
\****************************************************************************/
void GetObjectFullName(word oa, char *OFN)
{
    word wa;
    char StringBuffer[25];

    wa = ZXmem[oa + 0x0A] + 0x100 * ZXmem[oa + 0x0B];   // adjective 1
    if (wa)
    {
        GetDictWord(wa, StringBuffer);
        strcat(OFN, StringBuffer);
        strcat(OFN, " ");
    }
    wa = ZXmem[oa + 0x0C] + 0x100 * ZXmem[oa + 0x0D];   // adjective 2
    if (wa)
    {
        GetDictWord(wa, StringBuffer);
        if (!strcmp(StringBuffer, "INSIGNIFICANT"))
            strcpy(StringBuffer, "INSIGN.");
        strcat(OFN, StringBuffer);
        strcat(OFN, " ");
    }
    wa = ZXmem[oa + 0x08] + 0x100 * ZXmem[oa + 0x09];   // noun
    if (wa & 0x0FFF)
    {
        GetDictWord(wa & 0x0FFF, StringBuffer);
        strcat(OFN, StringBuffer);
    }

}


/****************************************************************************\
* PrintObject                                                                *
*                                                                            *
\****************************************************************************/
void PrintObject(word ai, struct TextWindowStruct *OW, struct CharSetStruct *CS, color_t ink, color_t paper)
{
    word oa;
    byte ObjectNumber;
    char ObjectPrintLine[100];
    char ObjectStringBuffer[100];

    ObjectNumber = ZXmem[ai++];
    sprintf(ObjectPrintLine, "%02X ", ObjectNumber);

    oa = ZXmem[ai] + 0x100 * ZXmem[ai + 1];
    // object name
    *ObjectStringBuffer = 0;
    GetObjectFullName(oa, ObjectStringBuffer);
    strcat(ObjectStringBuffer, "                              ");
    ObjectStringBuffer[21] = 0;
    strcat(ObjectPrintLine, ObjectStringBuffer);
    // 1st occurence
    sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + FIRST_OCCURRENCE]);
    strcat(ObjectPrintLine, ObjectStringBuffer);
    // occurences
    sprintf(ObjectStringBuffer, "%1X ", ZXmem[oa + OCC_OFF]);
    strcat(ObjectPrintLine, ObjectStringBuffer);
    // mother object
    sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + MO_OFF]);
    strcat(ObjectPrintLine, ObjectStringBuffer);
    // volume
    sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + VOLUME_OFF]);
    strcat(ObjectPrintLine, ObjectStringBuffer);
    // mass
    sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + MASS_OFF]);
    strcat(ObjectPrintLine, ObjectStringBuffer);
    // +04
    sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + A4_OFF]);
    strcat(ObjectPrintLine, ObjectStringBuffer);
    // +05
    sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + A5_OFF]);
    strcat(ObjectPrintLine, ObjectStringBuffer);
    // +06
    sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + A6_OFF]);
    strcat(ObjectPrintLine, ObjectStringBuffer);
    // ATTRIBUTE
    sbinprintf(ObjectStringBuffer, ZXmem[oa + ATTRIBUTE_OFF]);
    strcat(ObjectPrintLine, ObjectStringBuffer);

    if (ObjectNumber == 0x00 || ObjectNumber == 0x3E || ObjectNumber == 0x3F)
        SDLTWE_PrintString(OW, ObjectPrintLine, CS, SC_BRRED, paper);
    else
    {
        if (ZXmem[oa + FIRST_OCCURRENCE] == ZXmem[GetObjectAttributePointer(OBJNR_YOU, FIRST_OCCURRENCE)])
            SDLTWE_PrintString(OW, ObjectPrintLine, CS, paper, ink); // reverse
        else
            SDLTWE_PrintString(OW, ObjectPrintLine, CS, ink, paper);
    }
}


/****************************************************************************\
* PrintObjectsList                                                           *
*                                                                            *
\****************************************************************************/
void PrintObjectsList(word OIA, word OA, struct TextWindowStruct *OW, struct CharSetStruct *CS, color_t ink, color_t paper)
{
    word ai;
    OW->CurrentPrintPosX = 0;
    OW->CurrentPrintPosY = 0;
    SDLTWE_PrintString(OW, "Nr Name                 1L # MO Vo Ma 04 St 06 vAo*.bfFl", CS, ink, paper);
    SDLTWE_PrintString(OW, "--------------------------------------------------------", CS, ink, paper);
    ai = OIA;
    while (ZXmem[ai] != 0xFF)
    {
        if (ZXmem[ai] == 60)  // animals have object numbers >= 60
            SDLTWE_PrintString(OW, "--------------------------------------------------------", CS, ink, paper);
        PrintObject(ai, OW, CS, ink, paper - (ZXmem[ai] % 2 ? 0x00000040ul : 0)); // with alternate background
        ai += 3;
    }
}


/****************************************************************************\
* DisplayGameMap                                                             *
*                                                                            *
\****************************************************************************/
void DisplayGameMap(void)
{
    SDL_UpdateTexture (MapWin.texPtr, NULL, MapWin.framePtr, MapWin.pitch); // copy Frame to Texture
    SDL_RenderCopy (renPtr, MapWin.texPtr, NULL, &MapWin.rect);
    //SDL_RenderPresent (renPtr);
}


/****************************************************************************\
* PrepareGameMap                                                             *
*                                                                            *
\****************************************************************************/
void PrepareGameMap(void) {
    memcpy (MapWin.framePtr, GameMapSfcPtr->pixels, MapWin.frameSize); // map as background
}


/****************************************************************************\
* InitSpectrum                                                               *
*                                                                            *
\****************************************************************************/
int InitSpectrum(void)
{
    int i;
    FILE *fp;
    long int FileLength;
    size_t BytesRead;

    for (i = 0; i < 16384; i++)
        ZXmem[i] = 0xC9;  /* = RET, just in case... */

    // we need the character set for the lower window
    fp = fopen(SDLTWE_CHARSETFILEPATH, "rb");
    if (!fp)
    {
        fprintf(stderr, "ERROR in WL.c: can't open character set file '%s'\n", SDLTWE_CHARSETFILEPATH);
        return (1);
    }
    fseek(fp, 0, SEEK_END);
    FileLength = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (WL_DEBUG)
    {
        printf("WL: Reading character set with %li byte from %s ... ", FileLength, SDLTWE_CHARSETFILEPATH);
    }
    BytesRead = fread(ZXmem + SL_CHARSET - 1, 1, FileLength, fp);
    fclose(fp);
    if (WL_DEBUG)
    {
        if (FileLength != BytesRead)
            printf("ERROR!\n");
        else
            printf("read!\n");
    }

    return (0);
}


/****************************************************************************\
* InitGame                                                                   *
*                                                                            *
\****************************************************************************/
int InitGame(int hv)
{
    char GameFileName[100];
    word GameStartAddress, GameLength;
    FILE *fp;
    word FileLength, BytesRead;

    switch (hv)
    {
        case OWN:
            strcpy(GameFileName, FN_OWN);
            GameStartAddress = STARTADR_OWN;
            GameLength = LENGTH_OWN;
            DictionaryBaseAddress = DICTIONARY_BASE_OWN;
            ObjectsIndexAddress = OBJECTS_INDEX_OWN;
            ObjectsAddress = OBJECTS_OWN;
            printf("WL: Using The Hobbit binary vOWN\n");
            break;
        case V10:
            strcpy(GameFileName, FN_V10);
            GameStartAddress = STARTADR_V10;
            GameLength = LENGTH_V10;
            DictionaryBaseAddress = DICTIONARY_BASE_V10;
            ObjectsIndexAddress = OBJECTS_INDEX_V10;
            ObjectsAddress = OBJECTS_V10;
            printf("WL: Using The Hobbit binary v1.0\n");
            break;
        case V12:
            strcpy(GameFileName, FN_V12);
            GameStartAddress = STARTADR_V12;
            GameLength = LENGTH_V12;
            DictionaryBaseAddress = DICTIONARY_BASE_V12;
            ObjectsIndexAddress = OBJECTS_INDEX_V12;
            ObjectsAddress = OBJECTS_V12;
            printf("WL: Using The Hobbit binary v1.2\n");
            break;
        default:
            fprintf(stderr, "WL: ERROR in 'InitGame'. Unknown version %d.\n", hv);
            return (-1);
    }

    fp = fopen(GameFileName, "rb");
    if (!fp)
    {
        fprintf(stderr, "WL: ERROR in 'InitGame'. Can't open game file '%s'\n", GameFileName);
        return (-1);
    }

    fseek(fp, 0, SEEK_END);
    FileLength = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (WL_DEBUG)
    {
        printf("WL: Reading game file '%s' with %i byte ... \n", GameFileName, FileLength);
    }

    BytesRead = fread(ZXmem + GameStartAddress, 1, FileLength, fp);
    fclose(fp);

    if (BytesRead != GameLength)
    {
        fprintf(stderr, "WL: ERROR in 'InitGame'. Number of bytes read doesn't match filelength.\n");
        return (-1);
    }

    return 0;
}


/****************************************************************************\
* InitGraphicsSystem                                                         *
*                                                                            *
* The global variables  SDL_Window *winPtr and renPtr must be defined!       *
\****************************************************************************/
int InitGraphicsSystem(Uint32 WinMode)
{
    SDL_Surface *temp;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf (stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (!(winPtr = SDL_CreateWindow("Wilderland - A Hobbit Environment", 110, 25, MAINWINWIDTH, MAINWINHEIGHT, WinMode | SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL|SDL_WINDOW_ALLOW_HIGHDPI))) // SDL_WINDOW_VULKAN|SDL_WINDOW_FULLSCREEN_DESKTOP //SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_CENTERED
    {
        fprintf (stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    // Creating a Renderer (window, driver index, flags: HW accelerated + vsync)
    renPtr = SDL_CreateRenderer (winPtr, -1, SDL_RENDERER_ACCELERATED /*| SDL_RENDERER_PRESENTVSYNC*/);
    if (renPtr == NULL){
       fprintf (stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
       SDL_DestroyWindow (winPtr);
       SDL_Quit ();
       return 1;
    }
    SDL_SetHint (SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize (renPtr, MAINWINWIDTH, MAINWINHEIGHT);
    SDL_SetWindowSize (winPtr, MAINWINWIDTH, MAINWINHEIGHT);
    SDL_SetRenderDrawColor(renPtr, components(SDL_ALPHA_OPAQUE<<24|SC_CYAN)); // White. Black:R,G,B=0, Alpha=full. SDL_ALPHA_OPAQUE
    SDL_RenderClear(renPtr);
//SDL_RenderPresent(renPtr); // show CYAN background
//SDL_Delay (delay); // ms

    CharSet.Bitmap = malloc(SDLTWE_CHARSETLENGTH);
    if (!(CharSet.Bitmap))
    {
        fprintf(stderr, "WL: ERROR in allocationg memory for character set.\n");
        return 1;
    }
    if (!SDLTWE_ReadCharSet(SDLTWE_CHARSETFILEPATH, CharSet.Bitmap))
    {
        fprintf(stderr, "WL: ERROR while reading character set.\n");
        return 1;
    }
    CharSet.Height = 8;
    CharSet.Width = 8;
    CharSet.CharMin = '\x20';
    CharSet.CharMax = '\x7F';
    CharSet.CharSubstitute = '?';

    /*** Create 5 accelerated Textures for: log, game, help, objects, map ***/
    LogWin.texPtr = SDL_CreateTexture (renPtr, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, LOGWINWIDTH, LOGWINHEIGHT);
    if (LogWin.texPtr == NULL) {
       fprintf (stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
       SDL_DestroyRenderer (renPtr);
       SDL_DestroyWindow (winPtr);
       SDL_Quit ();
       return 1;
    }
    LogWin.pitch = LOGWINWIDTH * BYTESPERPIXEL;
    LogWin.frameSize = LogWin.pitch * LOGWINHEIGHT;
    LogWin.framePtr = malloc (LogWin.frameSize);
    if (LogWin.framePtr == NULL) {
       fprintf (stderr, "malloc Error\n");
       SDL_DestroyRenderer (renPtr);
       SDL_DestroyWindow (winPtr);
       SDL_Quit ();
       return 1;
    }
    memset (LogWin.framePtr, SC_BLACK, LogWin.frameSize); // Black
    LogWin.rect.x = LOGWINPOSX;
    LogWin.rect.y = LOGWINPOSY;
    LogWin.rect.w = LOGWINWIDTH;
    LogWin.rect.h = LOGWINHEIGHT;
    LogWin.CurrentPrintPosX = 0;
    LogWin.CurrentPrintPosY = 0;
    SDL_UpdateTexture (LogWin.texPtr, NULL, LogWin.framePtr, LogWin.pitch); // copy Frame to Texture
    SDL_RenderCopy (renPtr, LogWin.texPtr, NULL, &LogWin.rect); // copy Texture to Renderer

    GameWin.texPtr = SDL_CreateTexture (renPtr, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, GAMEWINWIDTH, GAMEWINHEIGHT);
    if (GameWin.texPtr == NULL) {
       fprintf (stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
       SDL_DestroyRenderer (renPtr);
       SDL_DestroyWindow (winPtr);
       SDL_Quit ();
       return 1;
    }
    GameWin.pitch = GAMEWINWIDTH * BYTESPERPIXEL;
    GameWin.frameSize = GameWin.pitch * GAMEWINHEIGHT;
    GameWin.framePtr = malloc (GameWin.frameSize);
    if (GameWin.framePtr == NULL) {
       fprintf (stderr, "malloc Error\n");
       SDL_DestroyRenderer (renPtr);
       SDL_DestroyWindow (winPtr);
       SDL_Quit ();
       return 1;
    }
    memset (GameWin.framePtr, SC_BLACK, GameWin.frameSize); // Black
    GameWin.rect.x = GAMEWINPOSX;
    GameWin.rect.y = GAMEWINPOSY;
    GameWin.rect.w = GAMEWINWIDTH;
    GameWin.rect.h = GAMEWINHEIGHT;
    GameWin.CurrentPrintPosX = 0;
    GameWin.CurrentPrintPosY = 0;
    SDL_UpdateTexture (GameWin.texPtr, NULL, GameWin.framePtr, GameWin.pitch); // copy Frame to Texture
    SDL_RenderCopy (renPtr, GameWin.texPtr, NULL, &GameWin.rect); // copy Texture to Renderer

    HelpWin.texPtr = SDL_CreateTexture (renPtr, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, HELPWINWIDTH, HELPWINHEIGHT);
    if (HelpWin.texPtr == NULL) {
       fprintf (stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
       SDL_DestroyRenderer (renPtr);
       SDL_DestroyWindow (winPtr);
       SDL_Quit ();
       return 1;
    }
    HelpWin.pitch = HELPWINWIDTH * BYTESPERPIXEL;
    HelpWin.frameSize = HelpWin.pitch * HELPWINHEIGHT;
    HelpWin.framePtr = malloc (HelpWin.frameSize);
    if (HelpWin.framePtr == NULL) {
       fprintf (stderr, "malloc Error\n");
       SDL_DestroyRenderer (renPtr);
       SDL_DestroyWindow (winPtr);
       SDL_Quit ();
       return 1;
    }
    memset (HelpWin.framePtr, SC_BLACK, HelpWin.frameSize); // Black
    HelpWin.rect.x = HELPWINPOSX;
    HelpWin.rect.y = HELPWINPOSY;
    HelpWin.rect.w = HELPWINWIDTH;
    HelpWin.rect.h = HELPWINHEIGHT;
    HelpWin.CurrentPrintPosX = 0;
    HelpWin.CurrentPrintPosY = 0;
    SDL_UpdateTexture (HelpWin.texPtr, NULL, HelpWin.framePtr, HelpWin.pitch); // copy Frame to Texture
    SDL_RenderCopy (renPtr, HelpWin.texPtr, NULL, &HelpWin.rect); // copy Texture to Renderer

    ObjWin.texPtr = SDL_CreateTexture (renPtr, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, OBJWINWIDTH, OBJWINHEIGHT);
    if (ObjWin.texPtr == NULL) {
       fprintf (stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
       SDL_DestroyRenderer (renPtr);
       SDL_DestroyWindow (winPtr);
       SDL_Quit ();
       return 1;
    }
    ObjWin.pitch = OBJWINWIDTH * BYTESPERPIXEL;
    ObjWin.frameSize = ObjWin.pitch * OBJWINHEIGHT;
    ObjWin.framePtr = malloc (ObjWin.frameSize);
    if (ObjWin.framePtr == NULL) {
       fprintf (stderr, "malloc Error\n");
       SDL_DestroyRenderer (renPtr);
       SDL_DestroyWindow (winPtr);
       SDL_Quit ();
       return 1;
    }
    memset (ObjWin.framePtr, SC_BLACK, ObjWin.frameSize); // Black
    ObjWin.rect.x = OBJWINPOSX;
    ObjWin.rect.y = OBJWINPOSY;
    ObjWin.rect.w = OBJWINWIDTH;
    ObjWin.rect.h = OBJWINHEIGHT;
    ObjWin.CurrentPrintPosX = 0;
    ObjWin.CurrentPrintPosY = 0;
    SDL_UpdateTexture (ObjWin.texPtr, NULL, ObjWin.framePtr, ObjWin.pitch); // copy Frame to Texture
    SDL_RenderCopy (renPtr, ObjWin.texPtr, NULL, &ObjWin.rect); // copy Texture to Renderer

    MapWin.texPtr = SDL_CreateTexture (renPtr, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, MAPWINWIDTH, MAPWINHEIGHT);
    if (MapWin.texPtr == NULL) {
       fprintf (stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
       SDL_DestroyRenderer (renPtr);
       SDL_DestroyWindow (winPtr);
       SDL_Quit ();
       return 1;
    }
    MapWin.pitch = MAPWINWIDTH * BYTESPERPIXEL;
    MapWin.frameSize = MapWin.pitch * MAPWINHEIGHT;
    MapWin.framePtr = malloc (MapWin.frameSize);
    if (MapWin.framePtr == NULL) {
       fprintf (stderr, "malloc Error\n");
       SDL_DestroyRenderer (renPtr);
       SDL_DestroyWindow (winPtr);
       SDL_Quit ();
       return 1;
    }
    memset (MapWin.framePtr, SC_BLACK, MapWin.frameSize); // Black
    MapWin.rect.x = MAPWINPOSX;
    MapWin.rect.y = MAPWINPOSY;
    MapWin.rect.w = MAPWINWIDTH;
    MapWin.rect.h = MAPWINHEIGHT;
    MapWin.CurrentPrintPosX = 0;
    MapWin.CurrentPrintPosY = 0;
    SDL_UpdateTexture (MapWin.texPtr, NULL, MapWin.framePtr, MapWin.pitch); // copy Frame to Texture
    SDL_RenderCopy (renPtr, MapWin.texPtr, NULL, &MapWin.rect); // copy Texture to Renderer
//SDL_RenderPresent (renPtr); // show empty canvas
//SDL_Delay (delay); // ms

#if 0
Uint32 pixelFormat;
int access;
int w;
int h;
const char* pixelFormatName;
SDL_QueryTexture(MapWin.texPtr, &pixelFormat, &access, &w, &h);
SDL_Log("SDL_BITSPERPIXEL: %d", SDL_BITSPERPIXEL(pixelFormat));
SDL_Log("SDL_ISPIXELFORMAT_ALPHA: %d", SDL_ISPIXELFORMAT_ALPHA(pixelFormat));
SDL_Log("MapWin.texPtr format: %d", pixelFormat);
pixelFormatName = SDL_GetPixelFormatName(pixelFormat);
SDL_Log("MapWin.texPtr pixelformat:%s", pixelFormatName);
SDL_Log("\n");
#endif

    // Load Game Map
    IMG_Init(IMG_INIT_PNG);
    temp = IMG_Load("GameMap.png");
    if (temp == NULL)
    {
        fprintf(stderr, "WL: ERROR in 'InitGraphicsSystem'. Unable to load GameMap.png\n");
        return (-1);
    }
#if 0
pixelFormat = temp->format->format;
SDL_Log("SDL_BITSPERPIXEL: %d", SDL_BITSPERPIXEL(pixelFormat));
SDL_Log("SDL_ISPIXELFORMAT_ALPHA: %d", SDL_ISPIXELFORMAT_ALPHA(pixelFormat));
SDL_Log("temp->format->format: %d", pixelFormat);
pixelFormatName = SDL_GetPixelFormatName(pixelFormat);
SDL_Log("temp pixelformat:%s", pixelFormatName);
SDL_Log("\n");
#endif
    GameMapSfcPtr = SDL_ConvertSurfaceFormat(temp, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(temp);
    if (GameMapSfcPtr == NULL) {
       fprintf (stderr, "SDL_ConvertSurfaceFormat Error: %s\n", SDL_GetError());
       SDL_DestroyRenderer (renPtr);
       SDL_DestroyWindow (winPtr);
       SDL_Quit ();
       return 1;
    }
    int GameMapFrameSize = GameMapSfcPtr->pitch * GameMapSfcPtr->h;
    if (GameMapFrameSize != MapWin.frameSize) {
       fprintf (stderr, "GameMapSfcPtr size:%u expected:%u\n", GameMapFrameSize, MapWin.frameSize);
       SDL_DestroyRenderer (renPtr);
       SDL_DestroyWindow (winPtr);
       SDL_Quit ();
       return 1;
    }
#if 0
pixelFormat = GameMapSfcPtr->format->format;
SDL_Log("SDL_BITSPERPIXEL: %d", SDL_BITSPERPIXEL(pixelFormat));
SDL_Log("SDL_ISPIXELFORMAT_ALPHA: %d", SDL_ISPIXELFORMAT_ALPHA(pixelFormat));
SDL_Log("GameMapSfcPtr->format->format: %d", pixelFormat);
pixelFormatName = SDL_GetPixelFormatName(pixelFormat);
SDL_Log("GameMapSfcPtr pixelformat:%s", pixelFormatName);
SDL_Log("\n");
#endif

    return 0;
}


/****************************************************************************\
* Help line                                                                  *
*                                                                            *
\****************************************************************************/
void helpLine () {
    printf ("Use: WL [-V10|-OWN|-V12] [-FULLSCREEN] [-MAXSPEED] [-NOSCANLINES] [-SEEDRND]\n");
    printf ("Use: WL [-HELP]\n");
}


/****************************************************************************\
*                                   main                                     *
*                                                                            *
\****************************************************************************/
#if defined(__MINGW32__) || defined(__MINGW64__) // to have stdout/err
    #undef main // Prevent SDL from overriding the program's entry point.
#endif
int main(int argc, char *argv[])
{
    SDL_Event event;
    int i;
    int RunMainLoop;

    byte b;
    FILE *f;
    Uint32 WinMode = 0; // window mode (vs. full screen);
    int SeedRND    = 0;
    int MaxSpeed   = 0;
    Uint32 msTimer = 0;
    Sint32 DeltaT  = 0;
    int FrameCount = 0;

    static byte ObjectsPerRoom[ROOMS_NROF_MAX]; // 0x50=80

    HV = -1;  // Hobbit version (global variable)

    printf("Wilderland "WLVER" - A Hobbit Environment\n");
    printf("(c) 2012-2019 by CH, Copyright 2019-2021 Valerio Messina\n");

    // process command line arguments
    int fl=0;
    for (i = 1; i < argc; i++, fl=0)
    {
        if (!strcmp(argv[i], "-own") || !strcmp(argv[i], "-OWN"))
           { HV = OWN; fl = 1; }
        if (!strcmp(argv[i], "-v10") || !strcmp(argv[i], "-V10"))
           { HV = V10; fl = 1; }
        if (!strcmp(argv[i], "-v12") || !strcmp(argv[i], "-V12"))
           { HV = V12; fl = 1; }
        if (!strcmp(argv[i], "-fullscreen") || !strcmp(argv[i], "-FULLSCREEN"))
           { WinMode = SDL_WINDOW_FULLSCREEN; fl = 1; }
        if (!strcmp(argv[i], "-fit") || !strcmp(argv[i], "-FIT"))
           { WinMode = SDL_WINDOW_FULLSCREEN_DESKTOP; fl = 1; }
        if (!strcmp(argv[i], "-maxspeed") || !strcmp(argv[i], "-MAXSPEED"))
           { MaxSpeed = 1; fl = 1; }
        if (!strcmp(argv[i], "-noscanlines") || !strcmp(argv[i], "-NOSCANLINES"))
           { NoScanLines = 1; fl = 1; }
        if (!strcmp(argv[i], "-seedrnd") || !strcmp(argv[i], "-SEEDRND"))
           { SeedRND = 1; fl = 1; }
        if (!strcmp(argv[i], "-help") || !strcmp(argv[i], "-HELP")) {
           helpLine();
           exit (0);
        }
        if (fl == 0) {
           printf ("WL: Ignoring unknown option: '%s'\n", argv[i]);
           helpLine();
        }
    }

    if (HV == -1) HV = V12; // default to V12

    if (InitSpectrum())
    {
        fprintf(stderr, "WL: ERROR initializing spectrum. Program aborted.\n");
        exit(-1);
    }

    if (InitGame(HV))
    {
        fprintf(stderr, "WL: ERROR initializing game. Program aborted.\n");
        exit(-1);
    }

    if (InitGraphicsSystem(WinMode))
    {
        fprintf(stderr, "WL: ERROR initializing graphic system. Program aborted.\n");
        exit(-1);
    }

//LockLevel=0;
    LockScreen(winPtr);
    //SDL_RenderClear (renPtr); // clear renderer

    /*** start to drawn on the empty renderer ***/
    SDLTWE_DrawTextWindowFrame(&LogWin , 4, BORDERGRAY);
    SDLTWE_DrawTextWindowFrame(&GameWin, 4, BORDERGRAY);
    SDLTWE_DrawTextWindowFrame(&HelpWin, 4, BORDERGRAY);
    SDLTWE_DrawTextWindowFrame(&ObjWin , 4, BORDERGRAY);

/*** start to drawn on the empty textures ***/
#if 0
for (int i=0; i<LOGWINHEIGHT; i++) {
   int x=i;
   int y=i;
   if (i>=LOGWINWIDTH) x=LOGWINWIDTH-1;
   SDLTWE_SetPixel(&LogWin, x, y, SC_RED); // White drawing
}
SDL_UpdateTexture (LogWin.texPtr, NULL, LogWin.framePtr, LogWin.pitch); // copy Frame to Texture
SDL_RenderCopy (renPtr, LogWin.texPtr, NULL, &LogWin.rect); // LOGWIN texture to all Renderer

for (int i=0; i<GAMEWINWIDTH; i++) {
   int x=i;
   int y=i;
   if (i>=GAMEWINHEIGHT) y=GAMEWINHEIGHT-1;
   SDLTWE_SetPixel(&GameWin, x, y, SC_GREEN); // White drawing
}
SDL_UpdateTexture (GameWin.texPtr, NULL, GameWin.framePtr, GameWin.pitch); // copy Frame to Texture
SDL_RenderCopy (renPtr, GameWin.texPtr, NULL, &GameWin.rect); // GAMEWIN texture to all Renderer

for (int i=0; i<HELPWINWIDTH; i++) {
   int x=i;
   int y=i;
   if (i>=HELPWINHEIGHT) y=HELPWINHEIGHT-1;
   SDLTWE_SetPixel(&HelpWin, x, y, SC_BLUE); // White drawing
}
SDL_UpdateTexture (HelpWin.texPtr, NULL, HelpWin.framePtr, HelpWin.pitch); // copy Frame to Texture
SDL_RenderCopy (renPtr, HelpWin.texPtr, NULL, &HelpWin.rect); // HELPWIN texture to all Renderer

for (int i=0; i<OBJWINHEIGHT; i++) {
   int x=i;
   int y=i;
   if (i>=OBJWINWIDTH) x=OBJWINWIDTH-1;
   SDLTWE_SetPixel(&ObjWin, x, y, SC_MAGENTA); // White drawing
}
SDL_UpdateTexture (ObjWin.texPtr, NULL, ObjWin.framePtr, ObjWin.pitch); // copy Frame to Texture
SDL_RenderCopy (renPtr, ObjWin.texPtr, NULL, &ObjWin.rect); // OBJWIN texture to all Renderer

for (int i=0; i<MAPWINWIDTH; i++) {
   int x=i;
   int y=i;
   if (i>=MAPWINHEIGHT) y=MAPWINHEIGHT-1;
   SDLTWE_SetPixel(&MapWin, x, y, SC_YELLOW); // White drawing
}
SDL_UpdateTexture (MapWin.texPtr, NULL, MapWin.framePtr, MapWin.pitch); // copy Frame to Texture
SDL_RenderCopy (renPtr, MapWin.texPtr, NULL, &MapWin.rect); // MAPWIN texture to all Renderer
#endif

//SDL_Delay (1000); // ms
//Uint32 start, stop;
//start = SDL_GetTicks();
    SDLTWE_PrintCharTextWindow(&LogWin, '\f', &CharSet, SC_BLACK, SC_WHITE); // clear LOG
//stop = SDL_GetTicks();
//printf("Ticks:%u ms\n", stop-start);
//SDL_Delay (1000); // ms
//exit (1);

    //SDL_RenderPresent(renPtr);
    //SDL_Delay (delay); // ms

    // Game title screen
    f = fopen("HOBBIT.SCR", "rb");
    if (!f)
    {
        fprintf(stderr, "WL: ERROR - Can't open title screen file 'HOBBIT.SCR'\n");
        exit(-1);
    }
    else
    {
        for (i = SL_SCREENSTART; i < SL_ATTRIBUTEEND; i++)
        //for (i = SL_SCREENSTART; i < 0x4000+512; i++)
        {
            if (fscanf(f, "%c", &b) != 1)
            {
               fprintf(stderr, "WL: Error - %%c does not match format\n");
               fclose(f);
               exit(-1);
            }
            WrZ80(i, b);
        }
        fclose(f);
    }
    SDL_UpdateTexture (GameWin.texPtr, NULL, GameWin.framePtr, GameWin.pitch); // copy Frame to Texture
    SDL_RenderCopy (renPtr, GameWin.texPtr, NULL, &GameWin.rect); // GAMEWIN texture to all Renderer
//SDL_RenderPresent(renPtr);
//SDL_Delay (delay); // ms

    HelpInfo(&HelpWin, &CharSet);
    SDL_UpdateTexture (HelpWin.texPtr, NULL, HelpWin.framePtr, HelpWin.pitch); // copy Frame to Texture
    SDL_RenderCopy (renPtr, HelpWin.texPtr, NULL, &HelpWin.rect); // HELPWIN texture to all Renderer

    UnLockScreen(winPtr);

    ResetZ80(&z80);
    switch (HV)
    {
        case OWN:
            z80.PC.W = L_START_OWN;
            break;
        case V10:
            z80.PC.W = L_START_V10;
            break;
        case V12:
            z80.PC.W = L_START_V12;
            break;
        default:
            exit(-1);
    }
    if (SeedRND)
        z80.R = (byte) SDL_GetTicks();

    //SDL_EnableUNICODE(1);
    RunMainLoop = 1;
SDL_Delay (25); // ms

    /****************************** MAIN LOOP ******************************/
    while (RunMainLoop)
    {

        // we run the main loop with FPS Hz
        DeltaT = SDL_GetTicks() - msTimer;  // on my Intel Core2 Duo E6850@3GHz ca. 5 ms
//SDL_Log("FrameCount:%02u DeltaT:%d", FrameCount, DeltaT);
        if (DeltaT < DELAY_MS)
            if (!MaxSpeed)
                SDL_Delay(DELAY_MS - DeltaT); // wait to finish the 40 ms
        msTimer = SDL_GetTicks();
//SDL_Log("msTimer:%u", msTimer);
        FrameCount++;

        LockScreen(winPtr);
//SDL_RenderClear (renPtr); // clear renderer: require redraw border,log,map

        ExecZ80(&z80, TSTATES_PER_LOOP); // execute for about 140 kperiods
        SDL_UpdateTexture (LogWin.texPtr, NULL, LogWin.framePtr, LogWin.pitch); // copy Frame to Texture
        SDL_RenderCopy (renPtr, LogWin.texPtr, NULL, &LogWin.rect); // Texture to renderer
        SDL_UpdateTexture (GameWin.texPtr, NULL, GameWin.framePtr, GameWin.pitch); // copy Frame to Texture
        SDL_RenderCopy (renPtr, GameWin.texPtr, NULL, &GameWin.rect); // GAMEWIN texture to all Renderer

        PrintObjectsList(ObjectsIndexAddress, ObjectsAddress, &ObjWin, &CharSet, SC_BRYELLOW, SC_BRBLUE);
        SDL_UpdateTexture (ObjWin.texPtr, NULL, ObjWin.framePtr, ObjWin.pitch); // copy Frame to Texture
        SDL_RenderCopy (renPtr, ObjWin.texPtr, NULL, &ObjWin.rect); // OBJWIN texture to all Renderer

        if (FrameCount > FPS) // update map only once every 25 frame
        {
            FrameCount = 0;
            for (i = 0; i < ROOMS_NROF_MAX; i++) // 0x50=80
                ObjectsPerRoom[i] = 0;
            PrepareGameMap();
            PrepareAnimalPositions(MapWin.framePtr, &CharSet, ObjectsPerRoom);
            //SDL_Delay (delay); // ms
            DisplayGameMap();
            //SDL_Delay (delay); // ms
        }
        SDL_RenderPresent (renPtr); // update screen
//SDL_Delay (delay); // ms
        UnLockScreen(winPtr);

        if (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    RunMainLoop = 0;
                    //SDL_Log("SDL_QUIT");
                    break;
                case SDL_KEYDOWN:
                    CurrentPressedKey = event.key.keysym.sym;
                    CurrentPressedMod = event.key.keysym.mod;
#if 0
                    SDL_Log("SDL_KEYDOWN key:%u='%c'", CurrentPressedKey, CurrentPressedKey);
                    SDL_Log("SDL_KEYDOWN mod:%u", event.key.keysym.mod);
                    //SDL_Log("'q':%d=%c 'Q':%d=%c SDLK_q:%d=%c", 'q', 'q', 'Q', 'Q', SDLK_q, SDLK_q);
                    //SDL_Log("KMOD_LSHIFT:%d=%c KMOD_RSHIFT:%d=%c", KMOD_LSHIFT, KMOD_LSHIFT, KMOD_RSHIFT, KMOD_RSHIFT);
                    //SDL_Log("KMOD_SHIFT:%d=%c KMOD_CAPS:%d=%c", KMOD_SHIFT, KMOD_SHIFT, KMOD_CAPS, KMOD_CAPS);
                    SDL_Log("\n");
#endif
                    if (CurrentPressedKey == SDLK_BACKSPACE)
                        CurrentPressedKey = SDLK_0; // '0' used as backspace
                    if (CurrentPressedMod&KMOD_CAPS || CurrentPressedMod&KMOD_SHIFT) { // capital
                       if (CurrentPressedKey == SDLK_q)
                           RunMainLoop = 0;
                       if (CurrentPressedKey == SDLK_s) { // 'S'
                           SaveGame(&HelpWin, &CharSet, HV, SC_BRGREEN, SC_BRBLACK);
                           CurrentPressedKey = 0;
                       }
                       if (CurrentPressedKey == SDLK_l) { // 'L'
                           LoadGame(&HelpWin, &CharSet, HV, SC_BRGREEN, SC_BRBLACK);
                           CurrentPressedKey = 0;
                       }
                       if (CurrentPressedKey == SDLK_h) { // 'H'
                           HelpInfo(&HelpWin, &CharSet);
                           CurrentPressedKey = 0;
                       }
                       if (CurrentPressedKey == SDLK_i) { // 'I'
                           Info(&HelpWin, &CharSet);
                           CurrentPressedKey = 0;
                       }
                       if (CurrentPressedKey == SDLK_g) { // 'G'
                           Go(&HelpWin, &CharSet, HV, SC_BRGREEN, SC_BRBLACK);
                           CurrentPressedKey = 0;
                       }
                    }
                    break;
                case SDL_KEYUP:
                    CurrentPressedKey = 0;
                    break;
            }
        }

    } // while (RunMainLoop)

    if (LogWin.framePtr)  free(LogWin.framePtr);
    if (GameWin.framePtr) free(GameWin.framePtr);
    if (HelpWin.framePtr) free(HelpWin.framePtr);
    if (ObjWin.framePtr)  free(ObjWin.framePtr);
    if (MapWin.framePtr)  free(MapWin.framePtr);
    if (GameMapSfcPtr)  SDL_FreeSurface (GameMapSfcPtr);
    if (LogWin.texPtr)  SDL_DestroyTexture(LogWin.texPtr);
    if (GameWin.texPtr) SDL_DestroyTexture(GameWin.texPtr);
    if (HelpWin.texPtr) SDL_DestroyTexture(HelpWin.texPtr);
    if (ObjWin.texPtr)  SDL_DestroyTexture(ObjWin.texPtr);
    if (MapWin.texPtr)  SDL_DestroyTexture(MapWin.texPtr);
    if (renPtr) SDL_DestroyRenderer(renPtr);
    if (winPtr) SDL_DestroyWindow(winPtr);
    SDL_Quit();
    exit(0);
}
