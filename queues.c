#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>


#define INCL_SPL
#define INCL_SPLERRORS
#define INCL_SPLDOSPRINT
#define INCL_DOSERRORS
#define INCL_DOS
#include <os2.h>

INT iShowQueues(VOID);
INT iShowDevices(VOID);
INT iShowPrinters(VOID);
INT iShowDrivers(VOID);
BOOL GetQueueParms(PSZ pszQueueName);



INT main(INT iArgv, PSZ rgpszArgv[])
{
   iShowQueues();
   iShowDevices();
/*   iShowPrinters(); */
/*   iShowDrivers(); */
   return 0;
}

/*****************************************************************
* Show all Queues
*****************************************************************/
INT iShowQueues(VOID)
{
SPLERR rc;
PSZ  * prgpszOutBuf = NULL;
ULONG  ulEntryCount,
       ulTotalEntries,
       ulBytesNeeded;
ULONG  ulIndex;


   /*
      First query size of needed memory
   */

   rc = SplEnumQueue(NULL,    /* Local queues                  */
      5,                      /* Information level requested   */
      prgpszOutBuf,           /* Output buffer                 */
      0,                      /* sizeof output buffer          */
      &ulEntryCount,
      &ulTotalEntries,
      &ulBytesNeeded,
      NULL);

   if (rc && rc != ERROR_MORE_DATA && rc != NERR_BufTooSmall)
      {
      printf("SplEnumQueue: Return code %d\n", rc);
      exit(1);
      }

   prgpszOutBuf = malloc(ulBytesNeeded);
   if (!prgpszOutBuf)
      {
      printf("Not enough memory!\n");
      exit(1);
      }

   rc = SplEnumQueue(NULL,    /* Local queues                  */
      5,                      /* Information level requested   */
      prgpszOutBuf,           /* Output buffer                 */
      ulBytesNeeded,          /* sizeof output buffer          */
      &ulEntryCount,
      &ulTotalEntries,
      &ulBytesNeeded,
      NULL);

   if (rc)
      {
      printf("SplEnumQueue: Return code %d\n", rc);
      exit(1);
      }


   printf("Available printer QUEUES:\n");
   printf("=========================\n");
   for (ulIndex = 0; ulIndex < ulEntryCount; ulIndex++)
      {
      GetQueueParms(prgpszOutBuf[ulIndex]);

      printf("%d: %s\n", ulIndex + 1, prgpszOutBuf[ulIndex]);
      }
   printf("\n");



   free(prgpszOutBuf);
   return 0;
}

/*****************************************************************
* Show all Devices
*****************************************************************/
INT iShowDevices(VOID)
{
SPLERR rc;
PSZ  * prgpszOutBuf = NULL;
ULONG  ulEntryCount,
       ulTotalEntries,
       ulBytesNeeded;
ULONG  ulIndex;


   /*
      First query size of needed memory
   */

   rc = SplEnumDevice(NULL,   /* Local queues                  */
      2,                      /* Information level requested   */
      prgpszOutBuf,           /* Output buffer                 */
      0,                      /* sizeof output buffer          */
      &ulEntryCount,
      &ulTotalEntries,
      &ulBytesNeeded,
      NULL);

   if (rc && rc != ERROR_MORE_DATA && rc != NERR_BufTooSmall)
      {
      printf("SplEnumDevice: Return code %d\n", rc);
      exit(1);
      }

   prgpszOutBuf = malloc(ulBytesNeeded);
   if (!prgpszOutBuf)
      {
      printf("Not enough memory!\n");
      exit(1);
      }

   rc = SplEnumDevice(NULL,    /* Local queues                  */
      2,                      /* Information level requested   */
      prgpszOutBuf,           /* Output buffer                 */
      ulBytesNeeded,          /* sizeof output buffer          */
      &ulEntryCount,
      &ulTotalEntries,
      &ulBytesNeeded,
      NULL);

   if (rc)
      {
      printf("SplEnumDevice: Return code %d\n", rc);
      exit(1);
      }


   printf("Available printer DEVICES:\n");
   printf("=========================\n");
   for (ulIndex = 0; ulIndex < ulEntryCount; ulIndex++)
      {
      PPRDINFO3 pPrdInfo;
      printf("%d: %s\n", ulIndex + 1, prgpszOutBuf[ulIndex]);
      rc = SplQueryDevice(NULL,  prgpszOutBuf[ulIndex],
         3,
         NULL,
         0,
         &ulBytesNeeded);
      if (rc && rc != ERROR_MORE_DATA && rc != NERR_BufTooSmall)
         {
         printf("SplQueryDevice: Return code %d\n", rc);
         exit(1);
         }
      pPrdInfo = malloc(ulBytesNeeded);
      if (!pPrdInfo)
         {
         printf("Not enough memory!\n");
         exit(1);
         }
      rc = SplQueryDevice(NULL,  prgpszOutBuf[ulIndex],
         3,
         pPrdInfo,
         ulBytesNeeded,
         &ulBytesNeeded);
      printf("      %s %s %s %s\n",
         pPrdInfo->pszPrinterName,
         pPrdInfo->pszLogAddr,
         pPrdInfo->pszComment,
         pPrdInfo->pszDrivers);
      free(pPrdInfo);
      }
   printf("\n");

   free(prgpszOutBuf);
   return 0;
}


/*****************************************************************
* Show all Printers
*****************************************************************/
INT iShowPrinters(VOID)
{
SPLERR rc;
PPRINTERINFO prgPrinterInfo = NULL;
ULONG  ulEntryCount,
       ulTotalEntries,
       ulBytesNeeded;
ULONG  ulIndex;
ULONG  flType;

   flType = SPL_PR_QUEUE | SPL_PR_DIRECT_DEVICE | SPL_PR_QUEUED_DEVICE | SPL_PR_LOCAL_ONLY;


   /*
      First query size of needed memory
   */

   rc = SplEnumPrinter(NULL,   /* Local queues                  */
      0,                      /* Information level requested   */
      flType,
      prgPrinterInfo,           /* Output buffer                 */
      0,                      /* sizeof output buffer          */
      &ulEntryCount,
      &ulTotalEntries,
      &ulBytesNeeded,
      NULL);

   if (rc && rc != ERROR_MORE_DATA && rc != NERR_BufTooSmall)
      {
      printf("SplEnumPrinter: Return code %d\n", rc);
      exit(1);
      }

   prgPrinterInfo = malloc(ulBytesNeeded);
   if (!prgPrinterInfo)
      {
      printf("Not enough memory!\n");
      exit(1);
      }

   rc = SplEnumPrinter(NULL,    /* Local queues                  */
      0,                      /* Information level requested   */
      flType,
      prgPrinterInfo,           /* Output buffer                 */
      ulBytesNeeded,          /* sizeof output buffer          */
      &ulEntryCount,
      &ulTotalEntries,
      &ulBytesNeeded,
      NULL);

   if (rc)
      {
      printf("SplEnumPrinter: Return code %d\n", rc);
      exit(1);
      }


   printf("Available printer PRINTERS:\n");
   printf("=========================\n");
   for (ulIndex = 0; ulIndex < ulEntryCount; ulIndex++)
      {
      BYTE szType[256];
      memset(szType, 0, sizeof szType);
      if (prgPrinterInfo[ulIndex].flType & SPL_PR_QUEUE)
         strcat(szType, "Queue ");
      if (prgPrinterInfo[ulIndex].flType & SPL_PR_DIRECT_DEVICE)
         strcat(szType, "Direct Device ");
      if (prgPrinterInfo[ulIndex].flType & SPL_PR_QUEUED_DEVICE)
         strcat(szType, "Queued Device ");
      if (prgPrinterInfo[ulIndex].flType & SPL_PR_LOCAL_ONLY)
         strcat(szType, "Local ");

      printf("%d: Computer: %s Dest: %s Descr: %s LName: %s %s\n", ulIndex + 1,
         prgPrinterInfo[ulIndex].pszComputerName,
         prgPrinterInfo[ulIndex].pszPrintDestinationName,
         prgPrinterInfo[ulIndex].pszDescription,
         prgPrinterInfo[ulIndex].pszLocalName,
         szType);
      }
   printf("\n");

   free(prgPrinterInfo);
   return 0;
}

/*****************************************************************
* Show all Drivers
*****************************************************************/
INT iShowDrivers(VOID)
{
SPLERR rc;
PPRDRIVINFO prgDriverInfo = NULL;
ULONG  ulEntryCount,
       ulTotalEntries,
       ulBytesNeeded;
ULONG  ulIndex;


   /*
      First query size of needed memory
   */

   rc = SplEnumDriver(NULL,   /* Local queues                  */
      0,                      /* Information level requested   */
      prgDriverInfo,           /* Output buffer                 */
      0,                      /* sizeof output buffer          */
      &ulEntryCount,
      &ulTotalEntries,
      &ulBytesNeeded,
      NULL);

   if (rc && rc != ERROR_MORE_DATA && rc != NERR_BufTooSmall)
      {
      printf("SplEnumDriver: Return code %d\n", rc);
      exit(1);
      }

   prgDriverInfo = malloc(ulBytesNeeded);
   if (!prgDriverInfo)
      {
      printf("Not enough memory!\n");
      exit(1);
      }

   rc = SplEnumDriver(NULL,    /* Local queues                  */
      0,                      /* Information level requested   */
      prgDriverInfo,           /* Output buffer                 */
      ulBytesNeeded,          /* sizeof output buffer          */
      &ulEntryCount,
      &ulTotalEntries,
      &ulBytesNeeded,
      NULL);

   if (rc)
      {
      printf("SplEnumDriver: Return code %d\n", rc);
      exit(1);
      }


   printf("Available printer DRIVERS:\n");
   printf("=========================\n");
   for (ulIndex = 0; ulIndex < ulEntryCount; ulIndex++)
      {
      printf("%d: %s\n", ulIndex + 1,
         prgDriverInfo[ulIndex].szDrivName);
      }
   printf("\n");

   free(prgDriverInfo);
   return 0;
}

/*********************************************************************
* 
*********************************************************************/
BOOL GetQueueParms(PSZ pszQueueName)
{
APIRET rc;
ULONG ulBufSize;
PPRQINFO6 prq6;
INT iHandle;
BYTE szFileName[256];


   rc = SplQueryQueue(NULL, pszQueueName,
      3L,
      NULL,
      0L,
      &ulBufSize);

   if (rc && rc != ERROR_MORE_DATA && rc != NERR_BufTooSmall)
      return FALSE;

   prq6 = (PPRQINFO6)malloc(ulBufSize);
   if (!prq6)
      return FALSE;

   rc = SplQueryQueue(NULL, pszQueueName,
      3L,
      prq6,
      ulBufSize,
      &ulBufSize);
   if (rc)
      {
      printf("SplQueryQueue returned %d\n", rc);
      free(prq6);
      return FALSE;
      }

   strcpy(szFileName, pszQueueName);
   strcat(szFileName, ".JPR");

#ifdef WRITE
   iHandle = open(szFileName,
      O_BINARY|O_CREAT|O_TRUNC|O_WRONLY, S_IWRITE);
   if (iHandle == -1)
      {
      printf("Cannot open file!\n");
      return FALSE;
      }
   write(iHandle, prq6->pDriverData, prq6->pDriverData->cb);
   close(iHandle);
#endif

   free(prq6);
   return TRUE;
}



