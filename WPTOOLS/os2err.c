#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#define INCL_KBD
#define INCL_VIO
#define INCL_PM
#include <os2.h>

#include "wptools.h"
#include "msg.h"
#include "language.h"

static BYTE szPress[]= "Press any key...";
static BOOL fIsVIO = 99;
/*********************************************************************
*  Debug Box
*********************************************************************/
BOOL _System MessageBox(PSZ pszTitle, PSZ pszMes, ...)
{
static BYTE szMessage[1025];
va_list va;

   if (fIsVIO == 99)
      {
      ULONG ulLVB;
      USHORT uscbLVB;
      APIRET rc;

      rc = VioGetBuf(&ulLVB, &uscbLVB, (HVIO)0);
      if (!rc)
         fIsVIO = TRUE;
      else
         fIsVIO = FALSE;
      }


   va_start(va, pszMes);
   vsprintf(szMessage, pszMes, va);

   if (WinMessageBox(HWND_DESKTOP,HWND_DESKTOP, 
       (PSZ) szMessage , (PSZ) pszTitle, 0, 
       MB_OK | MB_INFORMATION ) != MBID_ERROR)
      return TRUE;

   if (fIsVIO)
      {
      SetLanguage(LANG_UK);
      msg_nrm("%s//%s",
         pszTitle,
         szMessage);
      }
   else
      {
      USHORT usWait = VP_WAIT|VP_OPAQUE;
      USHORT usLine;
      KBDKEYINFO kbdKey;
      PSZ p, pEnd;

      VioPopUp(&usWait, (HVIO)0);
      VioWrtCharStr(pszTitle, strlen(pszTitle),
         12, 5, (HVIO)0);

      usLine = 14;

      p = szMessage;

      while (strlen(p))
         {
         pEnd = strchr(p, '\n');
         if (!pEnd)
            pEnd = p + strlen(p);

         VioWrtCharStr(p, pEnd - p,
            usLine++, 5, (HVIO)0);

         p = pEnd;
         if (*p)
            p++;
         }

      VioWrtCharStr(szPress, strlen(szPress),
         usLine+1, 5, (HVIO)0);
      KbdCharIn(&kbdKey, IO_WAIT, 0);
      VioEndPopUp((HVIO)0);
      }


   return TRUE;
}

