#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#define INCL_WIN
#define INCL_DOS
#define INCL_WPERRORS
#include <os2.h>

#include "icondef.h"
#include "..\wptools\wptools.h"

extern HPOINTER hptrUnknown;

BOOL  GetAbstractName     (HOBJECT hObject, PSZ pszExe, USHORT usMax);
USHORT GetObjectData       (PBYTE pObjData, PSZ pszObjName, USHORT usTag, PVOID pOutBuf, USHORT usMaxBuf);
BOOL GetAssocType(PSZ pszFileName, PULONG pulAssoc, USHORT usMax);
BOOL GetObjectDataString(PBYTE pObjData, PSZ pszObjName, USHORT usTag,
                         USHORT usStringTag, PVOID pOutBuf, USHORT usMax);
BOOL GetActiveHandles(HINI hIniSystem, PSZ pszHandlesAppName, USHORT usMax);
BOOL _System fReadAllBlocks(HINI hiniSystem, PSZ pszActiveHandles, PBYTE * ppBlock, PULONG pulSize);

/***************************************************************
* GetAssocFilter
***************************************************************/
BOOL GetAssocFilter(PSZ pszFileName, PULONG pulAssoc, USHORT usMax)
{
PBYTE pFilterArray,
      pFilter;
PBYTE pAssocArray,
      pAssoc,
      pszShortName;
HINI  hini = HINI_USERPROFILE;
ULONG ulProfileSize;
BYTE  szResult[512];
APIRET rc;
HOBJECT hObject;
BOOL  bRetco = FALSE;


   memset((PBYTE)pulAssoc, 0, usMax * sizeof (ULONG));

   pszShortName = strrchr(pszFileName, '\\');
   if (pszShortName)
      pszShortName++;
   else
      pszShortName = pszFileName;

   pFilterArray = GetAllProfileNames(ASSOC_FILTER, hini, &ulProfileSize);
   pFilter = pFilterArray;
   while (pFilter && *pFilter)
      {
      rc = DosEditName(1, pszShortName, pFilter, szResult, sizeof szResult);
      if (!rc && !stricmp(pszShortName, szResult))
         {
         pAssocArray = GetProfileData(ASSOC_FILTER, pFilter, hini, &ulProfileSize);
         if (pAssocArray)
            {
            pAssoc = pAssocArray;
            while (pAssoc < pAssocArray + ulProfileSize)
               {
               hObject = atol(pAssoc);
               if (usMax > 0)
                  {
                  *pulAssoc++ = hObject;
                  usMax--;
                  bRetco = TRUE;
                  }
               pAssoc += strlen(pAssoc) + 1;
               }
            free(pAssocArray);
            }
         }
      pFilter += strlen(pFilter) + 1;
      }
   if (pFilterArray)
      free(pFilterArray);

   if (!bRetco && GetAssocType(pszFileName, pulAssoc, usMax))
      return TRUE;

   return bRetco;
}

/***************************************************************
* GetAssocFilter
***************************************************************/
BOOL GetAssocType(PSZ pszFileName, PULONG pulAssoc, USHORT usMax)
{
PBYTE pTypeArray,
      pType;
PBYTE pAssocArray,
      pAssoc;
HINI  hini = HINI_USERPROFILE;
PBYTE pchType;
USHORT usTypeSize;
ULONG ulProfileSize;
HOBJECT hObject;
BOOL  bRetco = FALSE;


   if (!GetEAValue(pszFileName, ".TYPE", &pchType, &usTypeSize))
      pchType = NULL;
   else
      {
      PSZ p = strpbrk(pchType, "\r\n");
      if (p)
         *p = 0;
      }

   memset((PBYTE)pulAssoc, 0, usMax * sizeof (ULONG));

   pTypeArray = GetAllProfileNames(ASSOC_TYPE, hini, &ulProfileSize);
   if (!pTypeArray)
      return FALSE;

   pType = pTypeArray;
   while (*pType)
      {
      PBYTE p, pEnd;
      BOOL  bFound = FALSE;

      if (pchType)
         {
         pEnd = pchType + usTypeSize;
         for (p = pchType; *p && p < pEnd; p += strlen(p) + 1)
            {
            if (!stricmp(p, pType))
               {
               bFound = TRUE;
               break;
               }
            }
         }
      else
         {
         if (!stricmp(pType,"Plain Text"))
            bFound = TRUE;
         }

      if (bFound)
         {
         pAssocArray = GetProfileData(ASSOC_TYPE, pType, hini, &ulProfileSize);
         if (!pAssocArray)
            {
            free(pTypeArray);
            return FALSE;
            }

         pAssoc = pAssocArray;
         while (pAssoc < pAssocArray + ulProfileSize)
            {
            hObject = atol(pAssoc);
            if (usMax > 0)
               {
               *pulAssoc++ = hObject;
               usMax--;
               bRetco = TRUE;
               }
            pAssoc += strlen(pAssoc) + 1;
            }
         free(pAssocArray);
         }
      pType += strlen(pType) + 1;
      }
   free(pTypeArray);
   if (pchType)
      free(pchType);
   return bRetco;
}


/*************************************************************
* Get the object name as specified in hObject
*************************************************************/
BOOL GetObjectName(HOBJECT hObject, PSZ pszFname, USHORT usMax)
{
USHORT usObjID;
PBYTE pBuffer;
HINI hini = HINI_SYSTEMPROFILE;
ULONG ulProfileSize;
BYTE  szHandles[100];


   usObjID   = LOUSHORT(hObject);
   if (IsObjectAbstract(hObject))
      return GetAbstractName(hObject, pszFname, usMax);

   if (!IsObjectDisk(hObject))
      return FALSE;

   GetActiveHandles(hini, szHandles, sizeof szHandles);


   if (!fReadAllBlocks(hini, szHandles, &pBuffer, &ulProfileSize))
      return FALSE;

   memset(pszFname, 0, usMax);

   if (!GetPartName(pBuffer, ulProfileSize, usObjID, pszFname, usMax, NULL))
      {
      free(pBuffer);
      return FALSE;
      }
   free(pBuffer);
   return TRUE;
}

/*************************************************************
* Get active handles
*************************************************************/
BOOL GetActiveHandles(HINI hIniSystem, PSZ pszHandlesAppName, USHORT usMax)
{
PBYTE pBuffer;
ULONG ulProfileSize;

   pBuffer = GetProfileData(ACTIVEHANDLES, HANDLESAPP,
      hIniSystem,&ulProfileSize);
   if (!pBuffer)
      {
      strncpy(pszHandlesAppName, HANDLES, usMax - 1);
      return TRUE;
      }
   strncpy(pszHandlesAppName, pBuffer, usMax -1);
   free(pBuffer);
   return TRUE;
}


/*************************************************************
* Get Abstract name
*************************************************************/
BOOL GetAbstractName(HOBJECT hObject, PSZ pszExe, USHORT usMax)
{
USHORT  usObjID = LOUSHORT(hObject);
ULONG   ulProfileSize;
BYTE    szObjID[10];
HOBJECT hObj;
PBYTE   pObjectData;
WPPGMDATA wpData;


   memset(pszExe, 0, usMax);
   sprintf(szObjID, "%X", usObjID);
   pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
   if (!pObjectData)
      return FALSE;

   if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_DATA, &wpData, sizeof wpData))
      {
      if (wpData.hExeHandle)
         hObj = wpData.hExeHandle;

      else
         {
         if (GetObjectDataString(pObjectData, "WPProgramRef", WPPROGRAM_STRINGS,
            WPPGM_STR_EXENAME, pszExe, usMax))
            {
            free(pObjectData);
            return TRUE;
            }
         free(pObjectData);
         return FALSE;
         }
      }
   else
      {
      if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_EXENAME, pszExe, usMax))
         {
         free(pObjectData);
         return TRUE;
         }

      if (!GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_EXEHANDLE, &hObj, sizeof hObj))
         {
         free(pObjectData);
         return FALSE;
         }
      }
   /*
      Special situation, name is cmd or command
   */
   if (hObj == 0xFFFF)
      {
      strcpy(pszExe, "*");
      free(pObjectData);
      return TRUE;
      }
   free(pObjectData);
   hObj += 0x30000;
   return GetObjectName(hObj, pszExe, usMax);
}

/*************************************************************
* Get CurDir
*************************************************************/
BOOL GetObjectCurDir(HOBJECT hObject, PSZ pszCurDir, USHORT usMax)
{
USHORT  usObjID = LOUSHORT(hObject);
ULONG   ulProfileSize;
BYTE    szObjID[10];
HOBJECT hObj;
PBYTE   pObjectData;
WPPGMDATA wpData;

   sprintf(szObjID, "%X", usObjID);
   pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
   if (!pObjectData)
      return FALSE;
   if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_DATA, &wpData, sizeof wpData))
      {
      if (wpData.hCurDirHandle)
         hObj = wpData.hCurDirHandle;
      else
         {
         free(pObjectData);
         return FALSE;
         }
      }
   else
      {
      if (!GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_DIRHANDLE, &hObj, sizeof hObj))
         {
         free(pObjectData);
         return FALSE;
         }
      }
   free(pObjectData);
   hObj += 0x30000;
   return GetObjectName(hObj, pszCurDir, usMax);
}

/****************************************************
*
*****************************************************/
USHORT GetObjectData(PBYTE pObjData, PSZ pszObjName, USHORT usTag, PVOID pOutBuf, USHORT usMax)
{
PBYTE pb, pBufEnd;
ULONG ulObjSize;

   memset(pOutBuf, 0, usMax);

   ulObjSize = *(PULONG)pObjData;
   pb = pObjData + sizeof (ULONG);

   pb += strlen(pb) + 1;

   pb += 16;

   pBufEnd = pObjData + ulObjSize;
   while (pb < pBufEnd)
      {
      POINFO pOinfo;
      PBYTE  pbGroepBufEnd;

      pOinfo = (POINFO)pb;
      pb += sizeof(OINFO) + strlen(pOinfo->szName);
      pbGroepBufEnd = pb + pOinfo->cbData;
      while (pb < pbGroepBufEnd)
         {
         PTAG pTag   = (PTAG)pb;
         pb += sizeof (TAG);
         if (!stricmp(pOinfo->szName, pszObjName) &&
             pTag->usTag == usTag)
            {
            if (usMax < pTag->cbTag)
               return 0;
            memcpy(pOutBuf, pb, pTag->cbTag);
            return pTag->usTagFormat;
            }
         pb += pTag->cbTag;
         }
      }
   return 0;
}

/***********************************************************************
*  Get Object icon
***********************************************************************/
HPOINTER GetObjectIcon(HOBJECT hObject, PBYTE pObjectData)
{
PBYTE pBuffer;
USHORT usObjID;
HPOINTER hptr = NULL;
ULONG ulProfileSize;
BYTE  szFileName[CCHMAXPATH];
BYTE  szPath[CCHMAXPATH];

   usObjID = LOUSHORT(hObject);
   if (IsObjectAbstract(hObject))
      {
      BYTE  szObjID[10];
      BOOL  fObjectDataRead = FALSE;

      sprintf(szObjID, "%X", usObjID);
      pBuffer = GetProfileData(ICONS, szObjID, HINI_USERPROFILE, &ulProfileSize);
      if (pBuffer)
         {
         USHORT usIconCX = WinQuerySysValue(HWND_DESKTOP, SV_CXICON);

         hptr = Buffer2Icon(HWND_DESKTOP, pBuffer, usIconCX);
         free(pBuffer);
         return hptr;
         }

      if (!pObjectData)
         {
         pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
         if (!pObjectData)
            return NULL;
         fObjectDataRead = TRUE;
         }
      if (!memicmp(pObjectData  + 6, "Shadow", 5))
         {
         HOBJECT hObjShadow;
         if (GetObjectData(pObjectData, "WPShadow", WPSHADOW_LINK, &hObjShadow, sizeof hObjShadow))
            hptr = GetObjectIcon(hObjShadow, NULL);
         }
      if (!memicmp(pObjectData + 4, "WPProgram", 9))
         {
         HOBJECT hObjProgram;
         WPPGMDATA wpData;

         if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_DATA, &wpData, sizeof wpData))
            {
            if (wpData.hExeHandle)
               {
               hObjProgram = wpData.hExeHandle  + 0x30000;
               hptr = GetObjectIcon(hObjProgram, NULL);
               }
            else
               {
               if (GetObjectDataString(pObjectData, "WPProgramRef", WPPROGRAM_STRINGS,
                  WPPGM_STR_EXENAME, szFileName, sizeof szFileName))
                  {
                  _searchenv(szFileName, "PATH", szPath);
                  hptr = WinLoadFileIcon(szPath, FALSE);
                  }   
               }
            }
         else if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_EXEHANDLE, &hObjProgram, sizeof hObjProgram))
            hptr = GetObjectIcon(hObjProgram, NULL);
         else if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_EXENAME, szFileName, sizeof szFileName))
            hptr = WinLoadFileIcon(szFileName, FALSE);
         }

      if (fObjectDataRead)
         free(pObjectData);
      return hptr;
      }

   if (!GetObjectName(hObject, szFileName, sizeof szFileName))
      return NULL;

   if (szFileName[0] == '*')
      return NULL;

   return WinLoadFileIcon(szFileName, FALSE);
}

/*******************************************************************
* Get object progtype
*******************************************************************/
ULONG GetObjectProgType(HOBJECT hObject)
{
USHORT usObjID;
PBYTE  pObjectData;
ULONG  ulProfileSize;
BYTE  szObjID[10];
ULONG ulType;
WPPGMDATA wpData;


   usObjID = LOUSHORT(hObject);
   if (!IsObjectAbstract(hObject))
      return 0L;

   sprintf(szObjID, "%X", usObjID);
   pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
   if (!pObjectData)
      return FALSE;

   if (GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_DATA, &wpData, sizeof wpData))
      ulType = wpData.ulProgType;
   else
      {
      if (!GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_PROGTYPE, &ulType, sizeof ulType))
         ulType = 0L;
      }
   free(pObjectData);
   return ulType;
}

/*******************************************************************
* Get the objects title
*******************************************************************/
BOOL GetObjectTitle(HOBJECT hObject, PSZ pszTitle, USHORT usMax)
{
USHORT usObjID;
PBYTE  pObjectData;
ULONG  ulProfileSize;
BYTE  szObjID[10];

   usObjID = LOUSHORT(hObject);
   if (!IsObjectAbstract(hObject))
      return FALSE;

   sprintf(szObjID, "%X", usObjID);
   pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
   if (!pObjectData)
      return FALSE;
   if (!GetObjectData(pObjectData, "WPAbstract", WPABSTRACT_TITLE, pszTitle, usMax))
      {
      free(pObjectData);
      return FALSE;
      }
   free(pObjectData);
   return TRUE;
}

/*******************************************************************
* Get the folder contents
*******************************************************************/
BOOL GetFolderContent(PSZ pszFolder, PBYTE *pBuffer, PULONG pulProfileSize)
{
HOBJECT hObject = WinQueryObject(pszFolder);
USHORT usObjID;
BYTE  szObjID[10];

   if (!hObject)
      return FALSE;

   usObjID = LOUSHORT(hObject);
   if (!IsObjectDisk(hObject))
      return FALSE;

   sprintf(szObjID, "%X", usObjID);
   *pBuffer = GetProfileData(FOLDERCONTENT, szObjID, HINI_USERPROFILE, pulProfileSize);
   if (*pBuffer)
      return TRUE;
   return FALSE;
}               
/*******************************************************************
* Get the abstract object information
*******************************************************************/
BOOL GetAbstractObject(HOBJECT hObject, PFILEINFO pFinfo)
{
USHORT usObjID;
PBYTE  pObjectData;
ULONG  ulProfileSize;
BYTE  szObjID[10];

   usObjID = LOUSHORT(hObject);
   if (!IsObjectAbstract(hObject))
      return FALSE;

   memset(pFinfo, 0, sizeof (FILEINFO));
   sprintf(szObjID, "%X", usObjID);
   pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
   if (!pObjectData)
      return FALSE;

   pFinfo->chType = TYPE_ABSTRACT;
   pFinfo->hObject = hObject;

   if (!GetObjectData(pObjectData, "WPAbstract", WPABSTRACT_TITLE, pFinfo->szTitle, sizeof pFinfo->szTitle))
      strcpy(pFinfo->szTitle, "Unknown title");

   strcpy(pFinfo->szFileName, pFinfo->szTitle);
   pFinfo->hptrIcon     = GetObjectIcon(hObject, pObjectData);
   if (pFinfo->hptrIcon)
      pFinfo->bIconCreated = TRUE;
   else
      pFinfo->hptrIcon = hptrUnknown;
   pFinfo->hptrMiniIcon = pFinfo->hptrIcon;
   free(pObjectData);
   return TRUE;
}

/*******************************************************************
* Get the abstract parameters
*******************************************************************/
BOOL GetObjectParameters(HOBJECT hObject, PSZ pszParms, USHORT usMax)
{
USHORT  usObjID = LOUSHORT(hObject);
ULONG   ulProfileSize;
BYTE    szObjID[10];
PBYTE   pObjectData;

   sprintf(szObjID, "%X", usObjID);
   pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
   if (!pObjectData)
      return FALSE;

   if (!GetObjectDataString(pObjectData, "WPProgramRef", WPPROGRAM_STRINGS,
         WPPGM_STR_ARGS, pszParms, usMax))
         {
         if (!GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_PARAMS, pszParms, usMax))
            {
            free(pObjectData);
            return FALSE;
            }
         }
   free(pObjectData);
   return TRUE;
}


/*******************************************************************
* Get the DOS Settings
*******************************************************************/
BOOL GetDosSettings(HOBJECT hObject, PSZ pszDosSettings, USHORT usMax)
{
USHORT  usObjID = LOUSHORT(hObject);
ULONG   ulProfileSize;
BYTE    szObjID[10];
PBYTE   pObjectData;

   sprintf(szObjID, "%X", usObjID);
   pObjectData = GetProfileData(OBJECTS, szObjID, HINI_USERPROFILE, &ulProfileSize);
   if (!pObjectData)
      return FALSE;
   if (!GetObjectData(pObjectData, "WPProgramRef", WPPROGRAM_DOSSET, pszDosSettings, usMax))
      {
      free(pObjectData);
      return FALSE;
      }
   free(pObjectData);
   return TRUE;
}

/*********************************************************************
* GetObjectSubString
*********************************************************************/
BOOL GetObjectDataString(PBYTE pObjData, PSZ pszObjName, USHORT usTag,
                         USHORT usStringTag, PVOID pOutBuf, USHORT usMax)
{
static BYTE pBuffer[300];
PBYTE p;

   if (!GetObjectData(pObjData, pszObjName, usTag, pBuffer, sizeof pBuffer))
      return FALSE;

   p = pBuffer;
   while (*(PUSHORT)p != 0xFFFF)
      {
      if (*(PUSHORT)p == usStringTag)
         {
         strncpy(pOutBuf, p + 2, usMax);
         return TRUE;
         }
      p+=2;
      p+=strlen(p) + 1;
      }
   return FALSE;

}

/*******************************************************************
* MySetObjectData
*******************************************************************/
BOOL MySetObjectData(HOBJECT hObject, PSZ pszSettings)
{
   return WinSetObjectData(hObject, pszSettings);
}

/*******************************************************************
* MyDestroyObjectData
*******************************************************************/
BOOL MyDestroyObject(HOBJECT hObject)
{
   return WinDestroyObject(hObject);
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
      DebugBox("fReadAllBlocks", "Not enough memory for profile data!", FALSE);
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


