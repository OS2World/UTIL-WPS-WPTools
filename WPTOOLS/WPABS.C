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
* Get Abstract options
*******************************************************************/
BOOL _System GetWPAbstractOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
BYTE szOptionText[256];

   /*
      NOTE: This value also seems te be stored in WPObject Tag 7

      Is it a template?
      if so don't write the object

   if (GetObjectValue(pObjectData, "WPAbstract", WPABSTRACT_STYLE, &ulType, sizeof ulType))
      {
      if (ulType & OBJSTYLE_TEMPLATE)
         return FALSE;
      }
   */


   /*
      Get Object Title
   */
   memset(szOptionText, 0, sizeof szOptionText);
   GetObjectValue(pObjectData, WPABSTRACT_TITLE,
      szOptionText, sizeof szOptionText);
   if (*szOptionText)
      {
      strcat(pszOptions, "TITLE=");
      ConvertTitle(pszOptions + strlen(pszOptions), OPTIONS_SIZE - strlen(pszOptions) - 1,
         szOptionText, strlen(szOptionText));
      strcat(pszOptions, ";");
      }

   return TRUE;

}

/*************************************************************
* Convert the title
*************************************************************/
VOID _System ConvertTitle(PBYTE pTarget, USHORT usTargetSize, PSZ pSrc, USHORT usSrcSize)
{
PSZ pSrcEnd = pSrc + usSrcSize;
PBYTE pTargetEnd;
PSZ pszLastNonSpace = pTarget;

   pTargetEnd = pTarget + usTargetSize;
   while (pSrc < pSrcEnd &&
          pTarget < pTargetEnd)
      {
      switch (*pSrc)
         {
         case '\n' :
         case '\r' :
            *pTarget++ = '^';
            if (*(pSrc + 1) == '\n')
               pSrc++;
            break;

         case '\"' :
            *pTarget++ = '^';
            *pTarget++ = *pSrc;
            break;

         default   :
            *pTarget++ = *pSrc;
            break;
         }
      if (*pSrc != ' ')
         pszLastNonSpace = pTarget;
      pSrc++;
      }
   *pszLastNonSpace = 0;
}
