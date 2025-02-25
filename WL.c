/****************************************************************************\
*                                                                            *
*                                    WL.c                                    *
*                                                                            *
* Wilderland - A Hobbit Environment                                          *
*                                                                            *
* (c) 2012-2019 by CH, Contact: wilderland@aon.at                            *
* Copyright 2019-2025 Valerio Messina efa@iol.it                             *
*                                                                            *
* Simple Direct media Layer library (SDL 2.0) from www.libsdl.org (LGPL)     *
* Z80 emulator based on Marat Fayzullin's work from 2007 (fms.komkon.org)    *
* z80emu based on Lin Ke-Fong work from 2017 (github.com/anotherlin/z80emu)  *
* 8x8 character set from ZX Spectrum ROM (c) by Amstrad, PD for emulators    *
*                                                                            *
* Compiler: Pelles C for Windows 6.5.80 with 'Microsoft Extensions' enabled, *
*           GCC, MinGW/Msys2, Clang/LLVM                                     *
*                                                                            *
* V 2.10 - 20250106                                                          *
*                                                                            *
*  WL.c is part of Wilderland - A Hobbit Environment                         *
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
#define _DEFAULT_SOURCE // realpath()
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>     // PATH_MAX,NAME_MAX
#include <string.h>
#include <unistd.h>     // getcwd(),chdir()
#include <errno.h>      // errno
#include <sys/stat.h>   // mkdir()
#include <SDL.h>        // SDL2
#include <SDL_image.h>  // png support

#include "WL.h"
#include "SDLTWE.h"
#include "Spectrum.h"
#include "MapCoordinates.h"

/*** GLOBAL VARIABLES *******************************************************/
#include "GlobalVars.h"

#ifdef _WIN32
   #define realpath(N,R) _fullpath((R),(N),PATH_MAX)
#endif
#ifndef SDL_HINT_VIDEODRIVER // support for SDL < 2.0.22
   #define SDL_HINT_VIDEODRIVER "SDL_VIDEO_DRIVER"
#endif

#define MAXNAMELEN 100

int NoScanLines;
int HV; //Hobbit version
struct CharSetStruct CharSet;
word DictionaryBaseAddress;
word ObjectsIndexAddress, ObjectsAddress;
bool dockMap=true; // when false the map is undocked
int SeedRND = 0; // when 1 use Z80 refresh register to generate random numbers
char* dlgMsgPtr; // keep the dialog box error message
char* urlLnkPtr; // keep the Hobbit game download url
char zipFileName[FILENAME_MAX]; // keep the Hobbit zip filename
char binPath[PATH_MAX]=""; // path of binary files
char cfgPath[PATH_MAX]=""; // path of config files
char savPath[PATH_MAX]=""; // path where save and unzip TAP/TZX
char wlsPath[PATH_MAX]=""; // path where save/load WLS

SDL_Window*   winPtr;
SDL_Window*   MapWinPtr;
SDL_Renderer* renPtr;
SDL_Renderer* MapRenPtr;
SDL_Surface*  MapSfcPtr;
struct TextWindowStruct LogWin;   // left frame
struct TextWindowStruct GameWin;  // game frame
struct TextWindowStruct HelpWin;  // center frame with commands
struct TextWindowStruct ObjWin;   // right frame with attributes
struct TextWindowStruct MapWin;   // lower frame with map

// select the CPU emulator, CPUEMUL must be: eZ80 or ez80emu
#if CPUEMUL == eZ80
   Z80 z80;                      // my Z80 processor
#endif
#if CPUEMUL == ez80emu
   Z80_STATE z80;                // my Z80 processor
   int context;
#endif
size_t SizeOfZ80 = sizeof(z80);
size_t FileOfZ80; // fixed size vars(bye,word,char,short,int), skip void/register pointers and 32/64 bit padding
byte ZXmem[SL_48K];          // Spectrum 64 kB memory
//unsigned char ZXmem[1 << 16]; // Spectrum 64 kB memory for z80emu
uint8_t bit = 8*sizeof(void*); // 32 or 64 bit

//SDL_Scancode CurrentPressedCod;
SDL_Keycode CurrentPressedKey;  // used by InZ80()
uint16_t    CurrentPressedMod;  // used by InZ80()

char dp=0; // used to hack unknown property 04 and 06
byte fou[OBJECTS_NR_MAX+1]; // used to hack unknown property 04
byte fiv[OBJECTS_NR_MAX+1]; // used to hack unknown property 05
byte six[OBJECTS_NR_MAX+1]; // used to hack unknown property 06


/****************************************************************************\
* WL_Setpixel on map                                                         *
*                                                                            *
\****************************************************************************/
void WL_SetPixel(struct TextWindowStruct* TW, int x, int y, color_t color) {
   int maxw = TW->rect.w;
   int maxh = TW->rect.h;
   
   if (x>=maxw) return; // MAPWINWIDTH
   if (y>=maxh) return; // MAPWINHEIGHT
   TW->framePtr[y*maxw + x] = color;
} // WL_SetPixel()


/****************************************************************************\
* DrawLineRelative: x,y start; dx,dy offset to end; rx,ry thick; color       *
*                                                                            *
\****************************************************************************/
void DrawLineRelative(struct TextWindowStruct* TW, int x, int y, int dx, int dy, int rx, int ry, color_t color) {
   int x1, y1, sx, sy;

   if (rx == 0 || ry == 0)
      return;

   sx = (dx > 0) ? 1 : -1;
   dx *= sx;

   sy = (dy > 0) ? 1 : -1;
   dy *= sy;

   while (rx > 1 || ry > 1) {
      if (rx > 1)
         rx--;
      if (ry > 1)
         ry--;
      if (dx > dy)
         for (x1 = 0; x1 < dx; x1++)
            WL_SetPixel(TW, x + sx * (x1 + (rx - 1)), sy * (y + ((int)(dy * x1 / dx)) + (ry - 1)), color);
      else
         for (y1 = 0; y1 < dy; y1++)
            WL_SetPixel(TW, x + sx * (((int)(dx * y1 / dy)) + (rx - 1)), y + sy * (y1 + (ry - 1)), color);
   }
} // DrawLineRelative()


/****************************************************************************\
* PrintChar on map                                                           *
*                                                                            *
\****************************************************************************/
void PrintChar(struct TextWindowStruct* TW, struct CharSetStruct* cs, char a, int x, int y, color_t ink, color_t paper) {
   int i, j;
   int CharIndex;
   byte mask;
   int ymul;
   uint32_t *pixm;

   if (WL_DEBUG) {
      if ((cs->Width != 8) || (cs->Height != 8)) {
         fprintf(stderr, "WL: ERROR in PrintChar. Only 8x8 pixel charsets are supported.\n");
         return;
      }
   }

   //ymul = MAPWINWIDTH; // scr->pitch / BYTESPERPIXEL;
   ymul = TW->rect.w;
   for (i = 0; i <= 7; i++) {
      mask = 0x80;
      CharIndex = (a - cs->CharMin) * cs->Height;
      for (j = 0; j <= 7; j++, mask >>= 1) {
         pixm = (uint32_t*) TW->framePtr + (y + i) * ymul + x + j;
         if (cs->Bitmap[CharIndex + i] & mask)
            *pixm = ink;
         else
            *pixm = paper;
      }
   }
} // PrintChar()


/****************************************************************************\
* PrintString on map                                                         *
*                                                                            *
\****************************************************************************/
void PrintString(struct TextWindowStruct* TW, struct CharSetStruct* cs, char* ps, int x, int y, color_t ink, color_t paper) {
   while (*ps) {
      PrintChar(TW, cs, *ps++, x, y, ink, paper);
      x += cs->Width;
   }
} // PrintString()


/****************************************************************************\
* SearchByteWordTable for map and ?                                          *
*                                                                            *
* Returns pointer value or 0 if not found.                                   *
\****************************************************************************/
word SearchByteWordTable(byte a, word address) {
   while (ZXmem[address] != 0xFF) {
      if (ZXmem[address] == a)
         return (ZXmem[address + 1] + 0x100 * ZXmem[address + 2]);
      else
         address += 3;
   }
   return 0;
} // SearchByteWordTable()


/****************************************************************************\
* GetObjectAttributePointer for map and ?                                    *
*                                                                            *
* Returns pointer value or 0 if not found.                                   *
\****************************************************************************/
word GetObjectAttributePointer(byte a, byte attributeoffset) {
   word OAP;

   OAP = SearchByteWordTable(a, ObjectsIndexAddress);
   if (OAP)
      return (OAP + attributeoffset);
   else
      return 0;
} // GetObjectAttributePointer()


/****************************************************************************\
* DrawAnimalPositon on map                                                   *
*                                                                            *
\****************************************************************************/
void DrawAnimalPosition(struct TextWindowStruct* TW, struct CharSetStruct* cs, byte animalNr, word x, word y, byte objectoffset) {
   char initials[10];

   DrawLineRelative(TW, x, y + objectoffset*15, 10, -30, 4, 1, SC_BRRED);

   strcpy(initials, AnimalInitials[animalNr]);
   if (ZXmem[GetObjectAttributePointer(animalNr, ATTRIBUTE_OFF)] & ATTR_BROKEN)
      strcat(initials, "+");
   if (animalNr == OBJNR_YOU || animalNr == OBJNR_GANDALF || animalNr == OBJNR_THORIN)
      PrintString(TW, cs, initials, x + 13, y + objectoffset*15 - 33, SC_BRRED, 0x00000000ul);
   else
      PrintString(TW, cs, initials, x + 13, y + objectoffset*15 - 33, SC_BRGREEN, 0x00000000ul);
} // DrawAnimalPosition()


/****************************************************************************\
* PrepareOneAnimalPositon on map                                             *
*                                                                            *
\****************************************************************************/
void PrepareOneAnimalPosition(struct TextWindowStruct* TW, struct CharSetStruct* cs, byte animalNr, byte *objectsperroom) {
   byte CurrentAnimalRoom;
   int i;
   int x, y;

   CurrentAnimalRoom = ZXmem[GetObjectAttributePointer(animalNr, P10_OFF_ROOM)];

   for(i = 0; i<ROOMS_NROF_MAX; i++) {
      if (MapCoordinates[i].RoomNumber == CurrentAnimalRoom) {
         if (dockMap==true) {
            x = MapCoordinates[i].X;
            y = MapCoordinates[i].Y;
         } else {
            x = (float)MAPWINWIDTHND/MAPWINWIDTH*MapCoordinates[i].X;
            y = (float)MAPWINWIDTHND/MAPWINWIDTH*MapCoordinates[i].Y;
         }
         DrawAnimalPosition(TW, cs, animalNr, x - INDICATOROFFSET,
            y + INDICATOROFFSET,
            objectsperroom[CurrentAnimalRoom]);
         objectsperroom[CurrentAnimalRoom]++;
      }
   }
} // PrepareOneAnimalPosition()


/****************************************************************************\
* PrepareAnimalPositions on map                                              *
*                                                                            *
\****************************************************************************/
void PrepareAnimalPositions(struct TextWindowStruct* TW, struct CharSetStruct* cs, byte* objectsperroom) {
   byte AnimalNr;

   PrepareOneAnimalPosition(TW, cs, OBJNR_YOU, objectsperroom);

   for(AnimalNr = OBJNR_DRAGON; AnimalNr <= OBJNR_DISGUSTINGGOBLIN; AnimalNr++)
      PrepareOneAnimalPosition(TW, cs, AnimalNr, objectsperroom);
} // PrepareAnimalPositions()


/****************************************************************************\
* ShowTextWindow                                                             *
*                                                                            *
\****************************************************************************/
void ShowTextWindow(struct TextWindowStruct* TW) {
   SDL_UpdateTexture(TW->texPtr, NULL, TW->framePtr, TW->pitch); // Frame to Texture
   SDL_RenderCopy(renPtr, TW->texPtr, NULL, &TW->rect); // Texture to Renderer
   SDL_RenderPresent(renPtr); // to screen
   SDL_Delay(delay); // to avoid flickering
} // ShowTextWindow()


/****************************************************************************\
* printZ80struct                                                             *
*                                                                            *
\****************************************************************************/
void printZ80struct() {
   uintptr_t p;
   uintptr_t z = (uintptr_t)&z80;
   uintptr_t fo=98; // file offset, was 64 on filever1
   printf("z80 size:%zu\n", SizeOfZ80);
   printf("z80 file:%zu\n", FileOfZ80);
   printf("z80 addr:0x%p\n", &z80);
#if CPUEMUL == eZ80
   printf(                             "fil ind address_____________ reg_______ value_____________\n");
   p=(uintptr_t)&z80.AF.B.l;     printf("%03tu %03tu @:0x%16tX F         :0x%02X\n",  p-z+fo, p-z, p, z80.AF.B.l);
   p=(uintptr_t)&z80.AF.B.h;     printf("%03tu %03tu @:0x%16tX A         :0x%02X\n",  p-z+fo, p-z, p, z80.AF.B.h);
   p=(uintptr_t)&z80.BC.B.l;     printf("%03tu %03tu @:0x%16tX C         :0x%02X\n",  p-z+fo, p-z, p, z80.BC.B.l);
   p=(uintptr_t)&z80.BC.B.h;     printf("%03tu %03tu @:0x%16tX B         :0x%02X\n",  p-z+fo, p-z, p, z80.BC.B.h);
   p=(uintptr_t)&z80.DE.B.l;     printf("%03tu %03tu @:0x%16tX E         :0x%02X\n",  p-z+fo, p-z, p, z80.DE.B.l);
   p=(uintptr_t)&z80.DE.B.h;     printf("%03tu %03tu @:0x%16tX D         :0x%02X\n",  p-z+fo, p-z, p, z80.DE.B.h);
   p=(uintptr_t)&z80.HL.B.l;     printf("%03tu %03tu @:0x%16tX L         :0x%02X\n",  p-z+fo, p-z, p, z80.HL.B.l);
   p=(uintptr_t)&z80.HL.B.h;     printf("%03tu %03tu @:0x%16tX H         :0x%02X\n",  p-z+fo, p-z, p, z80.HL.B.h);
   p=(uintptr_t)&z80.IX.B.l;     printf("%03tu %03tu @:0x%16tX IXl       :0x%02X\n",  p-z+fo, p-z, p, z80.IX.B.l);
   p=(uintptr_t)&z80.IX.B.h;     printf("%03tu %03tu @:0x%16tX IXh       :0x%02X\n",  p-z+fo, p-z, p, z80.IX.B.h);
   p=(uintptr_t)&z80.IY.B.l;     printf("%03tu %03tu @:0x%16tX IYl       :0x%02X\n",  p-z+fo, p-z, p, z80.IY.B.l);
   p=(uintptr_t)&z80.IY.B.h;     printf("%03tu %03tu @:0x%16tX IYh       :0x%02X\n",  p-z+fo, p-z, p, z80.IY.B.h);
   p=(uintptr_t)&z80.PC.B.l;     printf("%03tu %03tu @:0x%16tX PCl       :0x%02X\n",  p-z+fo, p-z, p, z80.PC.B.l);
   p=(uintptr_t)&z80.PC.B.h;     printf("%03tu %03tu @:0x%16tX PCh       :0x%02X\n",  p-z+fo, p-z, p, z80.PC.B.h);
   p=(uintptr_t)&z80.SP.B.l;     printf("%03tu %03tu @:0x%16tX SPl       :0x%02X\n",  p-z+fo, p-z, p, z80.SP.B.l),
   p=(uintptr_t)&z80.SP.B.h;     printf("%03tu %03tu @:0x%16tX SPh       :0x%02X\n",  p-z+fo, p-z, p, z80.SP.B.h),

   p=(uintptr_t)&z80.AF.W;       printf("%03tu %03tu @:0x%16tX AF        :0x%02X\n",  p-z+fo, p-z, p, z80.AF.W);
   p=(uintptr_t)&z80.BC.W;       printf("%03tu %03tu @:0x%16tX BC        :0x%02X\n",  p-z+fo, p-z, p, z80.BC.W);
   p=(uintptr_t)&z80.DE.W;       printf("%03tu %03tu @:0x%16tX DE        :0x%02X\n",  p-z+fo, p-z, p, z80.DE.W);
   p=(uintptr_t)&z80.HL.W;       printf("%03tu %03tu @:0x%16tX HL        :0x%02X\n",  p-z+fo, p-z, p, z80.HL.W);
   p=(uintptr_t)&z80.IX.W;       printf("%03tu %03tu @:0x%16tX IX        :0x%02X\n",  p-z+fo, p-z, p, z80.IX.W);
   p=(uintptr_t)&z80.IY.W;       printf("%03tu %03tu @:0x%16tX IY        :0x%02X\n",  p-z+fo, p-z, p, z80.IY.W);
   p=(uintptr_t)&z80.PC.W;       printf("%03tu %03tu @:0x%16tX PC        :0x%02X\n",  p-z+fo, p-z, p, z80.PC.W);
   p=(uintptr_t)&z80.SP.W;       printf("%03tu %03tu @:0x%16tX SP        :0x%02X\n",  p-z+fo, p-z, p, z80.SP.W);

   p=(uintptr_t)&z80.AF1.W;      printf("%03tu %03tu @:0x%16tX AF'       :0x%02X\n",  p-z+fo, p-z, p, z80.AF1.W);
   p=(uintptr_t)&z80.BC1.W;      printf("%03tu %03tu @:0x%16tX BC'       :0x%02X\n",  p-z+fo, p-z, p, z80.BC1.W);
   p=(uintptr_t)&z80.DE1.W;      printf("%03tu %03tu @:0x%16tX DE'       :0x%02X\n",  p-z+fo, p-z, p, z80.DE1.W);
   p=(uintptr_t)&z80.HL1.W;      printf("%03tu %03tu @:0x%16tX HL'       :0x%02X\n",  p-z+fo, p-z, p, z80.HL1.W);

   p=(uintptr_t)&z80.IFF;        printf("%03tu %03tu @:0x%16tX IFF       :0x%02X\n",  p-z+fo, p-z, p, z80.IFF);
   p=(uintptr_t)&z80.I;          printf("%03tu %03tu @:0x%16tX I         :0x%02X\n",  p-z+fo, p-z, p, z80.I);
   p=(uintptr_t)&z80.R;          printf("%03tu %03tu @:0x%16tX R         :0x%02X\n",  p-z+fo, p-z, p, z80.R);

   p=(uintptr_t)&z80.IPeriod;    printf("%03tu %03tu @:0x%16tX IPeriod   :0x%08X\n",  p-z+fo, p-z, p, z80.IPeriod);
   p=(uintptr_t)&z80.ICount;     printf("%03tu %03tu @:0x%16tX ICount    :0x%08X\n",  p-z+fo, p-z, p, z80.ICount);
   p=(uintptr_t)&z80.IBackup;    printf("%03tu %03tu @:0x%16tX IBackup   :0x%08X\n",  p-z+fo, p-z, p, z80.IBackup);
   p=(uintptr_t)&z80.IRequest;   printf("%03tu %03tu @:0x%16tX IRequest  :0x%02X\n",  p-z+fo, p-z, p, z80.IRequest);
   p=(uintptr_t)&z80.IAutoReset; printf("%03tu %03tu @:0x%16tX IAutoReset:0x%02X\n",  p-z+fo, p-z, p, z80.IAutoReset);
   p=(uintptr_t)&z80.TrapBadOps; printf("%03tu %03tu @:0x%16tX TrapBadOps:0x%02X\n",  p-z+fo, p-z, p, z80.TrapBadOps);
   p=(uintptr_t)&z80.Trap;       printf("%03tu %03tu @:0x%16tX Trap      :0x%02X\n",  p-z+fo, p-z, p, z80.Trap);
   p=(uintptr_t)&z80.Trace;      printf("%03tu %03tu @:0x%16tX Trace     :0x%02X\n",  p-z+fo, p-z, p, z80.Trace);
   p=(uintptr_t)&z80.User;       printf("%03tu %03tu @:0x%16tX &User     :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.User);
#endif
#if CPUEMUL == ez80emu
   printf(                                     "fil ind address_____________ reg_ value_____________\n");
   p=(uintptr_t)&z80.status;             printf("%03tu %03tu @:0x%16tX stat:0x%08X\n",  p-z+fo, p-z, p, z80.status);
   p=(uintptr_t)&z80.registers.byte[0];  printf("%03tu %03tu @:0x%16tX C   :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[0]);
   p=(uintptr_t)&z80.registers.byte[1];  printf("%03tu %03tu @:0x%16tX B   :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[1]);
   p=(uintptr_t)&z80.registers.byte[2];  printf("%03tu %03tu @:0x%16tX E   :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[2]);
   p=(uintptr_t)&z80.registers.byte[3];  printf("%03tu %03tu @:0x%16tX D   :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[3]);
   p=(uintptr_t)&z80.registers.byte[4];  printf("%03tu %03tu @:0x%16tX L   :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[4]);
   p=(uintptr_t)&z80.registers.byte[5];  printf("%03tu %03tu @:0x%16tX H   :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[5]);
   p=(uintptr_t)&z80.registers.byte[6];  printf("%03tu %03tu @:0x%16tX F   :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[6]);
   p=(uintptr_t)&z80.registers.byte[7];  printf("%03tu %03tu @:0x%16tX A   :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[7]);
   p=(uintptr_t)&z80.registers.byte[8];  printf("%03tu %03tu @:0x%16tX IXl :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[8]);
   p=(uintptr_t)&z80.registers.byte[9];  printf("%03tu %03tu @:0x%16tX IXh :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[9]);
   p=(uintptr_t)&z80.registers.byte[10]; printf("%03tu %03tu @:0x%16tX IYl :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[10]);
   p=(uintptr_t)&z80.registers.byte[11]; printf("%03tu %03tu @:0x%16tX IYh :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[11]);
   p=(uintptr_t)&z80.registers.byte[12]; printf("%03tu %03tu @:0x%16tX Pl  :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[12]);
   p=(uintptr_t)&z80.registers.byte[13]; printf("%03tu %03tu @:0x%16tX Sh  :0x%02X\n",  p-z+fo, p-z, p, z80.registers.byte[13]);

   p=(uintptr_t)&z80.registers.word[0];  printf("%03tu %03tu @:0x%16tX BC  :0x%04X\n",  p-z+fo, p-z, p, z80.registers.word[0]);
   p=(uintptr_t)&z80.registers.word[1];  printf("%03tu %03tu @:0x%16tX DE  :0x%04X\n",  p-z+fo, p-z, p, z80.registers.word[1]);
   p=(uintptr_t)&z80.registers.word[2];  printf("%03tu %03tu @:0x%16tX HL  :0x%04X\n",  p-z+fo, p-z, p, z80.registers.word[2]);
   p=(uintptr_t)&z80.registers.word[3];  printf("%03tu %03tu @:0x%16tX AF  :0x%04X\n",  p-z+fo, p-z, p, z80.registers.word[3]);
   p=(uintptr_t)&z80.registers.word[4];  printf("%03tu %03tu @:0x%16tX IX  :0x%04X\n",  p-z+fo, p-z, p, z80.registers.word[4]);
   p=(uintptr_t)&z80.registers.word[5];  printf("%03tu %03tu @:0x%16tX IY  :0x%04X\n",  p-z+fo, p-z, p, z80.registers.word[5]);
   p=(uintptr_t)&z80.registers.word[6];  printf("%03tu %03tu @:0x%16tX SP  :0x%04X\n",  p-z+fo, p-z, p, z80.registers.word[6]);

   p=(uintptr_t)&z80.alternates[0];      printf("%03tu %03tu @:0x%16tX BC' :0x%04X\n",  p-z+fo, p-z, p, z80.alternates[0]);
   p=(uintptr_t)&z80.alternates[1];      printf("%03tu %03tu @:0x%16tX DE' :0x%04X\n",  p-z+fo, p-z, p, z80.alternates[1]);
   p=(uintptr_t)&z80.alternates[2];      printf("%03tu %03tu @:0x%16tX HL' :0x%04X\n",  p-z+fo, p-z, p, z80.alternates[2]);
   p=(uintptr_t)&z80.alternates[3];      printf("%03tu %03tu @:0x%16tX AF' :0x%04X\n",  p-z+fo, p-z, p, z80.alternates[3]);

   p=(uintptr_t)&z80.i;                  printf("%03tu %03tu @:0x%16tX I   :0x%08X\n",  p-z+fo, p-z, p, z80.i);
   p=(uintptr_t)&z80.r;                  printf("%03tu %03tu @:0x%16tX R   :0x%08X\n",  p-z+fo, p-z, p, z80.r);
   p=(uintptr_t)&z80.pc;                 printf("%03tu %03tu @:0x%16tX PC  :0x%08X\n",  p-z+fo, p-z, p, z80.pc);
   p=(uintptr_t)&z80.iff1;               printf("%03tu %03tu @:0x%16tX IFF1:0x%08X\n",  p-z+fo, p-z, p, z80.iff1);
   p=(uintptr_t)&z80.iff2;               printf("%03tu %03tu @:0x%16tX IFF2:0x%08X\n",  p-z+fo, p-z, p, z80.iff2);
   p=(uintptr_t)&z80.im;                 printf("%03tu %03tu @:0x%16tX IM  :0x%08X\n",  p-z+fo, p-z, p, z80.im);

   p=(uintptr_t)&z80.register_table[0];  printf("%03tu %03tu @:0x%16tX &B  :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[0]);
   p=(uintptr_t)&z80.register_table[1];  printf("%03tu %03tu @:0x%16tX &C  :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[1]);
   p=(uintptr_t)&z80.register_table[2];  printf("%03tu %03tu @:0x%16tX &D  :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[2]);
   p=(uintptr_t)&z80.register_table[3];  printf("%03tu %03tu @:0x%16tX &E  :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[3]);
   p=(uintptr_t)&z80.register_table[4];  printf("%03tu %03tu @:0x%16tX &H  :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[4]);
   p=(uintptr_t)&z80.register_table[5];  printf("%03tu %03tu @:0x%16tX &L  :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[5]);
   p=(uintptr_t)&z80.register_table[6];  printf("%03tu %03tu @:0x%16tX &HL :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[6]);
   p=(uintptr_t)&z80.register_table[7];  printf("%03tu %03tu @:0x%16tX &A  :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[7]);
   p=(uintptr_t)&z80.register_table[8];  printf("%03tu %03tu @:0x%16tX &BC :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[8]);
   p=(uintptr_t)&z80.register_table[9];  printf("%03tu %03tu @:0x%16tX &DE :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[9]);
   p=(uintptr_t)&z80.register_table[10]; printf("%03tu %03tu @:0x%16tX &HL :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[10]);
   p=(uintptr_t)&z80.register_table[11]; printf("%03tu %03tu @:0x%16tX &SP :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[11]);
   p=(uintptr_t)&z80.register_table[12]; printf("%03tu %03tu @:0x%16tX &BC :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[12]);
   p=(uintptr_t)&z80.register_table[13]; printf("%03tu %03tu @:0x%16tX &DE :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[13]);
   p=(uintptr_t)&z80.register_table[14]; printf("%03tu %03tu @:0x%16tX &HL :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[14]);
   p=(uintptr_t)&z80.register_table[15]; printf("%03tu %03tu @:0x%16tX &AF :0x%16tX\n", p-z+fo, p-z, p, (uintptr_t)z80.register_table[15]);
#endif
} // printZ80struct()


/****************************************************************************\
* printZ80                                                                   *
*                                                                            *
\****************************************************************************/
void printZ80() {
   unsigned int i;
   byte mem;
   printf("z80 size:%zu\n", SizeOfZ80);
   printf("z80 file:%zu\n", FileOfZ80);
   printf("z80 addr:0x%p\n", &z80);
   for (i=0; i<SizeOfZ80; i++) {
      mem = *( ((byte*)&z80) + i);
      //printf("z80[%03u]='%c'=0x%02X=%03u\n", i, mem, mem, mem );
      printf("z80[%03u]=0x%02X\n", i, mem );
   }
} // printZ80()


/****************************************************************************\
* GetFileName                                                                *
*                                                                            *
\****************************************************************************/
void GetFileName(char* fnstr, struct TextWindowStruct* TW, struct CharSetStruct* CS, color_t ink, color_t paper) {
   SDL_Keycode s;
   uint16_t m;
   char* f = fnstr;
   SDL_Event event;

   while (1) {
      if (SDL_PollEvent(&event)) {
         switch (event.type) {
            case SDL_KEYDOWN:
               s = event.key.keysym.sym;
               m = event.key.keysym.mod;
               //printf("sym:'%c'=%d mod:'%c'=%d\n", s, s, m, m);
               if ((s >= 'a' && s <= 'z') || (s >= '0' && s <= '9') ||
                   s == '.' || s == '-') {
                  if (s=='-' && (m==KMOD_LSHIFT||m==KMOD_RSHIFT)) s='_';
                  *fnstr = s;
                  fnstr++;
                  SDLTWE_PrintCharTextWindow(TW, s, CS, ink, paper);
                  ShowTextWindow(TW);
                  if (fnstr-f >= MAXNAMELEN) {
                     *fnstr = 0;
                     return;
                  }
                  break;
               }
               if (s == SDLK_BACKSPACE) {
                  //printf("backspace\n");
                  if (f!=fnstr) { // not on first editable
                     *fnstr = '\b'; // backspace
                     fnstr--;
                     SDLTWE_PrintCharTextWindow(TW, '\b', CS, ink, paper);
                     ShowTextWindow(TW);
                  }
                  break;
               }
               if (s == SDLK_RETURN) {
                  *fnstr = 0;
                  return;
               }
            case SDL_KEYUP:
               s = 0;
               break;
         }
      }
   }
} // GetFileName()

/****************************************************************************\
* SaveGame                                                                   *
*                                                                            *
\****************************************************************************/
void SaveGame(struct TextWindowStruct* TW, struct CharSetStruct* CS, int hv, color_t ink, color_t paper) {
   char fn[MAXNAMELEN+4];
   FILE *f;
   int i;

   SDLTWE_PrintString(TW, "\n\nSave Filename: ", CS, ink, paper);
   ShowTextWindow(TW);
   firstEditable = 15; // "Save Filename: "
   GetFileName(fn, TW, CS, ink, paper);
   firstEditable = 0;
   if (fn[0] != '\0') strcat(fn, ".wls");

   chdir(wlsPath); // go to WLS path to let read/write screenshot files
   f = fopen(fn, "wb");
   if (!f) {
      fprintf(stderr, "WL: ERROR - Can't open file '%s' for saving game.\n", fn);
      SDLTWE_PrintString(TW, "Can't open file for saving\n", CS, ink, paper);
      ShowTextWindow(TW);
      chdir(cfgPath); // back to config path to let find assets files
      return;
   }

   fprintf(f, "GAMEVERSION=%i\r\n", hv);
   fprintf(f, "FILEVERSION=2\r\n");
   fprintf(f, "Z80VERSION=%i\r\n", CPUEMUL); // new in fileversion 2
   fprintf(f, "SIZE(Z80)=0x%03zX\r\n", SizeOfZ80);
   fprintf(f, "FILE(Z80)=0x%02zX\r\n", FileOfZ80);
   #if CPUEMUL == eZ80
      fprintf(f, "PC=0x%04X\r\n", z80.PC.W);
   #endif
   #if CPUEMUL == ez80emu
      fprintf(f, "PC=0x%04X\r\n", (word)z80.pc);
   #endif
   fprintf(f, "***Z80***\n");
   for (i = 0; i < FileOfZ80; i++) { // filever 2 save z80 partially
      byte b = *( ((byte*)&z80) + i);
      fprintf(f, "%c", b);
   }
   fprintf(f, "\n");

   fprintf(f, "***MEMORY***\n");
   for (i = 0; i <= SL_RAMEND; i++)
      fprintf(f, "%c", ZXmem[i]);

   fclose(f);

   SDLTWE_PrintString(TW, ".wls saved.\n\n", CS, ink, paper);
   ShowTextWindow(TW);
   chdir(cfgPath); // back to config path to let find assets files
} // SaveGame()


/****************************************************************************\
* LoadGame                                                                   *
*                                                                            *
\****************************************************************************/
void LoadGame(struct TextWindowStruct* TW, struct CharSetStruct* CS, int hv, color_t ink, color_t paper) {
   char fn[MAXNAMELEN+4];
   FILE *f;
   int ret;
   size_t num;
   int i, gameversion;
   int fileversion, z80version;
   size_t sizeOfZ80, fileOfZ80;

   SDLTWE_PrintString(TW, "\n\nLoad Filename: ", CS, ink, paper);
   ShowTextWindow(TW);
   firstEditable = 15; // "Load Filename: "
   GetFileName(fn, TW, CS, ink, paper);
   firstEditable = 0;
   strcat(fn, ".wls");

   chdir(wlsPath); // go to WLS path to let read/write screenshot files
   f = fopen(fn, "rb");
   if (!f) {
      fprintf(stderr, "WL: ERROR - Can't open file '%s' for loading game.\n", fn);
      SDLTWE_PrintString(TW, ".wls not found!\n\n", CS, ink, paper);
      ShowTextWindow(TW);
      chdir(cfgPath); // back to config path to let find assets files
      return;
   }

   ret = fscanf(f, "GAMEVERSION=%i\r\n", &gameversion);
   if (ret != 1) {
      printf("ret:%d gamever:%d\n", ret, gameversion);
      SDLTWE_PrintString(TW, "\nError: GAMEVERSION not found.\n\n", CS, ink, paper);
      ShowTextWindow(TW);
      chdir(cfgPath); // back to config path to let find assets files
      return;
   }
   if (gameversion != hv) {
      SDLTWE_PrintString(TW, "\nError: Wrong game version.\n\n", CS, ink, paper);
      ShowTextWindow(TW);
      chdir(cfgPath); // back to config path to let find assets files
      return;
   }

   ret=fscanf(f, "FILEVERSION=%i\r\n", &fileversion);
   if (ret != 1) {
      printf("ERROR: FILEVERSION not found.\n");
      SDLTWE_PrintString(TW, "\nError: FILEVERSION not found.\n\n", CS, ink, paper);
      ShowTextWindow(TW);
      chdir(cfgPath); // back to config path to let find assets files
      return;
   }
   if (fileversion < 1 || fileversion > 2) {
      printf("ret:%d filever:%d\n", ret, fileversion);
      SDLTWE_PrintString(TW, "\nError: Unsupported Fileversion.\n\n", CS, ink, paper);
      ShowTextWindow(TW);
      chdir(cfgPath); // back to config path to let find assets files
      return;
   }

   if (fileversion == 2) { // compare z80 emulator
      if (fscanf(f, "Z80VERSION=%i\r\n", &z80version) != 1) {
         printf("ERROR: Z80VERSION not found.\n");
         SDLTWE_PrintString(TW, "\nError: Z80VERSION not found.\n\n", CS, ink, paper);
         ShowTextWindow(TW);
         chdir(cfgPath); // back to config path to let find assets files
         return;
      }
      if (z80version != CPUEMUL) {
         SDLTWE_PrintString(TW, "\nError: Wrong z80 emulator version.\n\n", CS, ink, paper);
         ShowTextWindow(TW);
         chdir(cfgPath); // back to config path to let find assets files
         return;
      }
   }

   if (fscanf(f, "SIZE(Z80)=%05zX\r\n", &sizeOfZ80) != 1) {
      SDLTWE_PrintString(TW, "\nError: SIZE(Z80) not found.\n\n", CS, ink, paper);
      ShowTextWindow(TW);
      chdir(cfgPath); // back to config path to let find assets files
      return;
   }
   if (sizeOfZ80 != SizeOfZ80) {
      printf("WARN: z80size file:%zu mem:%zu FileOfZ80:%zu '%s'\n", sizeOfZ80, SizeOfZ80, FileOfZ80, fn);
      if (fileversion == 1) {
         if (bit==32 && sizeOfZ80==56 && SizeOfZ80==52) { // 56 on 64bit and 52 on 32bit
            goto bitDiffer;
         }
         if (bit==32 && sizeOfZ80==440 && SizeOfZ80==244) { // 440 on 64bit and 244 on 32bit
            goto bitDiffer;
         }
         if (bit==64 && sizeOfZ80==52 && SizeOfZ80==56) { // 56 on 64bit and 52 on 32bit
            goto bitDiffer;
         }
         if (bit==64 && sizeOfZ80==244 && SizeOfZ80==440) { // 440 on 64bit and 244 on 32bit
            goto bitDiffer;
         }
         SDLTWE_PrintString(TW, "\nError: Z80-structure sizes do not match.\n\n", CS, ink, paper);
         ShowTextWindow(TW);
         chdir(cfgPath); // back to config path to let find assets files
         return;
      } else // on filever=2 will use and check FileOfZ80
         bitDiffer:
         printf("CONTINUE: savefile generated on different 32/64 bit architecture\n");
   }

   if (fileversion == 2) { // new in filever 2: read FILE(Z80)
      if (fscanf(f, "FILE(Z80)=%04zX\r\n", &fileOfZ80) != 1) {
         SDLTWE_PrintString(TW, "\nError: FILE(Z80) not found.\n\n", CS, ink, paper);
         ShowTextWindow(TW);
         chdir(cfgPath); // back to config path to let find assets files
         return;
      }
      if (fileOfZ80 != FileOfZ80) {
         printf("ERROR: z80file:%zu mem:%zu\n", fileOfZ80, FileOfZ80);
         SDLTWE_PrintString(TW, "\nError: Z80-structure File sizes do not match.\n\n", CS, ink, paper);
         ShowTextWindow(TW);
         chdir(cfgPath); // back to config path to let find assets files
         return;
      }
   }

   // from here overwrite Z80 structure
   #if CPUEMUL == eZ80
      if (fscanf(f, "PC=%06hX\r\n", &z80.PC.W) != 1) {
   #endif
   #if CPUEMUL == ez80emu
      word pc;
      ret=fscanf(f, "PC=%06hX\r\n", &pc);
      z80.pc=pc;
      if (ret != 1) {
   #endif
      SDLTWE_PrintString(TW, "\nError: PC address not found.\n\n", CS, ink, paper);
      ShowTextWindow(TW);
      chdir(cfgPath); // back to config path to let find assets files
      return;
   }

   ret = fscanf(f, "***Z80***\n"); // should be "***Z80***", so ret = 0 conversions
   if (ret != 0) { // so is EOF=-1
      SDLTWE_PrintString(TW, "\nError: ***Z80*** not found.\n\n", CS, ink, paper);
      ShowTextWindow(TW);
      chdir(cfgPath); // back to config path to let find assets files
      return;
   }

   // SizeOfZ80: Z80:56 on 64bit, 52 on 32bit, z80emu:440 on 64bit, 244 on 32bit
   // FileOfZ80: Z80:47 on 64/32bit, z80emu:52 on 64/32bit
   for (i = 0; i < FileOfZ80; i++) { // read z80 partially
      if (fscanf(f, "%c", ( ((byte*)&z80) + i) ) != 1) {
         SDLTWE_PrintString(TW, "\nError: Z80struct:%%c does not match format.\n\n", CS, ink, paper);
         ShowTextWindow(TW);
         chdir(cfgPath); // back to config path to let find assets files
         return;
      }
   }

   if (fileversion == 1) { // on filever=1 there are pointer bytes to skip
      num = sizeOfZ80-FileOfZ80; // bytes to discard
      //    56/52    - 47
      printf("skipping num:%zu bytes of pointers+padding\n", num);
      for (int i=1; i<=num; i++) { // Z80: 56 on 64bit,52 on 32bit, skip bytes
         ret=fscanf(f, "%*c");
      }
   }

   ret = fscanf(f, "\n");

   ret = fscanf(f, "***MEMORY***\n"); // should be "***MEMORY***", so ret = 0 conversions
   if (ret != 0) { // so is EOF=-1
      SDLTWE_PrintString(TW, "\nError: ***MEMORY*** not found.\n\n", CS, ink, paper);
      ShowTextWindow(TW);
      chdir(cfgPath); // back to config path to let find assets files
      return;
   }

   for (i = 0; i <= SL_RAMEND; i++)
      if (fscanf(f, "%c", ZXmem + i) != 1) {
         SDLTWE_PrintString(TW, "\nError: MEMORY:%%c does not match format.\n\n", CS, ink, paper);
         ShowTextWindow(TW);
         chdir(cfgPath); // back to config path to let find assets files
         return;
      }
   fclose(f);

   SDLTWE_PrintString(TW, ".wls loaded.\n\n", CS, ink, paper);
   ShowTextWindow(TW);

   for (i = SL_SCREENSTART; i <= SL_ATTRIBUTEEND; i++) // display buffer
      WrZ80(i, ZXmem[i]); // force call to WriteScreenByte+WriteAttributeByte

   #if CPUEMUL == ez80emu
      Z80ResetTable (&z80); // regenerate the register pointers to new address
   #endif
   chdir(cfgPath); // back to config path to let find assets files
} // LoadGame()


/****************************************************************************\
* GetHexByte                                                                 *
*                                                                            *
\****************************************************************************/
void GetHexByte(byte* b, struct TextWindowStruct* TW, struct CharSetStruct* CS, color_t ink, color_t paper) {
   SDL_Keycode s;
   SDL_Event event;
   int nibble_count = 0;

   *b = 0; // returned value

   while (1) {
      if (SDL_PollEvent(&event)) {
         switch (event.type) {
            case SDL_KEYDOWN:
               s = event.key.keysym.sym;
               if (s >= '0' && s <= '9') {
                  if (nibble_count >= 2) break;
                  *b <<= 4;
                  *b += (s - '0');
                  nibble_count++;
                  SDLTWE_PrintCharTextWindow(TW, s, CS, ink, paper);
                  ShowTextWindow(TW);
                  break;
               }
               if ((s >= 'a' && s <= 'f') || (s >= 'A' && s <= 'F')) {
                  if (nibble_count >= 2) break;
                  s |= 0x20;
                  *b <<= 4;
                  *b += (s - 'a' + 0x0a);
                  s += 'A' - 'a'; // to upper case
                  nibble_count++;
                  SDLTWE_PrintCharTextWindow(TW, s, CS, ink, paper);
                  ShowTextWindow(TW);
                  break;
               }
               if (s == SDLK_BACKSPACE) {
                  //printf("backspace\n");
                  if (nibble_count>0) {
                     *b >>= 4;
                     nibble_count--;
                     SDLTWE_PrintCharTextWindow(TW, '\b', CS, ink, paper);
                     ShowTextWindow(TW);
                  }
                  break;
               }
               if (s == SDLK_RETURN) {
                   return;
               }
            case SDL_KEYUP:
                s = 0;
                break;
         }
      }
   }
} // GetHexByte()


/****************************************************************************\
* Go                                                                         *
*                                                                            *
\****************************************************************************/
void Go(struct TextWindowStruct* TW, struct CharSetStruct* CS, int hv, color_t ink, color_t paper) {
   byte rn;

   SDLTWE_PrintString(TW, "\n\nEnter room number 0x", CS, ink, paper);
   ShowTextWindow(TW);

   firstEditable=20;
   GetHexByte(&rn, TW, CS, ink, paper);
   firstEditable=0;

   if (rn < 1 || rn > ROOMS_NR_MAX) {
      SDLTWE_PrintString(TW, ". ERROR, invalid room number.\n", CS, ink, paper);
      ShowTextWindow(TW);
      return;
   }

   switch (hv) {
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

   // move carried object too
   word ai, oa;
   for (ai=ObjectsIndexAddress+1; ZXmem[ai-1] != 0xFF; ai+=3) {
      oa = ZXmem[ai] + 0x100 * ZXmem[ai + 1];
      if (ZXmem[oa + P1_OFF_MO] == OBJNR_YOU) { // by Bilbo
         ZXmem[oa + P10_OFF_ROOM] = rn;
      }
   }

   SDLTWE_PrintString(TW, ". OK, you are now in room: 0x", CS, ink, paper);
   char rnStr[4];
   sprintf(rnStr, "%02X\n", rn);
   SDLTWE_PrintString(TW, rnStr, CS, ink, paper);
   ShowTextWindow(TW);
} // Go()


/****************************************************************************\
* Help                                                                       *
*                                                                            *
\****************************************************************************/
void Help(struct TextWindowStruct* TW, struct CharSetStruct* CS) {
   char bitStr[20];
   char str[65]="         WILDERLAND - A Hobbit Environment v"WLVER" "; // 45+WLVER(5)=50
   sprintf(bitStr, "%u bit        ", bit); // 14
   if (strlen(str)+strlen(bitStr) <= 64) strcat(str, bitStr); // 64

   //                     "0000000001111111111222222222233333333334444444444555555555566666"
   //                     "1234567890123456789012345678901234567890123456789012345678901234"
 //SDLTWE_PrintString(TW, "            WILDERLAND - A Hobbit Environment v"WLVER"             ", CS, SC_BRWHITE, SC_BRBLACK);
   SDLTWE_PrintString(TW, str, CS, SC_BRWHITE, SC_BRBLACK);
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW, "       (c) 2012-2019 by CH, Copyright 2019-"WLYEAR" V.Messina       ", CS, SC_WHITE, SC_BLACK);
   ShowTextWindow(TW);
   #if CPUEMUL == eZ80
   SDLTWE_PrintString(TW, "           Using Z80 emulator: Z80 by Marat Fayzullin            ", CS, SC_BRWHITE, SC_BRBLACK);
   #endif
   #if CPUEMUL == ez80emu
   SDLTWE_PrintString(TW, "           Using Z80 emulator: z80emu by Lin Ke-Fong             ", CS, SC_BRWHITE, SC_BRBLACK);
   #endif
   ShowTextWindow(TW);
   if (HV == OWN) SDLTWE_PrintString(TW, "              Selected 'The Hobbit' binary VOWN                    \n", CS, SC_BRWHITE, SC_BRBLACK);
   if (HV == V10) SDLTWE_PrintString(TW, "              Selected 'The Hobbit' binary V1.0                    \n", CS, SC_BRWHITE, SC_BRBLACK);
   if (HV == V12) SDLTWE_PrintString(TW, "              Selected 'The Hobbit' binary V1.2                    \n", CS, SC_BRWHITE, SC_BRBLACK);
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW, " INFO: ", CS, SC_BRMAGENTA, SC_BRBLACK);
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW,        "1L=place #=qty MO=parent Vo=volume Ma=mass St=strength   \n", CS, SC_BRCYAN, SC_BRBLACK);
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW, " a-z.,\"@0[BS] -> game        ATTRIBUTES                         ", CS, SC_BRMAGENTA, SC_BRBLACK);
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW, " S-ave   H-elp   G-o         v_isible  A_nimal  o_pen   *_light ", CS, SC_BRCYAN, SC_BRBLACK);
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW, " L-oad   I-nfo               b_roken   f_ull    F_luid  l_ocked ", CS, SC_BRCYAN, SC_BRBLACK);
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW, " Q-uit                                                          \n", CS, SC_BRWHITE, SC_BRBLACK);
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW, " Press 'n' as first key to play without graphics (not game 1.0) \n", CS, SC_RED, SC_BLACK);
   ShowTextWindow(TW);

} // Help()


/****************************************************************************\
* Info                                                                       *
*                                                                            *
\****************************************************************************/
void Info(struct TextWindowStruct* TW, struct CharSetStruct* CS) {

   SDLTWE_PrintString(TW, "\nWILDERLAND - A Hobbit Environment (c) 2012-2019 by CH, "WLYEAR" VM\n\n", CS, SC_BRWHITE, SC_BRBLACK);
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW, "\"The Hobbit\" (c) Melbourne House, 1982. Written by Philip\n", CS, SC_BRMAGENTA, SC_BRBLACK);
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW, "Mitchell and Veronika Megler.\n\n", CS, SC_BRMAGENTA, SC_BRBLACK);
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW, "Simple Direct media Layer library (SDL 2.0) from www.libsdl.org\n", CS, SC_WHITE, SC_BLACK);
   ShowTextWindow(TW);
   #if CPUEMUL == eZ80
      SDLTWE_PrintString(TW, "Z80 emulator based on Marat Fayzullin's work (fms.komkon.org)\n", CS, SC_WHITE, SC_BLACK);
   #endif
   #if CPUEMUL == ez80emu
      SDLTWE_PrintString(TW, "z80emu based on Lin Ke-Fong work (github.com/anotherlin/z80emu)\n", CS, SC_WHITE, SC_BLACK);
   #endif
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW, "8x8 charset from ZX Spectrum ROM (c) by Amstrad,PD for emulators\n\n", CS, SC_WHITE, SC_BLACK);
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW, "You are not allowed to distribute 'WILDERLAND' with Hobbit\nbinaries included!\n\n", CS, SC_BRWHITE, SC_BRBLACK);
   ShowTextWindow(TW);
   SDLTWE_PrintString(TW, "Contact: wilderland@aon.at, efa@iol.it\n", CS, SC_WHITE, SC_BLACK);
   ShowTextWindow(TW);

} // Info()


/****************************************************************************\
* GetDictWord                                                                *
*                                                                            *
\****************************************************************************/
void GetDictWord(word a, char* buf) {
   word i = 1; // char count

   a += DictionaryBaseAddress;

   // the following line relies on evaluation from left to right
   while (!(ZXmem[a] & 0x80) || (i == 2) || ((i == 3) && (ZXmem[a - 1] & 0x80))) {
      if (ZXmem[a] & 0x1F) {
         *buf++ = (ZXmem[a] & 0x1F) + 'A' - 1;
      }
      a++;
      i++;
   }
   if (ZXmem[a] & 0x1F)
      *buf++ = (ZXmem[a] & 0x1F) + 'A' - 1;
   *buf = 0;

} // GetDictWord()


/****************************************************************************\
* GetObjectFullName                                                          *
*                                                                            *
* +0A/0B: ADJECTIVE1, +0C/0D: ADJECTIVE2, +08/09: NOUN                       *
\****************************************************************************/
void GetObjectFullName(word oa, char* OFN) {
   word wa;
   char StringBuffer[25];

   wa = ZXmem[oa + 0x0A] + 0x100 * ZXmem[oa + 0x0B];   // adjective 1
   if (wa) {
      GetDictWord(wa, StringBuffer);
      strcat(OFN, StringBuffer);
      strcat(OFN, " ");
   }
   wa = ZXmem[oa + 0x0C] + 0x100 * ZXmem[oa + 0x0D];   // adjective 2
   if (wa) {
      GetDictWord(wa, StringBuffer);
      if (!strcmp(StringBuffer, "INSIGNIFICANT"))
         strcpy(StringBuffer, "INSIGN.");
      strcat(OFN, StringBuffer);
      strcat(OFN, " ");
   }
   wa = ZXmem[oa + 0x08] + 0x100 * ZXmem[oa + 0x09];   // noun
   if (wa & 0x0FFF) {
      GetDictWord(wa & 0x0FFF, StringBuffer);
      strcat(OFN, StringBuffer);
   }

} // GetObjectFullName()


/****************************************************************************\
* sbinprintf                                                                 *
*                                                                            *
\****************************************************************************/
void sbinprintf(char* buf, byte b) {
   int i;
   byte mask = 0x80;

   for (i = 0; i < 8; i++, mask >>= 1) {
      *buf++ = '0' + ((b & mask) != 0);
      if (i == 3)
         *buf++ = '.';
   }
   *buf = 0;
} // sbinprintf()


/****************************************************************************\
* PrintObject                                                                *
*                                                                            *
\****************************************************************************/
void PrintObject(word ai, struct TextWindowStruct* OW, struct CharSetStruct* CS, color_t ink, color_t paper) {
   word oa;
   byte ObjectNumber;
   char ObjectPrintLine[100];
   char ObjectStringBuffer[100];

   ObjectNumber = ZXmem[ai++];
   if (ObjectNumber>OBJECTS_NR_MAX) {
      printf("obj:0x%02X\n", ObjectNumber);
      return;
   }
   //printf("obj:0x%02X\n", ObjectNumber);
   sprintf(ObjectPrintLine, "%02X ", ObjectNumber);

   oa = ZXmem[ai] + 0x100 * ZXmem[ai + 1];
   // object name
   *ObjectStringBuffer = 0;
   GetObjectFullName(oa, ObjectStringBuffer);
   strcat(ObjectStringBuffer, "                              ");
   ObjectStringBuffer[21] = 0;
   strcat(ObjectPrintLine, ObjectStringBuffer);

   if (ZXmem[oa+P4_OFF]!=fou[ObjectNumber] || ZXmem[oa+P5_OFF_STRENGTH]!=fiv[ObjectNumber] || ZXmem[oa+P6_OFF]!=six[ObjectNumber]) {
      if (dp==1) printf("obj:0x%02X '%s' P04:0x%02X->0x%02X, P05:0x%02X->0x%02X, P06:0x%02X->0x%02X\n", ObjectNumber, ObjectStringBuffer, fou[ObjectNumber], ZXmem[oa+P4_OFF], fiv[ObjectNumber], ZXmem[oa+P5_OFF_STRENGTH], six[ObjectNumber], ZXmem[oa+P6_OFF]);
      if (ZXmem[oa + P4_OFF] != fou[ObjectNumber])
         fou[ObjectNumber] = ZXmem[oa+P4_OFF];
      if (ZXmem[oa + P5_OFF_STRENGTH] != fiv[ObjectNumber])
         fiv[ObjectNumber] = ZXmem[oa+P5_OFF_STRENGTH];
      if (ZXmem[oa + P6_OFF] != six[ObjectNumber])
         six[ObjectNumber] = ZXmem[oa+P6_OFF];
   }

   // +16: 1st occurence/room
   sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + P10_OFF_ROOM]);
   strcat(ObjectPrintLine, ObjectStringBuffer);
   // +00: occurences
   sprintf(ObjectStringBuffer, "%1X " , ZXmem[oa + P0_OFF_QTY]);
   strcat(ObjectPrintLine, ObjectStringBuffer);
   // +01: mother object
   sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + P1_OFF_MO]);
   strcat(ObjectPrintLine, ObjectStringBuffer);
   // +02: volume
   sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + P2_OFF_VOLUME]);
   strcat(ObjectPrintLine, ObjectStringBuffer);
   // +03: mass
   sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + P3_OFF_MASS]);
   strcat(ObjectPrintLine, ObjectStringBuffer);
   // +04:
   sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + P4_OFF]);
   strcat(ObjectPrintLine, ObjectStringBuffer);
   // +05: strength
   sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + P5_OFF_STRENGTH]);
   strcat(ObjectPrintLine, ObjectStringBuffer);
   // +06:
   sprintf(ObjectStringBuffer, "%02X ", ZXmem[oa + P6_OFF]);
   strcat(ObjectPrintLine, ObjectStringBuffer);
   // ATTRIBUTEs
   sbinprintf(ObjectStringBuffer, ZXmem[oa + ATTRIBUTE_OFF]);
   strcat(ObjectPrintLine, ObjectStringBuffer);

   if (ObjectNumber == 0x00 || ObjectNumber == 0x3E || ObjectNumber == 0x3F)
      SDLTWE_PrintString(OW, ObjectPrintLine, CS, SC_BRRED, paper);
   else {
      if (ZXmem[oa + P10_OFF_ROOM] == ZXmem[GetObjectAttributePointer(OBJNR_YOU, P10_OFF_ROOM)])
         SDLTWE_PrintString(OW, ObjectPrintLine, CS, paper, ink); // reverse
      else
         SDLTWE_PrintString(OW, ObjectPrintLine, CS, ink, paper);
   }
} // PrintObject()


/****************************************************************************\
* PrintObjectsList                                                           *
*                                                                            *
\****************************************************************************/
void PrintObjectsList(word OIA, word OA, struct TextWindowStruct* OW, struct CharSetStruct* CS, color_t ink, color_t paper) {
   word ai;
   OW->CurrentPrintPosX = 0;
   OW->CurrentPrintPosY = 0;
   SDLTWE_PrintString(OW, "Nr Name                 1L # MO Vo Ma 04 St 06 vAo*.bfFl", CS, ink, paper);
   SDLTWE_PrintString(OW, "--------------------------------------------------------", CS, ink, paper);
   ai = OIA;
   while (ZXmem[ai] != 0xFF) {
      if (ZXmem[ai] == 0x3C)  // animals have object numbers >= 60
         SDLTWE_PrintString(OW, "--------------------------------------------------------", CS, ink, paper);
      PrintObject(ai, OW, CS, ink, paper - (ZXmem[ai] % 2 ? 0x00000040ul : 0)); // with alternate background
      ai += 3;
   }
} // PrintObjectsList()


/****************************************************************************\
* PrepareGameMap                                                             *
*                                                                            *
\****************************************************************************/
void PrepareGameMap(void) {
   memcpy (MapWin.framePtr, MapSfcPtr->pixels, MapWin.frameSize); // empty map as background
}


/****************************************************************************\
* DisplayGameMap                                                             *
*                                                                            *
\****************************************************************************/
void DisplayGameMap(void) {
   SDL_UpdateTexture(MapWin.texPtr, NULL, MapWin.framePtr, MapWin.pitch); // copy Frame to Texture
   SDL_RenderCopy(MapRenPtr, MapWin.texPtr, NULL, &MapWin.rect);
   //SDL_RenderPresent(renPtr);
}


/****************************************************************************\
* missGame                                                                   *
*                                                                            *
\****************************************************************************/
int missGame() {

   printf("%s", dlgMsgPtr);

   // create an SDL error dialog
   SDL_Window* dlgWinPtr = SDL_CreateWindow("Wilderland - Error", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, DIALOGWIDTH, DIALOGHEIGHT, SDL_WINDOW_SHOWN); // SDL_WINDOWPOS_UNDEFINED
   if (dlgWinPtr == NULL) {
      fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
      SDL_Quit();
      return 1;
   }
   // Creating a Renderer (window, driver index, flags)
   SDL_Renderer* dlgRenPtr = SDL_CreateRenderer(dlgWinPtr, -1, 0); /*SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC*/
   if (dlgRenPtr == NULL){
      fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
      SDL_DestroyWindow(dlgWinPtr);
      SDL_Quit();
      return 1;
   }
   SDL_SetRenderDrawColor(dlgRenPtr, components((unsigned)SDL_ALPHA_OPAQUE<<24|SC_BRWHITE)); // Alpha=full:SDL_ALPHA_OPAQUE, Black:R,G,B=0
   SDL_RenderClear(dlgRenPtr);
   struct TextWindowStruct dlgTxt; // dialog text window descriptor
   dlgTxt.texPtr = SDL_CreateTexture(dlgRenPtr, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, DIALOGWIDTH-DIALOGBORDER, DIALOGHEIGHT);
   if (dlgTxt.texPtr == NULL) {
      fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
      SDL_DestroyRenderer(dlgRenPtr);
      SDL_DestroyWindow(dlgWinPtr);
      SDL_Quit();
      return 1;
   }
   dlgTxt.pitch = (DIALOGWIDTH-DIALOGBORDER) * BYTESPERPIXEL;
   dlgTxt.frameSize = dlgTxt.pitch * DIALOGHEIGHT;
   dlgTxt.framePtr = malloc (dlgTxt.frameSize);
   if (dlgTxt.framePtr == NULL) {
      fprintf(stderr, "malloc Error\n");
      SDL_DestroyTexture(dlgTxt.texPtr);
      SDL_DestroyRenderer(dlgRenPtr);
      SDL_DestroyWindow(dlgWinPtr);
      SDL_Quit();
      return 1;
   }
   memset (dlgTxt.framePtr, SC_BRWHITE, dlgTxt.frameSize); // White
   dlgTxt.rect.x = DIALOGBORDER;
   dlgTxt.rect.y = 0;
   dlgTxt.rect.w = DIALOGWIDTH-DIALOGBORDER;
   dlgTxt.rect.h = DIALOGHEIGHT;
   dlgTxt.CurrentPrintPosX = 0;
   dlgTxt.CurrentPrintPosY = 0;

   // print error in dialog and show
   SDLTWE_PrintString (&dlgTxt, dlgMsgPtr, &CharSet, SC_BLACK, SC_BRWHITE); // Black on White
   SDL_UpdateTexture(dlgTxt.texPtr, NULL, dlgTxt.framePtr, dlgTxt.pitch); // copy Frame to Texture
   SDL_RenderCopy(dlgRenPtr, dlgTxt.texPtr, NULL, &dlgTxt.rect); // copy Texture to Renderer
   SDL_RenderPresent(dlgRenPtr); // show canvas

   // now wait for dialog close
   int RunLoop = 1;
   SDL_Event event;
   while (RunLoop) {
      while (SDL_PollEvent(&event)!=0) {
         switch (event.type) {
            case SDL_QUIT: // never happen with other windows
               RunLoop = 0;
               break;
            case SDL_WINDOWEVENT: // to catch close with other windows
               if (event.window.event==SDL_WINDOWEVENT_CLOSE) {
                  RunLoop = 0;
               }
               break;
            case SDL_KEYDOWN: // any key to exit
               RunLoop = 0;
               break;
         } // switch
      } // poll event
   } // while (RunLoop)
   free(dlgTxt.framePtr);
   SDL_DestroyTexture(dlgTxt.texPtr);
   SDL_DestroyRenderer(dlgRenPtr);
   SDL_DestroyWindow(dlgWinPtr);

   return 0;
} // missGame()


#ifndef NODL
#include <zip.h>
/****************************************************************************\
* unzipGame                                                                  *
*                                                                            *
\****************************************************************************/
int unzipGame(int hv) {
   //Open the ZIP archive
   int err = 0;
   chdir(savPath); // go to save path to let write zip/tap/tzx files
   struct zip* zipPtr = zip_open(zipFileName, 0, &err);
   if (zipPtr == NULL) {
      //fprintf(stderr, "WL: WARN cannot open zip file\n");
      chdir(cfgPath); // back to config path to let find assets files
      return -1;
   }
   //Search for the file of given name
   char namePtr[FILENAME_MAX];
   if (hv==V10) {
      strcpy(namePtr, V10_TAP_NAME);
   }
   if (hv==V12) {
      strcpy(namePtr, V12_TAP_NAME);
   }
   struct zip_stat zipStat;
   zip_stat_init(&zipStat);
   zip_stat(zipPtr, namePtr, 0, &zipStat);
   //Alloc memory for uncompressed contents
   uint64_t size=zipStat.size * sizeof(char);
   char* unzippedPtr = malloc(size);
   if (unzippedPtr == NULL) {
      fprintf(stderr, "WL: cannot allocate\n");
      chdir(cfgPath);
      return -1;
   }
   //Read the compressed file
   struct zip_file* zipFilePtr = zip_fopen(zipPtr, namePtr, 0);
   zip_fread(zipFilePtr, unzippedPtr, zipStat.size);
   zip_fclose(zipFilePtr);
   //And close the archive
   zip_close(zipPtr);
   // write the uncompressed file
   FILE* filePtr=fopen(namePtr, "wb");
   uintmax_t cnt=fwrite(unzippedPtr, 1, zipStat.size, filePtr);
   if (cnt != zipStat.size) {
      fprintf(stderr, "WL: file is:%ju but truncated to:%ju Bytes\n", (uintmax_t)zipStat.size, cnt);
      chdir(cfgPath);
      return -1;
   }
   printf("WL: unzipped file:'%s' sized:%ju Bytes\n", namePtr, cnt);
   fclose(filePtr);
   free(unzippedPtr);
   chdir(cfgPath); // back to config path to let find assets files

   return 0;
} // unzipGame()


#include <curl/curl.h>
/****************************************************************************\
* downloadGame                                                               *
*                                                                            *
\****************************************************************************/
int downloadGame(int hv) {
   CURL* curlPtr;
   FILE* filePtr;
   CURLcode res;
   //char* urlLnkPtr = "https://www.worldofspectrum.org/pub/sinclair/games/h/HobbitTheV1.2.tap.zip";
   //char zipFileName[FILENAME_MAX]; // = "HobbitTheV1.2.tap.zip";

   if (hv==OWN) return 1; // cannot download OWN version
   curlPtr = curl_easy_init();
   if (curlPtr==NULL) {
      fprintf(stderr, "WL: cannot init libcurl\n");
      chdir(cfgPath);
      return 1;
   }
   chdir(savPath); // go to save path to let write zip/tap/tzx files
   filePtr = fopen(zipFileName,"wb");
   curl_easy_setopt(curlPtr, CURLOPT_URL, urlLnkPtr);
   curl_easy_setopt(curlPtr, CURLOPT_FOLLOWLOCATION, 1L);
   curl_easy_setopt(curlPtr, CURLOPT_WRITEFUNCTION, NULL);
   curl_easy_setopt(curlPtr, CURLOPT_WRITEDATA, filePtr);
   res = curl_easy_perform(curlPtr);
   if (res!=0) {
      fprintf(stderr, "WL: download of 'The Hobbit' failed:%d\n", res);
      chdir(cfgPath);
      return 1;
   }
   printf ("WL: downloaded filename:'%s'\n", zipFileName);
   curl_easy_cleanup(curlPtr);
   fclose(filePtr);
   chdir(cfgPath); // back to config path to let find assets files

   return 0;
} // downloadGame()
#endif


/****************************************************************************\
* InitGame                                                                   *
*                                                                            *
\****************************************************************************/
int InitGame(int hv) {
   char GameFileName[FILENAME_MAX];
   char TapFileName[FILENAME_MAX];
   char TzxFileName[FILENAME_MAX];
   word GameStartAddress, GameLength;
   word TapLength, TzxLength;
   word TapStart, TzxStart;
   FILE *fp;
   word FileLength, BytesRead;

   switch (hv) {
      case OWN:
         printf("WL: Selected 'The Hobbit' binary version OWN\n");
         strcpy(GameFileName, FN_OWN);
         strcpy(TapFileName, OWN_TAP_NAME);
         strcpy(TzxFileName, OWN_TZX_NAME);
         GameStartAddress = STARTADR_OWN;
         GameLength = CODE_LENGTH_OWN;
         TapLength = OWN_TAP_LENGTH;
         TzxLength = OWN_TZX_LENGTH;
         TapStart = OWN_TAP_START;
         TzxStart = OWN_TZX_START;
         DictionaryBaseAddress = DICTIONARY_BASE_OWN;
         ObjectsIndexAddress = OBJECTS_INDEX_OWN;
         ObjectsAddress = OBJECTS_OWN;
         break;
      case V10:
         printf("WL: Selected 'The Hobbit' binary version 1.0\n");
         strcpy(GameFileName, FN_V10);
         strcpy(TapFileName, V10_TAP_NAME);
         strcpy(TzxFileName, V10_TZX_NAME);
         GameStartAddress = STARTADR_V10;
         GameLength = CODE_LENGTH_V10;
         TapLength = V10_TAP_LENGTH;
         TzxLength = V10_TZX_LENGTH;
         TapStart = V10_TAP_START;
         TzxStart = V10_TZX_START;
         DictionaryBaseAddress = DICTIONARY_BASE_V10;
         ObjectsIndexAddress = OBJECTS_INDEX_V10;
         ObjectsAddress = OBJECTS_V10;
         break;
      case V12:
         printf("WL: Selected 'The Hobbit' binary version 1.2\n");
         strcpy(GameFileName, FN_V12);
         strcpy(TapFileName, V12_TAP_NAME);
         strcpy(TzxFileName, V12_TZX_NAME);
         GameStartAddress = STARTADR_V12;
         GameLength = CODE_LENGTH_V12;
         TapLength = V12_TAP_LENGTH;
         TzxLength = V12_TZX_LENGTH;
         TapStart = V12_TAP_START;
         TzxStart = V12_TZX_START;
         DictionaryBaseAddress = DICTIONARY_BASE_V12;
         ObjectsIndexAddress = OBJECTS_INDEX_V12;
         ObjectsAddress = OBJECTS_V12;
         break;
      default:
         fprintf(stderr, "WL: ERROR 'InitGame': Unknown version %d.\n", hv);
         return -1;
   }

   // try to open .bin file
   chdir(savPath); // go to save path to let read zip/tap/tzx files
   fp = fopen(GameFileName, "rb");
   if (!fp) {
      fprintf(stderr, "WL: WARN 'InitGame': Can't open game file '%s'\n", GameFileName);
      // try to open .tap file
      fp = fopen(TapFileName, "rb");
      if (!fp) {
         fprintf(stderr, "WL: WARN 'InitGame': Can't open game file '%s'\n", TapFileName);
         // try to open .tzx file
         fp = fopen(TzxFileName, "rb");
         if (!fp) {
            fprintf(stderr, "WL: WARN 'InitGame': Can't open game file '%s'\n", TzxFileName);
            chdir(cfgPath); // back to config path to let find assets files
            return 1;
         }
         fseek(fp, 0, SEEK_END);
         FileLength = ftell(fp);
         if (FileLength == TzxLength) {
            fseek(fp, TzxStart, SEEK_SET);
            BytesRead = fread(ZXmem + GameStartAddress, 1, GameLength, fp);
            fclose(fp);
            if (BytesRead == GameLength) {
               printf("WL: Tape file:'%s' read successfully\n", TzxFileName);
               chdir(cfgPath); // back to config path to let find assets files
               return 0;
            } else {
               fprintf(stderr, "WL ERROR: File:'%s' len:%u read:%u expected:%u Bytes\n", TzxFileName, TzxLength, BytesRead, GameLength);
               chdir(cfgPath); // back to config path to let find assets files
               return -1;
            }
         }
         chdir(cfgPath); // back to config path to let find assets files
         return -1;
      }
      fseek(fp, 0, SEEK_END);
      FileLength = ftell(fp);
      if (FileLength == TapLength) {
         fseek(fp, TapStart, SEEK_SET);
         BytesRead = fread(ZXmem + GameStartAddress, 1, GameLength, fp);
         fclose(fp);
         if (BytesRead == GameLength) {
            printf("WL: Tape file:'%s' read successfully\n", TapFileName);
            chdir(cfgPath); // back to config path to let find assets files
            return 0;
         } else {
            fprintf(stderr, "WL ERROR: File:'%s' len:%u read:%u expected:%u Bytes\n", TapFileName, TapLength, BytesRead, GameLength);
            chdir(cfgPath); // back to config path to let find assets files
            return -1;
         }
      }
      chdir(cfgPath); // back to config path to let find assets files
      return -1;
   }
   fseek(fp, 0, SEEK_END);
   FileLength = ftell(fp);
   if (FileLength == GameLength) {
      fseek(fp, 0, SEEK_SET);
      if (WL_DEBUG) printf("WL: Reading game file '%s' with %i byte ... \n", GameFileName, FileLength);
      BytesRead = fread(ZXmem + GameStartAddress, 1, GameLength, fp);
      fclose(fp);
      if (BytesRead == GameLength) {
         printf("WL: Code file:'%s' read successfully\n", GameFileName);
         chdir(cfgPath); // back to config path to let find assets files
         return 0;
      } else {
         fprintf(stderr, "WL ERROR: File:'%s' len:%u read:%u expected:%u Bytes\n", GameFileName, GameLength, BytesRead, GameLength);
         chdir(cfgPath); // back to config path to let find assets files
         return -1;
      }
   }
   chdir(cfgPath); // back to config path to let find assets files
   return -1;
} // InitGame()


/****************************************************************************\
* InitSpectrum                                                               *
*                                                                            *
\****************************************************************************/
int InitSpectrum(void) {
   int i;
   FILE *fp;
   long int FileLength;
   size_t BytesRead;

   for (i = 0; i < SL_SCREENSTART; i++)
      ZXmem[i] = 0xC9;  /* = RET in ROM, just in case... */

   // we need the character set for the lower window
   fp = fopen(SDLTWE_CHARSETFILENAME, "rb");
   if (!fp) {
      fprintf(stderr, "ERROR in WL.c: can't open character set file '%s'\n", SDLTWE_CHARSETFILENAME);
      return 1;
   }
   fseek(fp, 0, SEEK_END);
   FileLength = ftell(fp);
   fseek(fp, 0, SEEK_SET);
   if (WL_DEBUG) {
      printf("WL: Reading character set with %li byte from %s ... ", FileLength, SDLTWE_CHARSETFILENAME);
   }
   BytesRead = fread(ZXmem + SL_CHARSET - 1, 1, FileLength, fp);
   fclose(fp);
   if (WL_DEBUG) {
      if (FileLength != BytesRead)
         printf("ERROR!\n");
      else
         printf("read!\n");
   }

   #if CPUEMUL == eZ80
      printf("WL: Using CPU emulator: 'Z80' by Marat Fayzullin\n");
      FileOfZ80 = sizeof(z80) - sizeof(void*) - ( ((void*)&(z80.User))-((void*)&(z80.Trace))-sizeof(z80.Trace) );
      // 48: fixed size vars(bye,word,int), skip void pointer and 32/64 bit padding
   #endif
   #if CPUEMUL == ez80emu
      printf("WL: Using CPU emulator: 'z80emu' by Lin Ke-Fong\n");
      FileOfZ80 = sizeof(z80) - 3*16*sizeof(void*) - ( ((void*)&(z80.register_table[0]))-((void*)&(z80.im))-sizeof(z80.im) );
      // 52: fixed size vars(char,short,int), skip register pointers area and 32/64 bit padding
   #endif
   //printf("SizeOfZ80:0x%zX FileOfZ80:0x%zX\n", SizeOfZ80, FileOfZ80);
   #if CPUEMUL == eZ80
      ResetZ80(&z80);
      switch (HV) {
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
   #endif
   #if CPUEMUL == ez80emu
      Z80Reset (&z80);
      switch (HV) {
         case OWN:
            z80.pc = L_START_OWN;
            break;
         case V10:
            z80.pc = L_START_V10;
            break;
         case V12:
            z80.pc = L_START_V12;
            break;
         default:
            exit(-1);
      }
      if (SeedRND)
         z80.r = (byte) SDL_GetTicks();
   #endif
   //printZ80struct();

   return 0;
} // InitSpectrum()


/****************************************************************************\
* InitGraphicsSystem                                                         *
*                                                                            *
* The global variables  SDL_Window *winPtr and renPtr must be defined!       *
\****************************************************************************/
int InitGraphicsSystem(uint32_t WinMode) {
   SDL_Surface* sfcPtr;

   if (SDL_Init(0) < 0) { // SDL_INIT_VIDEO
      fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
      return 1;
   }
   SDL_version compiled, linked;
   SDL_VERSION(&compiled); // SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL
   SDL_GetVersion(&linked);
   printf("WL: Compile time SDL v.%u.%u.%u, link time SDL v.%u.%u.%u\n",
            compiled.major, compiled.minor, compiled.patch,
            linked.major, linked.minor, linked.patch);

   if (dockMap==true) {
      if (!(winPtr = SDL_CreateWindow("Wilderland - A Hobbit Environment", 40, 24, MAINWINWIDTH, MAINWINHEIGHT, WinMode | SDL_WINDOW_HIDDEN|SDL_WINDOW_ALLOW_HIGHDPI))) { // SDL_WINDOW_OPENGL|SDL_WINDOW_VULKAN|SDL_WINDOW_FULLSCREEN_DESKTOP //SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_CENTERED
         fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
         SDL_Quit();
         return 1;
      }
      MapWinPtr = winPtr;
   } else {
      if (!(winPtr = SDL_CreateWindow("Wilderland - A Hobbit Environment", 40, 24, MAINWINWIDTH, MAPWINPOSY, WinMode | SDL_WINDOW_HIDDEN|SDL_WINDOW_ALLOW_HIGHDPI))) { // SDL_WINDOW_OPENGL|SDL_WINDOW_VULKAN|SDL_WINDOW_FULLSCREEN_DESKTOP //SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_CENTERED
         fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
         SDL_Quit();
         return 1;
      }
      MapWinPtr = SDL_CreateWindow("Wilderland - Map", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, MAPWINWIDTHND, MAPWINHEIGHTND, WinMode | SDL_WINDOW_HIDDEN|SDL_WINDOW_ALLOW_HIGHDPI); // SDL_WINDOW_OPENGL|SDL_WINDOW_VULKAN|SDL_WINDOW_FULLSCREEN_DESKTOP //SDL_WINDOWPOS_UNDEFINED
      if (MapWinPtr == NULL) {
         fprintf(stderr, "SDL_CreateWindow Error: %s\n", SDL_GetError());
         SDL_Quit();
         return 1;
      }
   }

   // Creating a Renderer (window, driver index, flags: SW|HW accelerated|vsync|texture)
#ifdef __MACH__ // macOS
   renPtr = SDL_CreateRenderer(winPtr, -1, SDL_RENDERER_SOFTWARE); // SDL_RENDERER_SOFTWARE|SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_TARGETTEXTURE
#endif
#if defined __gnu_linux__ || defined _WIN32
#if defined __arm__ || defined __thumb__ || defined __aarch64__   // should be for Linux/Raspberry Pi only
   renPtr = SDL_CreateRenderer(winPtr, -1, 0); // SDL_RENDERER_SOFTWARE|SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_TARGETTEXTURE
#else // Linux and Windows
   renPtr = SDL_CreateRenderer(winPtr, -1, SDL_RENDERER_ACCELERATED); // SDL_RENDERER_SOFTWARE|SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_TARGETTEXTURE
#endif
#endif
   if (renPtr == NULL){
      fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
      SDL_DestroyWindow(winPtr);
      SDL_Quit();
      return 1;
   }
   MapRenPtr = renPtr;
   if (dockMap==false) {
      #ifdef __MACH__
         MapRenPtr = SDL_CreateRenderer(MapWinPtr, -1, SDL_RENDERER_SOFTWARE); // SDL_RENDERER_SOFTWARE|SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_TARGETTEXTURE
      #endif
      #if defined __gnu_linux__ || defined _WIN32
      #if defined __arm__ || defined __thumb__ || defined __aarch64__   // should be for Linux only
         MapRenPtr = SDL_CreateRenderer(MapWinPtr, -1, 0); // SDL_RENDERER_SOFTWARE|SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_TARGETTEXTURE
      #else
         MapRenPtr = SDL_CreateRenderer(MapWinPtr, -1, SDL_RENDERER_ACCELERATED); // SDL_RENDERER_SOFTWARE|SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_TARGETTEXTURE
      #endif
      #endif
      if (MapRenPtr == NULL){
         fprintf(stderr, "SDL_CreateRenderer Error: %s\n", SDL_GetError());
         SDL_DestroyWindow(MapWinPtr);
         SDL_Quit();
         return 1;
      }
   }

   SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear"); // nearest, linear, best
   if (dockMap==true) {
      SDL_SetWindowSize(winPtr, MAINWINWIDTH, MAINWINHEIGHT);
      SDL_RenderSetLogicalSize(renPtr, MAINWINWIDTH, MAINWINHEIGHT);
   } else {
      SDL_SetWindowSize(winPtr, MAINWINWIDTH, MAPWINPOSY);
      SDL_RenderSetLogicalSize(renPtr, MAINWINWIDTH, MAPWINPOSY);
      SDL_SetWindowSize(MapWinPtr, MAPWINWIDTHND, MAPWINHEIGHTND);
      SDL_RenderSetLogicalSize(MapRenPtr, MAPWINWIDTHND, MAPWINHEIGHTND);
      SDL_SetRenderDrawColor(MapRenPtr, 0, 0, 0, 0); // Alpha=full:SDL_ALPHA_OPAQUE, Black:R,G,B=0
      SDL_RenderClear(MapRenPtr);
      //SDL_RenderPresent(MapRenPtr); // show BLACK background
      //SDL_Delay(delay); // ms
   }

   //SDL_SetRenderDrawColor(renPtr, components((uint32_t)SDL_ALPHA_OPAQUE<<24|SC_CYAN)); // Black:R,G,B=0, Alpha=full:SDL_ALPHA_OPAQUE
   SDL_SetRenderDrawColor(renPtr, 0x00, 0xC0, 0xC0, 0x00); // Black:R,G,B=0, Alpha=full:SDL_ALPHA_OPAQUE
   SDL_RenderClear(renPtr); // clear renderer with CYAN background
   SDL_Delay(delay); // to avoid flickering

#if 0
SDL_ShowWindow(winPtr);
if (dockMap==false) SDL_ShowWindow(MapWinPtr);
SDL_RaiseWindow(winPtr); // focus to main win
SDL_RenderPresent(renPtr);
if (dockMap==false) SDL_RenderPresent(MapRenPtr);
SDL_Delay(delay); // to avoid flickering
SDL_Delay(4000);
//SDL_Quit();
//exit(1);
#endif

   CharSet.Bitmap = malloc(SDLTWE_CHARSETLENGTH);
   if (!(CharSet.Bitmap)) {
      fprintf(stderr, "WL: ERROR in allocationg memory for character set.\n");
      return 1;
   }
   if (!SDLTWE_ReadCharSet(SDLTWE_CHARSETFILENAME, CharSet.Bitmap)) {
      fprintf(stderr, "WL: ERROR while reading character set.\n");
      return 1;
   }
   CharSet.Height = 8;
   CharSet.Width = 8;
   CharSet.CharMin = '\x20';
   CharSet.CharMax = '\x7F';
   CharSet.CharSubstitute = '?';

   /*** Create 5 accelerated Textures for: log, game, help, objects, map ***/
   LogWin.texPtr = SDL_CreateTexture(renPtr, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, LOGWINWIDTH, LOGWINHEIGHT);
   if (LogWin.texPtr == NULL) {
      fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
      SDL_DestroyRenderer(renPtr);
      SDL_DestroyWindow(winPtr);
      SDL_Quit();
      return 1;
   }
   LogWin.pitch = LOGWINWIDTH * BYTESPERPIXEL;
   LogWin.frameSize = LogWin.pitch * LOGWINHEIGHT;
   LogWin.framePtr = malloc (LogWin.frameSize);
   if (LogWin.framePtr == NULL) {
      fprintf(stderr, "malloc Error\n");
      SDL_DestroyTexture(LogWin.texPtr);
      SDL_DestroyRenderer(renPtr);
      SDL_DestroyWindow(winPtr);
      SDL_Quit();
      return 1;
   }
   memset (LogWin.framePtr, SC_BLACK, LogWin.frameSize); // Black
   LogWin.rect.x = LOGWINPOSX;
   LogWin.rect.y = LOGWINPOSY;
   LogWin.rect.w = LOGWINWIDTH;
   LogWin.rect.h = LOGWINHEIGHT;
   LogWin.CurrentPrintPosX = 0;
   LogWin.CurrentPrintPosY = 0;
   SDL_UpdateTexture(LogWin.texPtr, NULL, LogWin.framePtr, LogWin.pitch); // copy Frame to Texture
   SDL_RenderCopy(renPtr, LogWin.texPtr, NULL, &LogWin.rect); // copy Texture to Renderer

   GameWin.texPtr = SDL_CreateTexture(renPtr, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, GAMEWINWIDTH, GAMEWINHEIGHT);
   if (GameWin.texPtr == NULL) {
      fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
      SDL_DestroyRenderer(renPtr);
      SDL_DestroyWindow(winPtr);
      SDL_Quit();
      return 1;
   }
   GameWin.pitch = GAMEWINWIDTH * BYTESPERPIXEL;
   GameWin.frameSize = GameWin.pitch * GAMEWINHEIGHT;
   GameWin.framePtr = malloc (GameWin.frameSize);
   if (GameWin.framePtr == NULL) {
      fprintf(stderr, "malloc Error\n");
      SDL_DestroyTexture(GameWin.texPtr);
      SDL_DestroyRenderer(renPtr);
      SDL_DestroyWindow(winPtr);
      SDL_Quit();
      return 1;
   }
   memset (GameWin.framePtr, SC_BLACK, GameWin.frameSize); // Black
   GameWin.rect.x = GAMEWINPOSX;
   GameWin.rect.y = GAMEWINPOSY;
   GameWin.rect.w = GAMEWINWIDTH;
   GameWin.rect.h = GAMEWINHEIGHT;
   GameWin.CurrentPrintPosX = 0;
   GameWin.CurrentPrintPosY = 0;
   SDL_UpdateTexture(GameWin.texPtr, NULL, GameWin.framePtr, GameWin.pitch); // copy Frame to Texture
   SDL_RenderCopy(renPtr, GameWin.texPtr, NULL, &GameWin.rect); // copy Texture to Renderer

   HelpWin.texPtr = SDL_CreateTexture(renPtr, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, HELPWINWIDTH, HELPWINHEIGHT);
   if (HelpWin.texPtr == NULL) {
      fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
      SDL_DestroyRenderer(renPtr);
      SDL_DestroyWindow(winPtr);
      SDL_Quit();
      return 1;
   }
   HelpWin.pitch = HELPWINWIDTH * BYTESPERPIXEL;
   HelpWin.frameSize = HelpWin.pitch * HELPWINHEIGHT;
   HelpWin.framePtr = malloc (HelpWin.frameSize);
   if (HelpWin.framePtr == NULL) {
      fprintf(stderr, "malloc Error\n");
      SDL_DestroyTexture(HelpWin.texPtr);
      SDL_DestroyRenderer(renPtr);
      SDL_DestroyWindow(winPtr);
      SDL_Quit();
      return 1;
   }
   memset (HelpWin.framePtr, SC_BLACK, HelpWin.frameSize); // Black
   HelpWin.rect.x = HELPWINPOSX;
   HelpWin.rect.y = HELPWINPOSY;
   HelpWin.rect.w = HELPWINWIDTH;
   HelpWin.rect.h = HELPWINHEIGHT;
   HelpWin.CurrentPrintPosX = 0;
   HelpWin.CurrentPrintPosY = 0;
   SDL_UpdateTexture(HelpWin.texPtr, NULL, HelpWin.framePtr, HelpWin.pitch); // copy Frame to Texture
   SDL_RenderCopy(renPtr, HelpWin.texPtr, NULL, &HelpWin.rect); // copy Texture to Renderer

   ObjWin.texPtr = SDL_CreateTexture(renPtr, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, OBJWINWIDTH, OBJWINHEIGHT);
   if (ObjWin.texPtr == NULL) {
      fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
      SDL_DestroyRenderer(renPtr);
      SDL_DestroyWindow(winPtr);
      SDL_Quit();
      return 1;
   }
   ObjWin.pitch = OBJWINWIDTH * BYTESPERPIXEL;
   ObjWin.frameSize = ObjWin.pitch * OBJWINHEIGHT;
   ObjWin.framePtr = malloc (ObjWin.frameSize);
   if (ObjWin.framePtr == NULL) {
      fprintf(stderr, "malloc Error\n");
      SDL_DestroyTexture(ObjWin.texPtr);
      SDL_DestroyRenderer(renPtr);
      SDL_DestroyWindow(winPtr);
      SDL_Quit();
      return 1;
   }
   memset (ObjWin.framePtr, SC_BLACK, ObjWin.frameSize); // Black
   ObjWin.rect.x = OBJWINPOSX;
   ObjWin.rect.y = OBJWINPOSY;
   ObjWin.rect.w = OBJWINWIDTH;
   ObjWin.rect.h = OBJWINHEIGHT;
   ObjWin.CurrentPrintPosX = 0;
   ObjWin.CurrentPrintPosY = 0;
   SDL_UpdateTexture(ObjWin.texPtr, NULL, ObjWin.framePtr, ObjWin.pitch); // copy Frame to Texture
   SDL_RenderCopy(renPtr, ObjWin.texPtr, NULL, &ObjWin.rect); // copy Texture to Renderer

   if (dockMap==true) {
      MapWin.texPtr = SDL_CreateTexture(MapRenPtr, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, MAPWINWIDTH, MAPWINHEIGHT);
   } else {
      MapWin.texPtr = SDL_CreateTexture(MapRenPtr, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, MAPWINWIDTHND, MAPWINHEIGHTND);
   }
   if (MapWin.texPtr == NULL) {
      fprintf(stderr, "SDL_CreateTexture Error: %s\n", SDL_GetError());
      SDL_DestroyRenderer(renPtr);
      SDL_DestroyWindow(winPtr);
      SDL_Quit();
      return 1;
   }
   if (dockMap==true) {
      MapWin.pitch = MAPWINWIDTH * BYTESPERPIXEL;
      MapWin.frameSize = MapWin.pitch * MAPWINHEIGHT;
   } else {
      MapWin.pitch = MAPWINWIDTHND * BYTESPERPIXEL;
      MapWin.frameSize = MapWin.pitch * MAPWINHEIGHTND;
   }
   MapWin.framePtr = malloc (MapWin.frameSize);
   if (MapWin.framePtr == NULL) {
      fprintf(stderr, "malloc Error\n");
      SDL_DestroyTexture(MapWin.texPtr);
      SDL_DestroyRenderer(renPtr);
      SDL_DestroyWindow(winPtr);
      SDL_Quit();
      return 1;
   }
   memset (MapWin.framePtr, SC_BLACK, MapWin.frameSize); // Black
   MapWin.rect.x = MAPWINPOSX;
   MapWin.rect.y = MAPWINPOSY;
   MapWin.rect.w = MAPWINWIDTH;
   MapWin.rect.h = MAPWINHEIGHT;
   if (dockMap==false) {
      MapWin.rect.x = 0;
      MapWin.rect.y = 0;
      MapWin.rect.w = MAPWINWIDTHND;
      MapWin.rect.h = MAPWINHEIGHTND;
   }
   MapWin.CurrentPrintPosX = 0;
   MapWin.CurrentPrintPosY = 0;
   SDL_UpdateTexture(MapWin.texPtr, NULL, MapWin.framePtr, MapWin.pitch); // copy Frame to Texture
   SDL_RenderCopy(MapRenPtr, MapWin.texPtr, NULL, &MapWin.rect); // copy Texture to Renderer
   //SDL_RenderPresent(renPtr); // show empty canvas
   //SDL_Delay(delay); // ms

   // Load Game Map
   IMG_Init(IMG_INIT_PNG);
   if (dockMap==true) {
      sfcPtr = IMG_Load(GAMEMAPFILENAME);
   } else {
      sfcPtr = IMG_Load(GAMEMAPFILENAMEND);
   }
   if (sfcPtr == NULL) {
       fprintf(stderr, "WL: ERROR in 'InitGraphicsSystem'. Unable to load '"GAMEMAPFILENAME"'\n");
       return -1;
   }
   MapSfcPtr = SDL_ConvertSurfaceFormat(sfcPtr, SDL_PIXELFORMAT_ARGB8888, 0);
   SDL_FreeSurface(sfcPtr);
   if (MapSfcPtr == NULL) {
      fprintf(stderr, "SDL_ConvertSurfaceFormat Error: %s\n", SDL_GetError());
      SDL_DestroyRenderer(renPtr);
      SDL_DestroyWindow(winPtr);
      SDL_Quit();
      return 1;
   }
   int MapFrameSize = MapSfcPtr->pitch * MapSfcPtr->h;
   if (MapFrameSize != MapWin.frameSize) {
      fprintf(stderr, "MapSfcPtr size:%u expected:%u\n", MapFrameSize, MapWin.frameSize);
      SDL_FreeSurface(MapSfcPtr);
      SDL_DestroyTexture(MapWin.texPtr);
      SDL_DestroyRenderer(renPtr);
      SDL_DestroyWindow(winPtr);
      SDL_Quit();
      return 1;
   }

    return 0;
} // InitGraphicsSystem()


/****************************************************************************\
* readCfg                                                                    *
*                                                                            *
\****************************************************************************/
int readCfg() {
   FILE* filePtr;
   long int fileLength;
   size_t bytesRead;
   char* strPtr; // all config file from the beginning
   char* msgPtr; // begin of config param value
   char* endPtr; // end of config param value
   char prmStrPtr[128];
   size_t len;

   // read the config file for error messages and download links/URL
   filePtr = fopen(WLCONFIG, "rb");
   if (!filePtr) {
      fprintf(stderr, "ERROR in WL.c: can't open config file '%s'\n", WLCONFIG);
      return 1;
   }
   fseek(filePtr, 0, SEEK_END);
   fileLength = ftell(filePtr);
   strPtr=malloc(fileLength);
   if (strPtr==NULL) {
      fprintf(stderr, "readCfg() ERROR: cannot malloc\n");
      return 1;
   }
   fseek(filePtr, 0, SEEK_SET);
   if (WL_DEBUG) {
      printf("WL: Reading config file with %li byte from %s ... ", fileLength, WLCONFIG);
   }
   bytesRead = fread(strPtr, 1, fileLength, filePtr);
   fclose(filePtr);
   if (WL_DEBUG) {
      if (fileLength != bytesRead)
         printf("ERROR!\n");
      else
         printf("read!\n");
   }

   strcpy(prmStrPtr, "missGameStr=\"");
   len=strlen(prmStrPtr);
   msgPtr=strstr(strPtr, prmStrPtr); // look for param name
   if (msgPtr==NULL) {
      fprintf(stderr, "'%s' not found in config file\n", prmStrPtr);
      return 1;
   }
   msgPtr=msgPtr+len; // skip param name
   endPtr=strstr(msgPtr, "\""); // look for end of value
   if (endPtr==NULL) {
      fprintf(stderr, "'\"' not found in config file\n");
      return 1;
   }
   len=endPtr-msgPtr; // size of value
   dlgMsgPtr=malloc(len+1);
   strncpy(dlgMsgPtr, msgPtr, len);
   *(dlgMsgPtr+len)='\0';
   //printf("dlgMsg:'%s'\n", dlgMsgPtr);

   strcpy(prmStrPtr, "linkGame12Str=\"");
   if (HV==V10) strcpy(prmStrPtr, "linkGame10Str=\"");
   len=strlen(prmStrPtr);
   msgPtr=strstr(strPtr, prmStrPtr); // look for param name
   if (msgPtr==NULL) {
      fprintf(stderr, "'%s' not found in config file\n", prmStrPtr);
      return 1;
   }
   msgPtr=msgPtr+len; // skip param name
   endPtr=strstr(msgPtr, "\""); // look for end of value
   if (endPtr==NULL) {
      fprintf(stderr, "'\"' not found in config file\n");
      return 1;
   }
   len=endPtr-msgPtr; // size of value
   urlLnkPtr=malloc(len+1);
   strncpy(urlLnkPtr, msgPtr, len);
   *(urlLnkPtr+len)='\0';
   //printf("urlLnk:'%s'\n", urlLnkPtr);

//strcpy(zipFileName, "HobbitTheV1.2.tap.zip");
//printf("zipFileName:'%s'\n", zipFileName);
   endPtr=urlLnkPtr+strlen(urlLnkPtr);
   char* p;
   for (p=endPtr; *p!='/'; p--) {} // look for last '/'
   p++; // first char of filename
   strcpy(zipFileName, p);
   //printf("zipFileName:'%s'\n", zipFileName);

   free(strPtr);
   return 0;
} // readCfg()


/****************************************************************************\
* Help line                                                                  *
*                                                                            *
\****************************************************************************/
void helpLine() {
   printf("Syntax:\n");
   printf(" WL [-V10|-OWN|-V12] [-FULLSCREEN|FIT|MAP] [-MAXSPEED] [-NOSCANLINES] [-SEEDRND]\n");
   printf(" WL [-HELP]\n");
} // helpLine()


/****************************************************************************\
* create directory to save/load WLS files                                    *
*                                                                            *
\****************************************************************************/
void mkWlsDir() {
   chdir(savPath); // go to save path where can write files
#ifndef _WIN32 // all but Windows
   mode_t mode=S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH; // 0755
   int ret=mkdir("wls", mode);
#else // MinGW/Windows
   int ret=mkdir("wls");
#endif
   //printf("WL: ret:%d returned by 'mkdir wls'\n", ret);
   if(ret && errno != EEXIST) {
      fprintf(stderr, "WL: ERROR trying to create 'wls' directory\n");
      exit(-1);
   }
   //if(ret!=0) fprintf(stderr, "WL: 'wls' directory already exist, continue\n");
   chdir(cfgPath); // back to config path to let find assets files
   return;
} // mkWlsDir()


/****************************************************************************\
* from current working directory evaluate Config/Save files path             *
*                                                                            *
\****************************************************************************/
void readDirs(char* arg0) {
   realpath(arg0, savPath);
   //printf("realpath(arg0)='%s'\n", savPath);
   int len=strlen(savPath);
   for (int l=len; l>0; l--) {
      if (savPath[l]=='/' || savPath[l]=='\\') {
         savPath[l]='\0';
         break;
      }
   }
   //printf("savPath   ='%s'\n", savPath);
   char* appDirPtr;
   appDirPtr=getenv("APPDIR"); // to support Linux AppImage
   //printf("$APPDIR   ='%s'\n", appDirPtr);
   if (appDirPtr==NULL) {
      //printf("not AppImage\n");
      strcpy(binPath, savPath);
   } else { // support for Linux AppImage
      //printf("is AppImage\n");
      strcpy(binPath, appDirPtr);
      strcat(binPath, "/usr/bin");
   }
   strcpy(cfgPath, binPath); // as now RO config path = bin path
   strcpy(wlsPath, savPath); // as now RW WLS path = Save path
   strcat(wlsPath, "/wls");
   //printf("binPath='%s'\n", binPath); // AppImage can exec  binPath
   //printf("cfgPath='%s'\n", cfgPath); // AppImage can read  cfgPath
   //printf("savPath='%s'\n", savPath); // AppImage can write savPath
   //printf("wlsPath='%s'\n", wlsPath); // AppImage can write wlsPath
   return;
} // readDirs()


/****************************************************************************\
*                                   main                                     *
*                                                                            *
\****************************************************************************/
#if defined(__MINGW32__) || defined(__MINGW64__) // to have stdout/err
    #undef main // Prevent SDL from overriding the program's entry point.
#endif
int main(int argc, char *argv[]) {
   SDL_Event event;
   int i;
   int RunMainLoop;

   byte b;
   FILE *f;
   uint32_t WinMode = 0; // window mode (vs. full screen);
   int MaxSpeed = 0;
   uint32_t msTimer = 0; // time/tickstamp of prev loop
   int32_t DeltaT = 0;   // time spent in the loop
   int FrameCount = 0;

   static byte ObjectsPerRoom[ROOMS_NROF_MAX]; // 0x50=80

   HV = -1;  // Hobbit version (global variable)

   printf("Wilderland - A Hobbit Environment v"WLVER" %ubit\n", bit);
   printf("(c) 2012-2019 by CH, Copyright 2019-"WLYEAR" Valerio Messina\n");

   // process command line arguments
   int fl=0;
   for (i = 1; i < argc; i++, fl=0) {
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
      if (!strcmp(argv[i], "-map") || !strcmp(argv[i], "-MAP"))
         { dockMap=false; fl = 1; }
      if (!strcmp(argv[i], "-maxspeed") || !strcmp(argv[i], "-MAXSPEED"))
         { MaxSpeed = 1; fl = 1; }
      if (!strcmp(argv[i], "-noscanlines") || !strcmp(argv[i], "-NOSCANLINES"))
         { NoScanLines = 1; fl = 1; }
      if (!strcmp(argv[i], "-seedrnd") || !strcmp(argv[i], "-SEEDRND"))
         { SeedRND = 1; fl = 1; }
      if (!strcmp(argv[i], "-help") || !strcmp(argv[i], "-HELP") ||
          !strcmp(argv[i], "--help") || !strcmp(argv[i], "--HELP") ||
          !strcmp(argv[i], "-h") || !strcmp(argv[i], "-H")) {
         helpLine();
         exit (0);
      }
      if (fl == 0) {
         printf("WL: Ignoring unknown option: '%s'\n", argv[i]);
         helpLine();
      }
   }

   if (HV == -1) HV = V12; // default to V12

   readDirs(argv[0]); // this is OK for Linux(AppImage too), Win, macOS
   mkWlsDir(); // create WLS directory
   chdir(cfgPath); // go to config path to let find assets files
   readCfg(); // read configuration parameters

   if (InitGraphicsSystem(WinMode)) {
      fprintf(stderr, "WL: ERROR initializing graphic system. Program aborted.\n");
      exit(-1);
   }

   if (InitSpectrum()) {
      fprintf(stderr, "WL: ERROR initializing Spectrum. Program aborted.\n");
      exit(-1);
   }

   int ret=InitGame(HV);
   if (ret!=0) { // try call downloadGame(), unzipGame()
      #ifndef NODL
         ret=unzipGame(HV);
         if (ret!=0) {
            fprintf(stderr, "WL: WARN unzip of 'The Hobbit' failed\n");
            ret=downloadGame(HV);
            if (ret!=0) {
               fprintf(stderr, "WL: download of 'The Hobbit' failed\n");
               missGame();
               fprintf(stderr, "WL: ERROR initializing game. Program aborted.\n");
               exit(-1);
            }
            ret=unzipGame(HV);
            if (ret!=0) {
               fprintf(stderr, "WL: unzip of 'The Hobbit' failed\n");
               missGame();
               fprintf(stderr, "WL: ERROR initializing game. Program aborted.\n");
               exit(-1);
            }
         }
         ret=InitGame(HV);
         if (ret!=0) {
            missGame();
            fprintf(stderr, "WL: ERROR initializing game. Program aborted.\n");
            exit(-1);
         }
         goto initdone;
      #endif
      missGame();
      fprintf(stderr, "WL: ERROR initializing game. Program aborted.\n");
      exit(-1);
   }

#ifndef NODL
initdone:
#endif
   // game loaded, so can show (empty) windows
   printf("WL: init done, starting GUI ...\n");
   SDL_ShowWindow(winPtr);
   if (dockMap==false) SDL_ShowWindow(MapWinPtr);
   SDL_RaiseWindow(winPtr); // focus to main win
   SDL_Delay(delay); // to avoid flickering

#if 0
SDL_Delay(1000);
SDL_RenderPresent(renPtr);
if (dockMap==false) SDL_RenderPresent(MapRenPtr);
//SDL_Delay(1000);
//SDL_Quit();
//exit(1);
#endif

   /*** start to draw on the empty renderer ***/
   SDLTWE_DrawTextWindowFrame(&LogWin , 4, BORDERGRAY);
   SDLTWE_DrawTextWindowFrame(&GameWin, 4, BORDERGRAY);
   SDLTWE_DrawTextWindowFrame(&HelpWin, 4, BORDERGRAY);
   SDLTWE_DrawTextWindowFrame(&ObjWin , 4, BORDERGRAY);

   // Game title screen
   if (HV == V12) {
      f = fopen(TITLESCREEN12FILENAME, "rb");
      if (!f) {
         fprintf(stderr, "WL: ERROR - Can't open title screen file '"TITLESCREEN12FILENAME"'\n");
         exit(-1);
      }
   } else {
      f = fopen(TITLESCREEN10FILENAME, "rb");
      if (!f) {
         fprintf(stderr, "WL: ERROR - Can't open title screen file '"TITLESCREEN10FILENAME"'\n");
         exit(-1);
      }
   }
   for (i = SL_SCREENSTART; i <= SL_ATTRIBUTEEND; i++) {
      if (fscanf(f, "%c", &b) != 1) {
         fprintf(stderr, "WL: Error - %%c does not match format\n");
         fclose(f);
         exit(-1);
      }
      WrZ80(i, b);
   }
   fclose(f);
   SDL_UpdateTexture(GameWin.texPtr, NULL, GameWin.framePtr, GameWin.pitch); // copy Frame to Texture
   SDL_RenderCopy(renPtr, GameWin.texPtr, NULL, &GameWin.rect); // GAMEWIN texture to all Renderer
   SDL_Delay(delay); // to avoid cyan border sometime missing

   PrepareGameMap(); // empty map
   DisplayGameMap(); // SDL_UpdateTexture() and SDL_RenderCopy()
   SDL_Delay(delay); // to avoid cyan border sometime missing

   // show windows contents
   SDL_RenderPresent(renPtr);
   if (dockMap==false) SDL_RenderPresent(MapRenPtr);
   SDL_Delay(delay); // to avoid cyan border sometime missing

   Help(&HelpWin, &CharSet); // call SDL_UpdateTexture(),SDL_RenderCopy(),SDL_RenderPresent()
   SDL_Delay(delay); // to avoid cyan border sometime missing

   SDLTWE_PrintCharTextWindow(&LogWin, '\f', &CharSet, SC_BLACK, SC_WHITE); // clear LOG
   // ^^^^^^^^^^^^^ call SDL_UpdateTexture(),SDL_RenderCopy(),SDL_RenderPresent() only for scrool effect
   SDL_Delay(delay); // to avoid cyan border sometime missing

#if 0
//SDL_Log("out\n");
SDL_Delay(100);
exit(0);
#endif

   while(SDL_PollEvent(&event)) {} // discard prev queue

   /****************************** MAIN LOOP ******************************/
   RunMainLoop = 1;
   while (RunMainLoop) {

      // we run the main loop with FPS Hz
      DeltaT = SDL_GetTicks() - msTimer;  // time spent on one loop. On Intel Core2 Duo E6850@3GHz about 5 ms
      //SDL_Log("FrameCount:%02u DeltaT:%d msTimer:%u", FrameCount, DeltaT, msTimer);
#if 0
      if (MaxSpeed) {
         if (FrameCount == FPS-1) SDL_Log("spent time DeltaT:%d/40 ms running at %.2fx original speed", DeltaT, (float)DELAY_MS/DeltaT); // DELAY_MS = 40 ms/frame
      } else
         SDL_Log("spent time DeltaT:%d/40 ms", DeltaT); // DELAY_MS = 40 ms/frame
#endif
      if (DeltaT < DELAY_MS) {
         if (!MaxSpeed)
            SDL_Delay(DELAY_MS - DeltaT); // wait to finish the 40 ms
         else
            SDL_Delay(1); // try to avoid flicker
      } // else DeltaT = DELAY_MS;
      msTimer = SDL_GetTicks(); // start stopwatch
      FrameCount++;
      //SDL_Log("FrameCount:%02u DeltaT:%02d msTimer:%u", FrameCount, DeltaT, msTimer);

      #if CPUEMUL == eZ80
         ExecZ80 (&z80, TSTATES_PER_LOOP); // execute for about 140 kperiods
      #endif
      #if CPUEMUL == ez80emu
         Z80Emulate (&z80, TSTATES_PER_LOOP, &context); // execute for about 140 kperiods
      #endif
      SDL_UpdateTexture(LogWin.texPtr, NULL, LogWin.framePtr, LogWin.pitch); // copy Frame to Texture
      SDL_RenderCopy(renPtr, LogWin.texPtr, NULL, &LogWin.rect); // Texture to renderer
      SDL_UpdateTexture(GameWin.texPtr, NULL, GameWin.framePtr, GameWin.pitch); // copy Frame to Texture
      SDL_RenderCopy(renPtr, GameWin.texPtr, NULL, &GameWin.rect); // GAMEWIN texture to all Renderer
      // here the helpWin do not change
      PrintObjectsList(ObjectsIndexAddress, ObjectsAddress, &ObjWin, &CharSet, SC_BRYELLOW, SC_BRBLUE);
      SDL_UpdateTexture(ObjWin.texPtr, NULL, ObjWin.framePtr, ObjWin.pitch); // copy Frame to Texture
      SDL_RenderCopy(renPtr, ObjWin.texPtr, NULL, &ObjWin.rect); // OBJWIN texture to all Renderer
      dp=1; // used to hack unknown property 04 and 06

      if (FrameCount >= FPS) { // update map only once every 25 frame
         FrameCount = 0;
         for (i = 0; i < ROOMS_NROF_MAX; i++) // 0x50=80
            ObjectsPerRoom[i] = 0;
         PrepareGameMap(); // empty map as background
         PrepareAnimalPositions(&MapWin, &CharSet, ObjectsPerRoom);
         DisplayGameMap(); // SDL_UpdateTexture() and SDL_RenderCopy()
      }
      SDL_RenderPresent(renPtr); // update screen
      if (dockMap==false) SDL_RenderPresent(MapRenPtr); // update screen
      SDL_Delay(delay); // ms

      while (SDL_PollEvent(&event)!=0) { // poll until all events are handled

         switch (event.type) {
         case SDL_QUIT:
            RunMainLoop = 0;
            //SDL_Log("SDL_QUIT");
            break;
         case SDL_WINDOWEVENT: // to catch close when dockMap=false;
            //SDL_Log("SDL_WINDOWEVENT");
            if (event.window.event==SDL_WINDOWEVENT_CLOSE) {
               //SDL_Log("Window %d closed", event.window.windowID);
               RunMainLoop = 0;
            }
            break;
         case SDL_KEYDOWN:
            //printf("key down\n");
            CurrentPressedKey = event.key.keysym.sym;
            CurrentPressedMod = event.key.keysym.mod;
            //printf("key:%d mod:%d\n", CurrentPressedKey, CurrentPressedMod);
            if (CurrentPressedKey == SDLK_BACKSPACE)
               CurrentPressedKey = SDLK_0; // '0' used as backspace
            if (CurrentPressedMod&KMOD_CAPS || CurrentPressedMod&KMOD_SHIFT) { // capital
               if (CurrentPressedKey == SDLK_q) // 'Q' quit
                  RunMainLoop = 0;
               if (CurrentPressedKey == SDLK_s) { // 'S' save
                  SaveGame(&HelpWin, &CharSet, HV, SC_BRGREEN, SC_BRBLACK);
                  CurrentPressedKey = 0;
               }
               if (CurrentPressedKey == SDLK_l) { // 'L' load
                  LoadGame(&HelpWin, &CharSet, HV, SC_BRGREEN, SC_BRBLACK);
                  CurrentPressedKey = 0;
               }
               if (CurrentPressedKey == SDLK_h) { // 'H' help
                  Help(&HelpWin, &CharSet);
                  CurrentPressedKey = 0;
               }
               if (CurrentPressedKey == SDLK_i) { // 'I' info
                  Info(&HelpWin, &CharSet);
                  CurrentPressedKey = 0;
               }
               if (CurrentPressedKey == SDLK_g) { // 'G' goRoom
                  Go(&HelpWin, &CharSet, HV, SC_BRGREEN, SC_BRBLACK);
                  CurrentPressedKey = 0;
               }
            }
            while(SDL_PollEvent(&event)) {} // discard (keys) queue
            break;
         case SDL_KEYUP:
            //printf("key up\n\n");
            CurrentPressedKey = 0;
            break;
         } // switch

      } // poll event

   } // while (RunMainLoop)

   if (LogWin.framePtr)  free(LogWin.framePtr);
   if (GameWin.framePtr) free(GameWin.framePtr);
   if (HelpWin.framePtr) free(HelpWin.framePtr);
   if (ObjWin.framePtr)  free(ObjWin.framePtr);
   if (MapWin.framePtr)  free(MapWin.framePtr);
   if (MapSfcPtr)  SDL_FreeSurface(MapSfcPtr);
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
