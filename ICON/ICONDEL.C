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
typedef struct _DDATA
{
PINSTANCE wd;
PCNRDRAGINFO pcnrdinfo;
PDRAGINFO pdinfo;
} DDATA, * PDDATA;

static MRESULT EXPENTRY DialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
static BOOL BuildDelList(HWND hwndDialog, PINSTANCE wd, PDRAGINFO pdinfo);
static BOOL DeleteFiles(HWND hwndDialog, PINSTANCE wd);

/***********************************************************************
* Discard an object
***********************************************************************/
VOID DeleteObject(HWND hwnd)
{
PINSTANCE wd = WinQueryWindowPtr(hwnd, 0);
PRECINFO   pcrec, pTemp;
USHORT usIndex = 0;

   pcrec = wd->pcrec;
   if (!pcrec)
      return;

   if (!wd->hwndDelete)
      {
      wd->hwndDelete = WinLoadDlg(HWND_DESKTOP,
         0L,
         DialogProc,
         0L,
         ID_DELDLG,
         wd);

      WinSetDlgItemText(wd->hwndDelete, ID_LOCATION, wd->szCurDir);
      WinSendDlgItemMsg(wd->hwndDelete, ID_CONFIRM,
            BM_SETCHECK, (MPARAM)TRUE, 0L);
      }

   if (pcrec->Record.flRecordAttr & CRA_SELECTED)
      {
      pTemp = WinSendMsg(wd->hwndContainer,
         CM_QUERYRECORDEMPHASIS, (MPARAM)CMA_FIRST, (MPARAM)CRA_SELECTED);
      while ((LONG)pTemp > 0L)
         {
         if ((pTemp->finfo.chType == TYPE_FILE ||
              pTemp->finfo.chType == TYPE_DIRECTORY ||
              pTemp->finfo.chType == TYPE_ABSTRACT)
              && strcmp(pTemp->finfo.szFileName, ".."))
            {
            WinSendDlgItemMsg(wd->hwndDelete, ID_DELLIST,
               LM_INSERTITEM, MPFROMSHORT(LIT_END), (MPARAM)pTemp->finfo.szFileName);
            WinSendDlgItemMsg(wd->hwndDelete, ID_DELLIST,
               LM_SELECTITEM, MPFROMSHORT(usIndex), MPFROMSHORT(TRUE));
            WinSendDlgItemMsg(wd->hwndDelete, ID_DELLIST,
               LM_SETITEMHANDLE, (MPARAM)usIndex, (MPARAM)pTemp);
            usIndex++;
            }

         pTemp = WinSendMsg(wd->hwndContainer,
            CM_QUERYRECORDEMPHASIS, (MPARAM)pTemp, (MPARAM)CRA_SELECTED);
         }
      }
   else
      {
      if  (pcrec->finfo.chType == TYPE_FILE ||
           pcrec->finfo.chType == TYPE_ABSTRACT ||
           (pcrec->finfo.chType == TYPE_DIRECTORY && strcmp(pcrec->finfo.szFileName, "..")))
         {
         WinSendDlgItemMsg(wd->hwndDelete, ID_DELLIST,
            LM_INSERTITEM, MPFROMSHORT(LIT_END), (MPARAM)pcrec->finfo.szFileName);
         WinSendDlgItemMsg(wd->hwndDelete, ID_DELLIST,
            LM_SELECTITEM, MPFROMSHORT(usIndex), MPFROMSHORT(TRUE));
         WinSendDlgItemMsg(wd->hwndDelete, ID_DELLIST,
            LM_SETITEMHANDLE, MPFROMSHORT(usIndex), (MPARAM)pcrec);
         usIndex++;
         }
      }
   if (usIndex)
      DeleteDialog(hwnd);
   else
      {
      WinDestroyWindow(wd->hwndDelete);
      wd->hwndDelete = 0;
      wd->pcrec = 0L;
      }
}

/***********************************************************************
* Discard an object
***********************************************************************/
VOID DiscardObject(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
PDRAGINFO pdinfo;
PINSTANCE wd;

   wd = WinQueryWindowPtr(hwnd, 0);

   if (--wd->ulDragItemCount)
      return;

   if (!wd->hwndDelete)
      {
      wd->hwndDelete = WinLoadDlg(HWND_DESKTOP,
         0L,
         DialogProc,
         0L,
         ID_DELDLG,
         wd);

      pdinfo = (PDRAGINFO)mp1;
      WinSetDlgItemText(wd->hwndDelete, ID_LOCATION, wd->szCurDir);
      WinSendDlgItemMsg(wd->hwndDelete, ID_CONFIRM,
            BM_SETCHECK, (MPARAM)TRUE, 0L);

      BuildDelList(wd->hwndDelete, wd, pdinfo);
      }

   DrgFreeDraginfo(wd->pdinfo);
   wd->usDragCount--;
   wd->pdinfo = NULL;
   WinPostMsg(hwnd, WM_DELDLG, 0L, 0L);
}

/***********************************************************************
* Handle the deletion dialog
***********************************************************************/
VOID DeleteDialog(HWND hwnd)
{
PINSTANCE wd;
ULONG     ulResult;

   wd = WinQueryWindowPtr(hwnd, 0);
   WinSetWindowPos(wd->hwndDelete, HWND_TOP, 0, 0, 0, 0,
      SWP_ACTIVATE | SWP_SHOW);

   ulResult = WinProcessDlg(wd->hwndDelete);

   if (ulResult == DID_OK)
      {
      DeleteFiles(wd->hwndDelete, wd);

      WinPostMsg(wd->hwndContainer, CM_ARRANGE, 0L, 0L);
      if (wd->bFlowed)
         {
         wd->bFlowed  = FALSE;
         SetContainerAttr(wd);
         wd->bFlowed  = TRUE;
         SetContainerAttr(wd);
         }
      }

   WinDestroyWindow(wd->hwndDelete);
   wd->hwndDelete = 0L;
   wd->pcrec = 0L;
   return;
}


/*************************************************************************
* Winproc procedure for the dialog
*************************************************************************/
static MRESULT EXPENTRY DialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
MRESULT mResult;

   switch( msg )
      {
      case WM_CLOSE  :
         WinDismissDlg(hwnd, DID_CANCEL);
         return (MRESULT)FALSE;

      case WM_COMMAND :
         switch(SHORT1FROMMP(mp1))
            {
            case DID_OK :
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
static BOOL BuildDelList(HWND hwndDialog, PINSTANCE wd, PDRAGINFO pdinfo)
{
ULONG ulCount;
PRECINFO pcrec;
USHORT   usIndex;
PDRAGITEM   pditem;


   WinEnableWindowUpdate(hwndDialog, FALSE);
   ulCount = DrgQueryDragitemCount(pdinfo);

   for (usIndex = 0; usIndex < ulCount; usIndex++)
      {
      pditem = DrgQueryDragitemPtr(pdinfo, usIndex);
      pcrec  = (PRECINFO)pditem->ulItemID;

      WinSendDlgItemMsg(hwndDialog, ID_DELLIST,
         LM_INSERTITEM, MPFROMSHORT(LIT_END), (MPARAM)pcrec->finfo.szFileName);
      WinSendDlgItemMsg(hwndDialog, ID_DELLIST,
         LM_SELECTITEM, MPFROMSHORT(usIndex), MPFROMSHORT(TRUE));
      WinSendDlgItemMsg(hwndDialog, ID_DELLIST,
         LM_SETITEMHANDLE, (MPARAM)usIndex, (MPARAM)pcrec);
      }

   WinEnableWindowUpdate(hwndDialog, TRUE);
   return TRUE;
}

/*****************************************************************
* Delete files
*****************************************************************/
static BOOL DeleteFiles(HWND hwndDialog, PINSTANCE wd)
{
PRECINFO pcrec;
SHORT   sSelected, sLast;
BOOL    bConfirm, bDelete;

   bConfirm = (BOOL)WinSendDlgItemMsg(hwndDialog, ID_CONFIRM,
                  BM_QUERYCHECK, 0L, 0L);

   sLast = LIT_FIRST;
   sSelected = (SHORT)WinSendDlgItemMsg(hwndDialog, ID_DELLIST,
         LM_QUERYSELECTION, (MPARAM)LIT_FIRST, 0L);

   while (sSelected != LIT_NONE)
      {
      pcrec = (PRECINFO)WinSendDlgItemMsg(hwndDialog, ID_DELLIST,
         LM_QUERYITEMHANDLE, (MPARAM)sSelected, 0L);
      if (pcrec)
         {
         BYTE szFullName[MAX_PATH];
         strcpy(szFullName, wd->szCurDir);
         if (szFullName[strlen(szFullName) - 1] != '\\')
            strcat(szFullName, "\\");
         strcat(szFullName, pcrec->finfo.szFileName);

         bDelete = TRUE;
         if (bConfirm && pcrec->finfo.chType == TYPE_DIRECTORY)
            {
            BYTE   szMessage[80];
            sprintf(szMessage, "Delete %s ?", pcrec->finfo.szFileName);
            if (WinMessageBox(HWND_DESKTOP,HWND_DESKTOP, szMessage,"",
               0, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON1 ) != MBID_YES)
               bDelete = FALSE;
            }
         if (bDelete && pcrec->finfo.chType == TYPE_ABSTRACT)
            {
            if (MyDestroyObject(pcrec->finfo.hObject))
               {
               RemoveRecord(wd->hwndContainer, pcrec);
               WinSendDlgItemMsg(hwndDialog, ID_DELLIST,
                  LM_DELETEITEM, (MPARAM)sSelected, 0L);
               }
            else
               {
               BYTE szMess[80];
               sprintf(szMess, "Unable to delete object %lX!",
                  pcrec->finfo.hObject);
               DebugBox("ICONTOOL", szMess, TRUE);
               sLast = sSelected;
               }
            }
         else if (bDelete && DeletePath(szFullName))
            {
            RemoveRecord(wd->hwndContainer, pcrec);
            WinSendDlgItemMsg(hwndDialog, ID_DELLIST,
               LM_DELETEITEM, (MPARAM)sSelected, 0L);
            }
         else
            sLast = sSelected;
         }
      sSelected = (SHORT)WinSendDlgItemMsg(hwndDialog, ID_DELLIST,
            LM_QUERYSELECTION, (MPARAM)sLast, 0L);

      }

   return TRUE;
}
