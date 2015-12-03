#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>

#define INCL_WIN
#define INCL_DOS

#include <os2.h>


INT main(INT iArgc, PSZ pszArgv[])
{
PRFPROFILE Profile;
BYTE  szUserIni[256];
BYTE  szSystemIni[256];
BYTE  chChar;


   printf("ResetWPS - Restarts OS/2's workplace shell, version 1.1 (%s)\n", __DATE__);
   printf("Made by Henk Kelder\n\n");

   if (iArgc < 2 || stricmp(pszArgv[1], "/y"))
      {
      printf("Are you sure you wanne restart the WorkPlace Shell [Y/N]?\n");
      chChar = getch();
      if (chChar != 'y' && chChar != 'Y')
         {
         printf("As I already thought - no guts..");
         exit(1);
         }
      }

   Profile.pszUserName = szUserIni;
   Profile.cchUserName = sizeof szUserIni;
   Profile.pszSysName = szSystemIni;
   Profile.cchSysName = sizeof szSystemIni;
   printf("Querying location of INI-files...");
   fflush(stdout);
   if (!PrfQueryProfile(0, &Profile))
      {
      printf("Error while retrieving current profile info!\n");
      exit(1);
      }
   printf("done!\n");
   printf("User INI is %s\n", szUserIni);
   printf("System INI is %s\n", szSystemIni);

   if (!PrfReset(0, &Profile))
      {
      printf("Error while resetting the workplace shell\n");
      exit(1);
      }
   printf("Workplace shell should now be restarting\n");
   printf("If not: Sorry, you'll have to press C-A-D!\n");
   return 0;
}
