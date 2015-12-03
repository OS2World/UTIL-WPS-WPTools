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
* 
*******************************************************************/
BOOL _System GetObjectLocation(HOBJECT hObject, PSZ pszLocation, USHORT usMax)
{
PSZ  pszObjectID;
BYTE szPath[256];

   if (IsObjectAbstract(hObject))
      {
      HOBJECT hObjFolder;
      BOOL  fFound = FALSE;
      PSZ   pszAllFolders,
            pszFolder;
      ULONG ulProfileSize;

      pszAllFolders = GetAllProfileNames(FOLDERCONTENT,
         hIniUser,
         &ulProfileSize);
      if (!pszAllFolders)
         return FALSE;

      pszFolder = pszAllFolders;
      while (*pszFolder && !fFound)
         {
         ULONG ulIndex;
         PSZ   pEnd;

         PULONG prgObject = (PULONG)GetProfileData(FOLDERCONTENT,
            pszFolder, hIniUser, &ulProfileSize);
         if (prgObject)
            {
            hObjFolder = MakeDiskHandle(strtol(pszFolder, &pEnd, 16));

            ulProfileSize /= sizeof (ULONG);
            for (ulIndex = 0; ulIndex < ulProfileSize; ulIndex++)
               {
               if (LOUSHORT(hObject) == prgObject[ulIndex])
                  {
                  fFound = TRUE;
                  break;
                  }
               }
            free(prgObject);
            }
         pszFolder += strlen(pszFolder) + 1;
         }
      free(pszAllFolders);
      if (!fFound)
         return FALSE;
      hObject = hObjFolder;
      }
   else
      {
      PSZ p;
      PNODE pNode = PathFromObject(NULL, hObject, szPath, sizeof szPath, NULL);
      if (!pNode)
         return FALSE;

      hObject = MakeDiskHandle(pNode->usParentID);
      }
   pszObjectID = pszObjectIDFromHandle(hObject);
   if (!pszObjectID)
      return FALSE;

   strncpy(pszLocation, pszObjectID, usMax);
   return TRUE;
}

