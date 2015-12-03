#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <direct.h>

#define INCL_VIO
#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_GPI
#define INCL_SPL
#define INCL_SPLERRORS
#define INCL_SPLDOSPRINT
#include <os2.h>

#include "wptools.h"


PRIVATE PPRQINFO6 GetQueueParms(PSZ pszQueueName);
PRIVATE PPRDINFO3 GetDeviceParms(PSZ pszDeviceName);


/*********************************************************************
* 
*********************************************************************/
BOOL   _System GetPrintDestOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
BYTE szQueueName[128];
BYTE szRemoveMachine[256];
ULONG ulFlags;
BOOL fRemote;
PPRQINFO6 prq6;
PPRDINFO3 prd3;

   fRemote = FALSE;
   if (GetObjectValue(pObjectData, IDKEY_PRNCOMPUTER, szRemoveMachine, sizeof szRemoveMachine))
      fRemote = TRUE;

   if (!GetObjectValue(pObjectData, IDKEY_PRNQUEUENAME, szQueueName, sizeof szQueueName))
      {
      if (fRemote)
         {
         if (!GetObjectValue(pObjectData, IDKEY_PRNREMQUEUE, szQueueName, sizeof szQueueName))
            return TRUE;
         }
      else
         return TRUE;
      }



   sprintf(pszOptions + strlen(pszOptions),
      "QUEUENAME=%s;", szQueueName);

   if (GetObjectValue(pObjectData, IDKEY_PRNJOBDIALOG, &ulFlags, sizeof ulFlags))
      {
      if (ulFlags)
         strcat(pszOptions, "JOBDIALOGBEFOREPRINT=YES;");
      else
         strcat(pszOptions, "JOBDIALOGBEFOREPRINT=NO;");
      }

   prq6 = GetQueueParms(szQueueName);
   if (!prq6)
      return TRUE;

   if (prq6->fsType & PRQ3_TYPE_APPDEFAULT)
      strcat(pszOptions, "APPDEFAULT=YES;");
   else
      strcat(pszOptions, "APPDEFAULT=NO;");

   if (prq6->fsType & PRQ3_TYPE_RAW)
      strcat(pszOptions, "PRINTERSPECIFICFORMAT=YES;");
   else
      strcat(pszOptions, "PRINTERSPECIFICFORMAT=NO;");

   if (prq6->fsType & PRQ3_TYPE_BYPASS)
      strcat(pszOptions, "PRINTWHILESPOOLING=YES;");
   else
      strcat(pszOptions, "PRINTWHILESPOOLING=NO;");

   sprintf(pszOptions + strlen(pszOptions),
      "QSTARTTIME=%2.2d:%2.2d;",
      prq6->uStartTime / 60,
      prq6->uStartTime % 60);

   sprintf(pszOptions + strlen(pszOptions),
      "QSTOPTIME=%2.2d:%2.2d;",
      prq6->uUntilTime / 60,
      prq6->uUntilTime % 60);

   if (prq6->pszSepFile && strlen(prq6->pszSepFile))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "SEPARATORFILE=%s;", prq6->pszSepFile);
      }

   if (prq6->pszPrProc && strlen(prq6->pszPrProc))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "QUEUEDRIVER=%s;", prq6->pszPrProc);
      }

   if (prq6->pszDriverName && strlen(prq6->pszDriverName))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "PRINTDRIVER=%s;", prq6->pszDriverName);
      }

   sprintf(pszOptions + strlen(pszOptions),
      "PRIORITY=%d;", prq6->uPriority);

   if (prq6->pszPrinters && strlen(prq6->pszPrinters))
      {
      prd3 = GetDeviceParms(prq6->pszPrinters);
      if (prd3)
         {
         if (prd3->pszLogAddr && strlen(prd3->pszLogAddr))
            {
            if (!stricmp(prd3->pszLogAddr, "FILE"))
               strcat(pszOptions, "OUTPUTTOFILE=YES;");
            else
               {
               strcat(pszOptions, "OUTPUTTOFILE=NO;");
               sprintf(pszOptions + strlen(pszOptions),
                  "PORTNAME=%s;", prd3->pszLogAddr);
               }
            }
         }
      }


   free(prq6);
   return TRUE;
}

/*********************************************************************
* 
*********************************************************************/
PPRQINFO6 GetQueueParms(PSZ pszQueueName)
{
APIRET rc;
ULONG ulBufSize;
PPRQINFO6 prq6;


   rc = SplQueryQueue(NULL, pszQueueName,
      3L,
      NULL,
      0L,
      &ulBufSize);

   if (rc && rc != ERROR_MORE_DATA && rc != NERR_BufTooSmall)
      return NULL;

   prq6 = (PPRQINFO6)malloc(ulBufSize);
   if (!prq6)
      return NULL;

   rc = SplQueryQueue(NULL, pszQueueName,
      3L,
      prq6,
      ulBufSize,
      &ulBufSize);

   if (rc)
      {
      MessageBox("WPTOOLS", "SplQueryQueue returned %d", rc);
      free(prq6);
      return NULL;
      }
   return prq6;
}

/*********************************************************************
* 
*********************************************************************/
PPRDINFO3 GetDeviceParms(PSZ pszDeviceName)
{
APIRET rc;
ULONG ulBufSize;
PPRDINFO3 prd3;


   rc = SplQueryDevice(NULL, pszDeviceName,
      3L,
      NULL,
      0L,
      &ulBufSize);

   if (rc && rc != ERROR_MORE_DATA && rc != NERR_BufTooSmall)
      return NULL;

   prd3 = (PPRDINFO3)malloc(ulBufSize);
   if (!prd3)
      return NULL;

   rc = SplQueryDevice(NULL, pszDeviceName,
      3L,
      prd3,
      ulBufSize,
      &ulBufSize);

   if (rc)
      {
      MessageBox("WPTOOLS", "SplQueryDevice returned %d", rc);
      free(prd3);
      return NULL;
      }
   return prd3;
}


