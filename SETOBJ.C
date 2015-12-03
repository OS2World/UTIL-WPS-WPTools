#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INCL_PM
#include <os2.h>

PSZ pszWPError(VOID);

int main(INT iArgc, PSZ pszArg[])
{
HOBJECT hObject = 0L;
PBYTE   pbEnd;

   setlocale(LC_ALL, "");

   if (iArgc != 3)
      {
      printf("USAGE: SETOBJ ObjectId SetupString");
      printf("ObjectId can be:\n");
      printf("  A STRING like <WP_DESKTOP>; (enclosed in double quotes)\n");
      printf("  A file or directory name;\n");
      printf("  A hexadecimal number starting with 0x containing the HOBJECT of the object.\n"); 
      exit(1);
      }

   if (!memicmp(pszArg[1], "0x", 2))
      {
      hObject = strtol(pszArg[1], &pbEnd, 0);
      if (*pbEnd)
         {
         printf("Hex number is invalid\n");
         exit(1);
         }
      }
   else
      {
      WinUpper(0, 0, 0, pszArg[1]);
      hObject = WinQueryObject(pszArg[1]);
      }
   if (!hObject)
      {
      printf("Object %s not found !\n", pszArg[1]);
      exit(1);
      }

   printf("Setting %s for %s (%lX)\n",
      pszArg[2], pszArg[1], hObject);
   if (!WinSetObjectData(hObject, pszArg[2]))
      printf("Error while setting objectdata (%s)\n", pszWPError());
   else
      printf("Objectdata set!\n");
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

