/****************************************************************************\
*                                                                            *
*                              MapCoordinates.h                              *
*                                                                            *
* Map Coordinates for WL - Wilderland V 10                                   *
*                                                                            *
* (c) 2012-2019 by CH, Copyright 2019-2025 Valerio Messina                   *
*                                                                            *
* V 2.10 - 20250106                                                          *
*                                                                            *
*  MapCoordinates.h is part of Wilderland - A Hobbit Environment             *
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


#ifndef MAPCOORDINATES_H
#define MAPCOORDINATES_H


struct MapCoordinatesStruct {
   byte RoomNumber;
   word X;
   word Y;
};

struct MapCoordinatesStruct MapCoordinates[ROOMS_NROF_MAX] = {
   {0x01,   31,  245},
   {0x02,  914,  415},
   {0x03, 1035,  415},
   {0x04,   92,  245},
   {0x05,   92,  185},
   {0x06,   92,  124},
   {0x07,   92,   64},
   {0x08, 1213,  463},
   {0x09,  153,  245},
   {0x0A,  204,  245},
   {0x0B,  515,  245},
   {0x0C,  563,  245},
   {0x0D,  500,  294},
   {0x0E,  516,  201},
   {0x0F,  564,  427},
   {0x10,  437,  381},
   {0x11,  696,  469},
   {0x12,  479,  340},
   {0x13,  373,  382},
   {0x14,  394,  125},
   {0x15,  455,  125},
   {0x16,  621,  215},
   {0x17,  712,   95},
   {0x18,  672,  194},
   {0x19,  720,  194},
   {0x1A,  939,  134},
   {0x1B,  939,  195},
   {0x1C,  879,   74},
   {0x1D,  999,  134},
   {0x1E, 1045,   43},
   {0x1F, 1105,   43},
   {0x20, 1045,  103},
   {0x21, 1045,  164},
   {0x22, 1228,  337},
   {0x23, 1213,  352},
   {0x24, 1213,  289},
   {0x25, 1213,  239},
   {0x26, 1213,  191},
   {0x27, 1213,  144},
   {0x28, 1158,  144},
   {0x29, 1213,   94},
   {0x2A, 1158,   67},
   {0x2B, 1245,   67},
   {0x2C, 1213,   31},
   {0x2D, 1213,  415},
   {0x2E,  793,  415},
   {0x2F,  778,   43},
   {0x30,  687,   43},
   {0x31,  621,  109},
   {0x32,  878,  194},
   {0x33, 1159,   13},
   {0x34,  624,  469},
   {0x35,  660,  469},
   {0x36,  696,  357},
   {0x37,  696,  417},
   {0x38,  563,  321},
   {0x39,  437,  426},
   {0x3A,  575,  381},
   {0x3B,  594,  321},
   {0x3C,  609,  363},
   {0x3D,  654,  417},
   {0x3E,  612,  417},
   {0x3F,  519,  381},
   {0x40,  519,  339},
   {0x41,  398,  336},
   {0x42,  769,  194},
   {0x43,  830,  194},
   {0x44,  204,  185},
   {0x45,  249,  124},
   {0x46,  249,   76},
   {0x47,  332,  185},
   {0x48,  298,  124},
   {0x49,  204,  295},
   {0x4A,  331,  294},
   {0x4B,  279,  272},
   {0x4C,  279,  321},
   {0x4D,  279,  366},
   {0x4E,  279,  417},
   {0x4F,  331,  418}
};

char *AnimalInitials[100] = {
   "B", "", "", "", "", "", "", "",               //0x00
   "", "", "", "", "", "", "", "",
   "", "", "", "", "", "", "", "",                //0x10
   "", "", "", "", "", "", "", "",
   "", "", "", "", "", "", "", "",                //0x20
   "", "", "", "", "", "", "", "",
   "", "", "", "", "", "", "", "",                //0x30
   "", "", "", "", "D", "nG", "G", "T",
   "WE", "El", "Bu", "W", "Go", "hG", "Ba", "hT", //0x40
   "vT", "oG", "mG", "vG", "dG"
};

#endif /* MAPCOORDINATES_H */
