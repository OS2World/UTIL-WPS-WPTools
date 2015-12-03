#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include <io.h>

#define INCL_GPI
#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include "icon.h"
#include "icondef.h"

static MRESULT EXPENTRY DialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL BuildDirList               (HWND hwndDialog, PINSTANCE wd);

/***********************************************************************
* Ask a new dir
***********************************************************************/
BOOL ChangeDirDialog(HWND hwnd)
{
ULONG ulResult;
PINSTANCE wd = WinQueryWindowPtr(hwnd, 0);

   strcpy(wd->szSaveDir, wd->szCurDir);
   strcpy(wd->szCurDir2, wd->szCurDir);

   ulResult = WinDlgBox(HWND_DESKTOP,
      0L,
      DialogProc,
      0L,
      ID_OPENDLG,
      wd);

   if (ulResult != DID_OK)
      {
      strcpy(wd->szCurDir, wd->szSaveDir);
      return FALSE;
      }

   strupr(wd->szCurDir);

   WinPostMsg(hwnd, WM_REFILL, 0, 0);

   return TRUE;
}


/*************************************************************************
* Winproc procedure for the dialog
*************************************************************************/
static MRESULT EXPENTRY DialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
PINSTANCE wd;
MRESULT mResult;

   wd = (PINSTANCE)WinQueryWindowPtr(hwnd, 0);    // retrieve instance data pointer

   switch( msg )
      {
      case WM_INITDLG :
         wd = (PINSTANCE)mp2;
         WinSendDlgItemMsg(hwnd, ID_DIRNAME, EM_SETTEXTLIMIT,
            MRFROMSHORT(sizeof wd->szCurDir - 1), 0L);
         WinSetDlgItemText(hwnd, ID_DIRNAME,
            wd->szCurDir);

         WinSetWindowPtr(hwnd, 0, (PVOID)wd);
         BuildDirList(hwnd, wd);
         break;

      case WM_CLOSE  :
         WinDismissDlg(hwnd, DID_CANCEL);
         return (MRESULT)FALSE;

      case WM_CONTROL:
         if (SHORT1FROMMP(mp1) == ID_DIRLIST)
            {
            SHORT sSelIndex;
            BYTE szSelDir[128];
            HWND hwndList;

            hwndList = (HWND)mp2;
            sSelIndex = WinQueryLboxSelectedItem(hwndList);
            if (sSelIndex < 0 || sSelIndex == LIT_NONE)
               break;

            WinQueryLboxItemText(hwndList,
               sSelIndex, szSelDir, sizeof szSelDir);


            switch(SHORT2FROMMP(mp1))
               {
               case LN_ENTER:
//               case LN_SELECT:
                  memset(wd->szLastDir, 0, sizeof wd->szLastDir);

                  if (szSelDir[1] == ':')
                     {
                     if (!_getdcwd((INT)(*szSelDir - '@'),
                        wd->szCurDir, sizeof wd->szCurDir))
                        DebugBox("ICONTOOL",_strerror("Change drive"), FALSE);
                     }
                  else
                     {
                     strcpy(wd->szCurDir, wd->szCurDir2);
                     if (!strcmp(szSelDir, ".."))
                        {
                        
                        PBYTE p = strrchr(wd->szCurDir, '\\');
                        if (!p)
                           break;

                        strcpy(wd->szLastDir, p+1);

                        if (p - wd->szCurDir < 3)
                           p++;
                        *p = 0;

                        }
                     else
                        {
                        if (wd->szCurDir[strlen(wd->szCurDir) - 1] != '\\')
                           strcat(wd->szCurDir, "\\");
                        strcat(wd->szCurDir, szSelDir);
                        }
                     }
                  WinSetDlgItemText(hwnd, ID_DIRNAME,
                     wd->szCurDir);
                  if (SHORT2FROMMP(mp1) == LN_ENTER)
                     BuildDirList(hwnd, wd);
                  break;
               }
            }
         break;


      case WM_COMMAND :
         switch(SHORT1FROMMP(mp1))
            {
            case DID_OK :
               WinQueryDlgItemText(hwnd,
                  ID_DIRNAME, sizeof wd->szCurDir, wd->szCurDir);
               if (access(wd->szCurDir, 0))
                  {
                  DebugBox("ERROR !", "Directory does not exist !", FALSE);
                  WinSetDlgItemText(hwnd, ID_DIRNAME, wd->szSaveDir);
                  }
               else
                  WinDismissDlg(hwnd, DID_OK);
               return (MRESULT)FALSE;
            case DID_CANCEL:
               WinDismissDlg(hwnd, DID_CANCEL);
               return (MRESULT)FALSE;
            }
         break;

      default:
         break;

      }
   mResult = WinDefDlgProc( hwnd, msg, mp1, mp2 );
   return mResult;
}

/*********************************************************************
* Fill the directory list
*********************************************************************/
BOOL BuildDirList(HWND hwndDialog, PINSTANCE wd)
{
BYTE         szSearchPath[MAX_PATH];
ULONG        ulFindCount;
FILEFINDBUF3 find;
HDIR         FindHandle;
APIRET       rc;
ULONG        ulCurDisk, ulDriveMap;
USHORT       usIndex;

   strcpy(wd->szCurDir2, wd->szCurDir);

   strcpy(szSearchPath, wd->szCurDir);
   if (szSearchPath[strlen(szSearchPath) - 1] != '\\')
      strcat(szSearchPath, "\\");
   strcat(szSearchPath, "*.*");

   WinEnableWindowUpdate(hwndDialog, FALSE);


   WinSendDlgItemMsg(hwndDialog, ID_DIRLIST, LM_DELETEALL, 0L, 0L);

   FindHandle = HDIR_CREATE;
   ulFindCount = 1;
   rc =  DosFindFirst(szSearchPath, &FindHandle, MUST_HAVE_DIRECTORY | FILE_DIRECTORY | FILE_ARCHIVED | FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM, 
                    &find, sizeof (FILEFINDBUF3), &ulFindCount, 1);

   while (!rc)
      {
      if ((find.attrFile & FILE_DIRECTORY) && strcmp(find.achName, "."))
         {
         SHORT sIndex;
         sIndex = (SHORT)WinSendDlgItemMsg(hwndDialog, ID_DIRLIST,
            LM_INSERTITEM, MPFROMSHORT(LIT_SORTASCENDING), (MPARAM)find.achName);
         if (!stricmp(find.achName, wd->szLastDir))
            {
            WinSendDlgItemMsg(hwndDialog, ID_DIRLIST,
               LM_SELECTITEM, MPFROMSHORT(sIndex), MPFROMSHORT(TRUE));
            }
         }
      ulFindCount = 1;
      rc = DosFindNext(FindHandle, &find, sizeof (FILEFINDBUF3), &ulFindCount);
      }
   DosFindClose(FindHandle);

   DosQCurDisk(&ulCurDisk, &ulDriveMap);
   for (usIndex = 0; usIndex < 26; usIndex++)
      {
      BYTE szDrive[3];
      ULONG Mask = 0x0001 << usIndex;

      if (!(ulDriveMap & Mask))
         continue;
      szDrive[0] = (BYTE)(usIndex + 'A');
      szDrive[1] = ':';
      szDrive[2] = 0;

      if (*szDrive == *wd->szCurDir)
         continue;

      WinSendDlgItemMsg(hwndDialog, ID_DIRLIST,
         LM_INSERTITEM, MPFROMSHORT(LIT_END), (MPARAM)szDrive);
      }

   WinEnableWindowUpdate(hwndDialog, TRUE);
   return TRUE;
}

