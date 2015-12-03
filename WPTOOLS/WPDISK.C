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
* Get Objectoptions
*******************************************************************/
BOOL _System GetWPDiskOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
ULONG ulValue;
   
   if (GetObjectValue(pObjectData, IDKEY_DRIVENUM, &ulValue, sizeof ulValue))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "DRIVENUM=%lu;", ulValue);
      }
   return TRUE;
}

