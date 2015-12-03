#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <conio.h>
#define INCL_DOS
#define INCL_PM
#include <os2.h>

#include "wptools.h"
#include "msg.h"

VOID APIENTRY16 ShePiInitialise(PPRFPROFILE _Seg16);
VOID APIENTRY16 ShpPi16Shutdown(VOID);
BOOL APIENTRY16 SheUpdateIniFile(HINI hini);

BOOL fFindDesktop(PSZ pszPath, PSZ pszDesktopPath, USHORT usMaxSize);

/********************************
typedef struct _PRFPROFILE {
	ULONG  cchUserName;
	PSZ    pszUserName;
	ULONG  cchSysName;
	PSZ    pszSysName;
} PRFPROFILE;
typedef PRFPROFILE FAR *PPRFPROFILE;
**********************************/

BYTE szDesktopPath[300];


/*******************************************************************
* The main thing
*******************************************************************/
INT main(INT iArgc, PSZ pszArgv[])
{

   SheUpdateIniFile(HINI_USERPROFILE);


#if 0
HINI hIniUser;
HINI hIniSystem;
PRFPROFILE Profile;
HOBJECT hObjDesktop;

   Profile.pszUserName = "E:\\HEKTOOLS\\OS2.INI";
   Profile.cchUserName = strlen(Profile.pszUserName);
   Profile.pszSysName  = "E:\\HEKTOOLS\\OS2SYS.INI";
   Profile.cchSysName  = strlen(Profile.pszSysName);
   ShePiInitialise(&Profile);

   printf("Opening profile\n");
   hIniUser = PrfOpenProfile(0, "E:\\HEKTOOLS\\OS2.INI");
   if (!hIniUser)
      {
      printf("Unable to open OS2.INI!\n");
      exit(1);
      }
   hIniSystem = PrfOpenProfile(0, "E:\\HEKTOOLS\\OS2SYS.INI");
   if (!hIniSystem)
      {
      PrfCloseProfile(hIniUser);
      printf("Unable to open OS2.INI!\n");
      exit(1);
      }
   printf("Profile opened with succes\n");

   hObjDesktop = MyQueryObjectID(hIniUser, hIniSystem, "<WP_DESKTOP>");
   if (hObjDesktop)
      {
      printf("<WP_DESKTOP> points to object %5.5X\n", hObjDesktop);
      if (!PathFromObject(hIniSystem, hObjDesktop, szDesktopPath, sizeof szDesktopPath, NULL))
         printf("Unable to find fysical location of object %p\n", hObjDesktop);
      else
         printf("Object %5.5X is located at %s\n",
            hObjDesktop, szDesktopPath);
      }
   else
      printf("FATAL ERROR: <WP_DESKTOP> not found\n");


   /*
      Searching for desktop via disk
   */
   if (fFindDesktop("E:\\", szDesktopPath, sizeof szDesktopPath))
      printf("Object of class WPDesktop found at %s\n", szDesktopPath);
   else
      printf("No object of class WPDesktop found!\n");

   hObjDesktop = MyQueryObjectID(hIniUser, hIniSystem, szDesktopPath);
   printf("%s points to object %5.5X\n", szDesktopPath, hObjDesktop);

   PrfCloseProfile(hIniUser);
   PrfCloseProfile(hIniSystem);

   printf("Press any key...\n");
   getch();
#endif
   return 0;
}


/*******************************************************************
* Searching for the desktop
*******************************************************************/
BOOL fFindDesktop(PSZ pszPath, PSZ pszDesktopPath, USHORT usMaxSize)
{
BYTE szSearchPath[256];
BYTE szFullName[256];
APIRET rc;
HDIR FindHandle = HDIR_CREATE;
ULONG ulFindCount;
FILEFINDBUF3 Find;

   memset(szSearchPath, 0, sizeof szSearchPath);
   strcpy(szSearchPath, pszPath);
   if (szSearchPath[strlen(szSearchPath)-1] != '\\')
      strcat(szSearchPath, "\\");
   strcpy(szFullName, szSearchPath);
   strcat(szSearchPath, "*.*");

   ulFindCount = 1;
   rc = DosFindFirst(szSearchPath,
      &FindHandle,
      FILE_DIRECTORY | FILE_ARCHIVED | FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM,
      &Find,
      sizeof (FILEFINDBUF3),
      &ulFindCount,
      FIL_STANDARD);

   while (!rc)
      {
      if ((Find.attrFile & FILE_DIRECTORY) && Find.achName[0] != '.')
         {
         PBYTE pClassInfo;
         USHORT usClassSize;
         strcpy(szSearchPath, szFullName);
         strcat(szSearchPath, Find.achName);

         if (GetEAValue(szSearchPath, ".CLASSINFO", &pClassInfo, &usClassSize))
            {
            if (usClassSize > 17 && !strcmp(pClassInfo + 8, "WPDesktop"))
               {
               free(pClassInfo);
               strncpy(pszDesktopPath, szSearchPath, usMaxSize);
               return TRUE;
               }
            free(pClassInfo);
            }
         if (fFindDesktop(szSearchPath, pszDesktopPath, usMaxSize))
            return TRUE;
         }
      ulFindCount = 1;
      rc = DosFindNext(FindHandle, &Find, sizeof (FILEFINDBUF3), &ulFindCount);
      }
   DosFindClose(FindHandle);

   return FALSE;

}
