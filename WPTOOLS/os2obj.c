#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#define INCL_VIO
#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_WPERRORS
#include <os2.h>
#include <wpfolder.h>
#include "wptools.h"

#define OBJECT_ABSTRACT  0x0001
#define OBJECT_ABSTRACT2 0x0002
#define OBJECT_FSYS      0x0003

static USHORT usAbstract = 0xFFFF;
static USHORT usFileSystem = 0xFFFF;
static USHORT usTransient = 0xFFFF;
static PSZ    pszBaseClasses;

HINI hIniUser   = HINI_USER;
HINI hIniSystem = HINI_SYSTEM;

int _CRT_init(void);
void _CRT_term(ULONG);

ULONG _System _DLL_InitTerm(ULONG hMod, ULONG ulFlag)
{
PBYTE pszClass;
ULONG ulProfileSize;
USHORT usIndex;

   if (ulFlag)
      {
      _CRT_term(0L);
      return TRUE;
      }

   if (_CRT_init() == -1)
      return FALSE;

   pszBaseClasses = GetProfileData("PM_Workplace:BaseClass", "ClassList", hIniUser, &ulProfileSize);
   if (!pszBaseClasses)
      {
      MessageBox("ERROR!", "WPTOOLS.DLL : Unable to read base Classes in GetBaseClasses!");
      return FALSE;
      }

   pszClass = pszBaseClasses;
   usIndex = 1;
   while (*pszClass && pszClass < pszBaseClasses + ulProfileSize)
      {
      if (!stricmp(pszClass, "WPTransient"))
         usTransient = usIndex;
      else if (!stricmp(pszClass, "WPAbstract"))
         usAbstract = usIndex;
      else if (!stricmp(pszClass, "WPFileSystem"))
         usFileSystem = usIndex;
      pszClass += strlen(pszClass) + 1;
      usIndex++;
      }
   return TRUE;
}


VOID SetInis(HINI hUser, HINI hSystem)
{
   hIniUser = hUser;
   hSystem  = hSystem;
   _DLL_InitTerm(0, 0);
}


PSZ GetBaseClassString(HOBJECT hObject)
{
PSZ pszBaseClass = pszBaseClasses;
USHORT usIndex = 1;

   while (*pszBaseClass)
      {
      if (usIndex == HIUSHORT(hObject))
         return pszBaseClass;
      pszBaseClass += strlen(pszBaseClass) + 1;
      usIndex++;
      }
   return "Unknow baseclass!";
}

USHORT GetBaseClassType(HOBJECT hObject)
{
   if (HIUSHORT(hObject) == usAbstract)
      return BASECLASS_ABSTRACT;
   if (HIUSHORT(hObject) == usFileSystem)
      return BASECLASS_FILESYS;
   if (HIUSHORT(hObject) == usTransient)
      return BASECLASS_TRANSIENT;
   return BASECLASS_OTHER;
}

/************************************************************************
* Object data consist of the following layout:
*
*  ULONG cbObject;
*  BYTE  szObjectName[?];  = strlen() + 1 
*  BYTE  bUnknown[16];
*
*  and then several group of data: starting with OINFO struct
************************************************************************/
USHORT _System GetObjectValue(PBYTE pObjData, USHORT usTag, PVOID pOutBuf, USHORT usMax)
{
PBYTE pb, pBufEnd;
POINFO pOinfo;
PBYTE  pbGroepBufEnd;

   memset(pOutBuf, 0, usMax);
   pb = pObjData;
   pOinfo  = (POINFO)pb;
   pBufEnd = pb + sizeof(OINFO) + strlen(pOinfo->szName) + pOinfo->cbData;

   while (pb < pBufEnd)
      {
      pOinfo = (POINFO)pb;
      pb += sizeof(OINFO) + strlen(pOinfo->szName);
      pbGroepBufEnd = pb + pOinfo->cbData;
      if (pbGroepBufEnd > pBufEnd)
         pbGroepBufEnd = pBufEnd;
      while (pb < pbGroepBufEnd)
         {
         PTAG pTag   = (PTAG)pb;
         pb += sizeof (TAG);
         if (pTag->usTag == usTag)
            {
            if (usMax < pTag->cbTag)
               {
               MessageBox("Error", "%s - tag %u, buffer too small in GetObjectValue",
                  pOinfo->szName,
                  (UINT)usTag);

               return 0;
               }
            memcpy(pOutBuf, pb, pTag->cbTag);
            return pTag->usTagFormat;
            }
         pb += pTag->cbTag;
         }
      }
   return 0;
}

USHORT _System GetObjectValueSize(PBYTE pObjData, USHORT usTag)
{
PBYTE pb, pBufEnd;
POINFO pOinfo;
PBYTE  pbGroepBufEnd;

   pb = pObjData;
   pOinfo  = (POINFO)pb;
   pBufEnd = pb + sizeof(OINFO) + strlen(pOinfo->szName) + pOinfo->cbData;

   while (pb < pBufEnd)
      {
      pOinfo = (POINFO)pb;
      pb += sizeof(OINFO) + strlen(pOinfo->szName);
      pbGroepBufEnd = pb + pOinfo->cbData;
      if (pbGroepBufEnd > pBufEnd)
         pbGroepBufEnd = pBufEnd;
      while (pb < pbGroepBufEnd)
         {
         PTAG pTag   = (PTAG)pb;
         pb += sizeof (TAG);
         if (pTag->usTag == usTag)
            {
            return pTag->cbTag;
            }
         pb += pTag->cbTag;
         }
      }
   return 0;
}



/************************************************************************
* Object data consist of the following layout:
*
*  ULONG cbObject;
*  BYTE  szObjectName[?];  = strlen() + 1 
*  BYTE  bUnknown[16];
*
*  and then several group of data: starting with OINFO struct
************************************************************************/
USHORT _System GetGenObjectValue(PBYTE pObjData, PSZ pszObjName, USHORT usTag, PVOID pOutBuf, USHORT usMax)
{
PBYTE pb, pBufEnd;
ULONG ulObjSize;
POINFO pOinfo;
PBYTE  pbGroepBufEnd;

   memset(pOutBuf, 0, usMax);

   ulObjSize = *(PULONG)pObjData;
   pb = pObjData + sizeof (ULONG);
   pb += strlen(pb) + 1;
   pb += 16; /* some unknown data */
   pBufEnd = pObjData + ulObjSize;

   while (pb < pBufEnd)
      {
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
               {
               MessageBox("Error", "%s - tag %u, buffer too small in GetGenObjectValue",
                  pszObjName,
                  (UINT)usTag);

               return 0;
               }
            memcpy(pOutBuf, pb, pTag->cbTag);
            return pTag->usTagFormat;
            }
         pb += pTag->cbTag;
         }
      }
   return 0;
}

/*********************************************************************
* GetClassData
*********************************************************************/
PBYTE _System GetClassInfo(HINI hini, HOBJECT hObject, PULONG pulProfileSize)
{
PBYTE  pClassInfo;
BYTE   szObjectID[15];
BYTE   szPathName[256];
USHORT usEASize;
PBYTE  pEAValue;
BOOL   fRemote = FALSE;
BOOL   fRoot = FALSE;


   if (!hini)
      hini = HINI_PROFILE;

   if (IsObjectAbstract(hObject))
      {
      sprintf(szObjectID, "%X", LOUSHORT(hObject));
      pClassInfo = GetProfileData(OBJECTS, szObjectID, hini, pulProfileSize);
      if (pClassInfo && *pulProfileSize > 4 && *(PULONG)pClassInfo >= *pulProfileSize)
         *(PULONG)pClassInfo = *pulProfileSize - 6;
      return pClassInfo;
      }

   if (!PathFromObject(hini, hObject, szPathName, sizeof szPathName, NULL))
      {
//      MessageBox("GetClassInfo","Object %X does not exist!", hObject);
      return NULL;
      }

   if (!memcmp(szPathName, "\\\\", 2))
      {
      PSZ p = szPathName;
      USHORT usSlashCount = 0;
      fRemote = TRUE;

      while ((p = strchr(p, '\\')) != NULL)
         {
         usSlashCount++;
         p++;
         }
      if (usSlashCount == 3)
         fRoot = TRUE;
      }
   else if (strlen(szPathName) == 2)
      fRoot = TRUE;


   if (fRoot)
      {
      INT    iHandle;
      ULONG  ulFileLength;

      if (!fRemote)
         {
         BYTE   szDisk[3];
         ULONG  ulBufferSize;
         BYTE   Buffer[200];
         PFSQBUFFER2 fsqBuf = (PFSQBUFFER2)Buffer;
         APIRET rc;

         szDisk[0] = szPathName[0];
         szDisk[1] = ':';
         szDisk[2] = 0;

         ulBufferSize = sizeof (Buffer);
         rc = DosQueryFSAttach(szDisk,
            1,
            FSAIL_QUERYNAME,
            fsqBuf,
            &ulBufferSize);
         if (rc)
            MessageBox("GetClassInfo","DosQueryFSAttach returned %d!", rc);
         else
            {
            if (fsqBuf->iType & FSAT_REMOTEDRV)
               fRemote = TRUE;
            }
         }
      if (fRemote)
         strcat(szPathName, "\\WP SHARE. SF");
      else
         strcat(szPathName, "\\WP ROOT. SF");
      iHandle = sopen(szPathName, O_RDONLY | O_BINARY, SH_DENYNO);
      if (iHandle == -1)
         return NULL;
      ulFileLength = filelength(iHandle);
      pClassInfo = malloc(ulFileLength);
      if (!pClassInfo)
         {
         close(iHandle);
         MessageBox("GetClassInfo","Not enough memory for ClassInfo!");
         return NULL;
         }
      if (read(iHandle, pClassInfo, ulFileLength) != (INT)ulFileLength)
         {
         free(pClassInfo);
         close(iHandle);
         return NULL;
         }
      close(iHandle);
      memmove(pClassInfo, pClassInfo + 4, ulFileLength - 4);
      *pulProfileSize = ulFileLength - 4;
      if (pClassInfo && *pulProfileSize > 4 && *(PULONG)pClassInfo >= *pulProfileSize)
         *(PULONG)pClassInfo = *pulProfileSize - 6;
      close(iHandle);
      }
   else
      {
      if (!GetEAValue(szPathName, ".CLASSINFO", &pEAValue, &usEASize))
         return FALSE;

      pClassInfo = malloc(usEASize - 4);
      if (!pClassInfo)
         {
         free(pEAValue);
         MessageBox("GetClassInfo","Not enough memory for ClassInfo!");
         return NULL;
         }

      memcpy(pClassInfo, pEAValue + 4, usEASize - 4);
      free(pEAValue);
      *pulProfileSize = usEASize - 4;
      if (pClassInfo && *pulProfileSize > 4 && *(PULONG)pClassInfo >= *pulProfileSize)
         *(PULONG)pClassInfo = *pulProfileSize - 6;
      }
   return pClassInfo;
}

/*********************************************************************
* GetObjectSubString
*********************************************************************/
BOOL _System GetObjectValueSubValue(PBYTE pObjData, USHORT usTag,
                         USHORT usStringTag, PVOID pOutBuf, USHORT usMax)
{
static BYTE pBuffer[300];
PBYTE p;

   if (!GetObjectValue(pObjData, usTag, pBuffer, sizeof pBuffer))
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

/*********************************************************************
* GetObjectSubString
*********************************************************************/
BOOL _System GetGenObjectValueSubValue(PBYTE pObjData, PSZ pszObjName, USHORT usTag,
                         USHORT usStringTag, PVOID pOutBuf, USHORT usMax)
{
static BYTE pBuffer[1024];
PBYTE p;

   if (!GetGenObjectValue(pObjData, pszObjName, usTag, pBuffer, sizeof pBuffer))
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


/*********************************************************************
* Is Object Abstract
*********************************************************************/
BOOL _System IsObjectAbstract(HOBJECT hObject)
{
   return (HIUSHORT(hObject) == usAbstract);

#if 0
   if (!Is21())
      {
      if (HIUSHORT(hObject) == usAbstract)
         return TRUE;
      else
         return FALSE;
      }


   if (HIUSHORT(hObject) == OBJECT_ABSTRACT ||
       HIUSHORT(hObject) == OBJECT_ABSTRACT2)
      return TRUE;
   else
      return FALSE;
#endif
}

/*********************************************************************
* Is Object Abstract
*********************************************************************/
BOOL _System IsObjectDisk(HOBJECT hObject)
{
   return (HIUSHORT(hObject) == usFileSystem);
}

/*********************************************************************
* Make An abstract Object Handle
*********************************************************************/
HOBJECT _System MakeAbstractHandle(USHORT usObject)
{
   return MAKEULONG(usObject, usAbstract);
}


/*********************************************************************
* Make An Disk Object Handle
*********************************************************************/
HOBJECT _System MakeDiskHandle(USHORT usObject)
{
   return MAKEULONG(usObject, usFileSystem);
}


/*******************************************************************
* MySetObjectData
*******************************************************************/
BOOL _System MySetObjectData(HOBJECT hObject, PSZ pszSettings)
{
PBYTE pClassInfo;
ULONG ulClassLength;
PSZ   pszNewSettings;
BOOL  fRetco;

   pClassInfo = GetClassInfo(NULL, hObject, &ulClassLength);
   if (pClassInfo && !stricmp(pClassInfo+4, "WPSchemePalette"))
      {
      free(pClassInfo);
      pszNewSettings = malloc(strlen(pszSettings) + 40);
      if (!pszNewSettings)
         return FALSE;
      strcpy(pszNewSettings, pszSettings);
      if (pszNewSettings[strlen(pszNewSettings) - 1] != ';')
         strcat(pszNewSettings, ";");
      strcat(pszNewSettings, "AUTOSETUP=YES");
      fRetco = WinSetObjectData(hObject, pszNewSettings);
      free(pszNewSettings);
      return fRetco;
      }

   return WinSetObjectData(hObject, pszSettings);
}

/*******************************************************************
* MyDestroyObject
*******************************************************************/
BOOL _System MyDestroyObject(HOBJECT hObject)
{
   return WinDestroyObject(hObject);
}

/*******************************************************************
* Query OS/2 version 2.0 or 2.1
*******************************************************************/
BOOL _System Is21(VOID)
{
   if (_osmajor != 20)
      {
      MessageBox("ERROR!",
         "Unsupported version of OS/2 (%d.%d)", (int)_osmajor/10, (int)_osminor);
      exit(1);
      }

   if (!_osminor)
      return FALSE;

   return TRUE;
}

BOOL _System IsWarp4(VOID)
{
   if (_osmajor != 20)
      {
      MessageBox("ERROR!",
         "Unsupported version of OS/2 (%d.%d)", (int)_osmajor/10, (int)_osminor);
      exit(1);
      }

   if (_osminor != 40)
      return FALSE;

   return TRUE;
}

BOOL _System IsWarp(VOID)
{
   if (_osmajor != 20)
      {
      MessageBox("ERROR!",
         "Unsupported version of OS/2 (%d.%d)", (int)_osmajor/10, (int)_osminor);
      exit(1);
      }

   if (_osminor < 30)
      return FALSE;

   return TRUE;
}


/***************************************************************
* Get an OBJECTID or PATHNAME for an object
***************************************************************/
PSZ _System pszObjectIDFromHandle(HOBJECT hObject)
{
static BYTE szObject[CCHMAXPATH];
PBYTE  pObjectData;
ULONG  ulProfileSize;
PSZ pszClass;
PBYTE pb, pObjectEnd;
BOOL  fFound = FALSE;


   memset(szObject, 0, sizeof szObject);

   pObjectData = GetClassInfo(NULL, hObject, &ulProfileSize);
   if (pObjectData)
      {
      pszClass = pObjectData + 4;
      pObjectEnd = pObjectData + *(PULONG)pObjectData;
      pb = pszClass + strlen(pszClass) + 1 + 16;
      while (!fFound && pb < pObjectEnd)
         {
         POINFO pOinfo = (POINFO)pb;

         if (!strcmp(pOinfo->szName, "WPObject"))
            {
            if (ObjectIDFromData((PBYTE)pOinfo, szObject, sizeof szObject))
               fFound = TRUE;
            }
         pb += sizeof(OINFO) + strlen(pOinfo->szName) + pOinfo->cbData;
         }
      free(pObjectData);
      }

   if (!fFound)
      {
      if (IsObjectDisk(hObject))
         {
         if (PathFromObject(hIniSystem, hObject, szObject, sizeof szObject, NULL))
            {
            if (strlen(szObject) == 2)
               strcat(szObject, "\\");
            return szObject;
            }
         else
            return NULL;
         }
      return NULL;
      }
   return szObject;
}


/*********************************************************************
* GetObjectID
*********************************************************************/
BOOL _System GetObjectID(PBYTE pObjectData, PSZ pszObjectID, USHORT usMax)
{
   if (GetGenObjectValue(pObjectData, "WPObject", WPOBJECT_SZID, pszObjectID, usMax))
      return TRUE;

   return GetGenObjectValueSubValue(pObjectData, "WPObject", WPOBJECT_STRINGS,
      WPOBJ_STR_OBJID, pszObjectID, usMax);
}

