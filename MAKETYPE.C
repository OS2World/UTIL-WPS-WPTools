#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#define INCL_WIN
#include <os2.h>

BOOL GetData(PSZ pszApp, PSZ pszKey, HINI hini);

int main(int argc, PBYTE pArgv[])
{
BYTE pData[2];

   if (argc == 1)
      {
      printf("USAGE: MakeType NewType\n");
      printf("Example: Maketype MyOwnType\n");
      exit(1);
      }

   if (GetData("PMWP_ASSOC_TYPE", pArgv[1], HINI_USERPROFILE))
      {
      printf("This type already exist!");
      return 0;
      }


   if (!PrfWriteProfileData(HINI_USERPROFILE, "PMWP_ASSOC_TYPE",
      pArgv[1], pData, 1))
      printf("Error while writing output ini\n");
   return 0;
}


BOOL GetData(PSZ pszApp, PSZ pszKey, HINI hini)
{
ULONG ulProfileSize;
PBYTE pBuffer, p;

   if (!PrfQueryProfileSize(hini,
      pszApp,
      pszKey,
      &ulProfileSize))
      {
      return FALSE;
      }

   if (!ulProfileSize)
      return FALSE;
   pBuffer = malloc(ulProfileSize);

   if (!PrfQueryProfileData(hini,
      pszApp,
      pszKey,
      pBuffer,
      &ulProfileSize))
      {
      free(pBuffer);
      return FALSE;
      }
   free(pBuffer);
   return TRUE;
}
