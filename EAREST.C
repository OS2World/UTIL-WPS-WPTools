#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <conio.h>

#define REAL

#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include "wptools.h"
#include "eaback.h"

/**********************************************************************
* Globals
**********************************************************************/
static ULONG ulMaxPathLen    = 256;
static BYTE  szStartDir[512] = "?:\\";
static BYTE  szFileSpec[128] = "*.*";
static BOOL  fVerbose        = FALSE;
static BOOL  fRecurse        = TRUE;
static BOOL  fRecurseSet     = FALSE;
static BOOL  fRestore        = FALSE;
static BOOL  fAll            = FALSE;
static BYTE  szRestoreFile[255]; 
static HFILE hfRestoreFile = -1;

static BOOL InitProg(INT iArgc, PSZ pszArgv[]);
static VOID ErrMsg(PSZ pszOperation, PSZ pszText, APIRET rc);
static BOOL fRestoreEA(HFILE hfRestoreFile, PSZ pszFileName);
static PSZ GetOS2Error(APIRET rc);
static USHORT ErrorMessage(PSZ pszFormat, ...);
static BOOL ScanDirectory(PSZ pszStartDir, BOOL fCheckDir);



INT main(INT iArgc, PSZ rgpszArgv[])
{
APIRET rc;
ULONG  ulActionTaken;

   if (!InitProg(iArgc, rgpszArgv))
      exit(1);

   if (!fRestore)
      {
      printf("No restore file specified, exiting..\n");
      exit(1);
      }
   rc = DosOpen(szRestoreFile,
      &hfRestoreFile,
      &ulActionTaken,
      0L, /* file size */
      0L, /* file attributes */
      OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
      OPEN_SHARE_DENYWRITE | OPEN_ACCESS_READONLY,
      NULL);
   if (rc)
      {
      ErrMsg("DosOpen", szRestoreFile, rc);
      return 1;
      }

   if (!fAll)
      ScanDirectory(szStartDir, TRUE);
   else
      fRestoreEA(hfRestoreFile, NULL);

   DosClose(hfRestoreFile);
   return 0;
}


/**********************************************************************
* Init program
**********************************************************************/
BOOL InitProg(INT iArgc, PSZ pszArgv[])
{
APIRET rc;
ULONG  ulCurDisk,
       ulDiskMap;
INT    iArg;
PBYTE  p;


   printf("EAREST for OS/2 2.x - Made by Henk Kelder\n");
   printf("Version 0.1\n");

   /*
      Query max path length
   */
   rc = DosQuerySysInfo(QSV_MAX_PATH_LENGTH,
      QSV_MAX_PATH_LENGTH,
      &ulMaxPathLen,
      sizeof ulMaxPathLen);
   if (rc)
      {
      ErrMsg("DosQuerySysInfo", NULL, rc);
      exit(1);
      }

   /*
      Query current drive
   */
   rc = DosQueryCurrentDisk(&ulCurDisk, &ulDiskMap);
   if (rc)
      {
      ErrMsg("DosQueryCurrentDiskInfo", NULL, rc);
      exit(1);
      }
   szStartDir[0] = (BYTE)(ulCurDisk + '@');

   /*
      Parse all the arguments
   */
   for (iArg = 1; iArg < iArgc; iArg++)
      {
      strupr(pszArgv[iArg]);
      if (pszArgv[iArg][0] == '/' || pszArgv[iArg][0] == '-')
         {
         switch (pszArgv[iArg][1])
            {
            case 'A':
               fAll = TRUE;
               break;
            case 'V':
               fVerbose = TRUE;
               break;
            case 'S':
               fRecurseSet = TRUE;
               fRecurse    = TRUE;
               break;
            case 'F':
               fRestore = TRUE;
               p = strpbrk(pszArgv[iArg], ":=");
               if (!p)
                  {
                  ErrorMessage("Missing '=' or ';' after /F\n");
                  exit(1);
                  }
               strcpy(szRestoreFile, p+1);
               break;
            default :
               ErrorMessage("Unrecoqnized open %s ignored\n", pszArgv[iArg]);
               break;
            }
         }
      else /* A filename or path name */
         {

         if (!fRecurseSet)
            fRecurse = FALSE;

         /*
            Only a drive specified
         */
         if (pszArgv[iArg][1] == ':' && !pszArgv[iArg][2])
            {
            fRecurse = TRUE;
            strcpy(szStartDir, pszArgv[iArg]);
            strcat(szStartDir, "\\");
            }
         else
            {
            FILESTATUS3 fStat;

            if (!_fullpath(szStartDir, pszArgv[iArg], sizeof szStartDir))
               {
               ErrorMessage("Error: %s does not exist!\n", pszArgv[iArg]);
               exit(1);
               }

            if (strpbrk(szStartDir, "*?"))
               {
               p = strrchr(szStartDir, '\\');
               if (p && p - szStartDir > 1)  
                  {
                  *p = 0;
                  strcpy(szFileSpec, p+1);
                  }
               }
            else
               {
               rc = DosQueryPathInfo(szStartDir,
                  FIL_STANDARD,
                  &fStat,
                  sizeof fStat);
               if (rc == ERROR_FILE_NOT_FOUND)
                  {
                  ErrorMessage("Error: %s does not exist!\n", szStartDir);
                  return FALSE;
                  }
               if (rc)
                  {
                  ErrMsg("DosQueryPathInfo", szStartDir, rc);
                  return FALSE;
                  }
               if (!(fStat.attrFile & FILE_DIRECTORY))
                  {
                  p = strrchr(szStartDir, '\\');
                  if (p && p - szStartDir > 2)
                     {
                     *p = 0;
                     strcpy(szFileSpec, p+1);
                     }
                  }
               }
            }
         }
      }

   return TRUE;
}

/*********************************************************************
* Print a errormessage
*********************************************************************/
VOID ErrMsg(PSZ pszOperation, PSZ pszText, APIRET rc)
{
   ErrorMessage("%s%s%s - (SYS%4u)%s",
      pszOperation,
      (pszText ? " on " : ""),
      (pszText ? pszText : ""),
      rc,
      GetOS2Error(rc));
}


/***********************************************************************
* Error Message
***********************************************************************/
USHORT ErrorMessage(PSZ pszFormat, ...)
{
static BYTE szMessage[1000];
va_list va;
USHORT usRetco;

   va_start(va, pszFormat);
   vsprintf(szMessage, pszFormat, va);

   usRetco = printf(szMessage);
   printf("Hit any key to continue...");
   fflush(stdout);
   getch();
   printf("\n");

   return usRetco;
}

/*********************************************************************
* Get an OS/2 errormessage out of the message file
*********************************************************************/
PSZ GetOS2Error(APIRET rc)
{
static BYTE szErrorBuf[512];
APIRET rc2;
ULONG  ulReplySize;

   if (rc)
      {
      memset(szErrorBuf, 0, sizeof szErrorBuf);
      rc2 = DosGetMessage(NULL, 0, szErrorBuf, sizeof(szErrorBuf),
                          (ULONG) rc, "OSO001.MSG", &ulReplySize);
      switch (rc2)
         {
         case NO_ERROR:
            break;
         case ERROR_FILE_NOT_FOUND :
            sprintf(szErrorBuf, "SYS%04u (Message file not found!)", rc);
            break;
         default:
            sprintf(szErrorBuf, "SYS%04u (Error %d while retrieving message text!)", rc, rc2);
            break;
         }
      }
   return szErrorBuf;
}

/**********************************************************************
* Scan a directory for all files
**********************************************************************/
BOOL ScanDirectory(PSZ pszStartDir, BOOL fCheckDir)
{
HDIR           FindHandle;
ULONG          ulAttr;
ULONG          ulFindCount;
FILEFINDBUF4   find;
PSZ            pszSearch;
APIRET         rc;

   pszSearch = malloc(ulMaxPathLen);
   if (!pszSearch)
      {
      ErrorMessage("Not enough memory!\n");
      return FALSE;
      }

   if (fCheckDir && strlen(pszStartDir) > 3) /* leave out root dirs */
      fRestoreEA(hfRestoreFile, pszStartDir);

   /*
      Get all files
   */
   strcpy(pszSearch, pszStartDir);
   if (pszSearch[strlen(pszSearch) - 1] != '\\')
      strcat(pszSearch, "\\");
   strcat(pszSearch, szFileSpec);

   FindHandle = HDIR_CREATE;
   ulAttr = FILE_DIRECTORY | FILE_ARCHIVED | FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM;

   ulFindCount = 1;
   rc =  DosFindFirst(pszSearch,
      &FindHandle, ulAttr, 
      &find,
      sizeof find,
      &ulFindCount,
      FIL_QUERYEASIZE);

   while (!rc)
      {
      strcpy(pszSearch, pszStartDir);
      if (pszSearch[strlen(pszSearch) - 1] != '\\')
         strcat(pszSearch, "\\");
      strcat(pszSearch, find.achName);

      if (find.achName[0] != '.')
         {
         if (find.attrFile & FILE_DIRECTORY)
            {
            fRestoreEA(hfRestoreFile, pszSearch);
            if (fRecurse)
               ScanDirectory(pszSearch, FALSE);
            }
         else
            fRestoreEA(hfRestoreFile, pszSearch);
         }

      ulFindCount = 1;
      rc = DosFindNext(FindHandle, &find, sizeof find, &ulFindCount);
      }
   DosFindClose(FindHandle);
   if (rc != ERROR_NO_MORE_FILES)
      ErrMsg("DosFindFirst/Next", "", rc);

   free(pszSearch);
   return TRUE;
}


/*****************************************************************************
*  fProcessBackup File
*****************************************************************************/
BOOL fRestoreEA(HFILE hfRestoreFile, PSZ pszFileName)
{
APIRET rc;
BOOL   fContinue;
BHDR   bHdr;
ULONG  ulBytesRead;
EAOP2  EAop2;
ULONG  ulNewPos;


   DosSetFilePtr(hfRestoreFile,
      0L, FILE_BEGIN, &ulNewPos);


   fContinue = TRUE;
   while (fContinue)
      {
      rc = DosRead(hfRestoreFile,
            &bHdr,
            sizeof bHdr,
            &ulBytesRead);
      if (rc)
         {
         ErrMsg("DosRead header", szRestoreFile, rc);
         return FALSE;
         }
      if (ulBytesRead != sizeof bHdr)
         break;

      if (pszFileName && stricmp(pszFileName, bHdr.szFileName))
         {
         DosSetFilePtr(hfRestoreFile,
            bHdr.ulEASize, FILE_CURRENT, &ulNewPos);
         continue;
         }



      memset(&EAop2, 0, sizeof EAop2);

      EAop2.fpFEA2List = malloc(bHdr.ulEASize);
      if (!EAop2.fpFEA2List)
         {
         ErrorMessage("Not enough memory\n");
         return FALSE;
         }

      rc = DosRead(hfRestoreFile,
            EAop2.fpFEA2List,
            bHdr.ulEASize,
            &ulBytesRead);
      if (rc)
         {
         ErrMsg("DosRead EA data", szRestoreFile, rc);
         free(EAop2.fpFEA2List);
         return FALSE;
         }

      if (ulBytesRead != bHdr.ulEASize)
         {
         ErrorMessage("Error : Corrupted EA backup file\n");
         free(EAop2.fpFEA2List);
         return FALSE;
         }
      /*
         Apply ea's to file
      */
      if (fVerbose)
         printf("Restoring EA's for %s\n",  bHdr.szFileName);

#ifdef REAL
      rc = DosSetPathInfo(bHdr.szFileName,
         FIL_QUERYEASIZE,
         (PBYTE) &EAop2,
         sizeof EAop2,
         DSPI_WRTTHRU);
      if (rc)
         ErrMsg("DosSetPathInfo", szRestoreFile, rc);
#endif

      free(EAop2.fpFEA2List);

      if (pszFileName && !stricmp(pszFileName, bHdr.szFileName))
         return TRUE;
      }


   if (pszFileName)
      {
      if (fVerbose)
         printf("No EA's found for %s\n",  pszFileName);
      return FALSE;
      }
   else
      return TRUE;   
}
