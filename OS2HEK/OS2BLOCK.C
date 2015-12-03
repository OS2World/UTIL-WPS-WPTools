#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#include <locale.h>

#define INCL_WINSHELLDATA
#include <os2.h>
#include "os2hek.h"

static ULONG ulHandlesSize = 0L;
static PBYTE pHandlesBuffer = NULL;
static BYTE  szActiveHandles[100];
static BYTE  szCurHandles[100];

static BOOL  fNewFormat;


static USHORT _System GetFSObjectID(PBYTE pHandlesBuffer, ULONG ulBufSize, USHORT usParent, PSZ pszFname);
BOOL _System fReadAllBlocks(HINI hiniSystem, PSZ pszActiveHandles, PBYTE * ppBlock, PULONG pulSize);

/*************************************************************
* Reset the block buffer, make sure the buffer is re-read.
*************************************************************/
VOID _System ResetBlockBuffer(VOID)
{
   if (pHandlesBuffer)
      {
      free(pHandlesBuffer);
      pHandlesBuffer = NULL;
      }
}

/*************************************************************
* Get active handles
*************************************************************/
BOOL _System GetActiveHandles(HINI hIniSystem, PSZ pszHandlesAppName, USHORT usMax)
{
PBYTE pszHandles;
ULONG ulProfileSize;

   pszHandles = GetProfileData(ACTIVEHANDLES, HANDLESAPP,
      hIniSystem,&ulProfileSize);
   if (!pszHandles)
      {
      strncpy(pszHandlesAppName, HANDLES, usMax - 1);
      return TRUE;
      }
   fNewFormat = TRUE;
   strncpy(pszHandlesAppName, pszHandles, usMax -1);
   free(pszHandles);
   return TRUE;
}

/*************************************************************
* Get the object name as specified in hObject
*************************************************************/
PNODE _System PathFromObject(HINI hIniSystem, HOBJECT hObject, PSZ pszFname, USHORT usMax)
{
USHORT usObjID;

   GetActiveHandles(hIniSystem, szCurHandles, sizeof szCurHandles);
   if (strcmp(szCurHandles, szActiveHandles))
      {
      if (pHandlesBuffer)
         free(pHandlesBuffer);
      pHandlesBuffer = NULL;
      strcpy(szActiveHandles, szCurHandles);
      }

   usObjID   = LOUSHORT(hObject);

   if (!IsObjectDisk(hObject))
      return NULL;

   if (!pHandlesBuffer)
      if (!fReadAllBlocks(hIniSystem, szActiveHandles, &pHandlesBuffer, &ulHandlesSize))
         return NULL;

   memset(pszFname, 0, usMax);

   return GetPartName(pHandlesBuffer, ulHandlesSize, usObjID, pszFname, usMax);
}

/*************************************************************
* Get the object partname as specified in usID
*************************************************************/
PNODE _System GetPartName(PBYTE pHandlesBuffer, ULONG ulBufSize, USHORT usID, PSZ pszFname, USHORT usMax)
{
PDRIV pDriv;
PNODE pNode;
PBYTE p, pEnd;
USHORT usSize;

   pEnd = pHandlesBuffer + ulBufSize;
   p = pHandlesBuffer + 4;
   while (p < pEnd)
      {
      if (!memicmp(p, "DRIV", 4))
         {
         pDriv = (PDRIV)p;
         p += sizeof(DRIV) + strlen(pDriv->szName);
         }
      else if (!memicmp(p, "NODE", 4))
         {
         pNode = (PNODE)p;
         p += sizeof (NODE) + pNode->usNameSize;
         if (pNode->usID == usID)
            {
            usSize = usMax - strlen(pszFname);
            if (usSize > pNode->usNameSize)
               usSize = pNode->usNameSize;
            if (pNode->usParentID)
               {
               if (!GetPartName(pHandlesBuffer, ulBufSize, pNode->usParentID, pszFname, usMax))
                  return FALSE;
               strcat(pszFname, "\\");
               strncat(pszFname, pNode->szName, usSize);
               return pNode;
               }
            else
               {
               strncpy(pszFname, pNode->szName, usSize);
               return pNode;
               }
            }
         }
      else
         return NULL;
      }
   return NULL;
}

/*************************************************************
* Get the object name as specified in hObject
*************************************************************/
HOBJECT _System MyQueryObjectID(HINI hIniUser, HINI hIniSystem, PSZ pszName)
{
USHORT usObjID;
PVOID  pvData;
ULONG  ulProfileSize;
HOBJECT hObject = 0L;
BYTE    szFullPath[300];

   /*
      Try to get the objectID via PM_Workplace:Location
   */
   pvData = GetProfileData(LOCATION, pszName, hIniUser, &ulProfileSize);
   if (pvData)
      {
      if (ulProfileSize >= sizeof (HOBJECT))
         hObject = *(PULONG)pvData;
      free(pvData);
      }
   if (hObject)
      return hObject;

   /*
      Does is contain an existing pathname ?
   */
   if (access(pszName, 0))
      return hObject;
   _fullpath(szFullPath, pszName, sizeof szFullPath);

   /*
      Check if the HandlesBlock is valid
   */
   GetActiveHandles(hIniSystem, szCurHandles, sizeof szCurHandles);
   if (strcmp(szCurHandles, szActiveHandles))
      {
      if (pHandlesBuffer)
         free(pHandlesBuffer);
      pHandlesBuffer = NULL;
      strcpy(szActiveHandles, szCurHandles);
      }

   if (!pHandlesBuffer)
      if (!fReadAllBlocks(hIniSystem, szActiveHandles, &pHandlesBuffer, &ulHandlesSize))
         return NULL;

   usObjID = GetFSObjectID(pHandlesBuffer,
         ulHandlesSize,
         0,
         szFullPath);

   if (!usObjID)
      return hObject;

   hObject = MakeDiskHandle(usObjID);
   return hObject;
}


/*************************************************************
* Get the object partname as specified in usID
*************************************************************/
USHORT _System GetFSObjectID(PBYTE pHandlesBuffer, ULONG ulBufSize, USHORT usParent, PSZ pszFname)
{
PDRIV pDriv;
PNODE pNode;
PBYTE pCur;
PBYTE p,
      pEnd;
USHORT usPartSize;

   setlocale(LC_ALL, "");

   p = strchr(pszFname, '\\');
   if (p)
      usPartSize = p - pszFname;
   else
      usPartSize = strlen(pszFname);

   pEnd = pHandlesBuffer + ulBufSize;
   pCur = pHandlesBuffer + 4;
   while (pCur < pEnd)
      {
      if (!memicmp(pCur, "DRIV", 4))
         {
         pDriv = (PDRIV)pCur;
         pCur += sizeof(DRIV) + strlen(pDriv->szName);
         }
      else if (!memicmp(pCur, "NODE", 4))
         {
         pNode = (PNODE)pCur;
         pCur += sizeof (NODE) + pNode->usNameSize;

         if (usParent == pNode->usParentID &&
             pNode->usNameSize == usPartSize &&
             !memicmp(pszFname, pNode->szName, usPartSize))
            {
            if (strlen(pszFname) == usPartSize)
               return pNode->usID;

            pszFname += usPartSize + 1;
            p = strchr(pszFname, '\\');
            if (p)
               usPartSize = p - pszFname;
            else
               usPartSize = strlen(pszFname);
            usParent = pNode->usID;
            }
         }
      else
         return 0;
      }
   return 0;
}

/*************************************************************
* Read all BLOCK's
*************************************************************/
BOOL _System fReadAllBlocks(HINI hiniSystem, PSZ pszActiveHandles, PBYTE * ppBlock, PULONG pulSize)
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
