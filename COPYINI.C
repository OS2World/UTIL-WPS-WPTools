#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <signal.h>
#define INCL_WIN
#define INCL_DOS
#include <os2.h>

void CopyAllKeys(PSZ pszApp, HINI hini, HINI hOut);
void CopyData(PSZ pszApp, PSZ pszKey, HINI hIni, HINI hOut);
BOOL CopyIni(HINI hini, PSZ pszOut);
VOID CtrlCHandler        (int signal);

BOOL APIENTRY PrfChangeWriteThru(ULONG flOptions, BOOL fSet);

ULONG ulCount;

int main(int argc, PSZ pszArg[])
{
BYTE szOutUser  [256];
BYTE szOutSystem[256];
PRFPROFILE Profile;
BYTE  szUserIni[256];
BYTE  szSystemIni[256];


   signal(SIGINT, CtrlCHandler);
   signal(SIGTERM, CtrlCHandler);
   signal(SIGBREAK, CtrlCHandler);



   printf("COPYINI version 1.2 (%s) - Henk Kelder\n", __DATE__);
   if (argc < 2)
      {
      printf("USAGE: COPYINI full-destination-directory [/NWT]\n");
      exit(1);
      }
   if (argc ==  3)
      {
      if (!stricmp(pszArg[2], "/NWT"))
         PrfChangeWriteThru(0xFFFF, FALSE);
      else
         {
         printf("Error: Unknown option \'%s\' \n", pszArg[2]);
         exit(1);
         }
      }

   strupr(pszArg[1]);
   if (!_fullpath(szOutUser, pszArg[1], sizeof szOutUser))
      {
      printf("Error: Unable to locate %s\n", pszArg[1]);
      exit(1);
      }

   if (szOutUser[strlen(szOutUser) - 1] != '\\')
      strcat(szOutUser, "\\");
   strcpy(szOutSystem, szOutUser);
   strcat(szOutUser, "OS2.INI");
   strcat(szOutSystem, "OS2SYS.INI");

   Profile.pszUserName = szUserIni;
   Profile.cchUserName = sizeof szUserIni;
   Profile.pszSysName = szSystemIni;
   Profile.cchSysName = sizeof szSystemIni;
   printf("Querying location of INI-files...");
   fflush(stdout);
   if (!PrfQueryProfile(0, &Profile))
      {
      printf("Error while retrieving current profile info!");
      exit(1);
      }
   printf("done!\n");

   if (!stricmp(szOutUser, szUserIni) ||
       !stricmp(szOutSystem, szSystemIni))
      {
      printf("Error: destination-directory should not be the same as the\n");
      printf("       normal location of OS2.INI and OS2SYS.INI!\n");
      exit(1);
      }

   if (!access(szOutUser, 0) && unlink(szOutUser))
      {
      printf("Error: Unable to delete target ini-files!\n");
      exit(1);
      }
   if (!access(szOutSystem, 0) && unlink(szOutSystem))
      {
      printf("Error: Unable to delete target ini-files!\n");
      exit(1);
      }
   CopyIni(HINI_USERPROFILE,   szOutUser);
   CopyIni(HINI_SYSTEMPROFILE, szOutSystem);

   PrfChangeWriteThru(0xFFFF, TRUE);

return 0;
}

/*******************************************************
* Copy a inifile
*******************************************************/
BOOL CopyIni(HINI hini, PSZ pszOut)
{
HINI hOut;
ULONG ulProfileSize;
PBYTE pBuffer, p;

   if (!access(pszOut, 0) && unlink(pszOut))
      {
      printf("Error: cannot delete %s\n", pszOut);
      exit(1);
      }

   printf("===== Creating %s ====\n", pszOut);


   hOut = PrfOpenProfile(0, pszOut);
   if (!hOut)
      {
      printf("Fatal error, unable to create %s", pszOut);
      exit(1);
      }

   if (!PrfQueryProfileSize(hini,
      NULL,
      NULL,
      &ulProfileSize))
      {
      printf("Error while retrieving profile size\n");
      return 0;
      }

   pBuffer = malloc(ulProfileSize);

   if (!PrfQueryProfileData(hini,
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
      CopyAllKeys(p, hini, hOut);
      p += strlen(p) + 1;
      }
   free(pBuffer);
   PrfCloseProfile(hOut);
   return TRUE;
}

/*******************************************************
* Get all Keys in the ini-file
*******************************************************/
void CopyAllKeys(PSZ pszApp, HINI hini, HINI hOut)
{
ULONG ulProfileSize;
PBYTE pBuffer, p;

   printf("Copying %-50s       ", pszApp);
   fflush(stdout);

   if (!PrfQueryProfileSize(hini,
      pszApp,
      NULL,
      &ulProfileSize))
      {
      printf("Error while retrieving profile size for app %s\n", pszApp);
      return;
      }

   pBuffer = malloc(ulProfileSize);

   if (!PrfQueryProfileData(hini,
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
      printf("\b\b\b\b%4d", ++ulCount);
      fflush(stdout);

      CopyData(pszApp, p, hini, hOut);
      p += strlen(p) + 1;
      }
   printf(" Keys\n");
   free(pBuffer);
}

/*******************************************************
* Get the data in a ini-file
*******************************************************/
void CopyData(PSZ pszApp, PSZ pszKey, HINI hini, HINI hOut)
{
ULONG ulProfileSize;
PBYTE pBuffer;

//   printf("%s:%s\n", pszApp, pszKey);

   if (!PrfQueryProfileSize(hini,
      pszApp,
      pszKey,
      &ulProfileSize))
      {
      printf("Error while retrieving profile size for app %s\n", pszApp);
      return;
      }

   pBuffer = malloc(ulProfileSize);

   if (!PrfQueryProfileData(hini,
      pszApp,
      pszKey,
      pBuffer,
      &ulProfileSize))
      {
      printf("Error while retrieving application names for app %s\n", pszApp);
      return;
      }

   if (!PrfWriteProfileData(hOut, pszApp, pszKey, pBuffer, ulProfileSize))
      printf("Error while writing output ini\n");
   free(pBuffer);
}

void CtrlCHandler(int signal)
{
   printf("\nprogram aborted!\n");
   PrfChangeWriteThru(0xFFFF, TRUE);
   exit(1);
}

