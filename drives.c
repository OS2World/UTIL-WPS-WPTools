#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define INCL_DOSERRORS
#define INCL_DOS
#include <os2.h>


PFSQBUFFER2 GetFSName(PSZ pszDevice);
PSZ GetOS2Error(APIRET rc);




INT main(INT iArgc, PSZ pszArgv[])
{
BYTE szDrive[3];
ULONG ulCurDisk, ulDiskMap;
PFSQBUFFER2 pfs;
BYTE bFirst = 'C';
BYTE bLast = 'Z';

   printf("DRIVES 1.0 - Henk Kelder\n");

   if (iArgc > 1)
      bLast = bFirst = toupper(pszArgv[1][0]);


   DosError(FERR_DISABLEHARDERR | FERR_DISABLEEXCEPTION);

   strcpy(szDrive, "?:");
   DosQueryCurrentDisk(&ulCurDisk, &ulDiskMap);
   for (szDrive[0] = bFirst; szDrive[0] <= bLast; szDrive[0]++)
      {
      ULONG ulMask = 1L << (szDrive[0] - 'A');

      printf("\nDrive %s - ", szDrive);
      if (!(ulDiskMap & ulMask))
         {
         printf("not in use");
         continue;
         }

      pfs = GetFSName(szDrive);
      if (!pfs)
         continue;

      switch (pfs->iType)
         {
         case FSAT_CHARDEV:
            printf("Char device   ");
            break;
         case FSAT_PSEUDODEV:
            printf("Pseudo device ");
            break;
         case FSAT_LOCALDRV:
            printf("Local drive   ");
            break;
         case FSAT_REMOTEDRV:
            printf("Remote drive  ");
            break;
         }
      if (pfs->cbFSDName)
         printf(", %s", pfs->szName + pfs->cbName + 1);

      if (pfs->cbFSAData > 1)
         printf(", %s", pfs->szName + pfs->cbName + 1 + pfs->cbFSDName + 1);
      }


   return 0;

}
/*****************************************************************
* Get File system name
*****************************************************************/
PFSQBUFFER2 GetFSName(PSZ pszDevice)
{
ULONG ulOrdinal = 1L;
ULONG ulFSAInfoLevel = FSAIL_QUERYNAME;
ULONG ulBufferSize;
static BYTE Buffer[200];
PFSQBUFFER2 fsqBuf = (PFSQBUFFER2)Buffer;
APIRET rc;
  
   ulBufferSize = sizeof Buffer;

   rc = DosQueryFSAttach(pszDevice,
      ulOrdinal,
      ulFSAInfoLevel,
      fsqBuf,
      &ulBufferSize);
   switch (rc)
      {
      case NO_ERROR:
         break;
      default :
         printf(GetOS2Error(rc));
         return NULL;
      }

   return fsqBuf;
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
            szErrorBuf[strlen(szErrorBuf) - 2] = 0;
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


