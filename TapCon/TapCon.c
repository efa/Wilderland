/****************************************************************************\
*                                                                            *
*                                TapCon                                      *
*                                                                            *
* TAP/TZX format converter                                                   *
*                                                                            *
* This program is part of the 'Wilderland' package. It converts the TAP/TZX  *
* game files into binaries readable by the emulator.                         *
*                                                                            *
* (c) 2012 by CH. Contact: wilderland@aon.at                                 *
* Copyright 2019-2021 Valerio Messina efa@iol.it                             *
*                                                                            *
*                                                                            *
* Compiler: Pelles C for Windows 6.5.80 with 'Microsoft Extensions' enabled, *
*           GCC, MinGW/Msys2, Clang/LLVM                                     *
*                                                                            *
* V 1.08 - 20211128                                                          *
*                                                                            *
\****************************************************************************/


/*** INCLUDES ***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*** DEFINES ***************************************************************/
#define V10_CODE_SIZE     0x9400 // 37888
#define V10_TAP_NAME      "HOBBIT.TAP"
#define V10_TAP_LENGTH    0xB00A // 45066
#define V10_TAP_START     0x1C09 // 7177
#define V10_TZX_NAME      "The Hobbit v1.0.tzx"
#define V10_TZX_LENGTH    0xB044 // 45124
#define V10_TZX_START     0x1C43 // 7235

#define V12_CODE_SIZE     0x9C40 // 40000
#define V12_TAP_NAME      "HOBBIT12.TAP"
#define V12_TAP_LENGTH    0xB84A // 47178
#define V12_TAP_START     0x1C09 // 7177
#define V12_TZX_NAME      "The Hobbit v1.2.tzx"
#define V12_TZX_LENGTH    0xB884 // 47236
#define V12_TZX_START     0x1C43 // 7235


/****************************************************************************\
*                                   main                                     *
*                                                                            *
\****************************************************************************/
int main(int argc, char *argv[])
{
    FILE *fr, *fw;
    long int FileLength;
    size_t BytesRead;
    unsigned char conversion_buffer[0xC000];

    printf ("Wilderland - A Hobbit Environment\n");
    printf ("(c) 2012-2019 by CH, Copyright 2019-2021 Valerio Messina\n");
    printf ("TAP/TZX game files converter into binaries readable by the emulator\n");

    // V 1.0 conversion

    fr = fopen(V10_TAP_NAME, "rb");
    if (fr)
    {
    fseek(fr, 0, SEEK_END);
    FileLength = ftell(fr);
        if (FileLength == V10_TAP_LENGTH)
        {
            fw = fopen("HOB_V10.bin", "wb");
            if (fw)
            {
                fseek(fr, V10_TAP_START, SEEK_SET);
                BytesRead = fread(conversion_buffer, 1, V10_CODE_SIZE, fr);
                if (BytesRead == V10_CODE_SIZE)
                {
                    fwrite(conversion_buffer, 1, V10_CODE_SIZE, fw);
                    fprintf(stderr, "\nHOB_V10.bin successfully built from '%s'.\n", V10_TAP_NAME);
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
            fprintf(stderr, "Can't open 'HOB_V10.bin' for write; no conversion possible.\n");
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
            fw = fopen("HOB_V10.bin", "wb");
            if (fw)
            {
                fseek(fr, V10_TZX_START, SEEK_SET);
                BytesRead = fread(conversion_buffer, 1, V10_CODE_SIZE, fr);
                if (BytesRead == V10_CODE_SIZE)
                {
                    fwrite(conversion_buffer, 1, V10_CODE_SIZE, fw);
                    fprintf(stderr, "\nHOB_V10.bin successfully built from '%s'.\n", V10_TZX_NAME);
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
            fprintf(stderr, "Can't open 'HOB_V10.bin' for write; no conversion possible.\n");
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


    // V 1.2 conversion

    fr = fopen(V12_TAP_NAME, "rb");
    if (fr)
    {
    fseek(fr, 0, SEEK_END);
    FileLength = ftell(fr);
        if (FileLength == V12_TAP_LENGTH)
        {
            fw = fopen("HOB_V12.bin", "wb");
            if (fw)
            {
                fseek(fr, V12_TAP_START, SEEK_SET);
                BytesRead = fread(conversion_buffer, 1, V12_CODE_SIZE, fr);
                if (BytesRead == V12_CODE_SIZE)
                {
                    fwrite(conversion_buffer, 1, V12_CODE_SIZE, fw);
                    fprintf(stderr, "\nHOB_V12.bin successfully built from '%s'.\n", V12_TAP_NAME);
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
            fprintf(stderr, "Can't open 'HOB_V12.bin' for write; no conversion possible.\n");
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
            fw = fopen("HOB_V12.bin", "wb");
            if (fw)
            {
                fseek(fr, V12_TZX_START, SEEK_SET);
                BytesRead = fread(conversion_buffer, 1, V12_CODE_SIZE, fr);
                if (BytesRead == V12_CODE_SIZE)
                {
                    fwrite(conversion_buffer, 1, V12_CODE_SIZE, fw);
                    fprintf(stderr, "\nHOB_V12.bin successfully built from '%s'.\n", V12_TZX_NAME);
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
            fprintf(stderr, "Can't open 'HOB_V12.bin' for write; no conversion possible.\n");
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
