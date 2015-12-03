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
#include <wpfolder.h>

#include "wptools.h"


BOOL _System GetWPHostOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
BYTE szOptionText[256];
ULONG ulOption;
INT iIndex;
BYTE szTag11[20];

   if (GetObjectValue(pObjectData, 1, szOptionText, sizeof szOptionText))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "HOSTNAME=%s;", szOptionText);
      }

   if (GetObjectValue(pObjectData, 2, szOptionText, sizeof szOptionText))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "USERNAME=%s;", szOptionText);
      }


   if (GetObjectValue(pObjectData, 11, szTag11, sizeof szTag11))
      {
      if (GetObjectValue(pObjectData, 3, szOptionText, sizeof szOptionText))
         {
         USHORT usCount;
         for (iIndex = 0, usCount = 0; iIndex < strlen(szOptionText); iIndex++, usCount++)
            {
            if (usCount == 4)
               {
//               szTag11[0] -= 8;
               usCount = 0;
               }
            szOptionText[iIndex] = szTag11[0]^szOptionText[iIndex];
            }
         sprintf(pszOptions + strlen(pszOptions),
            "PASSWORD=%s;", szOptionText);
         }
      }


   if (GetObjectValue(pObjectData, 4, szOptionText, sizeof szOptionText))
      {
      if (strlen(szOptionText))
         sprintf(pszOptions + strlen(pszOptions),
            "ACCOUNT=%s;", szOptionText);
      }

   if (GetObjectValue(pObjectData, 5, szOptionText, sizeof szOptionText))
      {
      if (strlen(szOptionText))
         sprintf(pszOptions + strlen(pszOptions),
            "REMOTEDIR=%s;", szOptionText);
      }

   if (GetObjectValue(pObjectData, 6, szOptionText, sizeof szOptionText))
      {
      if (strlen(szOptionText))
         sprintf(pszOptions + strlen(pszOptions),
            "LOCALDIR=%s;", szOptionText);
      }

   if (GetObjectValue(pObjectData, 7, &ulOption, sizeof ulOption))
      {
      if (ulOption == 3)
         strcat(pszOptions, "FILETRANSFERTYPE=BINARY;");
      if (ulOption == 1)
         strcat(pszOptions, "FILETRANSFERTYPE=ASCII;");

      }


   if (GetObjectValue(pObjectData, 8, szOptionText, sizeof szOptionText))
      {
      if (strlen(szOptionText))
         sprintf(pszOptions + strlen(pszOptions),
            "INCLUDE=%s;", szOptionText);
      }



   
   return TRUE;
}

