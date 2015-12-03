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

VOID _System vGetFSysTitle(HOBJECT hObject, PSZ pszOptions)
{
BYTE    szOptionText[256];
USHORT  usEASize;
PSZ     pszLongName;
BYTE    szPath[256];

   memset(szOptionText, 0, sizeof szOptionText);
   if (PathFromObject(NULL, hObject, szPath, sizeof szPath, NULL))
      {
      if (GetEAValue(szPath, ".LONGNAME", &pszLongName, &usEASize))
         {
         ConvertTitle(szOptionText, sizeof szOptionText, pszLongName, usEASize);
         free(pszLongName);
         }
      else
         {
         PSZ p = strrchr(szPath, '\\');
         if (p)
            strcpy(szOptionText, p+1);
         }
      if (*szOptionText)
         sprintf(pszOptions + strlen(pszOptions),
            "TITLE=%s;", szOptionText);
      }
}
