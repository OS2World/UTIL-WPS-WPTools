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
#include <share.h>

#define INCL_VIO
#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_GPI
#include <os2.h>

#pragma pack(1)
typedef struct _TrayDef
{
BYTE bTrayNumber;
BYTE bInUse;
BYTE bUnknown[3];
BYTE szTitle[33];
ULONG ulIconCount;
ULONG rgulObjects[1];
} TRAYDEF, *PTRAYDEF;
#pragma pack()

#include "wptools.h"

IMPORT PSZ _System pszOrigID(HOBJECT hObject);

IMPORT HOBJECT _rgLaunchPadObjects[];
IMPORT USHORT usLPObjCount;

/*******************************************************************
* Get Objectoptions
*******************************************************************/
BOOL _System GetSCenterOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
static BYTE szConfigDir[CCHMAXPATH] = "E:\\OS2\\DLL\\";
static BYTE szFile[CCHMAXPATH];
APIRET rc;
ULONG  ulBootDrive;
INT    iHandle;
INT    iTray;

   rc = DosQuerySysInfo(QSV_BOOT_DRIVE, QSV_BOOT_DRIVE,
      &ulBootDrive, sizeof ulBootDrive);
   if (rc)
      return TRUE;

   szConfigDir[0] = (BYTE)(ulBootDrive + '@');

   for (iTray = 0; iTray < 16; iTray++)
      {
      sprintf(szFile, "%sDOCK%u.CFG", szConfigDir, iTray);
      iHandle = sopen(szFile, O_RDONLY|O_BINARY, SH_DENYNO);
      if (iHandle > 0)
         {
         ULONG ulLength = filelength(iHandle);
         PTRAYDEF pTray = malloc(ulLength);
         ULONG ulIcon;

         if (read(iHandle, pTray, ulLength) != ulLength)
            {
            MessageBox("WPTOOLS", "SCenter: cannot read from %s", szFile);
            close(iHandle);
            free(pTray);
            continue;
            }
         close(iHandle);
         if (!pTray->bInUse)
            {
            free(pTray);
            continue;
            }

         sprintf(pszOptions + strlen(pszOptions),
            "ADDTRAY=%s", pTray->szTitle);

         for (ulIcon = 0; ulIcon < pTray->ulIconCount; ulIcon++)
            {
            PSZ pszObjectID = pszOrigID(pTray->rgulObjects[ulIcon]);
            if (pszObjectID)
               {
               if (usLPObjCount < 200)
                  _rgLaunchPadObjects[usLPObjCount++] = pTray->rgulObjects[ulIcon];

               sprintf(pszOptions + strlen(pszOptions),
                  ",%s", pszObjectID);
               }
            }
         strcat(pszOptions, ";");
         free(pTray);
         }
      }

   return TRUE;
}

