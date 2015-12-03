#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <conio.h>
#define INCL_WIN
#define INCL_DOS
#include <os2.h>


#define DEFAULT "PM_DefaultSetup"


BOOL GetProfileData(PSZ pszApp, PSZ pszKey, HINI hini);

int main(int iArgc, PSZ pszArg[])
{
ULONG ulProfileSize;
PBYTE pBuffer, p;
HINI hini = HINI_USERPROFILE;
BOOL bExist = FALSE;
BYTE chChar;

   if (iArgc > 1)
      {
      strupr(pszArg[1]);
      bExist = GetProfileData(DEFAULT, pszArg[1], hini);
      if (iArgc == 2)
         {
         if (!bExist)
            {
            printf("%s:%s does not exist\n",
               DEFAULT, pszArg[1]);
            return 0;
            }
         else
            {
            printf("Do you want to remove this default? [YN]\n");
            do
               {
               chChar = getch();
               if (chChar == 'y' || chChar == 'Y')
                  chChar = 'Y';
               else if (chChar == 'n' || chChar == 'N')
                  chChar = 'N';
               else
                  DosBeep(1000, 50);
               } while (chChar != 'N' && chChar != 'Y');
            if (chChar == 'Y')
               PrfWriteProfileData(hini, DEFAULT, pszArg[1], NULL, 0L);
            return 0;
            }
         }
       PrfWriteProfileData(hini, DEFAULT, pszArg[1],
         pszArg[2], strlen(pszArg[2]) + 1);
      if (bExist)
         printf("Changed to %s\n", pszArg[2]);
      else
         printf("Default for %s set to %s\n", pszArg[1], pszArg[2]);
      return 0;
      }

   DosError(FERR_DISABLEHARDERR);

   if (!PrfQueryProfileSize(hini,
      DEFAULT,
      NULL,
      &ulProfileSize))
      {
      printf("No defaults found!\n");
      return 0;
      }

   pBuffer = malloc(ulProfileSize);

   if (!PrfQueryProfileData(hini,
      DEFAULT,
      NULL,
      pBuffer,
      &ulProfileSize))
      {
      printf("Error while retrieving profile data for %s", DEFAULT);
      return 0;
      }

   p = pBuffer;
   while (*p)
      {
      GetProfileData(DEFAULT, p, hini);
      p += strlen(p) + 1;
      }
   free(pBuffer);
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
   printf("%s defaults to %s\n", pszKey, pBuffer);
   free(pBuffer);
   return TRUE;
}

