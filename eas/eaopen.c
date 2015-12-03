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

#include "..\os2hek\os2hek.h"
#include "eabrowse.h"

extern OPTIONS Options;
static BYTE  szSaveDir[300];
static BYTE  szLastDir[300];


static MRESULT EXPENTRY DialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL BuildDirList               (HWND hwndDialog);

/***********************************************************************
* Ask a new dir
***********************************************************************/
BOOL ChangeDirDialog(HWND hwnd)
{
ULONG ulResult;

   strcpy(szSaveDir, Options.szCurDir);
   
   ulResult = WinDlgBox(HWND_DESKTOP,
      0L,
      DialogProc,
      0L,
      ID_OPENDLG,
      NULL);

   if (ulResult != DID_OK)
      {
      strcpy(Options.szCurDir, szSaveDir);
      return FALSE;
      }

   strupr(Options.szCurDir);

   return TRUE;
}


/*************************************************************************
* Winproc procedure for the dialog
*************************************************************************/
static MRESULT EXPENTRY DialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
MRESULT mResult;

   switch( msg )
      {
      case WM_INITDLG :
         WinSendDlgItemMsg(hwnd, ID_DIRNAME, EM_SETTEXTLIMIT,
            MRFROMSHORT(sizeof Options.szCurDir - 1), 0L);
         WinSetDlgItemText(hwnd, ID_DIRNAME,
            Options.szCurDir);

         BuildDirList(hwnd);
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
                  memset(szLastDir, 0, sizeof szLastDir);

                  if (szSelDir[1] == ':')
                     {
                     if (!_getdcwd((INT)(*szSelDir - '@'),
                        Options.szCurDir, sizeof Options.szCurDir))
                        MessageBox("ERROR!", "The specified drive does not exist!");
                     }
                  else
                     {
                     if (!strcmp(szSelDir, ".."))
                        {
                        
                        PBYTE p = strrchr(Options.szCurDir, '\\');
                        if (!p)
                           break;

                        strcpy(szLastDir, p+1);

                        if (p - Options.szCurDir < 3)
                           p++;
                        *p = 0;

                        }
                     else
                        {
                        if (Options.szCurDir[strlen(Options.szCurDir) - 1] != '\\')
                           strcat(Options.szCurDir, "\\");
                        strcat(Options.szCurDir, szSelDir);
                        }
                     }
                  WinSetDlgItemText(hwnd, ID_DIRNAME,
                     Options.szCurDir);
                  if (SHORT2FROMMP(mp1) == LN_ENTER)
                     BuildDirList(hwnd);
                  break;
               }
            }
         break;


      case WM_COMMAND :
         switch(SHORT1FROMMP(mp1))
            {
            case DID_OK :
               WinQueryDlgItemText(hwnd,
                  ID_DIRNAME, sizeof Options.szCurDir, Options.szCurDir);
               if (access(Options.szCurDir, 0))
                  {
                  MessageBox("ERROR !", "Directory does not exist !");
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
BOOL BuildDirList(HWND hwndDialog)
{
BYTE         szSearchPath[300];
ULONG        ulFindCount;
FILEFINDBUF3 find;
HDIR         FindHandle;
APIRET       rc;
ULONG        ulCurDisk, ulDriveMap;
USHORT       usIndex;

   strcpy(szSearchPath, Options.szCurDir);
   if (szSearchPath[strlen(szSearchPath) - 1] != '\\')
      strcat(szSearchPath, "\\");
   strcat(szSearchPath, "*.*");

   WinEnableWindowUpdate(hwndDialog, FALSE);


   WinSendDlgItemMsg(hwndDialog, ID_DIRLIST, LM_DELETEALL, 0L, 0L);

   FindHandle = HDIR_CREATE;
   ulFindCount = 1;
   rc =  DosFindFirst(szSearchPath, &FindHandle, MUST_HAVE_DIRECTORY | FILE_DIRECTORY | FILE_ARCHIVED, 
                    &find, sizeof (FILEFINDBUF3), &ulFindCount, 1);

   while (!rc)
      {
      if ((find.attrFile & FILE_DIRECTORY) && strcmp(find.achName, "."))
         {
         SHORT sIndex;
         sIndex = (SHORT)WinSendDlgItemMsg(hwndDialog, ID_DIRLIST,
            LM_INSERTITEM, MPFROMSHORT(LIT_SORTASCENDING), (MPARAM)find.achName);
         if (!stricmp(find.achName, szLastDir))
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

      if (*szDrive == *Options.szCurDir)
         continue;

      WinSendDlgItemMsg(hwndDialog, ID_DIRLIST,
         LM_INSERTITEM, MPFROMSHORT(LIT_END), (MPARAM)szDrive);
      }

   WinEnableWindowUpdate(hwndDialog, TRUE);
   return TRUE;
}

