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

#include <wpfolder.h>

#include "wptools.h"
/*******************************************************************
* Get Objectoptions
*******************************************************************/
BOOL _System GetWPObjectOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
static BYTE  szTest[1024];
BYTE szOptionText[256];
ULONG ulObjectStyle;
ULONG ulHelpPanel;
ULONG ulMinWin;
ULONG ulConcurrent;
ULONG ulViewButton;
WPOBJDATA wpData;
BOOL      bNewFormat = FALSE;

   memset(&wpData, 0, sizeof wpData);
   if (GetObjectValue(pObjectData, WPOBJECT_STRINGS, szTest, sizeof szTest))
      {
      if (GetObjectValue(pObjectData, WPOBJECT_DATA, &wpData, sizeof wpData))
         bNewFormat = TRUE;
      }


   /*
      Get object style
   */
   ulObjectStyle = 0;

   if (bNewFormat)
      ulObjectStyle = wpData.ulObjectStyle;
   else
      GetObjectValue(pObjectData, WPOBJECT_STYLE, &ulObjectStyle, sizeof ulObjectStyle);

   if (ulObjectStyle & OBJSTYLE_NOMOVE)
      strcat(pszOptions, "NOMOVE=YES;");

   if (ulObjectStyle & OBJSTYLE_NOLINK)
      strcat(pszOptions, "NOLINK=YES;");

   if (ulObjectStyle & OBJSTYLE_NOCOPY)
      strcat(pszOptions, "NOCOPY=YES;");

   if (ulObjectStyle & OBJSTYLE_TEMPLATE)
      strcat(pszOptions, "TEMPLATE=YES;");

   if (ulObjectStyle & OBJSTYLE_NODELETE)
      strcat(pszOptions, "NODELETE=YES;");

   if (ulObjectStyle & OBJSTYLE_NOPRINT)
      strcat(pszOptions, "NOPRINT=YES;");

   if (ulObjectStyle & OBJSTYLE_NODRAG)
      strcat(pszOptions, "NODRAG=YES;");

   if (ulObjectStyle & OBJSTYLE_NOTVISIBLE)
      strcat(pszOptions, "NOTVISIBLE=YES;");

   if (ulObjectStyle & OBJSTYLE_NORENAME) 
      strcat(pszOptions, "NORENAME=YES;");

   if (ulObjectStyle & OBJSTYLE_NOSETTINGS) 
      strcat(pszOptions, "NOSETTINGS=YES;");

   if ((ulObjectStyle & OBJSTYLE_NODROP) ||
      (ulObjectStyle & OBJSTYLE_NODROPON))
      strcat(pszOptions, "NODROP=YES;");

   if (IsWarp4())
      {
      if (ulObjectStyle & 0x00020000)
         strcat(pszOptions, "LOCKEDINPLACE=YES;");
      if (!wpData.ulMenuFlag)
         strcat(pszOptions, "MENUS=SHORT;");
      if (wpData.ulMenuFlag & 0x00000001)
         strcat(pszOptions, "MENUS=LONG;");
      if (wpData.ulMenuFlag & 0x00000002)
         strcat(pszOptions, "MENUS=DEFAULT;");

      }


   /*
      Get Help panel
   */

   ulHelpPanel = 0;
   if (bNewFormat)
      ulHelpPanel = wpData.ulHelpPanel;
   else
      GetObjectValue(pObjectData, WPOBJECT_HELPPANEL,
         &ulHelpPanel, sizeof ulHelpPanel);

   if (ulHelpPanel)
      {
      sprintf(szOptionText, "%ld", ulHelpPanel);
      strcat(pszOptions, "HELPPANEL=");
      strcat(pszOptions, szOptionText);
      strcat(pszOptions, ";");
      }

   /*
      Get ViewButton options
   */
   memset(szOptionText, 0, sizeof szOptionText);
   ulViewButton = DEFAULTBUTTON;
   if (bNewFormat)
      ulViewButton = wpData.ulViewButton;
   else
      GetObjectValue(pObjectData, WPOBJECT_VIEWBUTTON,
      &ulViewButton, sizeof ulViewButton);
   
   switch (ulViewButton)
      {
      case HIDEBUTTON :
         strcpy(szOptionText, "YES");
         break;
      case MINBUTTON  :
         strcpy(szOptionText, "NO");
         break;
      default:
         strcpy(szOptionText, "DEFAULT");
         break;
      }
   strcat(pszOptions, "HIDEBUTTON=");
   strcat(pszOptions, szOptionText);
   strcat(pszOptions, ";");


   /*
      Get Minimized windows options
   */
   memset(szOptionText, 0, sizeof szOptionText);
   ulMinWin = MINWIN_DEFAULT;
   if (bNewFormat)
      ulMinWin = wpData.ulMinWin;
   else
      GetObjectValue(pObjectData, WPOBJECT_MINWIN,
         &ulMinWin, sizeof ulMinWin);

   switch (ulMinWin)
      {
      case MINWIN_HIDDEN  :
         strcpy(szOptionText, "HIDE");
         break;
      case MINWIN_VIEWER  :
         strcpy(szOptionText, "VIEWER");
         break;
      case MINWIN_DESKTOP :
         strcpy(szOptionText, "DESKTOP");
         break;
      default:
         strcpy(szOptionText, "DEFAULT");
         break;
      }
   strcat(pszOptions, "MINWIN=");
   strcat(pszOptions, szOptionText);
   strcat(pszOptions, ";");

   /*
      Get Concurent view options
   */
   memset(szOptionText, 0, sizeof szOptionText);
   ulConcurrent = CCVIEW_DEFAULT;
   if (bNewFormat)
      ulConcurrent = wpData.ulConcurrent;
   else
      GetObjectValue(pObjectData, WPOBJECT_CONCURRENT,
         &ulConcurrent, sizeof ulConcurrent);

   switch (ulConcurrent)
      {
      case CCVIEW_ON  :
         strcpy(szOptionText, "YES");
         break;
      case CCVIEW_OFF :
         strcpy(szOptionText, "NO");
         break;
      default:
         strcpy(szOptionText, "DEFAULT");
         break;
      }
   strcat(pszOptions, "CCVIEW=");
   strcat(pszOptions, szOptionText);
   strcat(pszOptions, ";");

   /*
      Get defaultview
   */

   memset(szOptionText, 0, sizeof szOptionText);
   if (bNewFormat)
      {
      switch (wpData.lDefaultView)
         {
         case OPEN_SETTINGS:
            strcpy(szOptionText, "SETTINGS");
            break;
         case OPEN_CONTENTS:
            strcpy(szOptionText, "ICON");
            break;
         case OPEN_TREE:
            strcpy(szOptionText, "TREE");
            break;
         case OPEN_DETAILS:
            strcpy(szOptionText, "DETAILS");
            break;
         default          :
            if (wpData.lDefaultView > 256L)
               sprintf(szOptionText, "%ld", wpData.lDefaultView);
            else
               strcpy(szOptionText, "DEFAULT");
            break;
         }
      strcat(pszOptions, "DEFAULTVIEW=");
      strcat(pszOptions, szOptionText);
      strcat(pszOptions, ";");
      }


   /*
      Get Object ID
   */
   memset(szOptionText, 0, sizeof szOptionText);
   if (ObjectIDFromData(pObjectData, szOptionText, sizeof szOptionText))
      {
      strcat(pszOptions, "OBJECTID=");
      strcat(pszOptions, szOptionText);
      strcat(pszOptions, ";");
      }

   return TRUE;

}

/*********************************************************************
* ObjectIDFromData
*********************************************************************/
BOOL _System ObjectIDFromData(PBYTE pObjectData, PSZ pszObjectID, USHORT usMax)
{
   if (GetObjectValue(pObjectData, WPOBJECT_SZID, pszObjectID, usMax))
      return TRUE;

   return GetObjectValueSubValue(pObjectData, WPOBJECT_STRINGS,
      WPOBJ_STR_OBJID, pszObjectID, usMax);

   return FALSE;
}

