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

#include "wptools.h"
/*******************************************************************
* Get ShadowOptions
*******************************************************************/
BOOL _System GetWPShadowOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
BYTE    szOptionText[256];
HOBJECT hLink;
PSZ     pszObjectID;

   /*
      Get Shadow link handle
   */

   memset(szOptionText, 0, sizeof szOptionText);
   if (!GetObjectValue(pObjectData, 
      WPSHADOW_LINK, &hLink, sizeof hLink))
      return FALSE;

   pszObjectID = pszObjectIDFromHandle(hLink);
   if (!pszObjectID)
      return FALSE;

   strcat(pszOptions, "SHADOWID=");
   strcat(pszOptions, pszObjectID);
   strcat(pszOptions, ";");

   return TRUE;
   return GetWPAbstractOptions(hObject, pszOptions, pObjectData);
}
