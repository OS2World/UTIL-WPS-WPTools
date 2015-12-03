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
#include "wplnchpd.h"

PUBLIC PSZ _System pszOrigID(HOBJECT hObject);

PUBLIC HOBJECT _rgLaunchPadObjects[200];
PUBLIC USHORT usLPObjCount = 0;

/*******************************************************************
* Get Objectoptions
*******************************************************************/
BOOL _System GetWPLaunchPadOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
static BYTE szOptionText[1024];
ULONG ulObjCount;
ULONG rgulObjects[100];
ULONG ulValue;
ULONG ulDrawer;

   memset(_rgLaunchPadObjects, 0, sizeof _rgLaunchPadObjects);
   usLPObjCount = 0;

   /* Get Close drawer  setting */

   if (GetObjectValue(pObjectData,  501, &ulValue, sizeof ulValue))
      {
      if (ulValue)
         strcat(pszOptions, "LPCLOSEDRAWER=YES;");
      else
         strcat(pszOptions, "LPCLOSEDRAWER=NO;");
      }

   /* Get display vertical */

   if (GetObjectValue(pObjectData,  502, &ulValue, sizeof ulValue))
      {
      if (ulValue)
         strcat(pszOptions, "LPVERTICAL=YES;");
      else
         strcat(pszOptions, "LPVERTICAL=NO;");
      }


   /* Get text */

   if (GetObjectValue(pObjectData,  503, &ulValue, sizeof ulValue))
      {
      if (ulValue)
         strcat(pszOptions, "LPTEXT=YES;");
      else
         strcat(pszOptions, "LPTEXT=NO;");
      }


   /* Get small icons controls setting */

   if (GetObjectValue(pObjectData,  504, &ulValue, sizeof ulValue))
      {
      if (ulValue)
         strcat(pszOptions, "LPSMALLICONS=YES;");
      else
         strcat(pszOptions, "LPSMALLICONS=NO;");
      }

   /* Get hide frame controls setting */

   if (GetObjectValue(pObjectData,  505, &ulValue, sizeof ulValue))
      {
      if (ulValue)
         strcat(pszOptions, "LPHIDECTLS=YES;");
      else
         strcat(pszOptions, "LPHIDECTLS=NO;");
      }


   /* Get float on top setting */

   if (GetObjectValue(pObjectData,  506, &ulValue, sizeof ulValue))
      {
      if (ulValue)
         strcat(pszOptions, "LPFLOAT=YES;");
      else
         strcat(pszOptions, "LPFLOAT=NO;");
      }

   /* Get text in drawers setting */

   if (GetObjectValue(pObjectData,  507, &ulValue, sizeof ulValue))
      {
      if (ulValue)
         strcat(pszOptions, "LPDRAWERTEXT=YES;");
      else
         strcat(pszOptions, "LPDRAWERTEXT=NO;");
      }



   /* Get action buttons */

   if (GetObjectValue(pObjectData,  508, &ulValue, sizeof ulValue))
      {
      strcpy(szOptionText, "LPACTIONSTYLE=");
      switch (ulValue)
         {
         case  ACTION_BUTTONS_TEXT   :
            strcat(szOptionText, "TEXT;");
            break;

         case  ACTION_BUTTONS_OFF    :
            strcat(szOptionText, "OFF;");
            break;
         case  ACTION_BUTTONS_MINI   :
            strcat(szOptionText, "MINI;");
            break;
         default :
            strcat(szOptionText, "NORMAL;");
            break;
         }
      strcat(pszOptions, szOptionText);
      }
   

   for (ulDrawer = 0; ulDrawer < 50; ulDrawer ++)
      {
      if (!GetObjectValue(pObjectData,  ulDrawer * 2, &ulObjCount, sizeof ulObjCount))
         continue;

      if (GetObjectValue(pObjectData, ulDrawer * 2 + 1, rgulObjects, sizeof rgulObjects))
         {
         ULONG ulIndex;

            if (ulDrawer == 0)
               strcpy(szOptionText, "FPOBJECTS=");
            else
               sprintf(szOptionText, "DRAWEROBJECTS=%d,", ulDrawer);

            for (ulIndex = 0; ulIndex < ulObjCount; ulIndex++)
               {
               PSZ pszObjectID;

               pszObjectID = pszOrigID(rgulObjects[ulIndex]);
               if (pszObjectID)
                  {
                  if (usLPObjCount < 200)
                     _rgLaunchPadObjects[usLPObjCount++] = rgulObjects[ulIndex];
                  if (ulIndex > 0)
                     strcat(szOptionText, ",");
                  strcat(szOptionText, pszObjectID);
                  }
               }
            strcat(szOptionText, ";");
         }
      strcat(pszOptions, szOptionText);
      }

   return TRUE;
}


PSZ _System pszOrigID(HOBJECT hObject)
{
PBYTE pObjectData;
ULONG ulDataSize;
HOBJECT hShadow;
PSZ     pszObjectID;

   pszObjectID = NULL;
   pObjectData = GetClassInfo(NULL, hObject, &ulDataSize);
   if (pObjectData)
      {
      if (GetGenObjectValue(pObjectData, "WPShadow",
         WPSHADOW_LINK, &hShadow, sizeof hShadow))
         {
         pszObjectID = pszObjectIDFromHandle(hShadow);
         }
      free(pObjectData);
      }
   return pszObjectID;
}
