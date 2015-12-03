#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <stdarg.h>
#include <share.h>

#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#define INCL_WPERRORS
#define INCL_PM
#include <os2.h>

#include "display.h"
#include "msg.h"
#include "box.h"
#include "language.h"
#include "mouse.h"
#include "files.h"
#include "keyb.h"
#include "wptools.h"

#pragma pack(1)

typedef struct _DrivDev2
{
BYTE  chName[4];  /* = 'DRIV' */
ULONG rgLong[4];
BYTE  szName[1];
} DRIV1, *PDRIV1;



typedef struct _NodeDev2
{
BYTE   chName[4];  /* = 'NODE' */
USHORT usLevel;
USHORT usID;
USHORT usParentID;
USHORT usNul;
ULONG  rgLong[4];
USHORT fIsDir;   
USHORT usNameSize;
BYTE   szName[1];
} NODE1, *PNODE1;
#pragma pack()


#define MROW      8

static BOOL _System ReadAllBlocks(HINI hiniSystem, PSZ pszActiveHandles, PBYTE * ppBlock, PULONG pulSize);
static VOID CheckLocation(VOID);
static VOID CheckFolderPos(VOID);
static VOID fProcessLogTxt(VOID);

static VOID CheckObjectHandles(VOID);
static PSZ pszWPError(VOID);
static PSZ vDumpHex(PSZ psz, USHORT usSize);

static BYTE rgUsed[65535];
static BYTE szHandles[100];

HINI hIniSystem;
HINI hIniUser;

INT main(INT iArgc, PSZ rgArgv[])
{
BYTE szSystem[CCHMAXPATH];
BYTE szUser[CCHMAXPATH];

   if (iArgc < 2)
      {
      printf("USAGE: FIXINI directory\n");
      exit(1);
      }
   strcpy(szSystem, rgArgv[1]);
   strcat(szSystem, "\\OS2SYS.INI");

   strcpy(szUser, rgArgv[1]);
   strcat(szUser, "\\OS2.INI");


   hIniUser = PrfOpenProfile(0, szUser);
   if (!hIniUser)
      {
      printf("Unable to open %s\n%s", szUser, pszWPError());
      exit(1);
      }

   hIniSystem = PrfOpenProfile(0, szSystem);
   if (!hIniSystem)
      {
      printf("Unable to open %s\n%s", szSystem, pszWPError());
      exit(1);
      }

   fProcessLogTxt();

   CheckFolderPos();
   CheckLocation();

   GetActiveHandles(hIniSystem, szHandles, sizeof szHandles);
   CheckObjectHandles();

   PrfCloseProfile(hIniSystem);
   PrfCloseProfile(hIniUser);
   return 0;
}

VOID CheckLocation(VOID)
{
PBYTE pLocationArray;
PBYTE pLocation;
ULONG ulProfileSize;

   pLocationArray = GetAllProfileNames(LOCATION, hIniUser, &ulProfileSize);
   if (!pLocationArray)
      return ;

   pLocation = pLocationArray;
   while (*pLocation)
      {
      PULONG pulLocation = (PULONG)GetProfileData(LOCATION, pLocation, hIniUser, &ulProfileSize);
      if (pulLocation && ulProfileSize == 4)
         {
         HOBJECT hObject = (HOBJECT)*pulLocation;
         free(pulLocation);
         if (HIUSHORT(hObject) >= OBJECT_FSYS)
            {
            if (!rgUsed[LOUSHORT(hObject)])
               rgUsed[LOUSHORT(hObject)] = (BYTE)HIUSHORT(hObject) + '0';
            else if (rgUsed[LOUSHORT(hObject)] != (BYTE)HIUSHORT(hObject) + '0')
               rgUsed[LOUSHORT(hObject)] = 'B';
            }
         }
      if (pulLocation)
         free(pulLocation);
      pLocation += strlen(pLocation) + 1;
      }
   return;
}

VOID CheckFolderPos(VOID)
{
PBYTE pFolderposArray;
PBYTE pFolderpos;
ULONG ulProfileSize;

   pFolderposArray = GetAllProfileNames(FOLDERPOS, hIniUser, &ulProfileSize);
   if (!pFolderposArray)
      return ;

   pFolderpos = pFolderposArray;
   while (*pFolderpos)
      {
      HOBJECT hObject = atol(pFolderpos);

      if (HIUSHORT(hObject) >= OBJECT_FSYS)
         {
         if (!rgUsed[LOUSHORT(hObject)])
            rgUsed[LOUSHORT(hObject)] = (BYTE)HIUSHORT(hObject) + '0';
         else if (rgUsed[LOUSHORT(hObject)] != (BYTE)HIUSHORT(hObject) + '0')
            rgUsed[LOUSHORT(hObject)] = 'B';
         }
      pFolderpos += strlen(pFolderpos) + 1;
      }
   return;
}


/*********************************************************************
* Check physical object handles
*********************************************************************/
VOID CheckObjectHandles()
{
PBYTE pBuffer;
ULONG ulProfileSize;
PDRIV1 pDriv;
PNODE1 pNode;
PBYTE p;
ULONG ulCount = 0;
ULONG ul3Count = 0, ul4Count = 0;
ULONG ulIndex;


   /*
      Get all objectshandles
   */
   if (!ReadAllBlocks(hIniSystem, szHandles, &pBuffer, &ulProfileSize))
      {
      printf("%s not found in profile!\n", szHandles);
      return;
      }

   printf("FIRST 4 BYTES %s\n", vDumpHex(pBuffer, 4));

   p = pBuffer + 4;
   while (p < pBuffer + ulProfileSize)
      {
      if (!memicmp(p, "DRIV", 4))
         {
         pDriv = (PDRIV1)p;
         printf("%p : DRIV                  %8.8X %8.8X %8.8X %8.8X %s\n",
            p,
            pDriv->rgLong[0],
            pDriv->rgLong[1],
            pDriv->rgLong[2],
            pDriv->rgLong[3],
            pDriv->szName);
         p += sizeof(DRIV1) + strlen(pDriv->szName);
         }
      else if (!memicmp(p, "NODE", 4))
         {
         ulCount++;
         pNode = (PNODE1)p;
         if (!rgUsed[pNode->usID])
            rgUsed[pNode->usID] = '3';

         switch (rgUsed[pNode->usID])
            {
            case '3':
               ul3Count++;
               break;
            case '4':
               ul4Count++;
               break;
            case 'B':
               ul3Count++;
               ul4Count++;
               break;
            }

         printf("%p : NODE: %s %c%4.4X %4.4X %u %u %8.8X %8.8X %8.8X %8.8X %s\n",
            p,
            (pNode->fIsDir ? "DIR :" : "FILE:"),
            rgUsed[pNode->usID],
            pNode->usID,
            pNode->usParentID,
            pNode->usLevel,
            pNode->usNul,
            pNode->rgLong[0],
            pNode->rgLong[1],
            pNode->rgLong[2],
            pNode->rgLong[3],
            pNode->szName);
         if (pNode->usNameSize != strlen(pNode->szName))
            {
            printf("ERROR : Additional data after this name!\n");
            }
         rgUsed[pNode->usID] = 0;

         p += sizeof (NODE1) + pNode->usNameSize;
         }
      else
         {
         printf("%s:%s appears to be corrupted\n", szHandles, HANDLEBLOCK);
         return;
         }
      }

   printf("%u handles found\n", ulCount);
   printf("%u 3handles found\n", ul3Count);
   printf("%u 4handles found\n", ul4Count);

   free(pBuffer);
   for (ulIndex = 0; ulIndex < 0x10000; ulIndex++)
      {
      if (rgUsed[ulIndex])
         printf("NFT: %c%4.4X\n",
            rgUsed[ulIndex], ulIndex);
      }
}


PSZ pszWPError(VOID)
{
static BYTE szMessage[1024];
PERRINFO pErrInfo;
BYTE   szHErrNo[6];
USHORT usError;

   usError = ERRORIDERROR(WinGetLastError(0));
   memset(szMessage, 0, sizeof szMessage);
   sprintf(szMessage, "Error 0x%4.4X : ", usError);

   switch (usError)
      {
      case WPERR_PROTECTED_CLASS        :
         strcat(szMessage, "Objects class is protected");
         break;
      case WPERR_NOT_WORKPLACE_CLASS    :
      case WPERR_INVALID_CLASS          :
         strcat(szMessage, "Objects class is invalid");
         break;
      case WPERR_INVALID_SUPERCLASS     :
         strcat(szMessage, "Objects superclass is invalid");
         break;
      case WPERR_INVALID_OBJECT         :
         strcat(szMessage, "The object appears to be invalid");
         break;
      case WPERR_INI_FILE_WRITE         :
         strcat(szMessage, "An error occured when writing to the ini-files");
         break;
      case WPERR_INVALID_FOLDER         :
         strcat(szMessage, "The specified folder (location) is not valid");
         break;
      case WPERR_OBJECT_NOT_FOUND       :
         strcat(szMessage, "The object was not found");
         break;

      case WPERR_ALREADY_EXISTS         :
         strcat(szMessage,
            "The workplace shell claims that the object already exist.\nUse CHECKINI to check for possible causes!");
         break;
      case WPERR_INVALID_FLAGS          :
         strcat(szMessage, "Invalid flags were specified");
         break;
      case WPERR_INVALID_OBJECTID       :
         strcat(szMessage, "The OBJECTID is invalid");
         break;
      case PMERR_INVALID_PARAMETER:
         strcat(szMessage, "A parameter is invalid (invalid class name?)");
         break;
      default :
         memset(szMessage, 0, sizeof szMessage);
         pErrInfo = WinGetErrorInfo(0);
         if (pErrInfo)
            {
            PUSHORT pus = (PUSHORT)((PBYTE)pErrInfo + pErrInfo->offaoffszMsg);
            PSZ     psz = (PBYTE)pErrInfo + *pus;
            PSZ     p;

            strcat(szMessage, psz);
            p = psz + strlen(psz) + 1;
            strcat(szMessage, ", ");
            strcat(szMessage, p);

            itoa(ERRORIDERROR(pErrInfo->idError), szHErrNo, 16);
            strcat(szMessage, "\n");
            strcat(szMessage, "Error No = ");
            strcat(szMessage, "0x");
            strcat(szMessage, szHErrNo);
            WinFreeErrorInfo(pErrInfo);
            }
         else
            sprintf(szMessage, "PM returned error %d (0x%4.4X)\n",
               usError, usError);
            break;
         }



   return szMessage;
}

PSZ vDumpHex(PSZ psz, USHORT usSize)
{
static BYTE szText[512];

   memset(szText, 0, sizeof szText);

   while (usSize)
      {
      sprintf(szText + strlen(szText), "%2.2X ", *psz);
      psz++;
      usSize--;
      }
   return szText;
}


BOOL _System ReadAllBlocks(HINI hiniSystem, PSZ pszActiveHandles, PBYTE * ppBlock, PULONG pulSize)
{
PBYTE pbAllBlocks,
      pszBlockName,
      p;
ULONG ulBlockSize;
ULONG ulTotalSize;
BYTE  rgfBlock[100];
BYTE  szAppName[10];
USHORT usBlockNo;


   pbAllBlocks = GetAllProfileNames(pszActiveHandles, hiniSystem, &ulBlockSize);
   if (!pbAllBlocks)
      return FALSE;

   /*
      Query size of all individual blocks and calculate total
   */
   memset(rgfBlock, 0, sizeof rgfBlock);
   ulTotalSize = 0L;
   pszBlockName = pbAllBlocks;
   while (*pszBlockName)
      {
      if (!memicmp(pszBlockName, "BLOCK", 5))
         {
         usBlockNo = atoi(pszBlockName + 5);
         if (usBlockNo < 100)
            {
            rgfBlock[usBlockNo] = TRUE;

            if (!PrfQueryProfileSize(hiniSystem,
               pszActiveHandles,
               pszBlockName,
               &ulBlockSize))
               {
               free(pbAllBlocks);
               return FALSE;
               }
            ulTotalSize += ulBlockSize;
            }
         }
      pszBlockName += strlen(pszBlockName) + 1;
      }

   /*
      *pulSize contains total size of all blocks
   */
   *ppBlock = malloc(ulTotalSize);
   if (!(*ppBlock))
      {
      MessageBox("fReadAllBlocks", "Not enough memory for profile data!");
         free(pbAllBlocks);
      return FALSE;
      }

   /*
      Now get all blocks
   */

   p = *ppBlock;
   (*pulSize) = 0;
   for (usBlockNo = 1; usBlockNo < 100; usBlockNo++)
      {
      if (!rgfBlock[usBlockNo])
         break;

      sprintf(szAppName, "BLOCK%u", (UINT)usBlockNo);
      ulBlockSize = ulTotalSize;
      if (!PrfQueryProfileData(hiniSystem,
         pszActiveHandles,
         szAppName,
         p,
         &ulBlockSize))
         {
         free(pbAllBlocks);
         free(*ppBlock);
         return FALSE;
         }
      p += ulBlockSize;
      (*pulSize) += ulBlockSize;
      ulTotalSize -= ulBlockSize;
      }

   free(pbAllBlocks);
   return TRUE;
}

VOID fProcessLogTxt(VOID)
{
FILE *fp;
static BYTE szLine[1024];

   fp = fopen("LOG1.TXT", "r");
   if (!fp)
      return;
   while (fgets(szLine, sizeof szLine, fp))
      {
      if (strstr(szLine, "HAS MULTIPLE"))
         {
         PSZ p;
         HOBJECT hObject = strtol(szLine, &p, 16);
            if (!rgUsed[LOUSHORT(hObject)])
               rgUsed[LOUSHORT(hObject)] = '4';
            else if (rgUsed[LOUSHORT(hObject)] != '4')
               rgUsed[LOUSHORT(hObject)] = 'B';
         }
      }
   fclose(fp);
}
