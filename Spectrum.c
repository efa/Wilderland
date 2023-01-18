/****************************************************************************\
*                                                                            *
*                                 Spectrum.c                                 *
*                                                                            *
* Spectrum specific subroutines for the WL project                           *
*                                                                            *
* (c) 2012-2019 by CH, Copyright 2019-2022 Valerio Messina                   *
*                                                                            *
* V 2.09 - 20220907                                                          *
*                                                                            *
*  Spectrum.c is part of Wilderland - A Hobbit Environment                   *
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
#include <stdlib.h>
#include <string.h>

#include "Spectrum.h"
#include "WL.h"
#include "SDLTWE.h"

/*** GLOBAL VARIABLES *******************************************************/
#include "GlobalVars.h"

color_t ColorTable[] = {
   SC_BLACK, SC_BLUE, SC_RED, SC_MAGENTA, SC_GREEN, SC_CYAN, SC_YELLOW, SC_WHITE,
   SC_BRBLACK, SC_BRBLUE, SC_BRRED, SC_BRMAGENTA, SC_BRGREEN, SC_BRCYAN, SC_BRYELLOW, SC_BRWHITE
};

// Spectrum half row port addresses
//SHRP shrp[] = { SHRP_VCXZc, SHRP_GFDSA, SHRP_TREWQ, SHRP_54321, SHRP_67890, SHRP_YUIOP, SHRP_HJKLe, SHRP_BNMas };


/****************************************************************************\
* SetQuadPixel                                                               *
*                                                                            *
\****************************************************************************/
void SetQuadPixel(struct TextWindowStruct *TW, int x, int y, color_t color) {
   uint32_t* pixmem32;

   pixmem32 = (uint32_t*)TW->framePtr + 2 * (y * TW->rect.w) + 2 * x;
   *pixmem32++ = color;
   *pixmem32 = color;

   if (NoScanLines) {
      pixmem32 += TW->rect.w; //pitch / BYTESPERPIXEL : below 1 pixel line
      *pixmem32-- = color;
      *pixmem32 = color;
   }

   SDLTWE_SetPixel(TW, 2*x  , 2*y, color);
   SDLTWE_SetPixel(TW, 2*x+1, 2*y, color);
   if (NoScanLines) {
      SDLTWE_SetPixel(TW, 2*x  , 2*y+1, color);
      SDLTWE_SetPixel(TW, 2*x+1, 2*y+1, color);
   }
}


/****************************************************************************\
* WriteScreenByte update Game Texture with Spectrum frame buffer             *
*                                                                            *
\****************************************************************************/
void WriteScreenByte(word address, byte v) {
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
   for (i = 0; i <= 7; i++, mask >>= 1) {
      //SetQuadPixel(&GameWin, GAMEWINPOSX / 2 + (xx << 3) + i, GAMEWINPOSY / 2 + yy, (v & mask) ? ColI : ColP);
      SetQuadPixel(&GameWin, (xx << 3) + i, yy, (v & mask) ? ColI : ColP);
   }
}


/****************************************************************************\
* WriteAttributeByte                                                         *
*                                                                            *
\****************************************************************************/
void WriteAttributeByte(word address, byte v) {
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
* PrintObjNrActingAnimal                                                     *
*                                                                            *
\****************************************************************************/
void PrintObjNrActingAnimal(byte onaa) {
   char aa[20];

   strcpy(aa, "[");
   sprintf(aa+1, "%02X", onaa);
   strcat(aa,"]:");

   SDLTWE_PrintString(&LogWin, aa, &CharSet, SC_BLACK, SC_WHITE);
}


/****************************************************************************\
* RdZ80                                                                      *
*                                                                            *
\****************************************************************************/
byte RdZ80(word A) {
   return (ZXmem[A]);
}


/****************************************************************************\
* RdwZ80                                                                     *
*                                                                            *
\****************************************************************************/
word RdwZ80(word a) {
   byte h, l;
   l = RdZ80(a);
   h = RdZ80(a+1);
   return (((word)h)<<8 | l);
}


/****************************************************************************\
* WrZ80                                                                      *
*                                                                            *
\****************************************************************************/
void WrZ80(word A, byte v) {
   if (A <= SL_ROMEND) // skip write to the 16 k ROM
      return;
   ZXmem[A] = v; // update Spectrum memory
   if (A <= SL_SCREENEND) { // write to the frame buffer
      WriteScreenByte(A, v); // update SDL screen
      return;
   }
   if (A <= SL_ATTRIBUTEEND) { // write to the attrib buffer
      WriteAttributeByte(A, v); // update SDL screen
      return;
   }
}


/****************************************************************************\
* WrwZ80                                                                     *
*                                                                            *
\****************************************************************************/
void WrwZ80(word a, word w) {
   byte h, l;
   l = w & 0xFF;
   h = w >> 8;
   WrZ80(a, l);
   WrZ80(a+1, h);
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
byte InZ80(word P) {
   if ((P & 0xFF) != 0xFE) { // we only support the keyboard port 0xFE
      printf("Spectrum.c: InZ80() tried to read from unsupported port %04X.\n", P);
      return (0xFF);
   }

   if (P == 0x00FE) { // all keyboard rows at once (the upper 8 bit select the half row)
      if (CurrentPressedKey) {
         // happen on: no-key, ENTER=13, 32=' ',
         // 48=0, ..., 57=9, CURSORS<53-56>,
         // 97=a, ..., 122=z,
         // 232=, 242=, 249=
         // 1073742049=SDLK_LSHIFT, 1073742053=SDLK_RSHIFT, 1073741881=SDLK_CAPSLOCK, 1073742050=SDLK_LALT
         if (CurrentPressedKey!=SDLK_RETURN && CurrentPressedKey!=SDLK_SPACE && \
            (CurrentPressedKey<SDLK_0 || CurrentPressedKey>SDLK_9) && \
            (CurrentPressedKey<SDLK_a || CurrentPressedKey>SDLK_z) && \
            CurrentPressedKey!=SDLK_LSHIFT && CurrentPressedKey!=SDLK_RSHIFT && \
            CurrentPressedKey!=SDLK_LALT   && CurrentPressedKey!=SDLK_RALT   && \
            CurrentPressedKey!=SDLK_CAPSLOCK && \
            CurrentPressedKey!=232 && CurrentPressedKey!=242 && CurrentPressedKey!=249) \
               printf("P==0x00FE CurrentPressedKey:%d\n", CurrentPressedKey);
         switch (CurrentPressedKey) {
            case 'a':
            case 'q':
            case '1':
            case '0':
            case 'p':
            case SDLK_RETURN:
            case ' ':
               CurrentPressedKey=0;
               return (0xFF - 0x01);
            case 'z':
            case 's':
            case 'w':
            case '2':
            case '9':
            case 'o':
            case 'l':
               CurrentPressedKey=0;
               return (0xFF - 0x02);
            case 'x':
            case 'd':
            case 'e':
            case '3':
            case '8':
            case 'i':
            case 'k':
            case 'm':
               CurrentPressedKey=0;
               return (0xFF - 0x04);
            case 'c':
            case 'f':
            case 'r':
            case '4':
            case '7':
            case 'u':
            case 'j':
            case 'n':
               CurrentPressedKey=0;
               return (0xFF - 0x08);
            case 'v':
            case 'g':
            case 't':
            case '5':
            case '6':
            case 'y':
            case 'h':
            case 'b':
               CurrentPressedKey=0;
               return (0xFF - 0x10);
            default:
               CurrentPressedKey=0;
               return (0xFF);
         }
      } else {
         return (0xFF);  //no key pressed
      }
   }

   P >>= 8;

   switch (CurrentPressedKey) {
      // CAPS SHIFT is ignored
      case 'z':
         if (P == SHRP_VCXZc)
            return (0xFF - 0x02);
         break;
      case 'x':
         if (P == SHRP_VCXZc)
            return (0xFF - 0x04);
         break;
      case 'c':
         if (P == SHRP_VCXZc)
            return (0xFF - 0x08);
         break;
      case 'v':
         if (P == SHRP_VCXZc)
            return (0xFF - 0x10);
         break;

      case 'a':
         if (P == SHRP_GFDSA)
            return (0xFF - 0x01);
         break;
      case 's':
         if (P == SHRP_GFDSA)
            return (0xFF - 0x02);
         break;
      case 'd':
         if (P == SHRP_GFDSA)
            return (0xFF - 0x04);
         break;
      case 'f':
         if (P == SHRP_GFDSA)
            return (0xFF - 0x08);
         break;
      case 'g':
         if (P == SHRP_GFDSA)
            return (0xFF - 0x10);
         break;

      case 'q':
         if (P == SHRP_TREWQ)
            return (0xFF - 0x01);
         break;
      case 'w':
         if (P == SHRP_TREWQ)
            return (0xFF - 0x02);
         break;
      case 'e':
         if (P == SHRP_TREWQ)
            return (0xFF - 0x04);
         break;
      case 'r':
         if (P == SHRP_TREWQ)
            return (0xFF - 0x08);
         break;
      case 't':
         if (P == SHRP_TREWQ)
            return (0xFF - 0x10);
         break;

      case '1':
         if (P == SHRP_54321)
            return (0xFF - 0x01);
         break;
      case '2':   // ALT|SHIFT+2 = US:'@' or IT:'"' ==>
         //printf("port:0x%X Mod:%u Key:%u out='@'\n", P, CurrentPressedMod, CurrentPressedKey);
         if (CurrentPressedMod & (KMOD_LALT|KMOD_RALT|KMOD_LSHIFT|KMOD_RSHIFT)) {
            //printf("port:0x%X Mod:%u Key:%u out='@'\n", P, CurrentPressedMod, CurrentPressedKey);
            if (P == SHRP_BNMas) {
               //printf("port:0x%X Mod:%u Key:%u out='@'\n", P, CurrentPressedMod, CurrentPressedKey);
               return (0xFF - 0x02); // Symbol Shift (+ 2) ==> '@'
            }
            if (P == SHRP_54321) {
               //printf("port:0x%X Mod:%u Key:%u out='@'\n", P, CurrentPressedMod, CurrentPressedKey);
               return (0xFF - 0x02); // (Symbol Shift +) 2 ==> '@'
            }
         }
         if (P == SHRP_54321)
             return (0xFF - 0x02);  // simple 2
         break;
      case 242:   // US: SHIFT+;=: or IT: SHIFT+o'=c ==>
         //printf("port:0x%X Mod:%u Key:%u out='@'\n", P, CurrentPressedMod, CurrentPressedKey);
         if (CurrentPressedMod & (KMOD_LALT|KMOD_RALT|KMOD_LSHIFT|KMOD_RSHIFT)) {
            //printf("port:0x%X Mod:%u Key:%u out='@'\n", P, CurrentPressedMod, CurrentPressedKey);
            if (P == SHRP_BNMas) {
               //printf("port:0x%X Mod:%u Key:%u out='@'\n", P, CurrentPressedMod, CurrentPressedKey);
               return (0xFF - 0x02); // Symbol Shift (+ 2) ==> '@'
            }
            if (P == SHRP_54321) {
               //printf("port:0x%X Mod:%u Key:%u out='@'\n", P, CurrentPressedMod, CurrentPressedKey);
               return (0xFF - 0x02); // (Symbol Shift +) 2 ==> '@'
            }
         }
         break;
      case '3':
         if (P == SHRP_54321)
            return (0xFF - 0x04);
         break;
      case '4':
         if (P == SHRP_54321)
            return (0xFF - 0x08);
         break;
      case '5':
         if (P == SHRP_54321)
            return (0xFF - 0x10);
         break;

      case '0':   // SHIFT+0 ==>
         //printf("port:0x%X Mod:%u Key:%u out='0'\n", P, CurrentPressedMod, CurrentPressedKey);
         if (CurrentPressedMod & (KMOD_LSHIFT|KMOD_RSHIFT)) {
            //printf("port:0x%X Mod:%u Key:%u out='SHIFT+0'\n", P, CurrentPressedMod, CurrentPressedKey);
            if (P == SHRP_VCXZc) {
               //printf("port:0x%X Mod:%u Key:%u out='SHIFT+0'\n", P, CurrentPressedMod, CurrentPressedKey);
               return (0xFF - 0x01); // Caps Shift (+ 0) ==> 'SHIFT+0'
            }
            if (P == SHRP_67890) {
               //printf("port:0x%X Mod:%u Key:%u out='SHIFT+0'\n", P, CurrentPressedMod, CurrentPressedKey);
               return (0xFF - 0x01); // (Caps Shift +) 0 ==> 'SHIFT+0'
            }
         }
         if (P == SHRP_67890) {
            //printf("port:0x%X Mod:%u Key:%u out='0'\n", P, CurrentPressedMod, CurrentPressedKey);
            return (0xFF - 0x01); // simple 0
         }
         break;
      case '9':
         if (P == SHRP_67890)
            return (0xFF - 0x02);
         break;
      case '8':
         if (P == SHRP_67890)
            return (0xFF - 0x04);
         break;
      case '7':
         if (P == SHRP_67890)
            return (0xFF - 0x08);
         break;
      case '6':
         if (P == SHRP_67890)
            return (0xFF - 0x10);
         break;

      case 'p':   // ALT|SHIFT+p ==>
         //printf("port:0x%X Mod:%u Key:%u out='\"'\n", P, CurrentPressedMod, CurrentPressedKey);
         if (CurrentPressedMod & (KMOD_LALT|KMOD_RALT|KMOD_LSHIFT|KMOD_RSHIFT)) {
            //printf("port:0x%X Mod:%u Key:%u out='\"'\n", P, CurrentPressedMod, CurrentPressedKey);
            if (P == SHRP_BNMas) {
               //printf("port:0x%X Mod:%u Key:%u out='\"'\n", P, CurrentPressedMod, CurrentPressedKey);
               return (0xFF - 0x02); // Symbol Shift (+ p) ==> '"'
            }
            if (P == SHRP_YUIOP) {
               //printf("port:0x%X Mod:%u Key:%u out='\"'\n", P, CurrentPressedMod, CurrentPressedKey);
               return (0xFF - 0x01); // (Symbol Shift +) p ==> '"'
            }
         }
         if (P == SHRP_YUIOP)
            return (0xFF - 0x01); // simple p
         break;
      case 224:   // US: SHIFT+'=" or IT: SHIFT+a'=� ==>
         //printf("port:0x%X Mod:%u Key:%u out='\"'\n", P, CurrentPressedMod, CurrentPressedKey);
         if (CurrentPressedMod & (KMOD_LSHIFT|KMOD_RSHIFT)) {
            //printf("port:0x%X Mod:%u Key:%u out='\"'\n", P, CurrentPressedMod, CurrentPressedKey);
            if (P == SHRP_BNMas) {
               //printf("port:0x%X Mod:%u Key:%u out='\"'\n", P, CurrentPressedMod, CurrentPressedKey);
               return (0xFF - 0x02); // Symbol Shift (+ p) ==> '"'
            }
            if (P == SHRP_YUIOP) {
               //printf("port:0x%X Mod:%u Key:%u out='\"'\n", P, CurrentPressedMod, CurrentPressedKey);
               return (0xFF - 0x01); // (Symbol Shift +) p ==> '"'
            }
         }
         break;
      case 'o':
         if (P == SHRP_YUIOP)
            return (0xFF - 0x02);
         break;
      case 'i':
         if (P == SHRP_YUIOP)
            return (0xFF - 0x04);
         break;
      case 'u':
         if (P == SHRP_YUIOP)
            return (0xFF - 0x08);
         break;
      case 'y':
         if (P == SHRP_YUIOP)
            return (0xFF - 0x10);
         break;

      case SDLK_RETURN:
         if (P == SHRP_HJKLe)
            return (0xFF - 0x01);
         break;
      case 'l':
         if (P == SHRP_HJKLe)
            return (0xFF - 0x02);
         break;
      case 'k':
         if (P == SHRP_HJKLe)
            return (0xFF - 0x04);
         break;
      case 'j':
         if (P == SHRP_HJKLe)
            return (0xFF - 0x08);
         break;
      case 'h':
         if (P == SHRP_HJKLe)
            return (0xFF - 0x10);
         break;

      case ' ':
         if (P == SHRP_BNMas)
            return (0xFF - 0x01);
         break;
      // SYMBOL SHIFT is ignored
      case 'm':
         if (P == SHRP_BNMas)
            return (0xFF - 0x04);
         break;
      case '.':   // '.' ==>
         if (P == SHRP_BNMas) {
            //printf("port:0x%X Mod:%u Key:%u out='.'\n", P, CurrentPressedMod, CurrentPressedKey);
            return (0xFF - 0x04 - 0x02); // Symbol Shift + m = '.'
         }
         break;
      case 'n':
         if (P == SHRP_BNMas)
            return (0xFF - 0x08);
         break;
      case ',':   // ',' ==>
         if (P == SHRP_BNMas) {
            //printf("port:0x%X Mod:%u Key:%u out=','\n", P, CurrentPressedMod, CurrentPressedKey);
            return (0xFF - 0x08 - 0x02); // Symbol Shift + n = ','
         }
         break;
      case 'b':
         if (P == SHRP_BNMas)
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
void OutZ80(word P, byte v) {
   return;
}


#if CPUEMUL == eZ80
/****************************************************************************\
* LoopZ80                                                                    *
*                                                                            *
\****************************************************************************/
word LoopZ80(register Z80 *R) {
   return 0;
}


/****************************************************************************\
* PatchZ80                                                                   *
*                                                                            *
\****************************************************************************/
void PatchZ80(register Z80 *R) {
   return;
}
#endif


/****************************************************************************\
* JumpZ80 called on JP, JR, CALL, RST, or RET                                *
*                                                                            *
\****************************************************************************/
void JumpZ80(word PC) {
   static char LastChar = 0;
   byte a;

   #if CPUEMUL == eZ80
      a = z80.AF.B.h;
   #endif
   #if CPUEMUL == ez80emu
      a = z80.registers.byte[Z80_A];
   #endif

   switch (HV) { // to make this fast...

      case OWN:
         if (PC == L_PRINT_CHAR_OWN) {
            if (LastChar == 0x0D) {
               LastChar = 0;
               PrintObjNrActingAnimal(ZXmem[L_ACTING_ANIMAL_OWN]);
            }

            if (!ZXmem[L_SELECT_PRINTWIN_OWN]) { // only upper window
               SDLTWE_PrintCharTextWindow(&LogWin, a, &CharSet, SC_BLACK, SC_WHITE);
               LastChar = a;
            }
         }
         break;

      case V10:
         if (PC == L_PRINT_CHAR_V10) {
            if (LastChar == 0x0D) {
               LastChar = 0;
               PrintObjNrActingAnimal(ZXmem[L_ACTING_ANIMAL_V10]);
            }

            if (!ZXmem[L_SELECT_PRINTWIN_V10]) { // only upper window
               SDLTWE_PrintCharTextWindow(&LogWin, a, &CharSet, SC_BLACK, SC_WHITE);
               LastChar = a;
            }
         }
         break;

      case V12:
         if (PC == L_PRINT_CHAR_V12) {
            //printf("%02X=", a);
            //if (a>=32) printf("%c ", a);
            //else printf("  ");
            if (LastChar == 0x0D) {
               LastChar = 0;
               PrintObjNrActingAnimal(ZXmem[L_ACTING_ANIMAL_V12]);
            }

            if (!ZXmem[L_SELECT_PRINTWIN_V12]) { // only upper window
               SDLTWE_PrintCharTextWindow(&LogWin, a, &CharSet, SC_BLACK, SC_WHITE);
               LastChar = a;
            }
         }
         break;
   }
}

#if CPUEMUL == ez80emu
   void SystemCall (void* context) {
      return;
   }
#endif

