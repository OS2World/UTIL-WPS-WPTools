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
#include <os2.h>

#include "wptools.h"


#define MAX_ASSOC_SIZE 200

/*******************************************************************
* Get Program options
*******************************************************************/
BOOL _System GetWPProgramOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
static    BYTE szDosSetting[4096];
ULONG     ulProgType=0;
BYTE      szOptionText[256];
HOBJECT   hObjectPath=0;
ULONG     rgulStyle[10];
WPPGMDATA wpData;
BOOL      bNewFormat = FALSE;
PSZ       pszProgType = "";

   /******************************
      Is it a new version ?
   ******************************/
   
   if (GetObjectValue(pObjectData,  WPPROGRAM_DATA, &wpData, sizeof wpData))
      {
      bNewFormat = TRUE;
      /*
         Get program name
      */
      memset(szOptionText, 0, sizeof szOptionText);
      if (wpData.hExeHandle)
         {
         if (wpData.hExeHandle == 0xFFFF)
            strcpy(szOptionText, "*");
         else
            PathFromObject(hIniSystem,
               MakeDiskHandle(wpData.hExeHandle),
               szOptionText, sizeof szOptionText, NULL);
         }
      else
         {
         GetObjectValueSubValue(pObjectData,  WPPROGRAM_STRINGS,
            WPPGM_STR_EXENAME, szOptionText, sizeof szOptionText);
         }
      if (*szOptionText)
         {
         strcat(pszOptions, "EXENAME=");
         strcat(pszOptions, szOptionText);
         strcat(pszOptions, ";");
         }

      /*
         Get current directory
      */

      if (wpData.hCurDirHandle)
         {
         if (PathFromObject(hIniSystem,
               MakeDiskHandle(wpData.hCurDirHandle),
               szOptionText, sizeof szOptionText, NULL))
            {
            if (strlen(szOptionText) == 2)
               strcat(szOptionText, "\\");
            strcat(pszOptions, "STARTUPDIR=");
            strcat(pszOptions, szOptionText);
            strcat(pszOptions, ";");
            }
         }
      }
   else
      /******************************
         The old format
      ******************************/
      {

      /*
         Get program name
      */

      memset(szOptionText, 0, sizeof szOptionText);
      if (!GetObjectValue(pObjectData, 
         WPPROGRAM_EXENAME, szOptionText, sizeof szOptionText))
         {
         if (GetObjectValue(pObjectData, 
            WPPROGRAM_EXEHANDLE, (PBYTE)&hObjectPath, sizeof hObjectPath))
            {
            if (hObjectPath == 0xFFFF)
               strcpy(szOptionText, "*");
            else
               PathFromObject(hIniSystem,
                  MakeDiskHandle(hObjectPath),
                  szOptionText, sizeof szOptionText, NULL);
            }
         }
      if (*szOptionText)
         {
         strcat(pszOptions, "EXENAME=");
         strcat(pszOptions, szOptionText);
         strcat(pszOptions, ";");
         }

      /*
         Get CurDir
      */
      memset(szOptionText, 0, sizeof szOptionText);

      if (GetObjectValue(pObjectData, 
         WPPROGRAM_DIRHANDLE, (PBYTE)&hObjectPath, sizeof hObjectPath))
         {
         PathFromObject(hIniSystem,
            MakeDiskHandle(hObjectPath),
            szOptionText, sizeof szOptionText, NULL);
         }
      if (*szOptionText)
         {
         if (strlen(szOptionText) == 2)
            strcat(szOptionText, "\\");
         strcat(pszOptions, "STARTUPDIR=");
         strcat(pszOptions, szOptionText);
         strcat(pszOptions, ";");
         }
      }

   /*
      Get Program type
   */

   ulProgType = PROG_RESERVED;
   if (bNewFormat)
      ulProgType = wpData.ulProgType;
   else
      GetObjectValue(pObjectData, 
         WPPROGRAM_PROGTYPE, (PBYTE)&ulProgType, sizeof ulProgType);

   switch (ulProgType)
      {
      case PROG_DEFAULT       :
         break;
      case PROG_FULLSCREEN    :
         pszProgType = "FULLSCREEN";
         break;
      case PROG_WINDOWABLEVIO :    
         pszProgType = "WINDOWABLEVIO";
         break;
      case PROG_PM            :
         pszProgType = "PM";
         break;
      case PROG_VDM           :
         pszProgType="VDM";
         break;
      case PROG_WINDOWEDVDM   :
         pszProgType = "WINDOWEDVDM";
         break;
      case PROG_WINDOW_REAL   :
      case PROG_WINDOW_PROT   :
      case PROG_WINDOW_AUTO   :
         pszProgType = "WIN";
         break;
      case PROG_SEAMLESSVDM   :
         pszProgType = "SEPARATEWIN";
         break;
      case PROG_SEAMLESSCOMMON:
         pszProgType = "WINDOWEDWIN";
         break;

         /*
            Types for WIN31
         */
      case PROG_31_STDSEAMLESSVDM   :
         pszProgType = "SEPARATEWIN";
         break;
      case PROG_31_STDSEAMLESSCOMMON:
         pszProgType = "WINDOWEDWIN";
         break;
      case PROG_31_ENHSEAMLESSVDM   :
         pszProgType = "PROG_31_ENHSEAMLESSVDM";
         break;
      case PROG_31_ENHSEAMLESSCOMMON:
         pszProgType = "PROG_31_ENHSEAMLESSCOMMON";
         break;
      case PROG_31_ENH              :
         pszProgType = "PROG_31_ENH";
         break;
      case PROG_31_STD:
         pszProgType = "WIN";
         break;
      default :
         break;
      }

//   sprintf(szOptionText,"%s:%d", pszProgType, ulProgType); 
//   pszProgType = szOptionText;

   if (*pszProgType)
      {
      strcat(pszOptions, "PROGTYPE=");
      strcat(pszOptions, pszProgType);
      strcat(pszOptions, ";");
      }

   /*
      Get Parameters
   */

   memset(szOptionText, 0, sizeof szOptionText);
   if (bNewFormat)
      {
      GetObjectValueSubValue(pObjectData,  WPPROGRAM_STRINGS,
         WPPGM_STR_ARGS, szOptionText, sizeof szOptionText);
      }
   else
      {
      GetObjectValue(pObjectData, 
         WPPROGRAM_PARAMS, szOptionText, sizeof szOptionText);
      }
   if (*szOptionText)
      {
      strcat(pszOptions, "PARAMETERS=");
      strcat(pszOptions, szOptionText);
      strcat(pszOptions, ";");
      }

   /*
      Get DOS Settings
   */

   memset(szOptionText, 0, sizeof szOptionText);
   GetObjectValue(pObjectData, 
      WPPROGRAM_DOSSET, szDosSetting, sizeof szDosSetting);
   if (*szDosSetting)
      {
      PBYTE p = szDosSetting;
      while (*p)
         {
         strcat(pszOptions, "SET ");
         ConvertDosSetting(pszOptions + strlen(pszOptions),
            OPTIONS_SIZE - strlen(pszOptions) - 1, p, strlen(p));
         p+= strlen(p) + 1;
         strcat(pszOptions, ";");
         }
      }

   /*
      Get Autoclose
   */
   if (GetObjectValue(pObjectData, 
      WPPROGRAM_STYLE, rgulStyle, sizeof rgulStyle))
      {
      if (rgulStyle[0] & 0x8000)
         strcat(pszOptions, "NOAUTOCLOSE=YES;");
      if (rgulStyle[0] & 0x0400)
         strcat(pszOptions, "MINIMIZED=YES;");
      if (rgulStyle[0] & 0x0800)
         strcat(pszOptions, "MAXIMIZED=YES;");
      }

   /*
      Get Assoc Filter
   */
   if (GetAssocFilters(hObject, szOptionText, sizeof szOptionText))
      {
      strcat(pszOptions, "ASSOCFILTER=");
      strcat(pszOptions, szOptionText);
      strcat(pszOptions, ";");
      }

   /*
      Get Assoc Type
   */
   if (GetAssocTypes(hObject, szOptionText, sizeof szOptionText))
      {
      strcat(pszOptions, "ASSOCTYPE=");
      strcat(pszOptions, szOptionText);
      strcat(pszOptions, ";");
      }

   return TRUE;
   return GetWPAbstractOptions(hObject, pszOptions, pObjectData);

}

/*************************************************************
* Convert the DosSettings
*************************************************************/
VOID _System ConvertDosSetting(PBYTE pTarget, USHORT usTargetSize, PSZ pSrc, USHORT usSrcSize)
{
PSZ pSrcEnd = pSrc + usSrcSize;
PBYTE pTargetEnd = pTarget + usTargetSize;

   while (pSrc < pSrcEnd &&
          pTarget < pTargetEnd)
      {
      switch (*pSrc)
         {
         case '\n' :
         case '\r' :
            *pTarget++ = ',';
            if (*(pSrc + 1) == '\n')
               pSrc++;
            break;

         case ';'  :
         case ','  :
            *pTarget++ = '^';
            *pTarget++ = *pSrc;
            break;

         default   :
            *pTarget++ = *pSrc;
            break;
         }
      pSrc++;
      }
   *pTarget = 0;
}


/*****************************************************************
* Get the assoc filters for an object
*****************************************************************/
BOOL _System GetAssocFilters(HOBJECT hObject, PSZ pszAssoc, USHORT usMax)
{
PBYTE pFilterArray;
PBYTE pAssocArray;
ULONG ulProfileSize;
PBYTE  pFilter;
PBYTE  pAssoc;
HINI  hini = hIniUser;
BOOL  bRetco = FALSE;


   memset(pszAssoc, 0, usMax);

   /*
      Get the types for the first time
   */
   pFilterArray = GetAllProfileNames(ASSOC_FILTER, hini, &ulProfileSize);
   if (!pFilterArray)
      return FALSE;

   pFilter = pFilterArray;
   while (*pFilter)
      {
      pAssocArray =
         GetProfileData(ASSOC_FILTER, pFilter, hini,&ulProfileSize);
      if (pAssocArray)
         {
         pAssoc = pAssocArray;
         while (pAssoc < pAssocArray + ulProfileSize)
            {
            if (atol(pAssoc) == hObject)
               {
               if (strlen(pszAssoc) < MAX_ASSOC_SIZE)
                  {
                  if (strlen(pszAssoc))
                     strcat(pszAssoc, ",");
                  strcat(pszAssoc, pFilter);
                  bRetco = TRUE;
                  }
               }
            pAssoc += strlen(pAssoc) + 1;
            }
         free(pAssocArray);
         }
      pFilter += strlen(pFilter) + 1;
      }

   if (bRetco)
      strcat(pszAssoc, ",,");

   free(pFilterArray);
   return bRetco;
}

/*****************************************************************
* Get the assoc types for an object
*****************************************************************/
BOOL _System GetAssocTypes(HOBJECT hObject, PSZ pszAssoc, USHORT usMax)
{
PBYTE pTypeArray;
PBYTE pType;
PBYTE pAssocArray;
PBYTE pAssoc;
ULONG ulProfileSize;
HINI  hini = hIniUser;
BOOL  bRetco = FALSE;


   memset(pszAssoc, 0, usMax);

   /*
      Get the types for the first time
   */
   pTypeArray = GetAllProfileNames(ASSOC_TYPE, hini, &ulProfileSize);
   if (!pTypeArray)
      return FALSE;

   pType = pTypeArray;
   while (*pType)
      {
      pAssocArray =
         GetProfileData(ASSOC_TYPE, pType, hini,&ulProfileSize);
      if (pAssocArray)
         {
         pAssoc = pAssocArray;
         while (pAssoc < pAssocArray + ulProfileSize)
            {
            if (atol(pAssoc) == hObject)
               {
               if (strlen(pszAssoc) < MAX_ASSOC_SIZE)
                  {
                  if (strlen(pszAssoc))
                     strcat(pszAssoc, ",");
                  strcat(pszAssoc, pType);
                  bRetco = TRUE;
                  }
               }
            pAssoc += strlen(pAssoc) + 1;
            }
         free(pAssocArray);
         }
      pType += strlen(pType) + 1;
      }

   if (bRetco)
      strcat(pszAssoc, ",,");

   free(pTypeArray);
   return bRetco;
}

