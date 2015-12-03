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

typedef struct _LockUp
{
USHORT usFullScreen;
USHORT usAutoLock;
USHORT usDuration;
USHORT usUnknown1;
USHORT usAutoDim;
BYTE   Filler1[16];
USHORT usLockOnStart;
BYTE   Filler2[8];
BYTE   bBlue;
BYTE   bGreen;
BYTE   bRed;
BYTE   bExtra; 
BYTE   bColorOnly;    // 27=ColorOnly
BYTE   bFiller3;
BYTE   bImageType; /* 2=Normal,3=tiled,4=scaled */
BYTE   bFiller4;
BYTE   bScaleFactor; 
BYTE   bFiller5;
} LOCKUP, *PLOCKUP;

/*******************************************************************
* Get Folderoptions
*******************************************************************/
BOOL _System GetWPDesktopOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
PLOCKUP pLockup;
ULONG ulSize;
PBYTE pszFileName;
BYTE szOptionText[64];

   if (!IsWarp4())
      return TRUE;

   pszFileName = GetProfileData("PM_Lockup", "LockupBitmap", HINI_PROFILE, &ulSize);

   pLockup = (PLOCKUP)GetProfileData("PM_Lockup", "LockupOptions", HINI_PROFILE, &ulSize);
   if (!pLockup)
      {
      return TRUE;
      }

   sprintf(pszOptions+strlen(pszOptions), "AUTOLOCKUP=%s;",
      pLockup->usAutoLock ? "YES" : "NO");
   sprintf(pszOptions+strlen(pszOptions), "LOCKUPAUTODIM=%s;",
      pLockup->usAutoDim ? "YES" : "NO");
   sprintf(pszOptions+strlen(pszOptions), "LOCKUPFULLSCREEN=%s;",
      pLockup->usFullScreen ? "YES" : "NO");
   sprintf(pszOptions+strlen(pszOptions), "LOCKUPONSTARTUP=%s;",
      pLockup->usLockOnStart ? "YES" : "NO");
   sprintf(pszOptions+strlen(pszOptions), "LOCKUPTIMEOUT=%d;",
      pLockup->usDuration);


   if (!pszFileName)
      sprintf(szOptionText, "LOCKUPBACKGROUND=(none),");
   else
      sprintf(szOptionText, "LOCKUPBACKGROUND=%s,", pszFileName);

   switch (pLockup->bImageType)
      {
      case '3':
         strcat(szOptionText, "T,");
         break;
      case '4':
         strcat(szOptionText, "S,");
         break;
      default:
         strcat(szOptionText, "N,");
         break;
      }
   if (!pLockup->bExtra)
      {
      sprintf(szOptionText + strlen(szOptionText),
         "%u,%c,%u %u %u;",
         pLockup->bScaleFactor,
         (pLockup->bColorOnly == 0x27 ? 'C' : 'I'),
         pLockup->bRed,
         pLockup->bGreen, 
         pLockup->bBlue);
      }
   else
      {
      sprintf(szOptionText + strlen(szOptionText),
         "%u,%c;",
         pLockup->bScaleFactor,
         (pLockup->bColorOnly == 0x27 ? 'C' : 'I'));
      }
   strcat(pszOptions, szOptionText);


   if (pszFileName)
      free(pszFileName);
   free(pLockup);
   return TRUE;
}

