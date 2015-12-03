#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <conio.h>


#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include "wptools.h"
#include "eaback.h"

#define MAX_DENABUF 32000

/**********************************************************************
* Globals
**********************************************************************/
static ULONG ulMaxPathLen    = 256;
static BYTE  szStartDir[512] = "?:\\";
static BYTE  szFileSpec[128] = "*.*";
static BOOL  fVerbose        = FALSE;
static BOOL  fRecurse        = TRUE;
static BOOL  fRecurseSet     = FALSE;
static BOOL  fBackup         = FALSE;
static BYTE  bDena[MAX_DENABUF];
static BYTE  szBackupFile[255]; 
static ULONG ulTotalBytes;
static HFILE hfBackupFile = -1;

/**********************************************************************
* Function prototypes
**********************************************************************/
static PSZ  GetOS2Error       (APIRET rc);
static BOOL InitProg          (INT iArgc, PSZ pszArgv[]);
static VOID ErrMsg            (PSZ pszOperation, PSZ pszText, APIRET rc);
static BOOL ScanDirectory     (PSZ pszStartDir, BOOL fCheckDir);
static BOOL CheckDirEA        (PSZ pszName, ULONG ulEABytes);
static BOOL CheckFileEA       (PSZ pszName, ULONG ulEABytes);
static BOOL CheckEAs(ULONG ulDenaCnt, PDENA2 pDenaBlock, HFILE hfFile, PSZ pszName);
static USHORT ErrorMessage    (PSZ pszFormat, ...);


/**********************************************************************
* The Main Thing
**********************************************************************/
INT main(INT iArgc, PSZ pszArgv[])
{
APIRET rc;
ULONG  ulActionTaken;

   if (!InitProg(iArgc, pszArgv))
      exit(1);

   if (fBackup)
      {
      rc = DosOpen(szBackupFile,
         &hfBackupFile,
         &ulActionTaken,
         0L, /* file size */
         0L, /* file attributes */
         FILE_CREATE | OPEN_ACTION_REPLACE_IF_EXISTS,
         OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE,
         NULL);

      if (rc)
         {
         ErrorMessage("DosOpen %s : %s\n",
               szBackupFile, GetOS2Error(rc));
         exit(1);
         }
      }


   ScanDirectory(szStartDir, TRUE);

   if (fBackup)
      DosClose(hfBackupFile);

   printf("%ld kilobytes in extended attributes\n",  ulTotalBytes/1024L);
   printf("(%ld bytes)\n", ulTotalBytes);
   return 0;
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
      CheckDirEA(pszStartDir, 0);


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
            if (find.cbList > 4)
               CheckDirEA(pszSearch, find.cbList / 2);
            if (fRecurse)
               ScanDirectory(pszSearch, FALSE);
            }
         else
            {
            if (find.cbList > 4)
               CheckFileEA(pszSearch, find.cbList / 2);
            }
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

/**********************************************************************
* Check the EA's or a directory
**********************************************************************/
BOOL CheckDirEA(PSZ pszName, ULONG ulEABytes)
{
ULONG ulOrd;
ULONG ulDenaCnt;
APIRET rc;

   if (fVerbose)
      printf("Checking directory : %s (%ld bytes in EA's)\n", pszName, ulEABytes);

   ulOrd = 1;
   ulDenaCnt = (ULONG)-1;

   memset(bDena, 0, sizeof bDena);
   rc = DosEnumAttribute(ENUMEA_REFTYPE_PATH,
      pszName,
      ulOrd,
      bDena,
      sizeof bDena,
      &ulDenaCnt,
      ENUMEA_LEVEL_NO_VALUE);

   /*
      Let's do it one at a time
   */
   if (rc == ERROR_EA_LIST_INCONSISTENT)
      {
      ErrMsg("DosEnumAttribute", pszName, rc);
      printf("Trying individual calls now\n");
      ulDenaCnt = 1;
      for (ulOrd = 1; ulOrd < 15; ulOrd++)
         {
         rc = DosEnumAttribute(ENUMEA_REFTYPE_PATH,
            pszName,
            ulOrd,
            bDena,
            sizeof bDena,
            &ulDenaCnt,
            ENUMEA_LEVEL_NO_VALUE);
         if (rc)
            {
            ErrorMessage("DosEnum on EA #%d : %s\n",
               ulOrd, GetOS2Error(rc));
            }
         }
      }

   if (rc)
      {
      ErrMsg("DosEnumAttribute", pszName, rc);
      return FALSE;
      }
   if (!ulDenaCnt)
      return TRUE;

   CheckEAs(ulDenaCnt, (PDENA2)bDena, 0, pszName);

   return TRUE;
}

/**********************************************************************
* Check the EA's of a file
**********************************************************************/
BOOL CheckFileEA(PSZ pszName, ULONG ulEABytes)
{
ULONG ulOrd;
ULONG ulDenaCnt;
APIRET rc;
ULONG  ulActionTaken;
HFILE  hfFile;


   if (fVerbose)
      printf("Checking File : %s (%ld bytes in EA's)\n", pszName, ulEABytes);

   rc = DosOpen(pszName,
      &hfFile,
      &ulActionTaken,
      0L, /* file size */
      0L, /* file attributes */
      OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
      OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY,
      NULL);
   if (rc)
      {
      ErrMsg("DosOpen", pszName, rc);
      return FALSE;
      }

   /*
      Now query ea's
   */

   ulOrd = 1;
   ulDenaCnt = (ULONG)-1;

   memset(bDena, 0, sizeof bDena);
   rc = DosEnumAttribute(ENUMEA_REFTYPE_FHANDLE,
      &hfFile,
      ulOrd,
      bDena,
      sizeof bDena,
      &ulDenaCnt,
      ENUMEA_LEVEL_NO_VALUE);

   if (rc)
      {
      ErrMsg("DosEnumAttribute", pszName, rc);
      DosClose(hfFile);
      return FALSE;
      }
   if (!ulDenaCnt)
      {
      DosClose(hfFile);
      return TRUE;
      }
   CheckEAs(ulDenaCnt, (PDENA2)bDena, hfFile, pszName);
   DosClose(hfFile);
   return TRUE;
}

/**********************************************************************
* Check EA's
**********************************************************************/
BOOL CheckEAs(ULONG ulDenaCnt, PDENA2 pDenaBlock, HFILE hfFile, PSZ pszName)
{
ULONG  ulFEASize;
ULONG  ulGEASize;
ULONG  ulIndex;
EAOP2  EAop2;
PGEA2  pGea;
PDENA2 pDena;
APIRET rc;
ULONG  ulBytes = 2;

   /*
      Calculate size of needed buffers
   */
   ulFEASize = sizeof (ULONG);
   ulGEASize = sizeof (ULONG);
   pDena = (PDENA2)pDenaBlock;
   for (ulIndex = 0; ulIndex < ulDenaCnt; ulIndex++)
      {
      ULONG ulFSize = sizeof (FEA2) + pDena->cbName + pDena->cbValue;
      ULONG ulGSize = sizeof (GEA2) + pDena->cbName;
      ulBytes += (ulFSize - 3);

      ulFSize += (4 - (ulFSize % 4));
      ulFEASize += ulFSize;

      ulGSize += (4 - (ulGSize % 4));
      ulGEASize += ulGSize;

      if (!pDena->oNextEntryOffset)
         break;
      pDena = (PDENA2) ((PBYTE)pDena + pDena->oNextEntryOffset);
      }

   /*
      Allocate needed buffers
   */
   EAop2.fpGEA2List = malloc(ulGEASize);
   if (!EAop2.fpGEA2List)
      {
      ErrorMessage("Not enough memory\n");
      return FALSE;
      }
   memset(EAop2.fpGEA2List, 0, ulGEASize);

   EAop2.fpFEA2List = malloc(ulFEASize);
   if (!EAop2.fpFEA2List)
      {
      free(EAop2.fpGEA2List);
      ErrorMessage("Not enough memory\n");
      return FALSE;
      }
   memset(EAop2.fpFEA2List, 0, ulFEASize);


   /*
      Build the PGEALIST
   */
   EAop2.fpGEA2List->cbList = ulGEASize;
   EAop2.fpFEA2List->cbList = ulFEASize;

   pDena = (PDENA2)pDenaBlock;
   pGea  = EAop2.fpGEA2List->list;
   for (ulIndex = 0; ulIndex < ulDenaCnt; ulIndex++)
      {
      USHORT usDiff;

      pGea->cbName = pDena->cbName;
      memcpy(pGea->szName, pDena->szName, pDena->cbName + 1);

      if (ulIndex < ulDenaCnt - 1)
         pGea->oNextEntryOffset = sizeof (GEA2) + pDena->cbName;
      else
         pGea->oNextEntryOffset = 0;

      usDiff = pGea->oNextEntryOffset % 4;
      if (usDiff)
         pGea->oNextEntryOffset += 4 - usDiff;

      pGea  = (PGEA2)  ((PBYTE)pGea + pGea->oNextEntryOffset);
      pDena = (PDENA2) ((PBYTE)pDena + pDena->oNextEntryOffset);
      }

   if (hfFile)
      {
      rc = DosQueryFileInfo(hfFile,
         FIL_QUERYEASFROMLIST,
         (PBYTE) &EAop2,
         sizeof EAop2);
      }
   else
      {
      rc = DosQueryPathInfo(pszName,
         FIL_QUERYEASFROMLIST,
         (PBYTE) &EAop2,
         sizeof EAop2);
      }

   if (rc)
      {
      ErrMsg("DosQueryXXXXInfo", pszName, rc);
      free(EAop2.fpGEA2List);
      free(EAop2.fpFEA2List);
      return FALSE;
      }

   pDena = EAop2.fpFEA2List->list;
   for (ulIndex = 0; ulIndex < ulDenaCnt; ulIndex++)
      {
      if (fVerbose)
         {
         PSZ pszEAType ;
         switch (*(PUSHORT)(pDena->szName + pDena->cbName + 1))
            {
            case EA_LPBINARY    :
               pszEAType = "Length preceeding binary data";
               break;

            case EA_LPASCII     :
               pszEAType = "Length preceeding ASCII";
               break;

            case EA_LPBITMAP    :
               pszEAType = "Length preceeding bitmap";
               break;

            case EA_LPMETAFILE  :
               pszEAType = "Length preceeding metafile";
               break;

            case EA_LPICON      :
               pszEAType = "Length preceeding icon";
               break;

            case EA_ASCIIZ      :
               pszEAType = "ASCII-Zero data";
               break;

            case EA_ASCIIZFN    :
               pszEAType = "ASCII-Zero filename";
               break;

            case EA_ASCIIZEA    :
               pszEAType = "ASCII-Zero EA";
               break;

            case EA_MVMT        :
               pszEAType = "Multiple-Value Multi-Type";
               break;

            case EA_MVST        :
               pszEAType = "Multiple-Value Single-Type";
               break;

            case EA_ASN1        :
               pszEAType = "ASN.1 Field";
               break;

            default             :
               pszEAType = "Unknown type";
               break;
            }
         printf("  %-20.20s: %5d bytes, %s\n",
            pDena->szName, pDena->cbValue, pszEAType);
         }
      if (!pDena->oNextEntryOffset)
         break;
      pDena = (PDENA2) ((PBYTE)pDena + pDena->oNextEntryOffset);
      }

   if (fVerbose)
      printf("Total EA Size for %s is %ld bytes\n\n",
         pszName, ulBytes);

   ulTotalBytes += ulBytes;

   if (fBackup)
      {
      BHDR bHdr;
      ULONG ulBytesWritten;

      memset(&bHdr, 0, sizeof bHdr);
      bHdr.bVersion = VERSION;
      strcpy(bHdr.szFileName, pszName);
      bHdr.ulEASize = ulFEASize;

      rc = DosWrite(hfBackupFile,
            &bHdr,
            sizeof bHdr,
            &ulBytesWritten);

      if (rc)
         {
         ErrorMessage("DosWrite %s : %s\n",
               szBackupFile, GetOS2Error(rc));
         exit(1);
         }


      rc = DosWrite(hfBackupFile,
            EAop2.fpFEA2List,
            ulFEASize,
            &ulBytesWritten);

      if (rc)
         {
         ErrorMessage("DosWrite %s : %s\n",
               szBackupFile, GetOS2Error(rc));
         exit(1);
         }
      }


   free(EAop2.fpGEA2List);
   free(EAop2.fpFEA2List);

   return TRUE;
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


   printf("EABACK for OS/2 2.x - Made by Henk Kelder\n");
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
            case 'V':
               fVerbose = TRUE;
               break;
            case 'S':
               fRecurseSet = TRUE;
               fRecurse    = TRUE;
               break;
            case 'F':
               fBackup = TRUE;
               p = strpbrk(pszArgv[iArg], ":=");
               if (!p)
                  {
                  ErrorMessage("Missing '=' or ';' after /F\n");
                  exit(1);
                  }
               strcpy(szBackupFile, p+1);
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
