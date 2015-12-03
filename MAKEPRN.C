#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <share.h>
#include <fcntl.h>

#define INCL_WIN
#define INCL_SPL
#define INCL_SPLDOSPRINT
#define INCL_BASE
#define INCL_DOSMEMMGR
#define INCL_SPLERRORS
#include <os2.h>
#include "wptools.h"

BOOL   MakeDriver(PSZ pszDriverName, PSZ pszDriverFile);
SPLERR MakeDevice(PSZ pszName, PSZ pszPort, PSZ pszDriver, PSZ pszPrinter);
SPLERR MakeQueue(PSZ pszName, PSZ pszDriver, PSZ pszPrinter);
BOOL   IsPrinterInDriver(PSZ pszDriverFile, PSZ pszDriverName);
PSZ GetOS2Error(APIRET rc);

PDRIVDATA ReadDriverData(VOID);

BYTE szProgDir[512];
BYTE szDriverName[512];
BYTE szJPRFile[521];

/****************************************************************
*
****************************************************************/
INT main(INT iArgc, PSZ rgpszArgv[])
{
APIRET rc;
PSZ p;
PSZ pszQueueName,
    pszPortName,
    pszPrinterName,
    pszDriverFile;
HMODULE hMod;

   if (iArgc < 5)
      {
      printf("USAGE MAKEPRN Name Port PrinterName DriverFileName [file.JPR]");
      exit(1);
      }
   pszQueueName  = rgpszArgv[1];
   pszPortName   = rgpszArgv[2];
   pszPrinterName = rgpszArgv[3];
   pszDriverFile = rgpszArgv[4];
   if (iArgc == 6)
      strcpy(szJPRFile, rgpszArgv[5]);
   else
      strcpy(szJPRFile, pszPrinterName);

   p = strrchr(pszDriverFile, '\\');
   if (!p)
      {
      printf("Error: Missing \\ in DriverFileName\n");
      exit(1);
      }
   strcpy(szDriverName, p + 1);
   p = strchr(szDriverName, '.');
   if (!p)
      {
      printf("Error: missing . in DriverFileName\n");
      exit(1);
      }
   p++;
   *p = 0;
   strupr(szDriverName);
   strcat(szDriverName, pszPrinterName);

   strcpy(szProgDir, rgpszArgv[0]);

   rc = DosQueryModuleHandle("MAKEPRN.EXE", &hMod);
   if (!rc)
      {
      rc = DosQueryModuleName(hMod, sizeof szProgDir, szProgDir);
      if (rc)
         {
         printf("DosQueryModuleName:\n%s",
            GetOS2Error(rc));
         }
      }
   else
      {
      printf("DosQueryModuleHandle: SYS%4.4u\n%s",
         rc, GetOS2Error(rc));
      }

   p = strrchr(szProgDir, '\\');
   if (p)
      {
      p++;
      *p = 0;
      }
   else
      {
      p = strchr(szProgDir, ':');
      if (p)
         {
         p++;
         *p = 0;
         }
      else
         memset(szProgDir, 0, sizeof szProgDir);
      }




   if (!IsPrinterInDriver(pszDriverFile, pszPrinterName))
      exit(1);

   if (!MakeDriver(szDriverName, pszDriverFile))
      exit(1);

   if (MakeDevice(pszQueueName, pszPortName, szDriverName, pszPrinterName))
      exit(1);

   MakeQueue(pszQueueName, szDriverName, pszPrinterName);
   return 0;
}

/****************************************************************
*
****************************************************************/
BOOL IsPrinterInDriver(PSZ pszDriverFile, PSZ pszDriverName)
{
PSZ pBuffer;
USHORT usSize;
PSZ p;

   if (access(pszDriverFile, 0))
      {
      printf("%s cannot be accessed\n", pszDriverFile);
      return FALSE;
      }

   if (!GetEAValue(pszDriverFile, ".EXPAND",
      &pBuffer, &usSize))
      {
      printf(".EXPAND extended attribute not found!\n");
      return FALSE;
      }
   p = pBuffer;
   while (*p)
      {
      if (!stricmp(p, pszDriverName))
         {
         strcpy(pszDriverName, p);
         free(pBuffer);
         return TRUE;
         }

      p = p + strlen(p) + 1;
      }
   free(pBuffer);
   printf("%s is not supported by %s\n",
      pszDriverName, pszDriverFile);
   return FALSE;


}

/****************************************************************
*
****************************************************************/
BOOL MakeDriver(PSZ pszDriverName, PSZ pszDriverFile)
{
BYTE szData[100];
PSZ  p;

   /*
      Schrijf PM_DEVICE_DRIVERS, drivername, driverfilepath
      in OS2.INI
   */
   strcpy(szData, pszDriverName);
   p = strchr(szData, '.');
   if (p)
      *p = 0;

   PrfWriteProfileData(HINI_USERPROFILE,
      "PM_DEVICE_DRIVERS", szData,
      pszDriverFile, strlen(pszDriverFile) + 1);

   /*
      Schrijf PM_SPOOLER_DD, drivername.printername, driverfile;;;
      in OS2SYS.INI
   */

   p = strrchr(pszDriverFile, '\\');
   if (!p)
      p = pszDriverFile;
   else
      p++;
   strcpy(szData, p);
   strcat(szData, ";;;");
   strupr(szData);

   PrfWriteProfileData(HINI_SYSTEMPROFILE,
      "PM_SPOOLER_DD",
      pszDriverName, 
      szData, strlen(szData) + 1);

   return TRUE;
}

/****************************************************************
*
****************************************************************/
SPLERR MakeDevice(PSZ pszName, PSZ pszPort, PSZ pszDriver, PSZ pszPrinter)
{
SPLERR rc;
PRDINFO3 PrdInfo;


   memset(&PrdInfo, 0, sizeof PrdInfo);
   PrdInfo.pszPrinterName = pszName;
   PrdInfo.pszLogAddr = pszPort;
   PrdInfo.pszComment = pszPrinter;
   PrdInfo.pszDrivers = pszDriver;
   PrdInfo.usTimeOut = 45;

   rc = SplCreateDevice(NULL,
      3,
      &PrdInfo,
      sizeof PrdInfo);
   if (rc == NERR_DestExists)
      {
//      printf("Device %s already exists, changing settings!\n",
//         pszName);

      rc = SplSetDevice(NULL, pszName,
         3,
         &PrdInfo,
         sizeof PrdInfo,
         0);
      }
   if (rc)
      printf("SplCreateDevice:\n%s", GetOS2Error(rc));
   return rc;
}
/****************************************************************
*
****************************************************************/
SPLERR MakeQueue(PSZ pszName, PSZ pszDriver, PSZ pszPrinter)
{
PRQINFO3 PrqInfo;
SPLERR rc;
#if 0

ULONG ulBytesNeeded;

   rc = SplQueryQueue(NULL, pszName,
      3,
      NULL,
      0,
      &ulBytesNeeded);

   if (!rc || rc == ERROR_MORE_DATA || rc == NERR_BufTooSmall)
      rc = SplDeleteQueue(NULL, pszName);

#endif

   memset(&PrqInfo, 0, sizeof PrqInfo);
   PrqInfo.pszName         = pszName;
   PrqInfo.uPriority       = PRQ_DEF_PRIORITY;
   PrqInfo.pszPrProc       = "PMPRINT";
   PrqInfo.pszComment      = "Flexplek Printer";
   PrqInfo.pszPrinters     = pszName;
   PrqInfo.pszDriverName   = pszDriver;
   PrqInfo.pDriverData     = ReadDriverData();

   rc = SplCreateQueue(NULL, 3,
      &PrqInfo,
      sizeof PrqInfo);
   if (rc == NERR_QExists)
      {
//      printf("Queue %s already exists, changing settings!\n",
//         pszName);

      rc = SplSetQueue(NULL, pszName,
         3,
         &PrqInfo,
         sizeof PrqInfo,
         0);
      }
   if (rc)
      printf("SplCreateQueue:\n%s", GetOS2Error(rc));
   return rc;
}

/*********************************************************************
*
*********************************************************************/
PDRIVDATA ReadDriverData()
{
PDRIVDATA pDriv;
BYTE szFileName[521];
INT iHandle;
USHORT usFileSize;

   if (access(szJPRFile, 0))
      {
      strcpy(szFileName, szProgDir);
      strcat(szFileName, szJPRFile);
      }
   else
      strcpy(szFileName, szJPRFile);

   if (access(szFileName, 0))
      {
      printf("Driverdata file '%s' not found,\n", szFileName);
      printf("No jobproperties will be set!\n"); 
      return NULL;
      }

   iHandle = sopen(szFileName, O_RDONLY|O_BINARY, SH_DENYWR);
   if (iHandle == -1)
      {
      printf("Error: Cannot open %s\n", szFileName);
      return NULL;
      }
   usFileSize = (USHORT)filelength(iHandle);
   pDriv = malloc(usFileSize);
   if (!pDriv)
      {
      printf("Error: not enough memory %s\n", szFileName);
      close(iHandle);
      return NULL;
      }
   read(iHandle, pDriv, usFileSize);
   close(iHandle);
   return pDriv;
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

