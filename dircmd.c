#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INCL_DOS
#define INCL_DOSFILEMGR
#define INCL_DOSERRORS
#include <os2.h>

#define MAX_FILES 20
#define NO_ERROR 0


BOOL RecurseDirs(PSZ pszDir);
PSZ GetOS2Error(APIRET rc);

PSZ pszCommand;
BYTE szCommand[512];

INT main(INT iArgc, PSZ rgArgv[])
{
   if (iArgc < 3)
      {
      printf("USAGE: DirCMD directory Command");
      exit(1);
      }
   pszCommand = rgArgv[2];
   RecurseDirs(rgArgv[1]);

   return 0;
}

BOOL RecurseDirs(PSZ pszDir)
{
APIRET rc;
PSZ  pszPath;
HDIR FindHandle;
ULONG ulAttr;
PFILEFINDBUF3 pFind;
ULONG ulFindCount;
INT iIndex;


   pszPath = malloc(512);
   if (!pszPath)
      {
      printf("Not enough memory\n");
      exit(1);
      }
   strcpy(pszPath, pszDir);
   strcat(pszPath, "*.*");

   FindHandle = HDIR_CREATE;
   ulAttr = FILE_DIRECTORY | FILE_ARCHIVED | FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | MUST_HAVE_DIRECTORY;

   ulFindCount = MAX_FILES;
   pFind = (PFILEFINDBUF3) calloc(sizeof (FILEFINDBUF3), ulFindCount);

   rc =  DosFindFirst(pszPath,
      &FindHandle,
      ulAttr, 
      pFind,
      ulFindCount * sizeof (FILEFINDBUF3),
      &ulFindCount,
      FIL_STANDARD);

   while (!rc)
      {
      PFILEFINDBUF3 pf = pFind;

      for (iIndex = 0; iIndex < ulFindCount; iIndex++)
         {
         if (strcmp(pf->achName, ".") && strcmp(pf->achName, ".."))
            {
            strcpy(pszPath, pszDir);
            strcat(pszPath, pf->achName);
            printf("%s\n", pszPath);

            sprintf(szCommand, pszCommand, pszPath);
            printf("%s\n", szCommand);
            system(szCommand);

            strcat(pszPath, "\\");
            if (!RecurseDirs(pszPath))
               return FALSE;
            
            }

         if (!pf->oNextEntryOffset)
            break;
         pf = (PFILEFINDBUF3)((PBYTE)pf + pf->oNextEntryOffset);
         }
      ulFindCount = MAX_FILES;
      rc = DosFindNext(FindHandle, pFind,
         ulFindCount * sizeof (FILEFINDBUF3), &ulFindCount);
      }
   if (rc != 18)
      printf("DosFindFirst/Next, rc = %s\n", GetOS2Error(rc));
   free(pszPath);
   DosFindClose(FindHandle);

   return TRUE;
}

/*********************************************************************
* Get an OS/2 errormessage out of the message file
*********************************************************************/
PSZ GetOS2Error(APIRET rc)
{
static BYTE szErrorBuf[2048];
static BYTE szErrNo[12];
APIRET rc2;
ULONG  ulReplySize;

   memset(szErrorBuf, 0, sizeof szErrorBuf);
   if (rc)
      {
      sprintf(szErrNo, "SYS%4.4u: ", rc);
      rc2 = DosGetMessage(NULL, 0, szErrorBuf, sizeof(szErrorBuf),
                          (ULONG) rc, "OSO001.MSG", &ulReplySize);
      switch (rc2)
         {
         case NO_ERROR:
            strcat(szErrorBuf, "\n");
            break;
         case ERROR_FILE_NOT_FOUND :
            sprintf(szErrorBuf, "SYS%04u (Message file not found!)", rc);
            break;
         default:
            sprintf(szErrorBuf, "SYS%04u (Error %d while retrieving message text!)", rc, rc2);
            break;
         }
      DosGetMessage(NULL,
         0,
         szErrorBuf + strlen(szErrorBuf),
         sizeof(szErrorBuf) - strlen(szErrorBuf),
         (ULONG) rc,
         "OSO001H.MSG",
         &ulReplySize);
      }

   if (memicmp(szErrorBuf, "SYS", 3))
      {
      memmove(szErrorBuf + strlen(szErrNo), szErrorBuf, strlen(szErrorBuf) + 1);
      memcpy(szErrorBuf, szErrNo, strlen(szErrNo));
      }
   return szErrorBuf;
}
