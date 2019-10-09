/****************************************************************************\
*                                                                            *
*                              GLOBAL_VARS.h                                 *
*                                                                            *
* The gloabal variables for the WL project.                                  *
*                                                                            *
* (c) 2012-2019 by CH, Copyright 2019 Valerio Messina                        *
*                                                                            *
* V 1.08 - 20191005                                                          *
*                                                                            *
\****************************************************************************/


#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

#include "SDLTWE.h"


int HV;      // Hobbit version

word DictionaryBaseAddress;
word ObjectsIndexAddress, ObjectsAddress;

SDL_Window*   winPtr;
SDL_Renderer* renPtr;
SDL_Surface* GameMapSfcPtr;

int NoScanLines;
int LockLevel;
struct CharSetStruct CharSet;

struct TextWindowStruct LogWin;
struct TextWindowStruct GameWin;
struct TextWindowStruct HelpWin;
struct TextWindowStruct ObjWin;
struct TextWindowStruct MapWin;

Z80 z80;                   // my Z80 processor
byte ZXmem[0x10000];       // Spectrum 64 kB memory

Uint16 CurrentPressedKey;  // used by InZ80()
Uint16 CurrentPressedMod;  // used by InZ80()


#endif /* GLOBAL_VARS_H */
