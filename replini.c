#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>
#include <io.h>

#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>


#define MAXBUF  32000

typedef struct _DiskInfo
{
ULONG  filesys_id;
ULONG  sectors_per_cluster;
ULONG  total_clusters;
ULONG  avail_clusters;
USHORT bytes_per_sector;
} DISKINFO;

/******************* GLOBALS ************************************/
PRFPROFILE CurProfile,
           NewProfile;
BYTE  szCurUserIni[256];
BYTE  szCurSysIni[256];
BYTE  szNewUserIni[256];
BYTE  szNewSysIni[256];
BYTE  szTempFname[256];
BYTE  szBackSysIni[256];
BYTE  szBackUserIni[256];
BYTE  szMessage[]=
"This program will replace OS2.INI and OS2SYS.INI in the \\OS2 directory\n\
with a version of these files in the specified directory.\n\
\n\
It will do so by restarting the workplace shell two times. The first time\n\
the workplace shell will be restarted with the new ini's.\n\
The original ini's will be renamed and the new ini's will be copied to \n\
the \\OS2 directory.\n\
Then the workplace shell will be restarted with the copied ini's in the \n\
\\OS2 directory.\n\
\n\
WARNING: Make sure you have closed *ALL* applications before replacing the\n\
         ini's!\n";


static ULONG GetDiskSpace(BYTE bDrive);
static APIRET CopyFile(PSZ pszTarget, PSZ pszSource);
static VOID RecoverFromError(USHORT usActionsDone);




/***************************************************
* The main thing
***************************************************/
INT main(INT iArgc, PSZ pszArgv[])
{
APIRET rc;
USHORT usIndex;
BYTE   chChar;
USHORT usActionsDone = 0;


   printf("REPLINI version 1.1 (%s) - Made by Henk Kelder\n\n", __DATE__);
   if (iArgc < 3 || stricmp(pszArgv[2], "/y"))
      printf(szMessage);
   if (iArgc < 2)
      {
      printf("USAGE: REPLINI location_of_new_inis [/y]\n");
      exit(1);
      }

   /*
      Construct new ini file names
   */
   strupr(pszArgv[1]);
   if (!_fullpath(szNewUserIni, pszArgv[1], sizeof szNewUserIni))
      {
      printf("Error: Unable to locate %s\n", pszArgv[1]);
      exit(1);
      }
   if (szNewUserIni[strlen(szNewUserIni) - 1] != '\\')
      strcat(szNewUserIni, "\\");
   strcpy(szNewSysIni, szNewUserIni);
   strcat(szNewUserIni, "OS2.INI");
   strcat(szNewSysIni,  "OS2SYS.INI");

   if (access(szNewUserIni, 0))
      {
      printf("Unable to access %s\n", szNewUserIni);
      exit(1);
      }
   if (access(szNewSysIni, 0))
      {
      printf("Unable to access %s\n", szNewSysIni);
      exit(1);
      }
   NewProfile.pszUserName = szNewUserIni;
   NewProfile.cchUserName = strlen(szNewUserIni);
   NewProfile.pszSysName  = szNewSysIni;
   NewProfile.cchSysName  = strlen(szNewSysIni);

   /*
      Get current location of ini-files
   */
   CurProfile.pszUserName = szCurUserIni;
   CurProfile.cchUserName = sizeof szCurUserIni;
   CurProfile.pszSysName = szCurSysIni;
   CurProfile.cchSysName = sizeof szCurSysIni;
   fflush(stdout);
   if (!PrfQueryProfile(0, &CurProfile))
      {
      printf("Error while retrieving current profile info!\n");
      exit(1);
      }

   /*
      Compare Current and New ini-files
   */

   if (!stricmp(szNewUserIni, szCurUserIni) ||
       !stricmp(szNewSysIni , szCurSysIni))
      {
      printf("Error: New ini-files should not be current ini-files!\n");
      exit(1);
      }
   /*
      Construct Backupnames for the current ini's
   */
   strcpy(szBackUserIni, szCurUserIni);
   strupr(szBackUserIni);
   strcpy(szBackSysIni,  szCurSysIni);
   strupr(szBackSysIni);

   for (usIndex = 1; usIndex < 1000; usIndex++)
      {
      sprintf(&szBackUserIni[strlen(szBackUserIni) - 3], "%3.3u", usIndex);
      sprintf(&szBackSysIni [strlen(szBackSysIni) - 3], "%3.3u", usIndex);
      if (access(szBackUserIni, 0) && access(szBackSysIni, 0))
         break;
      }
   if (usIndex >= 1000)
      {
      printf("Error: unable to construct backup names.\n");
      printf("All numeric extensions are in use! (.001-.999)\n");
      exit(1);
      }

   /*
      Ask confirmation from the use
   */

   if (iArgc < 3 || stricmp(pszArgv[2], "/y"))
      {
      printf("New ini's found in %s\n", pszArgv[1]);
      printf("Press [Y] to proceed\n");
      chChar = getch();
      if (chChar != 'y' && chChar != 'Y')
         exit(1);
      }

   /*
      Reset ini location to new ini's
   */
   printf("Resetting ini-files location to %s ...", pszArgv[1]);
   fflush(stdout);
   if (!PrfReset(0, &NewProfile))
      {
      printf("Error while resetting profile\n");
      exit(1);
      }
   printf("Done\n");
   usActionsDone=1;


   /*
      Renaming the original ini's
   */
   printf("Renaming original %s to %s\n",
      szCurUserIni, szBackUserIni);
   rc = DosMove(szCurUserIni, szBackUserIni);
   if (rc)
      {
      printf("\aSYS%4.4u", rc);
      RecoverFromError(usActionsDone);
      }
   usActionsDone=2;

   printf("Renaming original %s to %s\n",
      szCurSysIni, szBackSysIni);
   rc = DosMove(szCurSysIni,   szBackSysIni);
   if (rc)
      {
      printf("\aSYS%4.4u", rc);
      RecoverFromError(usActionsDone);
      }
   usActionsDone=3;

   /*
      Copy the ini's
   */
   printf("Copying %s to %s..",
      szNewUserIni, szCurUserIni);
   fflush(stdout);
   rc = CopyFile(szCurUserIni, szNewUserIni);
   if (!rc)
      printf("OK\n");
   else
      {
      printf("\aSYS%4.4u\n", rc);
      RecoverFromError(usActionsDone);
      }
   usActionsDone=4;


   printf("Copying %s to %s..",
      szNewSysIni, szCurSysIni);
   fflush(stdout);

   rc = CopyFile(szCurSysIni, szNewSysIni);
   if (!rc)
      printf("OK\n");
   else
      {
      printf("\aSYS%4.4u\n", rc);
      RecoverFromError(usActionsDone);
      }
   usActionsDone=5;

   printf("Resetting ini-files location back to original location...");
   fflush(stdout);
   if (!PrfReset(0, &CurProfile))
      {
      printf("Error while resetting profile\n");
      RecoverFromError(usActionsDone);
      }
   printf("Done\n");

   return 0;
}

/*******************************************************************
* Revert all changes
*******************************************************************/
VOID RecoverFromError(USHORT usActionsDone)
{
APIRET rc;

   printf("\n\aTrying to revert all changes....\n");

   switch (usActionsDone)
      {
      case 5:
         printf("Removing new %s\n", szCurSysIni);
         rc = DosDelete(szCurSysIni);
         if (rc)
            {
            printf("Failed, returncode %d\n", rc);
            exit(1);
            }

      case 4:
         printf("Removing new %s\n", szCurUserIni);
         rc = DosDelete(szCurUserIni);
         if (rc)
            {
            printf("Failed, returncode %d\n", rc);
            exit(1);
            }

      case 3:
         printf("Renaming %s to %s\n",
            szBackSysIni, szCurSysIni);
         rc = DosMove(szBackSysIni, szCurSysIni);
         if (rc)
            {
            printf("Failed, returncode %d\n", rc);
            exit(1);
            }

      case 2:
         printf("Renaming %s to %s\n",
            szBackUserIni, szCurUserIni);
         rc = DosMove(szBackUserIni, szCurUserIni);
         if (rc)
            {
            printf("Failed, returncode %d\n", rc);
            exit(1);
            }

      case 1:
         printf("Resetting ini-files location back to original location...");
         fflush(stdout);
         if (!PrfReset(0, &CurProfile))
            {
            printf("Error while resetting profile\n");
            exit(1);
            }
         printf("Done\n");
         break;

      }
   printf("All changes reverted with succes!\n");
   exit(1);
}


/*****************************************************************
* Copy a file
*****************************************************************/
APIRET CopyFile(PSZ pszTarget, PSZ pszSource)
{
APIRET rc;
FILESTATUS3 fStat;
ULONG       ulCurDrive,
            ulDriveMap,
            ulActionTaken,
            ulDiskFree,
            ulBuffSize,
            ulBytesRead,
            ulBytesWritten;
HFILE       hfSource,
            hfTarget;
PBYTE       pbBuffer;

   /*
      Get space on destination drive
   */
   if (pszTarget[1]==':')
      ulDiskFree = GetDiskSpace(*pszTarget);
   else
      {
      rc = DosQueryCurrentDisk(&ulCurDrive, &ulDriveMap);
      if (rc)
         return rc;
      ulDiskFree = GetDiskSpace((BYTE)('@' + ulCurDrive));
      }
   if (!ulDiskFree)
      return ERROR_DISK_FULL;

   /*
      Get size of target, if it exists
   */
   rc = DosQueryPathInfo(pszTarget,
       FIL_STANDARD,
       &fStat,
       sizeof fStat);
   if (rc && rc != ERROR_FILE_NOT_FOUND)
      return rc;
   if (!rc)
      ulDiskFree += fStat.cbFileAlloc;

   /*
      Open source file
   */
   rc = DosOpen(pszSource,
      &hfSource,
      &ulActionTaken,
      0L, /* file size */
      0L, /* file attributes */
      OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
      OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY,
      NULL);
   if (rc)
      return rc;

   /*
      Query settings from source file
   */

   rc = DosQueryFileInfo(hfSource,
      FIL_STANDARD,
       &fStat,
       sizeof fStat);
   if (rc)
      {
      DosClose(hfSource);
      return rc;
      }
   
   if (fStat.cbFileAlloc > ulDiskFree)
      {
      DosClose(hfSource);
      return ERROR_DISK_FULL;
      }

   /*
      Allocate the copybuffer
   */

   ulBuffSize = fStat.cbFile;
   if (ulBuffSize > MAXBUF)
      ulBuffSize = MAXBUF;
   pbBuffer = malloc(ulBuffSize);
   if (!pbBuffer)
      {
      DosClose(hfSource);
      return ERROR_NOT_ENOUGH_MEMORY;
      }


   /*
      Open(create) Target file
   */
   rc = DosOpen(pszTarget,
      &hfTarget,
      &ulActionTaken,
      fStat.cbFile,  /* file size */
      fStat.attrFile, /* file attributes */
      OPEN_ACTION_CREATE_IF_NEW | OPEN_ACTION_REPLACE_IF_EXISTS,
      OPEN_FLAGS_WRITE_THROUGH | OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_WRITEONLY,
      NULL);

   if (rc)
      {
      DosClose(hfSource);
      return rc;
      }

   while (!(rc = DosRead(hfSource, pbBuffer, ulBuffSize, &ulBytesRead)))
      {
      if (!ulBytesRead)
         break;
      rc = DosWrite(hfTarget, pbBuffer, ulBytesRead, &ulBytesWritten);
      if (rc)
         break;
      }
   DosClose(hfSource);
   if (rc)
      {
      DosClose(hfTarget);
      return rc;
      }
   rc = DosSetFileInfo(hfTarget, FIL_STANDARD, &fStat, sizeof fStat);
   DosClose(hfTarget);
   if (rc)
      return rc;

   free(pbBuffer);
   return NO_ERROR;
}


/**********************************************************
* Get free disk space
**********************************************************/
ULONG GetDiskSpace(BYTE bDrive)
{
ULONG ulDiskFree;
DISKINFO DiskInfo;

   if (DosQFSInfo(bDrive - '@', FSIL_ALLOC, (PBYTE)&DiskInfo, sizeof DiskInfo))
      return 0L;
   ulDiskFree =  DiskInfo.avail_clusters;
   ulDiskFree *= DiskInfo.sectors_per_cluster;
   ulDiskFree *= DiskInfo.bytes_per_sector;
   return ulDiskFree;
}

