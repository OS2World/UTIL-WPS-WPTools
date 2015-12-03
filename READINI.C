#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <signal.h>
#define INCL_WIN
#define INCL_DOS
#include <os2.h>

void GetAllKeys(PSZ pszApp, HINI hini);
void GetData(PSZ pszApp, PSZ pszKey, HINI hIni);
BOOL ReadIni(PSZ pszOut);
VOID CtrlCHandler        (int signal);

BOOL APIENTRY PrfChangeWriteThru(ULONG flOptions, BOOL fSet);

ULONG ulCount;

int main(int argc, PSZ pszArg[])
{
BYTE szOutUser  [256];


   printf("Read version 1.2 (%s) - Henk Kelder\n", __DATE__);
   if (argc < 2)
      {
      printf("USAGE: READINI inifile [/NWT]\n");
      exit(1);
      }

   strupr(pszArg[1]);

   if (!_fullpath(szOutUser, pszArg[1], sizeof szOutUser))
      {
      printf("Error: Unable to locate %s\n", pszArg[1]);
      exit(1);
      }
   ReadIni(szOutUser);


return 0;
}

/*******************************************************
* Copy a inifile
*******************************************************/
BOOL ReadIni(PSZ pszIni)
{
HINI hIni;
ULONG ulProfileSize;
PBYTE pBuffer, p;

   hIni = PrfOpenProfile(0, pszIni);
   if (!hIni)
      {
      printf("Fatal error, unable to create %s", pszIni);
      exit(1);
      }

   if (!PrfQueryProfileSize(hIni,
      NULL,
      NULL,
      &ulProfileSize))
      {
      printf("Error while retrieving profile size\n");
      return 0;
      }

   pBuffer = malloc(ulProfileSize);

   if (!PrfQueryProfileData(hIni,
      NULL,
      NULL,
      pBuffer,
      &ulProfileSize))
      {
      printf("Error while retrieving application names\n");
      return 0;
      }

   p = pBuffer;
   while (*p)
      {
      GetAllKeys(p, hIni);
      p += strlen(p) + 1;
      }
   free(pBuffer);
   PrfCloseProfile(hIni);
   return TRUE;
}

/*******************************************************
* Get all Keys in the ini-file
*******************************************************/
void GetAllKeys(PSZ pszApp, HINI hIni)
{
ULONG ulProfileSize;
PBYTE pBuffer, p;

   if (!PrfQueryProfileSize(hIni,
      pszApp,
      NULL,
      &ulProfileSize))
      {
      printf("Error while retrieving profile size for app %s\n", pszApp);
      return;
      }

   pBuffer = malloc(ulProfileSize);

   if (!PrfQueryProfileData(hIni,
      pszApp,
      NULL,
      pBuffer,
      &ulProfileSize))
      {
      printf("Error while retrieving application names for app %s\n", pszApp);
      return;
      }

   ulCount = 0;

   p = pBuffer;
   while (*p)
      {
      fflush(stdout);

      GetData(pszApp, p, hIni);
      p += strlen(p) + 1;
      }
   free(pBuffer);
}

/*******************************************************
* Get the data in a ini-file
*******************************************************/
void GetData(PSZ pszApp, PSZ pszKey, HINI hIni)
{
ULONG ulProfileSize;
PBYTE pBuffer;

   printf("%s:%s\n", pszApp, pszKey);

   if (!PrfQueryProfileSize(hIni,
      pszApp,
      pszKey,
      &ulProfileSize))
      {
      printf("Error while retrieving profile size for app %s\n", pszApp);
      return;
      }

   pBuffer = malloc(ulProfileSize);

   if (!PrfQueryProfileData(hIni,
      pszApp,
      pszKey,
      pBuffer,
      &ulProfileSize))
      {
      printf("Error while retrieving application names for app %s\n", pszApp);
      return;
      }

   free(pBuffer);
}


