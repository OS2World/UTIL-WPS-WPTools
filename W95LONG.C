#ifdef DOS
#include <dos.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef DOS
#define INCL_DOSDEVIOCTL
#define INCL_DOSDEVICES
#define INCL_DOS
#include <os2.h>

#include "wptools.h"

typedef struct _DiskInfo
{
ULONG filesys_id;
ULONG sectors_per_cluster;
ULONG total_clusters;
ULONG avail_clusters;
USHORT bytes_per_sector;
} DISKINFO, *PDISKINFO;

#define _A_SYSTEM FILE_SYSTEM
#define _A_SUBDIR FILE_DIRECTORY

#else

typedef struct _DiskInfo
{
unsigned sectors_per_cluster;
unsigned total_clusters;
unsigned avail_clusters;
unsigned bytes_per_sector;
} DISKINFO, *PDISKINFO;

#endif


#include "portable.h"

#define MAX_LONGNAME 512

#pragma pack(1)

#ifdef DOS
typedef struct _DPB
{
BYTE bDrive;
BYTE bUnit;
USHORT wBytesPerSector;
BYTE bSectorsPerCluster;
BYTE bClusterToSectorShift;
USHORT wBootSectorCount;
BYTE bFatCount;
USHORT wRootDirEntries;
USHORT wFirstDataSector;
USHORT wClusterCount;
BYTE bFatSectors;
USHORT wFirstRootDirSector;
PBYTE pDevHeader;
BYTE bMediaDescriptor;
BYTE fAccessFlag;
PBYTE pNextBlock;
} DPB, *PDPB;

#else /* OS2 */

typedef struct _DPB
{
USHORT wBytesPerSector;
BYTE bSectorsPerCluster;
USHORT wBootSectorCount;
BYTE bFatCount;
USHORT wRootDirEntries;
USHORT wTotalSectors;
BYTE bMediaDescriptor;
USHORT bFatSectors;
USHORT wSectorsPerTrack;
USHORT wNumberOfHeads;
ULONG ulHiddenSectors;
ULONG ulLargeTotalSectors;

USHORT wNrOfCylinders;
BYTE bDeviceType;
USHORT wDeviceAttributes;
BYTE bReserved[6];
} DPB, *PDPB;
#endif

typedef struct _DirEntry
{
BYTE bFileName[8];
BYTE bExtention[3];
BYTE bAttr;
BYTE bReserved[10];
USHORT wTime;
USHORT wData;
USHORT wCluster;
ULONG ulFileSize;
} DIRENTRY, *PDIRENTRY;

typedef struct _LongNameEntry
{
BYTE   bNumber;
USHORT usChar1[5];
BYTE   bAttr;
BYTE   bReserved;
BYTE   bVFATCheckSum;
USHORT usChar2[6];
USHORT wCluster;
USHORT usChar3[2];
} LNENTRY, * PLNENTRY;


typedef struct _ReadAbsParm
{
ULONG ulSector;
USHORT  ulSectorsToRead;
PBYTE pBuffer;
} READINFO, *PREADINFO;

#pragma pack()


static PDPB  GetDPB(BYTE bDrive);
static USHORT  fGetRootDirSize(BYTE bDrive);
#ifdef DOS
static BOOL  fGetDiskParms(BYTE bDrive, PDISKINFO pDiskInfo);
#endif
static BOOL  fAbsRead(BYTE bDrive, ULONG ulSector, USHORT wSectorCount, PVOID pBuffer);
static BOOL  fGetLongName(PDIRENTRY pDir, PSZ pszName, USHORT wMax);
static VOID  vDumpDirEntry(PDIRENTRY pDir);
static BOOL  fScanDir(BYTE bDrive, PDIRENTRY pDir, USHORT wMaxEntries, PSZ pszLongName, USHORT wMaxName, PSZ pszDir);
static BOOL fScanSubDir(BYTE bDrive, USHORT wCluster, PSZ pszDir);
static BOOL fReadCluster(BYTE bDrive, USHORT wCluster, PVOID pBuffer);
static USHORT wGetNextCluster(BYTE bDrive, USHORT wCluster);
static BOOL fRemoveDups(PSZ pszDir);


#ifndef DOS
static HFILE hDisk;
static DPB dpb;
#endif

static PDPB pDPB;
static USHORT wRootStart;
static USHORT wRootSectors;
static USHORT wFirstDataSector;

static PBYTE pbFatSector;
static ULONG ulFatSector;
static PBYTE pszPath = NULL;

/**************************************************************
*
**************************************************************/
INT main(INT iArgc, PSZ rgArgv[])
{
static BYTE szLongName[MAX_LONGNAME];
ULONG     ulRootDirSize;
PDIRENTRY pDir;
BYTE bDrive = 'C';
BYTE szDrive[] = "C:";

#ifndef DOS
ULONG ulAction;
APIRET rc;

   if (iArgc < 2)
      {
      printf("USAGE: W95LONG drive: [path]\n");
      exit(1);
      }
   szDrive[0] = *rgArgv[1];
   strupr(szDrive);
   bDrive = szDrive[0];

   if (iArgc > 2)
      pszPath = rgArgv[2];


//   fRemoveDups(szDrive);

   rc = DosOpen(szDrive,
      &hDisk,
      &ulAction,                          /* action taken */
      0L,                                 /* new size     */
      0L,                                 /* attributes   */
      OPEN_ACTION_OPEN_IF_EXISTS,         /* open flags   */
      OPEN_ACCESS_READONLY |              /* open mode    */
        OPEN_SHARE_DENYNONE |
        OPEN_FLAGS_DASD,
      NULL);                              /* ea data      */
   if (rc)
      {
      printf("Unable to open drive %s, SYS%4.4u\n",
         szDrive,
         rc);
      exit(1);
      }
#endif

   pDPB = GetDPB(bDrive);
   wRootStart = pDPB->wBootSectorCount + pDPB->bFatSectors * pDPB->bFatCount;

   wRootSectors = fGetRootDirSize(bDrive);
   printf("Root directory starts at sector %u and is %u sectors long\n",
      wRootStart,
      wRootSectors);

   wFirstDataSector = wRootStart + wRootSectors;
   printf("First data sector = %u\n", wFirstDataSector);

   pbFatSector = malloc(pDPB->wBytesPerSector);

   ulRootDirSize = pDPB->wBytesPerSector * wRootSectors;
   if (ulRootDirSize > 65535L)
      {
      printf("Error, Root dir too long!");
      exit(1);
      }
   pDir = (PDIRENTRY)malloc((size_t)ulRootDirSize);
   if (!pDir)
      {
      printf("Not enough memory for root dir!");
      exit(1);
      }
   if (!fAbsRead(bDrive, wRootStart, wRootSectors, pDir))
      {
      printf("Error while reading rootdir");
      exit(1);
      }

   memset(szLongName, 0, sizeof szLongName);
   fScanDir(bDrive, pDir, pDPB->wRootDirEntries, szLongName, sizeof szLongName, szDrive);
   free(pDir);

#ifndef DOS
   DosClose(hDisk);   
#endif
   return 0;
}

/**************************************************************
*
**************************************************************/
BOOL fScanDir(BYTE bDrive, PDIRENTRY pDir, USHORT wMaxEntries, PSZ pszLongName, USHORT wMaxName, PSZ pszDir)
{
static BYTE   szDosName[13];
USHORT wIndex;
PSZ    pszFullName;
WORD   wNameSize;
PSZ    p;

//   printf("Scanning directory %s\n", pszDir);


   for (wIndex = 0; wIndex < wMaxEntries; wIndex++)
      {
      if (pDir->bFileName[0] && pDir->bFileName[0] != 0xE5)
         {
         memset(szDosName, 0, sizeof szDosName);
         memcpy(szDosName, pDir->bFileName, sizeof pDir->bFileName);
         p = szDosName + strlen(szDosName);
         while (p > szDosName && *(p-1) == 0x20)
            p--;
         *p = 0;
         if (memicmp(pDir->bExtention, "   ", 3))
            {
            strcat(szDosName, ".");
            strncat(szDosName, pDir->bExtention, 3);
            p = szDosName + strlen(szDosName);
            while (p > szDosName && *(p-1) == 0x20)
               p--;
            *p = 0;
            }
         wNameSize = strlen(pszDir) + 1 + strlen(szDosName) + 1;
         pszFullName = malloc(wNameSize);
         memset(pszFullName, 0, wNameSize);
         strcpy(pszFullName, pszDir);
         strcat(pszFullName, "\\");
         strcat(pszFullName, szDosName);



         if (pDir->bAttr == 0x0F)
            {
//            vDumpDirEntry(pDir);
            fGetLongName(pDir, pszLongName, wMaxName);
            }
         else
            {
            if (strlen(pszLongName))
               {
               if (stricmp(szDosName, pszLongName) &&
                  (!pszPath || !memicmp(pszFullName, pszPath, strlen(pszPath))))
                  {
                  printf("%s %s\n",
                     szDosName,
                     pszLongName);

                  SetEAValue(pszFullName,
                     ".LONGNAME",
                     EA_LPASCII,
                     pszLongName,
                     strlen(pszLongName));
                  }
               else
                  SetEAValue(pszFullName,
                     ".LONGNAME",
                     EA_LPASCII,
                     "",
                     0);

               memset(pszLongName, 0, wMaxName);
               }

            if (pDir->bAttr & _A_SUBDIR && pDir->bFileName[0] != '.')
               fScanSubDir(bDrive, pDir->wCluster, pszFullName);
            }
         free(pszFullName);
         }

      pDir++;
      }
   return TRUE;
}

/**************************************************************
*
**************************************************************/
BOOL fScanSubDir(BYTE bDrive, USHORT wCluster, PSZ pszDir)
{
USHORT       wClusterSize;
PDIRENTRY  pDir;
PSZ        pszLongName;

   if (wCluster < 2)
      {
      printf("fScanSubDir: Cluster < 2!\n");
      return TRUE;
      }

   wClusterSize = pDPB->wBytesPerSector * (pDPB->bSectorsPerCluster + 1);
   pDir = (PDIRENTRY)malloc(wClusterSize);
   if (!pDir)
      {
      printf("Not enough memory for Cluster in fScanSubDir");
      exit(1);
      }
   pszLongName = malloc(MAX_LONGNAME);
   if (!pszLongName)
      {
      printf("Not enough memory for LongName in fScanSubDir");
      exit(1);
      }
   memset(pszLongName, 0, MAX_LONGNAME);

   while (wCluster != 0xFFFF)
      {
      fReadCluster(bDrive, wCluster, pDir);
      fScanDir(bDrive, pDir, wClusterSize / 32, pszLongName, MAX_LONGNAME, pszDir);
      wCluster = wGetNextCluster(bDrive, wCluster);
      }

   free(pszLongName);
   free(pDir);
   return TRUE;
}

/**************************************************************
*
**************************************************************/
USHORT wGetNextCluster(BYTE bDrive, USHORT wCluster)
{
ULONG ulOffset;
ULONG ulSector;

   ulOffset = (ULONG)wCluster * 2;

   ulSector = ulOffset / pDPB->wBytesPerSector + pDPB->wBootSectorCount;
   ulOffset = ulOffset % pDPB->wBytesPerSector;

   if (ulSector != ulFatSector)
      {
      if (!fAbsRead(bDrive, ulSector, 1, pbFatSector))
         return -1;
      ulFatSector = ulSector;
      }

   return *(PUSHORT)(pbFatSector + ulOffset);
}

/**************************************************************
*
**************************************************************/
BOOL fReadCluster(BYTE bDrive, USHORT wCluster, PVOID pBuffer)
{
ULONG ulSector;

   ulSector = (ULONG)wFirstDataSector +
      (ULONG)(wCluster - 2) * (ULONG)(pDPB->bSectorsPerCluster + 1);

//   printf("Reading cluster %u at sector %lu\n",
//      wCluster, ulSector);

   if (!fAbsRead(bDrive, ulSector, (USHORT)pDPB->bSectorsPerCluster + 1, pBuffer))
      return FALSE;
   return TRUE;
}

/**************************************************************
*
**************************************************************/
VOID vDumpDirEntry(PDIRENTRY pDir)
{
INT iIndex;
BYTE szText[17];

   szText[16] = 0;
   for (iIndex = 0; iIndex < 16; iIndex++)
      {
      BYTE bChar = ((PBYTE)pDir)[iIndex];
      printf("%2.2X ", bChar);
      if (bChar > 32)
         szText[iIndex] = bChar;
      else
         szText[iIndex] = '.';
      }
   printf("%s \n", szText);

   for (iIndex = 16; iIndex < 32; iIndex++)
      {
      BYTE bChar = ((PBYTE)pDir)[iIndex];
      printf("%2.2X ", bChar);
      if (bChar > 32)
         szText[iIndex-16] = bChar;
      else
         szText[iIndex-16] = '.';
      }
   printf("%s \n", szText);
}


/**************************************************************
*
**************************************************************/
USHORT fGetRootDirSize(BYTE bDrive)
{
USHORT wSectors;
USHORT wEntriesPerSector;

   if (!pDPB)
      return 0;
   wEntriesPerSector = pDPB->wBytesPerSector / 32;
   wSectors = pDPB->wRootDirEntries / wEntriesPerSector +
      ((pDPB->wRootDirEntries % wEntriesPerSector) ? 1 : 0);

   return wSectors;
}



/**************************************************************
*
**************************************************************/
PDPB GetDPB(BYTE bDrive)
{
#ifdef DOS
union REGS r;
struct SREGS s;
PDPB pDPB;

   r.h.ah = 0x32;
   r.h.dl = (BYTE)(bDrive - '@');
   intdosx(&r, &r, &s);
   if (r.h.al == 0xFF)
      return NULL;

   FP_SEG(pDPB) = s.ds;
   FP_OFF(pDPB) = r.x.bx;
   return pDPB;
#else
APIRET rc;
BYTE   rgParms[2];
ULONG  ulParmSize;
ULONG  ulDataSize;

   rgParms[0] = 1;
   rgParms[1] = 0;
   ulParmSize = sizeof rgParms;
   ulDataSize = sizeof dpb;

   rc = DosDevIOCtl(hDisk,
      IOCTL_DISK,
      DSK_GETDEVICEPARAMS,
      rgParms, sizeof rgParms, &ulParmSize,
      (PVOID)&dpb, sizeof dpb, &ulDataSize);

   if (rc)
      {
      printf("DosDevIOCtl failed, SYS%4.4u\n", rc);
      exit(1);
      }
   dpb.bSectorsPerCluster--;

   return &dpb;

#endif
}



/**************************************************************
*
**************************************************************/
BOOL fAbsRead(BYTE bDrive, ULONG ulSector, USHORT wSectorCount, PVOID pBuffer)
{
#ifdef DOS
DISKINFO DiskInfo;
union    REGS r;
struct   SREGS s;
PBYTE    p;
ULONG ulDiskSize;

   if (!fGetDiskParms(bDrive, &DiskInfo))
      return FALSE;

   r.h.al = (BYTE)(bDrive - 'A');

   ulDiskSize = DiskInfo.bytes_per_sector * DiskInfo.sectors_per_cluster;
   ulDiskSize *= DiskInfo.total_clusters;
   if (ulDiskSize > 32L * 1024L * 1024L)
      {
      READINFO rInfo;

      memset(&rInfo, 0, sizeof rInfo);
      rInfo.ulSector = ulSector;
      rInfo.ulSectorsToRead = wSectorCount;
      rInfo.pBuffer = pBuffer;

      r.x.cx = -1;
      p = (PBYTE)&rInfo;
      }
   else
      {
      r.x.cx = (USHORT)wSectorCount;
      r.x.dx = (USHORT)ulSector;
      p = pBuffer;
      }

   s.ds   = FP_SEG(p);
   r.x.bx = FP_OFF(p);

   int86x(0x25, &r, &r, &s);
   if (r.x.cflag)
      return FALSE;
   return TRUE;
#else
APIRET rc;
ULONG ulOffset;
ULONG ulSize;
ULONG ulRead;
ULONG ulOffsetSet;

   ulOffset = ulSector * pDPB->wBytesPerSector;
   ulSize = (ULONG)wSectorCount * pDPB->wBytesPerSector;

   rc = DosSetFilePtr(hDisk,
      ulOffset,
      FILE_BEGIN,
      &ulOffsetSet);
   if (rc)
      {
      printf("DosSetFilePtr failed, SYS%4.4u\n", rc);
      exit(1);
      }


   rc = DosRead(hDisk,
      pBuffer,
      ulSize,
      &ulRead);
   if (rc)
      {
      printf("DosRead failed, SYS%4.4u\n", rc);
      exit(1);
      }
   return TRUE;


#endif
}

#ifdef DOS
/**************************************************************
*
**************************************************************/
BOOL fGetDiskParms(BYTE bDrive, PDISKINFO pDiskInfo)
{
INT rc;

   rc = _dos_getdiskfree(bDrive - '@',pDiskInfo);
   if (rc)
      return FALSE;
   return TRUE;
}
#endif

/**************************************************************
*
**************************************************************/
BOOL fGetLongName(PDIRENTRY pDir, PSZ pszName, USHORT wMax)
{
BYTE szLongName[15];
USHORT wNameSize;
USHORT usIndex;
PLNENTRY pLN = (PLNENTRY)pDir;

   memset(szLongName, 0, sizeof szLongName);
   wNameSize = 0;

   for (usIndex = 0; usIndex < 5; usIndex ++)
      {
      if (pLN->usChar1[usIndex] != 0xFFFF)
         {
         szLongName[wNameSize++] = (BYTE)pLN->usChar1[usIndex];
         if (pLN->usChar1[usIndex] > 255)
            printf("Char %u found\n", pLN->usChar1[usIndex]);
         }
      }
   for (usIndex = 0; usIndex < 6; usIndex ++)
      {
      if (pLN->usChar2[usIndex] != 0xFFFF)
         {
         szLongName[wNameSize++] = (BYTE)pLN->usChar2[usIndex];
         if (pLN->usChar2[usIndex] > 255)
            printf("Char %u found\n", pLN->usChar2[usIndex]);
         }

      }
   for (usIndex = 0; usIndex < 2; usIndex ++)
      {
      if (pLN->usChar3[usIndex] != 0xFFFF)
         {
         szLongName[wNameSize++] = (BYTE)pLN->usChar3[usIndex];
         if (pLN->usChar3[usIndex] > 255)
            printf("Char %u found\n", pLN->usChar3[usIndex]);
         }

      }
   if (strlen(pszName) + wNameSize > wMax)
      return FALSE;

   memmove(pszName + wNameSize, pszName, strlen(pszName) + 1);
   memcpy(pszName, szLongName, wNameSize);
   return TRUE;
}

BOOL fRemoveDups(PSZ pszDir)
{
FILEFINDBUF3 tFind;
HDIR hFind;
APIRET rc;
PSZ pszPath;
USHORT usMaxPath;
ULONG  ulFindCount;
PSZ    pszLongName;
USHORT usSize;

   usMaxPath = strlen(pszDir) + 64;

   pszPath = malloc(usMaxPath);
   if (!pszPath)
      {
      printf("Not enough memory in fRemoveDups!\n");
      exit(1);
      }
   strcpy(pszPath, pszDir);
   strcat(pszPath, "\\*.*");


   hFind = HDIR_CREATE;
   ulFindCount = 1;
   rc = DosFindFirst(pszPath,
      &hFind,
      FILE_ARCHIVED | FILE_DIRECTORY | FILE_SYSTEM | FILE_HIDDEN | FILE_READONLY,
      &tFind,
      sizeof tFind,
      &ulFindCount,
      1);

   while (!rc)
      {
      if (tFind.achName[0] != '.')
         {
         strcpy(pszPath, pszDir);
         strcat(pszPath, "\\");
         strcat(pszPath, tFind.achName);
         if (GetEAValue(pszPath, ".LONGNAME", &pszLongName, &usSize))
            {
            printf("%s = %-*.*s ?, ",
               tFind.achName, (INT)usSize, (INT)usSize, pszLongName);
            if (usSize == strlen(tFind.achName) &&
               !strnicmp(tFind.achName, pszLongName, usSize))
               {
               printf("removing .LONGNAME\n",
                  tFind.achName, pszLongName);
               SetEAValue(pszPath, ".LONGNAME", EA_LPASCII, "", 0);
               }
            else
               printf("\n");
            free(pszLongName);
            }
         if (tFind.attrFile & FILE_DIRECTORY)
            fRemoveDups(pszPath);
         }
      ulFindCount = 1;
      rc = DosFindNext(hFind, &tFind, sizeof tFind, &ulFindCount);
      }
   free(pszPath);
   return TRUE;
}
