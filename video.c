#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <conio.h>
#define INCL_WIN
#define INCL_DOS
#include <os2.h>


#define APPLICATION "PM_DISPLAYDRIVERS"
#define KEY1 "DEFAULTDRIVER"
#define KEY2 "CURRENTDRIVER"


BOOL GetProfileData(PSZ pszApp, PSZ pszKey, HINI hini);

int main(int iArgc, PSZ pszArg[])
{
HINI hini = HINI_USERPROFILE;

   GetProfileData(APPLICATION, KEY1, hini);
   GetProfileData(APPLICATION, KEY2, hini);
   
   if (iArgc > 1)
      {
      strupr(pszArg[1]);
      if (strcmp(pszArg[1], "IBMVGA32") &&
          strcmp(pszArg[1], "WD90C24"))
         {
         printf("Only IBMVGA32 or WD90C24 are allowed\n");
         exit(1);
         }
      PrfWriteProfileData(hini, APPLICATION, KEY1,
         pszArg[1], strlen(pszArg[1]) + 1);
      PrfWriteProfileData(hini, APPLICATION, KEY2,
         pszArg[1], strlen(pszArg[1]) + 1);
      GetProfileData(APPLICATION, KEY1, hini);
      GetProfileData(APPLICATION, KEY2, hini);
      }

   return 0;
}

/*******************************************************************
* Get ProfileData
*******************************************************************/
BOOL GetProfileData(PSZ pszApp, PSZ pszKey, HINI hini)
{
ULONG ulProfileSize;
PBYTE pBuffer;

   if (!PrfQueryProfileSize(hini,
      pszApp,
      pszKey,
      &ulProfileSize))
      return FALSE;

   pBuffer = malloc(ulProfileSize+10);

   if (!PrfQueryProfileData(hini,
      pszApp,
      pszKey,
      pBuffer,
      &ulProfileSize))
      {
      printf("Error while retrieving profile data for %s", pszKey);
      return FALSE;
      }
   printf("%s is %s\n", pszKey, pBuffer);
   free(pBuffer);
   return TRUE;
}

