#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <direct.h>

#define INCL_VIO
#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_GPI
#define INCL_WINSTDFONT  /* Window Standard Font Functions     */
#include <os2.h>

#include <wppalet.h>
#include <wpclrpal.h>

#include "wptools.h"

BOOL _System GetWPColorPaletteOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
ULONG ulXCellCount,
      ulYCellCount,
      ulTotalCells;
USHORT usIndex;
PAINTPOT fPot;
BOOL    fAnyThing;
PSZ     pszStart = pszOptions + strlen(pszOptions);

   if (!GetObjectValue(pObjectData, IDKEY_PALXCELLCOUNT,
      &ulXCellCount, sizeof ulXCellCount))
      return FALSE;
   if (!GetObjectValue(pObjectData, IDKEY_PALYCELLCOUNT,
      &ulYCellCount, sizeof ulYCellCount))
      return FALSE;

   ulTotalCells = ulXCellCount * ulYCellCount;

   strcat(pszOptions, "COLORS=");

   fAnyThing = FALSE;
   for (usIndex = 0; usIndex < ulTotalCells; usIndex++)
      {
      if (!GetObjectValue(pObjectData, 
         IDKEY_PALCELLDATA_FIRST + usIndex,
         &fPot, sizeof fPot))
         continue;
      fAnyThing = TRUE;
      if (usIndex)
         strcat(pszOptions, ",");
      sprintf(pszOptions + strlen(pszOptions),
         "0x%6.6lX", fPot.ulRGB);
      }

   if (fAnyThing)
      strcat(pszOptions, ";");
   else
      *pszStart = 0;

   return TRUE;
}



