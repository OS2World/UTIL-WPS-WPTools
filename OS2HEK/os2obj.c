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
#include "os2hek.h"



/************************************************************************
* Object data consist of the following layout:
*
*  ULONG cbObject;
*  BYTE  szObjectName[?];  = strlen() + 1 
*  BYTE  bUnknown[16];
*
*  and then several group of data: starting with OINFO struct
************************************************************************/
USHORT _System GetObjectData(PBYTE pObjData, PSZ pszObjName, USHORT usTag, PVOID pOutBuf, USHORT usMax)
{
PBYTE pb, pBufEnd;
ULONG ulObjSize;

   memset(pOutBuf, 0, usMax);

   ulObjSize = *(PULONG)pObjData;
   pb = pObjData + sizeof (ULONG);

   pb += strlen(pb) + 1;

   pb += 16; /* some unknown data */

   pBufEnd = pObjData + ulObjSize;
   while (pb < pBufEnd)
      {
      POINFO pOinfo;
      PBYTE  pbGroepBufEnd;

      pOinfo = (POINFO)pb;
      pb += sizeof(OINFO) + strlen(pOinfo->szName);
      pbGroepBufEnd = pb + pOinfo->cbData;
      if (pbGroepBufEnd > pBufEnd)
         pbGroepBufEnd = pBufEnd;
      while (pb < pbGroepBufEnd)
         {
         PTAG pTag   = (PTAG)pb;
         pb += sizeof (TAG);
         if (!stricmp(pOinfo->szName, pszObjName) &&
             pTag->usTag == usTag)
            {
            if (usMax < pTag->cbTag)
               {
               MessageBox("Error", "%s - tag %u, buffer too small",
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


   if (!hini)
      hini = HINI_PROFILE;

   if (IsObjectAbstract(hObject))
      {
      sprintf(szObjectID, "%X", LOUSHORT(hObject));
      pClassInfo = GetProfileData(OBJECTS, szObjectID, hini, pulProfileSize);
      if (pClassInfo && *pulProfileSize > 4 && *(PULONG)pClassInfo >= *pulProfileSize)
         *(PULONG)pClassInfo = *pulProfileSize;
      return pClassInfo;
      }

   if (!IsObjectDisk(hObject))
      return NULL;

   if (!PathFromObject(hini, hObject, szPathName, sizeof szPathName))
      {
      MessageBox("GetClassInfo","Object %X does not exist!", hObject);
      return NULL;
      }

   if (strlen(szPathName) == 2)
      {
      INT    iHandle;
      ULONG  ulFileLength;

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
//         *(PULONG)pClassInfo = *pulProfileSize - 6;
         *(PULONG)pClassInfo = *pulProfileSize;
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
  //       *(PULONG)pClassInfo = *pulProfileSize - 6;
         *(PULONG)pClassInfo = *pulProfileSize;
      }
   return pClassInfo;
}
/*********************************************************************
* GetObjectID
*********************************************************************/
BOOL _System GetObjectID(PBYTE pObjectData, PSZ pszObjectID, USHORT usMax)
{
   if (GetObjectData(pObjectData, "WPObject", WPOBJECT_SZID, pszObjectID, usMax))
      return TRUE;

   return GetObjectDataString(pObjectData, "WPObject", WPOBJECT_STRINGS,
      WPOBJ_STR_OBJID, pszObjectID, usMax);
}

/*********************************************************************
* GetObjectSubString
*********************************************************************/
BOOL _System GetObjectDataString(PBYTE pObjData, PSZ pszObjName, USHORT usTag,
                         USHORT usStringTag, PVOID pOutBuf, USHORT usMax)
{
static BYTE pBuffer[1024];
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


/*********************************************************************
* Is Object Abstract
*********************************************************************/
BOOL _System IsObjectAbstract(HOBJECT hObject)
{
   if (!Is21())
      {
      if (HIUSHORT(hObject) == OBJECT_ABSTRACT)
         return TRUE;
      else
         return FALSE;
      }

   if (HIUSHORT(hObject) == OBJECT_ABSTRACT ||
       HIUSHORT(hObject) == OBJECT_ABSTRACT2)
      return TRUE;
   else
      return FALSE;
}

/*********************************************************************
* Is Object Abstract
*********************************************************************/
BOOL _System IsObjectDisk(HOBJECT hObject)
{
   if (HIUSHORT(hObject) >= OBJECT_FSYS)
      return TRUE;
   else
      return FALSE;
}

/*********************************************************************
* Make An abstract Object Handle
*********************************************************************/
HOBJECT _System MakeAbstractHandle(USHORT usObject)
{
HOBJECT hObject = MAKEULONG(usObject, OBJECT_ABSTRACT);
USHORT  usError;


   if (!Is21())
      return hObject;

   if (!WinSetObjectData(hObject, "ABCDEFG=ZYXWERZG;AUTOSETUP=YES"))
      {
      usError = ERRORIDERROR(WinGetLastError(0));
      if (usError == WPERR_INVALID_OBJECT)
         hObject = MAKEULONG(usObject, OBJECT_ABSTRACT2);
      }
   return hObject;
}


/*********************************************************************
* Make An Disk Object Handle
*********************************************************************/
HOBJECT _System MakeDiskHandle(USHORT usObject)
{
USHORT usError;
HOBJECT hObject = MAKEULONG(usObject, OBJECT_FSYS);

   return hObject;

   if (!WinSetObjectData(hObject, "ABCDEFG=ZYXWERZG;AUTOSETUP=YES"))
      {
      usError = ERRORIDERROR(WinGetLastError(0));
      if (usError == WPERR_INVALID_OBJECT)
         hObject = MAKEULONG(usObject, OBJECT_FSYS + 1);
      }

   return hObject;
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
         "Unsupported version of OS/2 (%d.%d)", (int)(_osmajor/10), (int)_osminor);
      exit(1);
      }

   if (!_osminor)
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
