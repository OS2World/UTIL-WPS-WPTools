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

/*******************************************************************
* Get Program options
*******************************************************************/
BOOL _System GetWPProgramFileOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
static    BYTE szDosSetting[2048];
ULONG     ulProgType=0;
BYTE      szOptionText[256];
ULONG     rgulStyle[10];
WPPGMFILEDATA wpData;
PSZ       pszProgType = "";

   /******************************
      Is it a new version ?
   ******************************/
   
   if (GetObjectValue(pObjectData,  WPPROGFILE_DATA, &wpData, sizeof wpData))
      {
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
      /*
         Get Program type
      */

      ulProgType = wpData.ulProgType;
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

      if (*pszProgType)
         {
         strcat(pszOptions, "PROGTYPE=");
         strcat(pszOptions, pszProgType);
         strcat(pszOptions, ";");
         }
      }


   /*
      Get Parameters
   */

   memset(szOptionText, 0, sizeof szOptionText);
   GetObjectValueSubValue(pObjectData,  WPPROGFILE_STRINGS,
      WPPRGFIL_STR_ARGS, szOptionText, sizeof szOptionText);
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
      WPPROGFILE_DOSSET, szDosSetting, sizeof szDosSetting);
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



