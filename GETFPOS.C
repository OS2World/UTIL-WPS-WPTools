#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#define INCL_WIN
#define INCL_DOS
#include <os2.h>

#define FOLDERPOS      "PM_Workplace:FolderPos"
#define STEP            16

PBYTE GetAllProfileNames(PSZ pszApp, HINI hini, PULONG pulProfileSize);
PVOID GetProfileData(PSZ pszApp, PSZ pszKey, HINI hini, PULONG pulProfileSize);


int main(int iArgc, PSZ pszArg[])
{
HOBJECT hObj;
BYTE    szObjectID[20];
PBYTE   pBuffer;
ULONG   ulProfileSize;
BYTE    szBlockText[STEP + 1];

   if (iArgc != 2)
      {
      printf("USAGE: GETFPOS ObjectName");
      exit(1);
      }

   hObj = WinQueryObject(pszArg[1]);
   if (!hObj)
      {
      printf("Object not found !");
      exit(1);
      }
   sprintf(szObjectID, "%ld@10", hObj);

   pBuffer = GetProfileData(FOLDERPOS, szObjectID, HINI_PROFILE, &ulProfileSize);
   if (pBuffer)
      {
      PBYTE p;
      USHORT usIndex;

      printf("%s(%lX) - %s\n", pszArg[1], hObj, szObjectID);
      memset(szBlockText, 0, sizeof szBlockText);
      usIndex = 0;
      for (p = pBuffer; p < pBuffer + ulProfileSize; p++)
         {
         if (usIndex >= STEP)
            {
            printf("  %s\n", szBlockText);
            memset(szBlockText, 0, sizeof szBlockText);
            usIndex = 0;
            }
         printf("%2.2X ",(USHORT)*p);
         if (*p == '\a' || *p == '\n' || *p == '\r' || *p == 0)
            szBlockText[usIndex] = '.';
         else
            szBlockText[usIndex] = *p;
         usIndex++;
         }
      if (usIndex)
         {
         while (usIndex < STEP)
            {
            printf("   ");
            usIndex++;
            }
         printf("  %s\n", szBlockText);
         }
      free(pBuffer);
      }
   else
      printf("%s:%s not found\n", FOLDERPOS, szObjectID);

   printf("\nx:%3d y:%3d width:%3d height:%3d\n",
      *(PSHORT)(pBuffer + 6),
      *(PSHORT)(pBuffer + 8),
      *(PSHORT)(pBuffer + 10),
      *(PSHORT)(pBuffer + 12));

   printf("\nx:%3d y:%3d width:%3d height:%3d\n",
      *(PSHORT)(pBuffer + 18),
      *(PSHORT)(pBuffer + 20),
      *(PSHORT)(pBuffer + 22),
      *(PSHORT)(pBuffer + 24));


   return 0;
}

/*************************************************************
* Get all names
*************************************************************/
PBYTE GetAllProfileNames(PSZ pszApp, HINI hini, PULONG pulProfileSize)
{
PBYTE pBuffer;

   if (!PrfQueryProfileSize(hini,
      pszApp,
      NULL,
      pulProfileSize))
      return NULL;

   pBuffer = malloc(*pulProfileSize);
   if (!pBuffer)
      return NULL;

   if (!PrfQueryProfileData(hini,
      pszApp,
      NULL,
      pBuffer,
      pulProfileSize))
      {
      free(pBuffer);
      return NULL;
      }

   return pBuffer;
}

/*************************************************************
* Get the data from the profile
*************************************************************/
PVOID GetProfileData(PSZ pszApp, PSZ pszKey, HINI hini, PULONG pulProfileSize)
{
PBYTE pBuffer;

   if (!PrfQueryProfileSize(hini,
      pszApp,
      pszKey,
      pulProfileSize))
      {
      return NULL;
      }

   pBuffer = malloc(*pulProfileSize);
   if (!pBuffer)
      return NULL;

   if (!PrfQueryProfileData(hini,
      pszApp,
      pszKey,
      pBuffer,
      pulProfileSize))
      {
      free(pBuffer);
      return NULL;
      }
   return pBuffer;
}


