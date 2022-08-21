/****************************************************************************\
*                                                                            *
*                                TapCon                                      *
*                                                                            *
* TAP/TZX format converter                                                   *
*                                                                            *
* (c) 2012 by CH. Contact: wilderland@aon.at                                 *
* Copyright 2019-2022 Valerio Messina efa@iol.it                             *
*                                                                            *
* V 2.08 - 20220820                                                          *
*                                                                            *
*  TapCon.c is part of Wilderland - A Hobbit Environment                     *
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
* TapCon converts TAP/TZX game files into binaries readable by the emulator  *
*                                                                            *
* Compiler: Pelles C for Windows 6.5.80 with 'Microsoft Extensions' enabled, *
*           GCC, MinGW/Msys2, Clang/LLVM                                     *
*                                                                            *
\****************************************************************************/


/*** INCLUDES ***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../WL.h"


/****************************************************************************\
*                                   main                                     *
*                                                                            *
\****************************************************************************/
int main(int argc, char *argv[])
{
    FILE *fr, *fw;
    long int FileLength;
    size_t BytesRead;
    unsigned char conversion_buffer[0xC000]; // 49152

    printf ("Wilderland - A Hobbit Environment\n");
    printf ("(c) 2012-2019 by CH, Copyright 2019-2022 Valerio Messina\n");
    printf ("TAP/TZX game files converter into binaries readable by the emulator\n");

    // V 1.0 conversion
    fr = fopen(V10_TAP_NAME, "rb");
    if (fr)
    {
    fseek(fr, 0, SEEK_END);
    FileLength = ftell(fr);
        if (FileLength == V10_TAP_LENGTH)
        {
            fw = fopen(FN_V10, "wb");
            if (fw)
            {
                fseek(fr, V10_TAP_START, SEEK_SET);
                BytesRead = fread(conversion_buffer, 1, CODE_LENGTH_V10, fr);
                if (BytesRead == CODE_LENGTH_V10)
                {
                    fwrite(conversion_buffer, 1, CODE_LENGTH_V10, fw);
                    fprintf(stderr, "\n"FN_V10" successfully built from '%s'.\n", V10_TAP_NAME);
                }
                else
                {
                    fprintf(stderr, "Read length error in '%s'; no conversion possible.\n", V10_TAP_NAME);
                }
                fclose(fw);
                fclose(fr);
            }
            else
            {
            fprintf(stderr, "Can't open '"FN_V10"' for write; no conversion possible.\n");
            fclose(fr);
            }
        }
        else
        {
            fprintf(stderr, "%s has wrong length; no conversion possible.\n", V10_TAP_NAME);
            fclose(fr);
        }
    }
    else
    {
        fprintf(stderr, "Can't open '%s' (V 1.0) for read; no conversion possible.\n", V10_TAP_NAME);
    }

    fr = fopen(V10_TZX_NAME, "rb");
    if (fr)
    {
    fseek(fr, 0, SEEK_END);
    FileLength = ftell(fr);
        if (FileLength == V10_TZX_LENGTH)
        {
            fw = fopen(FN_V10, "wb");
            if (fw)
            {
                fseek(fr, V10_TZX_START, SEEK_SET);
                BytesRead = fread(conversion_buffer, 1, CODE_LENGTH_V10, fr);
                if (BytesRead == CODE_LENGTH_V10)
                {
                    fwrite(conversion_buffer, 1, CODE_LENGTH_V10, fw);
                    fprintf(stderr, "\n"FN_V10" successfully built from '%s'.\n", V10_TZX_NAME);
                }
                else
                {
                    fprintf(stderr, "Read length error in '%s'; no conversion possible.\n", V10_TZX_NAME);
                }
                fclose(fw);
                fclose(fr);
            }
            else
            {
            fprintf(stderr, "Can't open '"FN_V10"' for write; no conversion possible.\n");
            fclose(fr);
            }
        }
        else
        {
            fprintf(stderr, "%s has wrong length; no conversion possible.\n", V10_TZX_NAME);
            fclose(fr);
        }
    }
    else
    {
        fprintf(stderr, "Can't open '%s' (V 1.0) for read; no conversion possible.\n", V10_TZX_NAME);
    }

    // V OWN conversion
    fr = fopen(OWN_TAP_NAME, "rb");
    if (fr)
    {
    fseek(fr, 0, SEEK_END);
    FileLength = ftell(fr);
        if (FileLength == OWN_TAP_LENGTH)
        {
            fw = fopen(FN_OWN, "wb");
            if (fw)
            {
                fseek(fr, OWN_TAP_START, SEEK_SET);
                BytesRead = fread(conversion_buffer, 1, CODE_LENGTH_OWN, fr);
                if (BytesRead == CODE_LENGTH_OWN)
                {
                    fwrite(conversion_buffer, 1, CODE_LENGTH_OWN, fw);
                    fprintf(stderr, "\n"FN_OWN" successfully built from '%s'.\n", OWN_TAP_NAME);
                }
                else
                {
                    fprintf(stderr, "Read length error in '%s'; no conversion possible.\n", OWN_TAP_NAME);
                }
                fclose(fw);
                fclose(fr);
            }
            else
            {
            fprintf(stderr, "Can't open '"FN_OWN"' for write; no conversion possible.\n");
            fclose(fr);
            }
        }
        else
        {
            fprintf(stderr, "%s has wrong length; no conversion possible.\n", OWN_TAP_NAME);
            fclose(fr);
        }
    }
    else
    {
        fprintf(stderr, "Can't open '%s' (V OWN) for read; no conversion possible.\n", OWN_TAP_NAME);
    }

    fr = fopen(OWN_TZX_NAME, "rb");
    if (fr)
    {
    fseek(fr, 0, SEEK_END);
    FileLength = ftell(fr);
        if (FileLength == OWN_TZX_LENGTH)
        {
            fw = fopen(FN_OWN, "wb");
            if (fw)
            {
                fseek(fr, OWN_TZX_START, SEEK_SET);
                BytesRead = fread(conversion_buffer, 1, CODE_LENGTH_OWN, fr);
                if (BytesRead == CODE_LENGTH_OWN)
                {
                    fwrite(conversion_buffer, 1, CODE_LENGTH_OWN, fw);
                    fprintf(stderr, "\n"FN_OWN" successfully built from '%s'.\n", OWN_TZX_NAME);
                }
                else
                {
                    fprintf(stderr, "Read length error in '%s'; no conversion possible.\n", OWN_TZX_NAME);
                }
                fclose(fw);
                fclose(fr);
            }
            else
            {
            fprintf(stderr, "Can't open '"FN_OWN"' for write; no conversion possible.\n");
            fclose(fr);
            }
        }
        else
        {
            fprintf(stderr, "%s has wrong length; no conversion possible.\n", OWN_TZX_NAME);
            fclose(fr);
        }
    }
    else
    {
        fprintf(stderr, "Can't open '%s' (V OWN) for read; no conversion possible.\n", OWN_TZX_NAME);
    }

    // V 1.2 conversion
    fr = fopen(V12_TAP_NAME, "rb");
    if (fr)
    {
    fseek(fr, 0, SEEK_END);
    FileLength = ftell(fr);
        if (FileLength == V12_TAP_LENGTH)
        {
            fw = fopen(FN_V12, "wb");
            if (fw)
            {
                fseek(fr, V12_TAP_START, SEEK_SET);
                BytesRead = fread(conversion_buffer, 1, CODE_LENGTH_V12, fr);
                if (BytesRead == CODE_LENGTH_V12)
                {
                    fwrite(conversion_buffer, 1, CODE_LENGTH_V12, fw);
                    fprintf(stderr, "\n"FN_V12" successfully built from '%s'.\n", V12_TAP_NAME);
                }
                else
                {
                    fprintf(stderr, "Read length error in '%s'; no conversion possible.\n", V12_TAP_NAME);
                }
                fclose(fw);
                fclose(fr);
            }
            else
            {
            fprintf(stderr, "Can't open '"FN_V12"' for write; no conversion possible.\n");
            fclose(fr);
            }
        }
        else
        {
            fprintf(stderr, "%s has wrong length; no conversion possible.\n", V12_TAP_NAME);
            fclose(fr);
        }
    }
    else
    {
        fprintf(stderr, "Can't open '%s' (V 1.2) for read; no conversion possible.\n", V12_TAP_NAME);
    }

    fr = fopen(V12_TZX_NAME, "rb");
    if (fr)
    {
    fseek(fr, 0, SEEK_END);
    FileLength = ftell(fr);
        if (FileLength == V12_TZX_LENGTH)
        {
            fw = fopen(FN_V12, "wb");
            if (fw)
            {
                fseek(fr, V12_TZX_START, SEEK_SET);
                BytesRead = fread(conversion_buffer, 1, CODE_LENGTH_V12, fr);
                if (BytesRead == CODE_LENGTH_V12)
                {
                    fwrite(conversion_buffer, 1, CODE_LENGTH_V12, fw);
                    fprintf(stderr, "\n"FN_V12" successfully built from '%s'.\n", V12_TZX_NAME);
                }
                else
                {
                    fprintf(stderr, "Read length error in '%s'; no conversion possible.\n", V12_TZX_NAME);
                }
                fclose(fw);
                fclose(fr);
            }
            else
            {
            fprintf(stderr, "Can't open '"FN_V12"' for write; no conversion possible.\n");
            fclose(fr);
            }
        }
        else
        {
            fprintf(stderr, "%s has wrong length; no conversion possible.\n", V12_TZX_NAME);
            fclose(fr);
        }
    }
    else
    {
        fprintf(stderr, "Can't open '%s' (V 1.2) for read; no conversion possible.\n", V12_TZX_NAME);
    }

    return 0;
}
