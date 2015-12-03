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
#define INCL_SPL
#define INCL_SPLERRORS
#define INCL_SPLDOSPRINT
#include <os2.h>

#include <wpprint.h>

#include "wptools.h"


/*******************************************************************
* Get Objectoptions
*******************************************************************/
BOOL _System GetWPRPrinterOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
BYTE szOptionText[256];
ULONG ulValue;

   if (GetObjectValue(pObjectData, IDKEY_RPRNNETID,
      szOptionText, sizeof szOptionText))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "NETID=%s;", szOptionText);
      }

   return TRUE;
}

