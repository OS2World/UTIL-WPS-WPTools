#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INCL_DOSDEVIOCTL
#define INCL_DOSDEVICES
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#define MAX_LIST 65535

typedef struct _MYFINDBUF3                 /* findbuf3 */
{
   ULONG   oNextEntryOffset;            /* new field */
   FDATE   fdateCreation;
   FTIME   ftimeCreation;
   FDATE   fdateLastAccess;
   FTIME   ftimeLastAccess;
   FDATE   fdateLastWrite;
   FTIME   ftimeLastWrite;
   ULONG   cbFile;
   ULONG   cbFileAlloc;
   ULONG   attrFile;                    /* widened field */
} MYFINDBUF3, *PMYFINDBUF3;

typedef struct _MYFINDNAME
{
   UCHAR   cchName;
   CHAR    achName[CCHMAXPATHCOMP];
} MYFINDNAME, *PMYFINDNAME;


#include "wptools.h"

BOOL fDelEA(PSZ pszDir, PGEA2LIST pGealist);

BYTE bBuffer[512];

INT main(INT iArgc, PSZ rgArgv[])
{
BYTE szDrive[] = "C:";
PGEA2LIST pGealist;

   if (iArgc < 3)
      {
      printf("USAGE: DELEA Drive EA-ToRemove\n");
      exit(1);
      }

   pGealist = (PGEA2LIST)bBuffer;
   pGealist->list[0].cbName = (BYTE)strlen(rgArgv[2]);
   strcpy(pGealist->list[0].szName, rgArgv[2]);
   pGealist->cbList = 4 + sizeof (GEA2) + strlen(rgArgv[2]);

   szDrive[0] = rgArgv[1][0];
   strupr(szDrive);
   return fDelEA(szDrive, pGealist);
}


BOOL fDelEA(PSZ pszDir, PGEA2LIST pGealist)
{
PEAOP2 peaop2;
HDIR hFind;
APIRET rc;
PSZ pszPath;
USHORT usMaxPath;
ULONG  ulFindCount;
PBYTE  pStart;

   usMaxPath = strlen(pszDir) + 64;

   pszPath = malloc(usMaxPath);
   if (!pszPath)
      {
      printf("Not enough memory in fDelEA!\n");
      exit(1);
      }
   strcpy(pszPath, pszDir);
   strcat(pszPath, "\\*.*");

   peaop2 = (PEAOP2)malloc(MAX_LIST);
   if (!peaop2)
      {
      printf("Not enough memory in fDelEA!\n");
      exit(1);
      }
   memset(peaop2, 0, MAX_LIST);
   peaop2->fpGEA2List = pGealist;

   hFind = HDIR_CREATE;
   ulFindCount = 65535;

   rc = DosFindFirst(pszPath,
      &hFind,
      FILE_ARCHIVED | FILE_DIRECTORY | FILE_SYSTEM | FILE_HIDDEN | FILE_READONLY,
      peaop2,
      MAX_LIST,
      &ulFindCount,
      FIL_QUERYEASFROMLIST);

   while (!rc)
      {
      PMYFINDBUF3 tFind;
      PFEA2LIST pFealist;
      PMYFINDNAME tName;
      ULONG ulIndex;

      tFind    = (PMYFINDBUF3)(peaop2 + 1);
      for (ulIndex = 0; ulIndex < ulFindCount; ulIndex++)
         {
         pFealist = (PFEA2LIST)(tFind + 1);
         tName    = (PMYFINDNAME)((PBYTE)pFealist + pFealist->cbList);
         if (tName->achName[0] != '.')
            {
            strcpy(pszPath, pszDir);
            strcat(pszPath, "\\");
            strcat(pszPath, tName->achName);
            if (pFealist->cbList > 4 && pFealist->list[0].cbValue > 0)
               {
               printf("removing %s for %s\n", pGealist->list[0].szName, pszPath);
               SetEAValue(pszPath, pGealist->list[0].szName, EA_LPASCII, "", 0);
               }
            if (tFind->attrFile & FILE_DIRECTORY)
               fDelEA(pszPath, pGealist);

            }
        tFind = (PMYFINDBUF3)((PBYTE)tFind + tFind->oNextEntryOffset);
         }

      ulFindCount = 65535;
      memset(peaop2, 0, MAX_LIST);
      peaop2->fpGEA2List = pGealist;
      rc = DosFindNext(hFind, peaop2, MAX_LIST, &ulFindCount);
      }
   if (rc != ERROR_NO_MORE_FILES)
      printf("DosFindFirst/Next returned %d\n", rc);

   free(pszPath);
   free(peaop2);
   return TRUE;
}

