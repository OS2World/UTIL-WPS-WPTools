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
static VOID ChkRadio(HWND hwnd, ULONG ulID);

//#define NOTEBOOK
#ifndef NOTEBOOK
BOOL OptionsDialog(HWND hwnd)
{
PINSTANCE wd = WinQueryWindowPtr(hwnd, 0);
ULONG     ulResult;
BOOL      bRealNames = wd->bRealNames;
BOOL      bAnimIcons = wd->bAnimIcons;
BYTE      szSaveFilter[FILTER_SIZE];

   strcpy(szSaveFilter, wd->szFilter);

   ulResult = WinDlgBox(HWND_DESKTOP,
      0L,
      DialogProc,
      0L,
      ID_OPTIONDLG,
      wd);

   if (ulResult != DID_OK && ulResult != ID_SAVE)
      return FALSE;

   if (ulResult == ID_SAVE)
      SaveOptions(hwnd);

   SetContainerAttr(wd);
   strupr(wd->szFilter);
   if (bRealNames != wd->bRealNames || bAnimIcons != wd->bAnimIcons)
      WinPostMsg(hwnd, WM_REFILL, 0L, 0L);
   else
      {
      WinSendMsg(wd->hwndContainer, CM_FILTER,
         (MPARAM)FilterContainerFunc, (MPARAM) wd);

      WinSendMsg(wd->hwndContainer,
            CM_SORTRECORD, (MPARAM)SortContainerFunc, wd);
      }

   ReadEAValue(hwnd, NULL, FALSE);
   ReadEAValue(hwnd, NULL, TRUE);
   WinPostMsg(hwnd, WM_SIZE, 0L, 0L);

   return TRUE;
}
#else
BOOL OptionsDialog(HWND hwnd)
{
PINSTANCE wd = WinQueryWindowPtr(hwnd, 0);
ULONG     ulResult;
BOOL      bRealNames = wd->bRealNames;
BYTE      szSaveFilter[FILTER_SIZE];
HWND      hwndNoteBook;
HWND      hwndSettings;
ULONG     ulPage;

   hwndNoteBook = WinLoadDlg(
      HWND_DESKTOP,
      0,
      WinDefDlgProc,
      0L,
      ID_DLGNOTEBOOK,
      wd);

   if (!hwndNoteBook)
      {
      DebugBox("ICONTOOL", "Unable to load notebook!", TRUE);
      return FALSE;
      }

   hwndSettings = WinLoadDlg(HWND_DESKTOP,
      hwndNoteBook,
      DialogProc,
      0L,
      ID_OPTIONDLG,
      wd);


   ulPage = (ULONG)WinSendDlgItemMsg(hwndNoteBook, ID_NOTEBOOK,
      BKM_INSERTPAGE,
      (MPARAM)BKA_FIRST,
      MPFROM2SHORT(BKA_MAJOR|BKA_AUTOPAGESIZE, BKA_FIRST));

   WinSendDlgItemMsg(hwndNoteBook, ID_NOTEBOOK,
      BKM_SETTABTEXT,
      MPFROMLONG(ulPage),
      MPFROMP("~Settings"));

   WinSendDlgItemMsg(hwndNoteBook, ID_NOTEBOOK,
      BKM_SETPAGEWINDOWHWND,
      MPFROMLONG(ulPage),
      MPFROMHWND(hwndSettings));


   ulResult = WinProcessDlg(hwndNoteBook);
   return FALSE;
}
#endif

/*************************************************************************
* Winproc procedure for the dialog
*************************************************************************/
static MRESULT EXPENTRY DialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
PINSTANCE wd;
MRESULT mResult;
BYTE    szExtractPath[128];

   wd = (PINSTANCE)WinQueryWindowPtr(hwnd, 0);    // retrieve instance data pointer

   switch( msg )
      {
      case WM_INITDLG :
         wd = (PINSTANCE)mp2;
         WinSetWindowPtr(hwnd, 0, (PVOID)wd);
         if (!wd->bFlowed)
            ChkRadio(hwnd, ID_NGRID);
         else
            ChkRadio(hwnd, ID_FLOWED);

         WinSendDlgItemMsg(hwnd, ID_DIRECTORIES,BM_SETCHECK, (MPARAM)wd->bDirectories, 0L);
         WinSendDlgItemMsg(hwnd, ID_DRIVES,     BM_SETCHECK, (MPARAM)wd->bDrives, 0L);
         WinSendDlgItemMsg(hwnd, ID_REALNAMES,  BM_SETCHECK, (MPARAM)wd->bRealNames, 0L);
         WinSendDlgItemMsg(hwnd, ID_MINIICONS,  BM_SETCHECK, (MPARAM)wd->bSmallIcons, 0L);
         WinSendDlgItemMsg(hwnd, ID_ANIMATED,   BM_SETCHECK, (MPARAM)wd->bAnimIcons, 0L);
         WinSendDlgItemMsg(hwnd, ID_SAVEEXIT,   BM_SETCHECK, (MPARAM)wd->bSaveOnExit, 0L);

         WinSetDlgItemText(hwnd, ID_FILTER, wd->szFilter);
         WinSetDlgItemText(hwnd, ID_EXTPATH, wd->szExtractPath);
         WinSendDlgItemMsg(hwnd, ID_EXTPATH, EM_SETTEXTLIMIT,
            MRFROMSHORT(sizeof wd->szExtractPath - 1), 0L);

         switch (wd->usSort)
            {
            case SORT_NONE:
            case SORT_NAME:
               ChkRadio(hwnd, ID_SORTNAME);
               break;
            case SORT_EXT:
               ChkRadio(hwnd, ID_SORTEXT);
               break;
            default           :
            case SORT_REALNAME:
               ChkRadio(hwnd, ID_SORTREALNAME);
               break;
            }

         switch (wd->usReadEA)
            {
            case READEA_NONE:
               ChkRadio(hwnd, ID_EANONE);
               break;
            case READEA_SUBJECT   :
               ChkRadio(hwnd, ID_EASUBJECT);
               break;
            case READEA_COMMENTS  :
               ChkRadio(hwnd, ID_EACOMMENTS);
               break;
            case READEA_HISTORY   :
               ChkRadio(hwnd, ID_EAHISTORY);
               break;
            case READEA_KEYPHRASES:
               ChkRadio(hwnd, ID_EAKEYPHRASES);
               break;
            }
         break;

      case WM_CLOSE  :
         WinDismissDlg(hwnd, DID_CANCEL);
         return (MRESULT)FALSE;

      case WM_COMMAND :
         switch(SHORT1FROMMP(mp1))
            {
            case DID_OK :
            case ID_SAVE:
               wd->bFlowed = (BOOL)WinSendDlgItemMsg(hwnd, ID_FLOWED,
                  BM_QUERYCHECK, 0L, 0L);
               wd->bDrives = (BOOL)WinSendDlgItemMsg(hwnd, ID_DRIVES,
                  BM_QUERYCHECK, 0L, 0L);
               wd->bDirectories = (BOOL)WinSendDlgItemMsg(hwnd, ID_DIRECTORIES,
                  BM_QUERYCHECK, 0L, 0L);
               wd->bRealNames = (BOOL)WinSendDlgItemMsg(hwnd, ID_REALNAMES,
                  BM_QUERYCHECK, 0L, 0L);
               wd->bSmallIcons = (BOOL)WinSendDlgItemMsg(hwnd, ID_MINIICONS,
                  BM_QUERYCHECK, 0L, 0L);
               wd->bAnimIcons = (BOOL)WinSendDlgItemMsg(hwnd, ID_ANIMATED,
                  BM_QUERYCHECK, 0L, 0L);
               wd->bSaveOnExit = (BOOL)WinSendDlgItemMsg(hwnd, ID_SAVEEXIT,
                  BM_QUERYCHECK, 0L, 0L);
               WinQueryDlgItemText(hwnd,
                  ID_FILTER, sizeof wd->szFilter, wd->szFilter);
               WinQueryDlgItemText(hwnd,
                  ID_EXTPATH, sizeof szExtractPath, szExtractPath);
               wd->usSort =(SHORT) WinSendDlgItemMsg(hwnd, ID_SORTNAME,
                  BM_QUERYCHECKINDEX, 0L, 0L) + 1;
               wd->usReadEA =(SHORT) WinSendDlgItemMsg(hwnd, ID_EANONE,
                  BM_QUERYCHECKINDEX, 0L, 0L) - 1;

               if (strlen(szExtractPath) && access(szExtractPath, 0))
                  {
                  DebugBox("ICONTOOL",
                     "The specified path for extracted icons cannot be accessed!", FALSE);
                  return (MRESULT)FALSE;
                  }
               strcpy(wd->szExtractPath, szExtractPath);
               WinDismissDlg(hwnd, SHORT1FROMMP(mp1));
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

/*******************************************************************
* Settabstop
*******************************************************************/
VOID ChkRadio(HWND hwnd, ULONG ulID)
{
ULONG ulStyle;

   ulStyle = WinQueryWindowULong(WinWindowFromID(hwnd, ulID), QWL_STYLE);
   WinSetWindowULong(WinWindowFromID(hwnd, ulID), QWL_STYLE, ulStyle | WS_TABSTOP);
   WinSendDlgItemMsg(hwnd, ulID, BM_SETCHECK, (MPARAM)TRUE, 0L);
}

