/****************************************************************************\
*                                                                            *
*                                TapCon                                      *
*                                                                            *
* TAP format converter                                                       *
*                                                                            *
* This program is part of the 'Wilderland' package. It converts the TAP game *
* files into binaries readable by the emulator.                              *
*                                                                            *
* (c) 2012 by CH. Contact: wilderland@aon.at                                 *
* Copyright 2019-2021 Valerio Messina efa@iol.it                             *
*                                                                            *
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


/*** DEFINES ***************************************************************/
#define V10_TAP_LENGTH    0xB00A
#define V10_CODE_START    0x1C09
#define V10_CODE_LENGTH   0x9400

#define V12_TAP_LENGTH    0xB84A
#define V12_CODE_START    0x1C09
#define V12_CODE_LENGTH   0x9C40


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


    // V 1.0 conversion

    fr = fopen("HOBBIT.TAP", "rb");
    if (fr)
    {
    fseek(fr, 0, SEEK_END);
    FileLength = ftell(fr);
        if (FileLength == V10_TAP_LENGTH)
        {
            fw = fopen("HOB_V10.bin", "wb");
            if (fw)
            {
                fseek(fr, V10_CODE_START, SEEK_SET);
                BytesRead = fread(conversion_buffer, 1, V10_CODE_LENGTH, fr);
                if (BytesRead == V10_CODE_LENGTH)
                {
                    fwrite(conversion_buffer, 1, V10_CODE_LENGTH, fw);
                    fprintf(stderr, "\nHOB_V10.bin successfully built.\n");
                }
                else
                {
                    fprintf(stderr, "Read length error in 'HOBBIT.TAP'; no conversion possible.\n");
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
            fprintf(stderr, "HOBBIT.TAP has wrong length; no conversion possible.\n");
            fclose(fr);
        }
    }
    else
    {
        fprintf(stderr, "Can't open 'HOBBIT.TAP' (V 1.0) for read; no conversion possible.\n");
    }



    // V 1.2 conversion

    fr = fopen("HOBBIT12.TAP", "rb");
    if (fr)
    {
    fseek(fr, 0, SEEK_END);
    FileLength = ftell(fr);
        if (FileLength == V12_TAP_LENGTH)
        {
            fw = fopen("HOB_V12.bin", "wb");
            if (fw)
            {
                fseek(fr, V12_CODE_START, SEEK_SET);
                BytesRead = fread(conversion_buffer, 1, V12_CODE_LENGTH, fr);
                if (BytesRead == V12_CODE_LENGTH)
                {
                    fwrite(conversion_buffer, 1, V12_CODE_LENGTH, fw);
                    fprintf(stderr, "\nHOB_V12.bin successfully built.\n");
                }
                else
                {
                    fprintf(stderr, "Read length error in 'HOBBIT12.TAP'; no conversion possible.\n");
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
            fprintf(stderr, "HOBBIT12.TAP has wrong length; no conversion possible.\n");
            fclose(fr);
        }
    }
    else
    {
        fprintf(stderr, "Can't open 'HOBBIT12.TAP' (V 1.2) for read; no conversion possible.\n");
    }
    return 0;
}
