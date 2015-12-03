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
#include <stdarg.h>

#define INCL_VIO
#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_GPI
#define INCL_REXXSAA

#include <os2.h>
#include <rexxsaa.h>

#include "wptools.h"


typedef struct _FuncArray
{
PSZ   pszClass;
PSZ   pszName;
BOOL  (* _System pfnGetSet)(HOBJECT, PSZ, PBYTE);
} FARRAY, *PFARRAY;

PRIVATE FARRAY rgFarray[] =
{
{NULL             , "PrintDest"    , GetPrintDestOptions },
{NULL             , "WPProgramRef" , GetWPProgramOptions },
{NULL             , "WPProgramFile", GetWPProgramFileOptions },
{NULL             , "WPUrl"        , GetWPUrlOptions },
{NULL             , "WPHost"       , GetWPHostOptions },
{NULL             , "WPFolder"     , GetWPFolderOptions  },
{NULL             , "WPDataFile"   , GetWPDataFileOptions  },
{NULL             , "WPShadow"     , GetWPShadowOptions },
{NULL             , "WPAbstract"   , GetWPAbstractOptions },
{NULL             , "WPFileSystem" , GetWPFsysOptions },
{NULL             , "WPObject"     , GetWPObjectOptions },
{NULL             , "WPRPrinter"   , GetWPRPrinterOptions },
{NULL             , "WPNetLink"    , GetWPRPrinterOptions },
{NULL             , "WPServer"     , GetWPRPrinterOptions },
{NULL             , "WPNetgrp"     , GetWPRPrinterOptions },
{NULL             , "WPDisk"       , GetWPDiskOptions },
{NULL             , "WPPalette"    , GetWPPaletteOptions },
{NULL             , "WPLaunchPad"  , GetWPLaunchPadOptions },
{NULL             , "SC01"         , GetSCenterOptions },
{NULL             , "WPDesktop"    , GetWPDesktopOptions },

{"WPFontPalette"  , "WPPalette"    , GetWPFontPaletteOptions },
{"WPColorPalette" , "WPPalette"    , GetWPColorPaletteOptions },

{NULL}
};
static  BYTE    szOptions [OPTIONS_SIZE + 200];

static INT SetRexxVariable(PSZ name, PSZ value, INT iStem);
static INT MySprintf(PSZ pszBuffer, PSZ pszFormat, ...);

PSZ _System GetWPToolsVersion(VOID)
{
static BYTE szVersion[50];

   strcpy(szVersion, VERSION);
   return szVersion;
}

/***********************************************************************
*
***********************************************************************/
PSZ _System pszGetObjectSettings(HOBJECT hObject, PSZ pszCls, USHORT usMax, BOOL fAddComment)
{
PBYTE   pObjectData,
        pObjectEnd;
ULONG   ulProfileSize;
PSZ     pszRetco = szOptions;
PSZ     pszClass;
BYTE    szLocation[256];
PBYTE   pb;

   memset(szOptions, 0, sizeof szOptions);
   if (fAddComment)
      ResetBlockBuffer();

   pObjectData = GetClassInfo(NULL, hObject, &ulProfileSize);
   if (!pObjectData)
      {
      if (!fAddComment)
         return NULL;
      sprintf(szOptions, "OBJECT %5.5X : No classdata found!",
         hObject);
      return szOptions;
      }
   pObjectEnd = pObjectData + *(PULONG)pObjectData;

   pszClass   = pObjectData + 4;
   if (pszCls)
      strncpy(pszCls, pszClass, usMax);

   if (fAddComment)
      sprintf(szOptions, "Class: %s\n", pszClass);

   pb = pszClass + strlen(pszClass) + 1 + 16;
   while (pb < pObjectEnd)
      {
      POINFO pOinfo = (POINFO)pb;
      PFARRAY pFar = rgFarray;

      if (fAddComment)
         sprintf(szOptions + strlen(szOptions),
            "Classdata from %s:\n", pOinfo->szName);

      while (pFar->pszName)
         {
         if ((!pFar->pszClass || !strcmp(pFar->pszClass, pszClass)) &&
             !strcmp(pOinfo->szName, pFar->pszName))
            {
            if (!(*pFar->pfnGetSet)(hObject, szOptions, (PBYTE)pOinfo))
               pszRetco = NULL;
            }
         pFar++;
         }
      pb += sizeof(OINFO) + strlen(pOinfo->szName) + pOinfo->cbData;
      }
   free(pObjectData);

   if (fAddComment)
      {
      memset(szLocation, 0, sizeof szLocation);
      GetObjectLocation(hObject, szLocation, sizeof szLocation);
      sprintf(szOptions+strlen(szOptions), "Location: %s", szLocation);
      }

   return szOptions;
}

BOOL _System fGetObjectClass(HOBJECT hObject, PSZ pszClass, USHORT usMax)
{
PBYTE pObjectData;
ULONG ulProfileSize;

   pObjectData = GetClassInfo(NULL, hObject, &ulProfileSize);
   if (!pObjectData)
      return FALSE;
   strncpy(pszClass, pObjectData + 4, usMax);
   free(pObjectData);
   return TRUE;
}

/***********************************************************************
*
***********************************************************************/
ULONG _System WPToolsQueryObject(PUCHAR pszFuncName,
                               ULONG ulArgc,
                               RXSTRING rgpxArg[],
                               PSZ pszQueue,
                               PRXSTRING pxReturn)
{
ULONG ulRetco = RXFUNC_OK;
HOBJECT hObject;
PBYTE   pObjectData,
        pObjectEnd;
ULONG   ulProfileSize;
PSZ     pszClass;
PBYTE   pb;

   strcpy(pxReturn->strptr, "0");
   pxReturn->strlength = 1;

   if (ulArgc < 1)
      return 40;

   if (!RXVALIDSTRING((rgpxArg[0])))
      return 40;

   if (rgpxArg[0].strptr[0] == '#')
      hObject = strtol(&rgpxArg[0].strptr[1], &pb, 16);
   else
      hObject = WinQueryObject(rgpxArg[0].strptr);
   if (!hObject)
      return 0;

   memset(szOptions, 0, sizeof szOptions);
   ResetBlockBuffer();

   pObjectData = GetClassInfo(NULL, hObject, &ulProfileSize);
   if (!pObjectData)
      return 0;

   pObjectEnd = pObjectData + *(PULONG)pObjectData;
   pszClass   = pObjectData + 4;

   pb = pszClass + strlen(pszClass) + 1 + 16;
   while (pb < pObjectEnd)
      {
      POINFO pOinfo = (POINFO)pb;
      PFARRAY pFar = rgFarray;

      while (pFar->pszName)
         {
         if ((!pFar->pszClass || !strcmp(pFar->pszClass, pszClass)) &&
             !strcmp(pOinfo->szName, pFar->pszName))
            (*pFar->pfnGetSet)(hObject, szOptions, (PBYTE)pOinfo);
         pFar++;
         }
      pb += sizeof(OINFO) + strlen(pOinfo->szName) + pOinfo->cbData;
      }

   if (ulArgc > 1 && RXVALIDSTRING(rgpxArg[1]))
      SetRexxVariable(rgpxArg[1].strptr, pszClass, -1);

   if (ulArgc > 2 && RXVALIDSTRING(rgpxArg[2]))
      {
      BYTE    szTitle[256];
      PSZ     p;

      memset(szTitle, 0, sizeof szTitle);
      p = strstr(szOptions, "TITLE=");
      if (p)
         {
         strncpy(szTitle, p + 6, sizeof szTitle);
         p = strchr(szTitle, ';');
         if (p)
            *p = 0;
         }
      SetRexxVariable(rgpxArg[2].strptr, szTitle, -1);
      }

   if (ulArgc > 3 && RXVALIDSTRING(rgpxArg[3]))
      SetRexxVariable(rgpxArg[3].strptr, szOptions, -1);

   if (ulArgc > 4 && RXVALIDSTRING(rgpxArg[4]))
      {
      BYTE szLocation[256];
      memset(szLocation, 0, sizeof szLocation);

      GetObjectLocation(hObject, szLocation, sizeof szLocation);
      SetRexxVariable(rgpxArg[4].strptr, szLocation, -1);
      }

   strcpy(pxReturn->strptr, "1");
   pxReturn->strlength = 1;

   free(pObjectData);
   return ulRetco;
}

/*********************************************************************/
/*                                                                   */
/* SetRexxVariable - Set the value of a REXX variable                */
/*                                                                   */
/*********************************************************************/
INT SetRexxVariable(PSZ name, PSZ value, INT iStem)
{
SHVBLOCK   block;                    /* variable pool control block*/
BYTE       szName[256];

   strcpy(szName, name);
   if (iStem != -1)
      {
      if (szName[strlen(szName) - 1] == '.')
         szName[strlen(szName) - 1] = 0;
      sprintf(szName + strlen(szName), ".%u", iStem);
      }

   block.shvcode = RXSHV_SYSET;         /* do a symbolic set operation*/
   block.shvret=(UCHAR)0;               /* clear return code field    */
   block.shvnext=(PSHVBLOCK)0;          /* no next block              */
                                          /* set variable name string   */
   MAKERXSTRING(block.shvname, szName, strlen(szName));
                                          /* set value string           */
   MAKERXSTRING(block.shvvalue, value, strlen(value));
   block.shvvaluelen=strlen(value);     /* set value length           */

   return RexxVariablePool(&block);     /* set the variable           */
}

/***********************************************************************
*
***********************************************************************/
IMPORT ULONG _System WPToolsUnloadByTrap(PUCHAR pszFuncName,
                                  ULONG ulArgc,
                                  RXSTRING rgpxArg[],
                                  PSZ pszQueue,
                                  PRXSTRING pxReturn)
{
   strcpy(rgpxArg[3].strptr, "TRAPJE");
   return 0;
}

/***********************************************************************
*
***********************************************************************/
IMPORT ULONG _System WPToolsFolderContent(PUCHAR pszFuncName,
                                    ULONG ulArgc,
                                    RXSTRING rgpxArg[],
                                    PSZ pszQueue,
                                    PRXSTRING pxReturn)
{
HOBJECT hObject;
USHORT usObjID;
USHORT usObjType;
PULONG prgulObjects;
ULONG  ulProfileSize;
USHORT usIndex;
BYTE   szObject[15];
BOOL   fIncludeFiles = FALSE;
static BYTE szPath[CCHMAXPATH];

   strcpy(pxReturn->strptr, "0");
   pxReturn->strlength = 1;

   if (ulArgc < 2)
      return 40;

   if (!RXVALIDSTRING((rgpxArg[0])))
      return 40;

   if (!RXVALIDSTRING((rgpxArg[1])))
      return 40;

   if (ulArgc == 3)
      {
      if (!RXVALIDSTRING((rgpxArg[2])))
         return 40;
      if (rgpxArg[2].strptr[0] == 'f' ||
          rgpxArg[2].strptr[0] == 'F' )
         fIncludeFiles = TRUE;
      }


   hObject = WinQueryObject(rgpxArg[0].strptr);
   if (!hObject)
      return 0;

   usObjID = LOUSHORT(hObject);
   usObjType = HIUSHORT(hObject);
   if (!IsObjectDisk(hObject))
      return 0;

   sprintf(szObject, "%X", usObjID);
   prgulObjects = GetProfileData(FOLDERCONTENT, szObject, hIniUser, &ulProfileSize);
   if (!prgulObjects)
      ulProfileSize = 0;
   ulProfileSize /= 4;
   sprintf(szObject, "%u", ulProfileSize);
   SetRexxVariable(rgpxArg[1].strptr, szObject, 0);

   for (usIndex = 0; usIndex < ulProfileSize; usIndex++)
      {
      PSZ pszObjectID = pszObjectIDFromHandle(MakeAbstractHandle((USHORT)prgulObjects[usIndex]));
      if (!pszObjectID)
         {
         sprintf(szObject, "#%5X", MakeAbstractHandle((USHORT)prgulObjects[usIndex]));
         SetRexxVariable(rgpxArg[1].strptr, szObject, usIndex+1);
         }
      else
         SetRexxVariable(rgpxArg[1].strptr, pszObjectID, usIndex+1);
      }
   if (prgulObjects)
      free(prgulObjects);

   if (fIncludeFiles)
      {
      if (WinQueryObjectPath(hObject, szPath, sizeof szPath))
         {
         static BYTE szFile[CCHMAXPATH];
         APIRET rc;
         ULONG ulFindCount;
         HDIR hFind;
         FILEFINDBUF3 tFind;
         HOBJECT hFound;

         strcpy(szFile, szPath);
         if (szFile[strlen(szFile)-1] != '\\')
            strcat(szFile, "\\");
         strcat(szFile, "*.*");

         ulFindCount = 1;
         hFind = HDIR_CREATE;

         rc =  DosFindFirst(szFile,
            &hFind,
            FILE_NORMAL|FILE_DIRECTORY|FILE_SYSTEM|FILE_HIDDEN|FILE_READONLY,
            &tFind,
            sizeof (FILEFINDBUF3), &ulFindCount, FIL_STANDARD);
         
         while (!rc)
            {
            if (strcmp(tFind.achName, ".") && strcmp(tFind.achName,".."))
               {
               strcpy(szFile, szPath);
               if (szFile[strlen(szFile)-1] != '\\')
                  strcat(szFile, "\\");
               strcat(szFile, tFind.achName);
               hFound = WinQueryObject(szFile);
               if (hFound)
                  {
                  PSZ pszObjectID = pszObjectIDFromHandle(hFound);

                  if (!pszObjectID || *pszObjectID != '<')
                     {
                     sprintf(szObject, "#%5X", hFound);
                     SetRexxVariable(rgpxArg[1].strptr, szObject, usIndex+1);
                     }
                  else
                     SetRexxVariable(rgpxArg[1].strptr, pszObjectID, usIndex+1);
                  usIndex++;
                  }
               }
            ulFindCount = 1;
            rc = DosFindNext(hFind,
               &tFind,
               sizeof (FILEFINDBUF3),
               &ulFindCount);
            }
         if (rc && rc != 18)
            MessageBox("WPTOOLS",
            "DosFindFirst/Next returned %u for %s",
            rc, szPath);

         DosFindClose(hFind);
         }
      else
         MessageBox("WPTOOLS",
         "ERROR: WinQueryObjectPath for %lX failed!",
         hObject);
      }

   sprintf(szObject, "%u", usIndex);
   SetRexxVariable(rgpxArg[1].strptr, szObject, 0);

   strcpy(pxReturn->strptr, "1");
   pxReturn->strlength = 1;
   return 0;
}

/***********************************************************************
*
***********************************************************************/
IMPORT ULONG _System WPToolsVersion(PUCHAR pszFuncName,
                                    ULONG ulArgc,
                                    RXSTRING rgpxArg[],
                                    PSZ pszQueue,
                                    PRXSTRING pxReturn)
{
   strcpy(pxReturn->strptr, VERSION);
   pxReturn->strlength = strlen(pxReturn->strptr);
   return 0;
}

/***********************************************************************
*
***********************************************************************/
ULONG _System WPToolsSetObjectData(PUCHAR pszFuncName,
                               ULONG ulArgc,
                               RXSTRING rgpxArg[],
                               PSZ pszQueue,
                               PRXSTRING pxReturn)
{
HOBJECT hObject;
BOOL    rc;
PBYTE   pb;


   strcpy(pxReturn->strptr, "0");
   pxReturn->strlength = 1;
   if (ulArgc < 2)
      return 40;

   if (!RXVALIDSTRING((rgpxArg[0])))
      return 40;

   if (!RXVALIDSTRING((rgpxArg[1])))
      return 40;

   if (rgpxArg[0].strptr[0] == '#')
      hObject = strtol(&rgpxArg[0].strptr[1], &pb, 16);
   else
      hObject = WinQueryObject(rgpxArg[0].strptr);
   if (!hObject)
      return 0;

   rc = WinSetObjectData(hObject, rgpxArg[1].strptr);
   if (!rc)
      return 0;

   strcpy(pxReturn->strptr, "1");
   pxReturn->strlength = 1;
   return 0;
}
/***********************************************************************
*
***********************************************************************/
IMPORT ULONG _System WPToolsLoadFuncs(PUCHAR pszFuncName,
                               ULONG ulArgc,
                               RXSTRING rgpxArg[],
                               PSZ pszQueue,
                               PRXSTRING pxReturn)

{
   if (ulArgc > 0)
      return 40;


   RexxRegisterFunctionDll("WPToolsQueryObject"  , "WPTOOLS", "WPToolsQueryObject"  );
   RexxRegisterFunctionDll("WPToolsFolderContent", "WPTOOLS", "WPToolsFolderContent");
   RexxRegisterFunctionDll("WPToolsUnloadByTrap" , "WPTOOLS", "WPToolsUnloadByTrap" );
   RexxRegisterFunctionDll("WPToolsVersion"      , "WPTOOLS", "WPToolsVersion" );
   RexxRegisterFunctionDll("WPToolsSetObjectData", "WPTOOLS", "WPToolsSetObjectData" );


   strcpy(pxReturn->strptr, "1");
   pxReturn->strptr[1] = 0;
   pxReturn->strlength = 1;
   return 0;
}

#define STEP 16
#define TAG_LONG        2
#define TAG_TEXT        3
#define TAG_BLOCK       4

/****************************************************
*
*****************************************************/
PSZ _System DumpObjectData(HOBJECT hObject)
{
PBYTE pb, pBufEnd, p;
USHORT usIndex;
ULONG ulObjSize;
BYTE  szBlockText[STEP+1];
ULONG ulProfileSize;
PBYTE pObjectData;
PSZ   pTarget;

   memset(szOptions, 0, sizeof szOptions);

   pObjectData = GetClassInfo(NULL, hObject, &ulProfileSize);
   if (!pObjectData)
      {
      sprintf(szOptions, "OBJECT %5.5X : No classdata found!",
         hObject);
      return szOptions;
      }

   pTarget = szOptions;

   ulObjSize = *(PULONG)pObjectData;
   pTarget += MySprintf(pTarget, "CLASS: %s\n", pObjectData + sizeof (ULONG));

   pb = pObjectData + sizeof (ULONG);
   pb += strlen(pb) + 1;

   for (usIndex = 0; usIndex < 16; usIndex++)
      {
      pTarget += MySprintf(pTarget, "%2.2X ", (USHORT)(*pb));
      pb++;
      }
   pTarget += MySprintf(pTarget, "\n");

   pBufEnd = pObjectData + ulObjSize;
   while (pb < pBufEnd)
      {
      POINFO pOinfo;
      PBYTE  pbGroepBufEnd;

      pOinfo = (POINFO)pb;
      pb += sizeof(OINFO) + strlen(pOinfo->szName);

      pTarget += MySprintf(pTarget, "\nCLASSDATA : %s ", pOinfo->szName);
      pTarget += MySprintf(pTarget, "LenData  : %2d\n", pOinfo->cbData);

      pbGroepBufEnd = pb + pOinfo->cbData;
      while (pb < pbGroepBufEnd)
         {
         PTAG pTag   = (PTAG)pb;
         pb += sizeof (TAG);
         pTarget += MySprintf(pTarget, "Tag %4d (%2d bytes):", pTag->usTag, pTag->cbTag);
         switch(pTag->usTagFormat)
            {
            case TAG_TEXT:
               pTarget += MySprintf(pTarget, "Text : %s\n", pb);
               break;
            case TAG_LONG:
               pTarget += MySprintf(pTarget, "Long : %8.8X", *(PULONG)pb);
               pTarget += MySprintf(pTarget, "\n");
               break;
            case TAG_BLOCK:
               pTarget += MySprintf(pTarget, "Block:\n");
               memset(szBlockText, 0, sizeof szBlockText);
               usIndex = 0;
               for (p = pb; p < pb + pTag->cbTag; p++)
                  {
                  if (usIndex >= STEP)
                     {
                     pTarget += MySprintf(pTarget, "  %s\n", szBlockText);
                     memset(szBlockText, 0, sizeof szBlockText);
                     usIndex = 0;
                     }
                  pTarget += MySprintf(pTarget, "%2.2X ",(USHORT)*p);
                  if (*p == '\a' || *p == '\n' || *p == '\r' || *p == 0)
                     szBlockText[usIndex] = '.';
                  else
                     szBlockText[usIndex] = *p;
                  usIndex++;
                  }
               if (usIndex)
                  {
                  while (usIndex < STEP)
                     {
                     pTarget += MySprintf(pTarget, "   ");
                     usIndex++;
                     }
                  pTarget += MySprintf(pTarget, "  %s\n", szBlockText);
                  }
               break;
            default :
               pTarget += MySprintf(pTarget, "Unknown tagformat %d\n", pTag->usTagFormat);
               break;
            }
         pb += pTag->cbTag;
         }
      }

   pTarget += MySprintf(pTarget, "Additional data:\n");
   memset(szBlockText, 0, sizeof szBlockText);
   usIndex = 0;
   pBufEnd = pObjectData + ulProfileSize;
   for (p = pb; p < pBufEnd; p++)
      {
      if (usIndex >= STEP)
         {
         pTarget += MySprintf(pTarget, "  %s\n", szBlockText);
         memset(szBlockText, 0, sizeof szBlockText);
         usIndex = 0;
         }
      pTarget += MySprintf(pTarget, "%2.2X ",(USHORT)*p);
      if (*p == '\a' || *p == '\n' || *p == '\r' || *p == 0)
         szBlockText[usIndex] = '.';
      else
         szBlockText[usIndex] = *p;
      usIndex++;
      }
   if (usIndex)
      {
      while (usIndex < STEP)
         {
         pTarget += MySprintf(pTarget, "   ");
         usIndex++;
         }
      pTarget += MySprintf(pTarget, "  %s\n", szBlockText);
      }
   pTarget += MySprintf(pTarget, "\n");

   free(pObjectData);
   return szOptions;

}

INT MySprintf(PSZ pszBuffer, PSZ pszFormat, ...)
{
va_list va;
static BOOL fBufferFull = FALSE;

   if (pszBuffer == szOptions)
      fBufferFull =  FALSE;

   if (pszBuffer - szOptions > OPTIONS_SIZE)
      {
      if (!fBufferFull)
         strcat(pszBuffer, "*** INTERNAL BUFFER FULL ***");
      fBufferFull = TRUE;
      return 0;
      }


   va_start(va, pszFormat);

   return vsprintf(pszBuffer, pszFormat, va);

}

