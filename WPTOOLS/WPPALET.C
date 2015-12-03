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
#include <os2.h>

#include <wppalet.h>

#include "wptools.h"

/*******************************************************************
* Get Objectoptions
*******************************************************************/
BOOL _System GetWPPaletteOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
BYTE szOptionText[256];
ULONG ulValue;

   
   if (GetObjectValue(pObjectData,  IDKEY_PALXCELLCOUNT, &ulValue, sizeof ulValue))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "XCELLCOUNT=%lu;", ulValue);
      }
   if (GetObjectValue(pObjectData,  IDKEY_PALYCELLCOUNT, &ulValue, sizeof ulValue))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "YCELLCOUNT=%lu;", ulValue);
      }
   if (GetObjectValue(pObjectData,  IDKEY_PALXCELLWIDTH, &ulValue, sizeof ulValue))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "XCELLWIDTH=%lu;", ulValue);
      }
   if (GetObjectValue(pObjectData,  IDKEY_PALYCELLHEIGHT, &ulValue, sizeof ulValue))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "YCELLHEIGHT=%lu;", ulValue);
      }
   if (GetObjectValue(pObjectData,  IDKEY_PALXGAP, &ulValue, sizeof ulValue))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "XCELLGAP=%lu;", ulValue);
      }
   if (GetObjectValue(pObjectData,  IDKEY_PALYGAP, &ulValue, sizeof ulValue))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "YCELLGAP=%lu;", ulValue);
      }
   return TRUE;
   return GetWPAbstractOptions(hObject, pszOptions, pObjectData);

}
