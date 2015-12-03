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
BOOL _System GetWPFsysOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
ULONG   ulMenuCount;
BYTE    szOptionText[256];
PFSYSMENU prgMenu;

   return TRUE;

   ulMenuCount = 0L;

   if (!GetObjectValue(pObjectData, 
      WPFSYS_MENUCOUNT, &ulMenuCount, sizeof ulMenuCount))
      ulMenuCount = 0L;

   if (ulMenuCount)
      {
      USHORT usAction, usMenu;

      prgMenu = calloc(ulMenuCount, sizeof (FSYSMENU));
      if (!prgMenu)
         printf("Not enough memory to backup menu up!\n");

      if (!GetObjectValue(pObjectData, 
         WPFSYS_MENUARRAY, prgMenu, ulMenuCount * sizeof (FSYSMENU)))
         ulMenuCount = 0;

      for (usMenu = 0; usMenu < ulMenuCount; usMenu++)
         {
         if (prgMenu[usMenu].usIDParent)
            continue;
         for (usAction = 0; usAction < ulMenuCount; usAction++)
            {
            if (!prgMenu[usAction].usIDParent)
               continue;
            if (prgMenu[usAction].usIDParent == prgMenu[usMenu].usIDMenu)
               {
               PSZ pszObject = pszObjectIDFromHandle(prgMenu[usAction].hObject);

               if (pszObject)
                  {
                  static BYTE szMenu[256];
                  static BYTE szAction[256];

                  memset(szOptionText, 0, sizeof szOptionText);
                  memset(szMenu, 0, sizeof szMenu);
                  memset(szAction, 0, sizeof szAction);


                  ConvertTitle(szMenu, sizeof szMenu,
                               prgMenu[usMenu].szTitle,
                               strlen(prgMenu[usMenu].szTitle) + 1);

                  ConvertTitle(szAction, sizeof szAction,
                               prgMenu[usAction].szTitle,
                               strlen(prgMenu[usAction].szTitle) + 1);

                  sprintf(szOptionText, "MENU=%s,%s,%s;",
                     szMenu,
                     szAction,
                     pszObject);
                  }
               strcat(pszOptions, szOptionText);
               }
            }
         }

      if (prgMenu)
         free(prgMenu);
      }


   return TRUE;
}

