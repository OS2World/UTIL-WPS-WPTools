#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INCL_PM
#include <os2.h>
#include "wptools.h"

PBYTE pszObjClass;
PBYTE pszTitle;
PBYTE pszParms=NULL;
PBYTE pszLocation="<WP_DESKTOP>";
PSZ pszWPError(VOID);

int main(INT iArgc, PSZ pszArg[])
{
HOBJECT hObject;
PBYTE   pObjectData;
ULONG   ulProfileSize;

   if (iArgc < 3)
      {
      printf("USAGE: MakeObj ClassName Title [Settings [Location]]");
      exit(1);
      }

   pszObjClass = pszArg[1];
   pszTitle    = pszArg[2];
   if (iArgc > 3)
      pszParms = pszArg[3];
   if (iArgc > 4)
      pszLocation = pszArg[4];
   hObject = WinQueryObject(pszLocation);
   if (!hObject)
      {
      printf("Location %s does not exist!\n", pszLocation);
      exit(1);
      }
   pObjectData = GetClassInfo(HINI_PROFILE, hObject, &ulProfileSize);
   if (pObjectData)
      {
      PSZ pszClass = pObjectData + 4;
      if (stricmp(pszClass, "WPFolder") &&
          stricmp(pszClass, "WPStartup") &&
          stricmp(pszClass, "WPTemplates") &&
          stricmp(pszClass, "WPDesktop") &&
          stricmp(pszClass, "IGSFolder") &&
          stricmp(pszClass, "WPNetwork") &&
          stricmp(pszClass, "LSDirectory") &&
          stricmp(pszClass, "PRDirectory") &&
          stricmp(pszClass, "WPRootFolder"))
         {
         printf("Location should be of class WPFolder or a derived class!\n");
         printf("%s is of class %s.\n", pszLocation, pszClass);
         exit(1);
         }
      }

   hObject = WinCreateObject(pszObjClass,
      pszTitle,
      pszParms,
      pszLocation,
//      CO_UPDATEIFEXISTS);
//      CO_FAILIFEXISTS))
      CO_REPLACEIFEXISTS);
   if (!hObject)
      printf("Error while creating object! (%s)\n", pszWPError());
   else
      printf("Object created succesfully! (Handle = %lX)\n", hObject);

   return 0;
}

/*********************************************************************
*  Debug Box
*********************************************************************/
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

