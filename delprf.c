#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INCL_WINSHELLDATA
#include <os2.h>

#include "keyb.h"

PVOID GetAllProfileNames(PSZ pszApp, HINI hini, PULONG pulProfileSize);


INT main(INT iArgc, PSZ pszArgv[])
{
PBYTE pBuffer = NULL;
PSZ   pszKeyName;
ULONG ulProfileSize;
WORD  wKey;
APIRET rc;
HINI  hini = HINI_USERPROFILE;

   if (iArgc == 1)
      {
      printf("USAGE: Delprf APPNAME [KEYNAME]\n");
      exit(1);
      }
   pBuffer = GetAllProfileNames(pszArgv[1], hini, &ulProfileSize);
   if (!pBuffer)
      {
      hini = HINI_SYSTEMPROFILE;
      pBuffer = GetAllProfileNames(pszArgv[1], hini, &ulProfileSize);
      if (pBuffer)
         printf("Found in System profile!\n");
      }
   else
      printf("Found in User Profile!\n");
   if (!pBuffer)
      {
      printf("ERROR: %s not found in profile\n", pszArgv[1]);
      exit(1);
      }
   if (iArgc > 2)
      {
      printf("%s:%s\n", pszArgv[1], pszArgv[2]);
      }
   else
      {
      pszKeyName = pBuffer;
      while (*pszKeyName)
         {
         printf("Found: %s:%s\n", pszArgv[1], pszKeyName);
         pszKeyName += strlen(pszKeyName) + 1;
         }
      }
   printf("Delete %s from profile [YN]?\n", pszArgv[1]);
   do
      {
      wKey = getchar();
      if (wKey == 'y')
         wKey = 'Y';
      else if (wKey == 'n')
         wKey = 'N';
      }
   while (wKey != 'N' && wKey != 'Y');

   if (wKey == 'N')
      return 0;
   if (iArgc == 2)
      {
      pszKeyName = pBuffer;
      while (*pszKeyName)
         {
         printf("Deleting %s:%s\n", pszArgv[1], pszKeyName);
         rc = PrfWriteProfileData(hini,
            pszArgv[1],
            pszKeyName,
            NULL, 
            0);
         if (!rc)
            printf("ERROR: Error while writing profile!\n");
         pszKeyName += strlen(pszKeyName) + 1;
         }
      }
   else
      {
      printf("Deleting %s:%s\n", pszArgv[1], pszArgv[2]);
      rc = PrfWriteProfileData(hini,
         pszArgv[1],
         pszArgv[2],
         NULL, 
         0);
      if (!rc)
         printf("ERROR: Error while writing profile!\n");
      }


   return 1;
}


/*************************************************************
* Get all names
*************************************************************/
PVOID GetAllProfileNames(PSZ pszApp, HINI hini, PULONG pulProfileSize)
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
