/****************************************************************************\
*                                                                            *
*                              GLOBAL_VARS.h                                 *
*                                                                            *
* The global variables for the WL project                                    *
*                                                                            *
* (c) 2012-2019 by CH, Copyright 2019-2021 Valerio Messina                   *
*                                                                            *
* V 1.08 - 20210905                                                          *
*                                                                            *
\****************************************************************************/


#ifndef GLOBAL_VARS_H
#define GLOBAL_VARS_H

#include "SDLTWE.h"


extern int HV;      // Hobbit version

extern word DictionaryBaseAddress;
extern word ObjectsIndexAddress, ObjectsAddress;

extern SDL_Window*   winPtr;
extern SDL_Renderer* renPtr;
extern SDL_Surface* GameMapSfcPtr;

extern int NoScanLines;
extern int LockLevel;
extern struct CharSetStruct CharSet;

extern struct TextWindowStruct GameWin;
extern struct TextWindowStruct LogWin;
extern struct TextWindowStruct ObjWin;
extern struct TextWindowStruct HelpWin;
extern struct TextWindowStruct MapWin;

extern Z80 z80;                   // my Z80 processor
extern byte ZXmem[0x10000];       // Spectrum 64 kB memory

extern Uint16 CurrentPressedKey;  // used by InZ80()
extern Uint16 CurrentPressedMod;  // used by InZ80()


#endif /* GLOBAL_VARS_H */
