/****************************************************************************\
*                                                                            *
*                               GlobalVars.h                                 *
*                                                                            *
* The global variables for the WL project                                    *
*                                                                            *
* (c) 2012-2019 by CH, Copyright 2019-2022 Valerio Messina                   *
*                                                                            *
* V 2.09 - 20220907                                                          *
*                                                                            *
*  GlobalVars.h is part of Wilderland - A Hobbit Environment                 *
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


#ifndef GLOBALVARS_H
#define GLOBALVARS_H

#include "SDLTWE.h"


extern int NoScanLines;
extern int HV; // Hobbit version
extern struct CharSetStruct CharSet;
extern word DictionaryBaseAddress;
extern word ObjectsIndexAddress, ObjectsAddress;

extern SDL_Window*   winPtr;
extern SDL_Renderer* renPtr;
extern SDL_Surface*  GameMapSfcPtr;

extern struct TextWindowStruct GameWin;
extern struct TextWindowStruct LogWin;
extern struct TextWindowStruct ObjWin;
extern struct TextWindowStruct HelpWin;
extern struct TextWindowStruct MapWin;

// select the CPU emulator, CPUEMUL must be: eZ80 or ez80emu, see Makefile
#if CPUEMUL == eZ80
   extern Z80 z80;         // my Z80 processor
#endif
#if CPUEMUL == ez80emu
   extern Z80_STATE z80;   // my Z80 processor
   extern int context;
#endif
extern byte ZXmem[];       // Spectrum 64 kB memory

extern SDL_Keycode CurrentPressedKey;  // used by InZ80()
extern uint16_t    CurrentPressedMod;  // used by InZ80()

#endif /* GLOBAL_VARS_H */
