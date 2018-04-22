#ifndef MAIN
#define MAIN

#define PROJECT_INFO "Tibia %s Client\n%s%sVersion %d.%02d\nModification by: Saiyans King\n©Copyright© 1997-2018\nCipSoft GmbH - ®rights reserved®"
#define PROJECT_NAME "Extented Tibia"

#include <windows.h>
#include <sys/timeb.h>
#include <stdint.h>
#include <stdio.h>

unsigned char* LoadSpriteAlpha(uint32_t sprite);

typedef uint32_t (*_GetEngineAddr) ();
extern _GetEngineAddr GetEngineAddr;

typedef void (*_DrawSkin) (int nSurface, int X, int Y, int W, int H, int SkinId, int dX, int dY);
extern _DrawSkin DrawSkin;

typedef void (*_PrintText) (int nSurface, int nX, int nY, int nFont, int nRed, int nGreen, int nBlue, const char* lpText, int nAlign);
extern _PrintText PrintText;

#endif
