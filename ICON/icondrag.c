//#define WPSDRAG
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#include <sys\types.h>
#include <sys\stat.h>

#define INCL_GPI
#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include "icon.h"
#include "icondef.h"

static BOOL bDragAfter = FALSE;
extern USHORT usWindowCount;
extern HWND hwndFirst, hwndSecond;

BOOL SetIconToFile(PSZ pszIcon, PSZ pszFile, BOOL fAnimated);
BOOL SetIconToObject(HOBJECT hObject, PSZ pszIcon, BOOL fAnimated);

static PFNWP OldMLEFunc;
static MRESULT EXPENTRY MyMLEProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);

#define OBJECT_FROM_PREC( prec )   ( (PVOID)( * (((PULONG)prec)-2) ) )

/*****************************************************************
* Create the container
*****************************************************************/
VOID CreateContainer(HWND hwnd)
{
PINSTANCE  wd;
ULONG      ulPrfSize;
BYTE       szFrameFont[50];
BYTE       szOptions[50];
ULONG      ulColor;

   wd = (PINSTANCE)malloc(sizeof(INSTANCE));
   WinSetWindowPtr(hwnd, 0, (PVOID)wd);

   memset(wd, 0, sizeof (INSTANCE));
   if (!hwndFirst)
      wd->usWindowNo = 1;
   else
      wd->usWindowNo = 2;

   ulPrfSize = sizeof (INSTANCE);
   sprintf(szOptions,"OPTIONS%d", wd->usWindowNo);
   if (!PrfQueryProfileData(HINI_USERPROFILE,
      APPS_NAME,
      szOptions,
      wd,
      &ulPrfSize) || ulPrfSize != sizeof (INSTANCE))
      {
      memset(wd, 0, sizeof (INSTANCE));
      getcwd(wd->szCurDir, sizeof wd->szCurDir);
      wd->usReadEA = READEA_SUBJECT;
      }
   if (access(wd->szCurDir, 0))
      getcwd(wd->szCurDir, sizeof wd->szCurDir);

   if (!hwndFirst)
      wd->usWindowNo = 1;
   else
      wd->usWindowNo = 2;

   /*
      Create an un-named, shared, initially not-owned mutex semaphore
   */
   if (DosCreateMutexSem(NULL, &wd->hmtx, DC_SEM_SHARED, FALSE))
      {
      DebugBox("ICONTOOL", "Unable to create semaphore", FALSE);
      exit(1);
      }


   wd->hwndContainer = WinCreateWindow(hwnd,
      WC_CONTAINER,
      NULL, 
      CCS_MINIRECORDCORE | CCS_AUTOPOSITION | WS_VISIBLE | CCS_EXTENDSEL |  CCS_MINIICONS , 
      0, 0, 0, 0, 
      hwnd,
      HWND_TOP,
      ID_CONTAINER,
      0,
      0);


   wd->hwndStatic = WinCreateWindow(hwnd,
      WC_STATIC,
      NULL,
      SS_TEXT | DT_CENTER,
      0,0,0,0,
      hwnd,
      HWND_TOP,
      0,
      0,
      0);


   wd->hwndMLE = WinCreateWindow(hwnd,
      WC_MLE,
      NULL,
      MLS_WORDWRAP|MLS_BORDER|MLS_VSCROLL|MLS_HSCROLL|WS_VISIBLE|WS_DISABLED,
      0,0,0,0,
      hwnd,
      HWND_TOP,
      ID_MLE,
      0,
      0);

   OldMLEFunc = WinSubclassWindow(wd->hwndMLE, MyMLEProc);

   WinPostMsg(hwnd, WM_REFILL, 0L, 0L);

   SetContainerAttr(wd);

   strcpy(szFrameFont, "8.Helv");
   WinSetPresParam(wd->hwndContainer, PP_FONTNAMESIZE,
      (ULONG)(strlen(szFrameFont) + 1), szFrameFont);
   WinSetPresParam(wd->hwndMLE, PP_FONTNAMESIZE,
      (ULONG)(strlen(szFrameFont) + 1), szFrameFont);
   WinSetPresParam(wd->hwndStatic, PP_FONTNAMESIZE,
      (ULONG)(strlen(szFrameFont) + 1), szFrameFont);
   ulColor = SYSCLR_DIALOGBACKGROUND;
   WinSetPresParam(wd->hwndStatic, PP_BACKGROUNDCOLORINDEX,
      sizeof ulColor, &ulColor);


}
ULONG  ulContextMsg = 0L;
USHORT ulContextKbState;

MRESULT EXPENTRY MyMLEProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   if (!ulContextMsg)
      {
      ULONG ulResult = WinQuerySysValue(HWND_DESKTOP, SV_CONTEXTMENU);
      ulContextMsg     = LOUSHORT(ulResult);
      ulContextKbState = HIUSHORT(ulResult);
      }

   if (msg == ulContextMsg)
      {
      if (!ulContextKbState || WinGetKeyState(HWND_DESKTOP, ulContextKbState) & 0x8000)
         WinPostMsg(WinQueryWindow(hwnd, QW_OWNER),
            WM_CONTEXTMENU,
            mp1,
            MPFROMSHORT(1));
      }                  

   return(*OldMLEFunc)(hwnd, msg, mp1, mp2);
}
/***********************************************************
* Init drag
***********************************************************/
VOID InitDrag(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
PCNRDRAGINIT pcnrdinit;
PDRAGINFO    pdinfo;
DRAGITEM     ditem;
PDRAGIMAGE   pdimg;
HWND         hwndDrop;
PRECINFO     pcrec;
PINSTANCE    wd;
ULONG        ulItems, ulIndex;
BOOL         bMultiple = FALSE;


   wd = WinQueryWindowPtr(hwnd, 0);

   pcnrdinit = (PCNRDRAGINIT)mp2;
   if((pcrec = (PRECINFO)(pcnrdinit->pRecord)) != NULL)
      {
      PRECINFO pTemp;

      if (pcrec->Record.flRecordAttr & CRA_SELECTED)
         {
         bMultiple = TRUE;
         ulItems = 0;
         pTemp = WinSendMsg(wd->hwndContainer,
            CM_QUERYRECORDEMPHASIS, (MPARAM)CMA_FIRST, (MPARAM)CRA_SELECTED);
         while ((LONG)pTemp > 0L)
            {
            if (pTemp->finfo.chType != TYPE_FILE &&
                pTemp->finfo.chType != TYPE_DIRECTORY)
               return;
            if (!strcmp(pTemp->finfo.szFileName, ".."))
               return;
            ulItems++;
            pTemp = WinSendMsg(wd->hwndContainer,
               CM_QUERYRECORDEMPHASIS, (MPARAM)pTemp, (MPARAM)CRA_SELECTED);
            }
         if (!ulItems)
            return;
         }
      else
         {
         ulItems = 1;
         if (pcrec->finfo.chType != TYPE_FILE &&
             pcrec->finfo.chType != TYPE_DIRECTORY)
            return;
         if (!strcmp(pcrec->finfo.szFileName, ".."))
            return;

         }

      pdinfo = DrgAllocDraginfo(ulItems);
      pdimg = calloc(ulItems, sizeof (DRAGIMAGE));
      memset(pdimg, 0, ulItems * sizeof (DRAGIMAGE));

      if (bMultiple)
         {
         pTemp = WinSendMsg(wd->hwndContainer,
            CM_QUERYRECORDEMPHASIS, (MPARAM)CMA_FIRST, (MPARAM)CRA_SELECTED);
         }
      else
         pTemp = pcrec;

      ulIndex = 0;
      while ((ULONG)pTemp > 0L)
         {
         memset(&ditem, 0, sizeof ditem);
         ditem.hwndItem = hwnd;
         ditem.ulItemID = (ULONG)pTemp;
         ditem.hstrType = DrgAddStrHandle(DRT_TEXT);    
         ditem.hstrRMF =
//         DrgAddStrHandle("<DRM_OS2FILE, DRF_TEXT>,<DRM_DISCARD, DRF_TEXT>");
         DrgAddStrHandle("(DRM_OS2FILE, DRM_DISCARD) x (DRF_TEXT)");
//         DrgAddStrHandle("<DRM_OS2FILE, DRF_TEXT>,<DRM_DISCARD, DRF_TEXT>,<DRM_OBJECT,DRF_OBJECT>");

         ditem.hstrContainerName = DrgAddStrHandle(wd->szCurDir);
         ditem.hstrSourceName = DrgAddStrHandle(pTemp->finfo.szFileName);
         ditem.hstrTargetName = DrgAddStrHandle(pTemp->finfo.szFileName);
         ditem.fsControl = 0;
         ditem.fsSupportedOps = DO_MOVEABLE|DO_LINKABLE|DO_COPYABLE;
         ditem.cxOffset = pcnrdinit->cx;
         ditem.cyOffset = pcnrdinit->cy;

         DrgSetDragitem(pdinfo, &ditem, (ULONG)sizeof(ditem), ulIndex);

         pdimg[ulIndex].cb = sizeof(DRAGIMAGE);
         pdimg[ulIndex].cptl = 0;
         pdimg[ulIndex].hImage = pTemp->finfo.hptrIcon;
         pdimg[ulIndex].sizlStretch.cx = 20L;
         pdimg[ulIndex].sizlStretch.cy = 20L;
         pdimg[ulIndex].fl = DRG_ICON;
         pdimg[ulIndex].cxOffset = ulIndex * 3;
         pdimg[ulIndex].cyOffset = ulIndex * 2;
         ulIndex++;


         if (bMultiple)
            pTemp = WinSendMsg(wd->hwndContainer,
               CM_QUERYRECORDEMPHASIS, (MPARAM)pTemp, (MPARAM)CRA_SELECTED);
         else
            break;
         }

      wd->pdinfo = pdinfo;
      wd->usDragCount++;
      wd->ulDragItemCount = ulIndex;
      hwndDrop = DrgDrag(hwnd, pdinfo, pdimg, ulIndex, VK_ENDDRAG, NULL);
      if (!hwndDrop)
         {
         wd->ulDragItemCount = 0L;
         wd->usDragCount--;
         DrgFreeDraginfo(pdinfo);
         }
      free(pdimg);
      }
}

/**********************************************************************
* Drag over
**********************************************************************/
MRESULT DragOver(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
PCNRDRAGINFO   pcnrdinfo;
PDRAGITEM      pditem;
PDRAGINFO      pdinfo;
BOOL           bDrop;
PRECINFO       pcrec;
BYTE           szDragFname[100];

      if (SHORT2FROMMP(mp1) == CN_DRAGAFTER)
         bDragAfter = TRUE;
      else
         bDragAfter = FALSE;

      pcnrdinfo = (PCNRDRAGINFO)mp2;
      pdinfo = pcnrdinfo->pDragInfo;
      DrgAccessDraginfo(pdinfo);
      bDrop = TRUE;

      /*
         Should we allow dropping in white space ?
      */

      if (!pcnrdinfo->pRecord || bDragAfter)
         {
         USHORT usOp = pdinfo->usOperation;
         ULONG  ulItem = DrgQueryDragitemCount(pdinfo);
         ULONG  ulIndex;
         for (ulIndex = 0; ulIndex < ulItem; ulIndex++)
            {
            pditem = DrgQueryDragitemPtr(pdinfo, ulIndex);
            /*
               No moving about in the same dir
            */
            if (pditem->hwndItem == hwnd)
               {
               bDrop = FALSE;
               break;
               }
            /*
               Only these types
            */
            if (!DrgVerifyRMF(pditem, "DRM_OS2FILE", "DRF_TEXT"))
               {
#ifdef WPSDRAG
               if (!DrgVerifyRMF(pditem, "DRM_OBJECT",  "DRF_OBJECT"))
#endif
                  {
                  bDrop = FALSE;
                  break;
                  }
               }
            }

         switch(usOp)
            {
            case DO_DEFAULT:
               usOp = DO_MOVE;
               break;
            case DO_COPY:
               break;
            default     :
               bDrop = FALSE;
               break;
            }
         DrgFreeDraginfo(pdinfo);
         if (!bDrop)
            return(MPFROM2SHORT(DOR_NODROP, 0));
         else
            return(MPFROM2SHORT(DOR_DROP, usOp));
         }

      /*
         Dropping on top of a record
      */

      if (DrgQueryDragitemCount(pdinfo) != 1L)
         bDrop = FALSE;
      pditem = DrgQueryDragitemPtr(pdinfo, 0);
      if(bDrop && !DrgVerifyRMF(pditem, "DRM_OS2FILE", "DRF_TEXT"))
         bDrop = FALSE;

      if (bDrop)
         {
         pcrec = (PRECINFO)pcnrdinfo->pRecord;
         if (pcrec->finfo.chType == TYPE_DRIVE)
            bDrop = FALSE;
         if (strstr(pcrec->finfo.szFileName, ".ICO"))
            bDrop = FALSE;
         if (strstr(pcrec->finfo.szFileName, ".ico"))
            bDrop = FALSE;
         if (!stricmp(pcrec->finfo.szFileName, ".."))
            bDrop = FALSE;
         }


      if (bDrop)
         {
         DrgQueryStrName(pditem->hstrSourceName, sizeof szDragFname, szDragFname);
         if (hwnd == pditem->hwndItem &&
            !stricmp(szDragFname, pcrec->finfo.szFileName))
            bDrop = FALSE;
         else
            strupr(szDragFname);
         }

      if (bDrop && !strstr(szDragFname, ".ICO"))
         {
         if (!DrgVerifyTrueType(pditem, DRT_ICON))
            bDrop = FALSE;
         }


      DrgFreeDraginfo(pdinfo);
      if (!bDrop)
         return(MPFROM2SHORT(DOR_NODROP, 0));
      else
         return(MPFROM2SHORT(DOR_DROP, DO_LINK));

}


/***************************************************************
* Drag drop
**************************************************************/
VOID DragDrop(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
PINSTANCE     wd;
PCNRDRAGINFO  pcnrdinfo;
PDRAGINFO     pdinfo;
PDRAGITEM     pditem;
BYTE          szSrcName[MAX_PATH],
              szSrcPath[MAX_PATH],
              szTarget[MAX_PATH];
BYTE          szMessage[100];
PRECINFO      pcrec;

   wd = WinQueryWindowPtr(hwnd, 0);

   pcnrdinfo = (PCNRDRAGINFO)mp2;
   pdinfo = pcnrdinfo->pDragInfo;
   DrgAccessDraginfo(pdinfo);

   /*
      Receiving something in the container
   */
   if (!pcnrdinfo->pRecord || bDragAfter)
      {
      ULONG  ulItem = DrgQueryDragitemCount(pdinfo);
      ULONG  ulIndex;
      APIRET rc;
      BOOL   bExist;
      HOBJECT hObject;
      for (ulIndex = 0; ulIndex < ulItem; ulIndex++)
         {
         pditem = DrgQueryDragitemPtr(pdinfo, ulIndex);

         DrgQueryStrName(pditem->hstrContainerName, sizeof szSrcPath, szSrcPath);
         DrgQueryStrName(pditem->hstrSourceName, sizeof szSrcName, szSrcName);

#ifdef WPSDRAG
         if (DrgVerifyRMF(pditem, "DRM_OBJECT",  "DRF_OBJECT"))
            {
//            hObject = (HOBJECT) OBJECT_FROM_PREC(pditem->ulItemID);
            switch (pdinfo->usOperation)
               {
               case DO_COPY :
                  break;
               case DO_MOVE :
                  break;
               case DO_LINK :
                  break;
               }

            if (1)
               {
               DrgSendTransferMsg(pditem->hwndItem, DM_ENDCONVERSATION,
                  (MPARAM)pditem->ulItemID, (MPARAM)DMFL_TARGETFAIL);
               }
            else
               {
               DrgSendTransferMsg(pditem->hwndItem, DM_ENDCONVERSATION,
                  (MPARAM)pditem->ulItemID, (MPARAM)DMFL_TARGETSUCCESSFUL);
               }

            continue;
            }
#endif

         if (DrgVerifyRMF(pditem, "DRM_OS2FILE",  "DRF_TEXT"))
            {
            if (szSrcPath[strlen(szSrcPath) - 1] != '\\')
               strcat(szSrcPath, "\\");
            strcat(szSrcPath, szSrcName);

            strcpy(szTarget, wd->szCurDir);
            if (szTarget[strlen(szTarget) - 1] != '\\')
               strcat(szTarget, "\\");
            strcat(szTarget, szSrcName);

            bExist = !access(szTarget, 0);

            if ((rc = DosCopy(szSrcPath, szTarget, DCPY_EXISTING)) != 0)
               {
               sprintf(szMessage, "DosCopy returned %ld", rc);
               DebugBox("IconTool", szMessage, FALSE);
               DrgSendTransferMsg(pditem->hwndItem, DM_ENDCONVERSATION,
                  (MPARAM)pditem->ulItemID, (MPARAM)DMFL_TARGETFAIL);
               continue;
               }
            else
               {
               FILEINFO Finfo;
               FILESTATUS3 fStat;

               if (!bExist)
                  {
                  memset(&Finfo, 0, sizeof Finfo);
                  strcpy(Finfo.szFileName, szSrcName);

                  DosQueryPathInfo(szSrcPath,
                     FIL_STANDARD,
                     &fStat,
                     sizeof fStat);

                  if (fStat.attrFile & FILE_DIRECTORY)
                     Finfo.chType = TYPE_DIRECTORY;
                  else
                     Finfo.chType = TYPE_FILE;
                  GetIcons(szTarget, &Finfo, wd);

                  if (wd->bRealNames || !GetLongName(szTarget, Finfo.szTitle, sizeof Finfo.szTitle))
                     strcpy(Finfo.szTitle, Finfo.szFileName);
                  if (!pcnrdinfo->pRecord)
                     AddRecord(wd, (PRECINFO)CMA_END, &Finfo, TRUE);
                  else
                     AddRecord(wd, (PRECINFO)pcnrdinfo->pRecord, &Finfo, TRUE);
                  }

               if (pdinfo->usOperation == DO_MOVE)
                  {
                  if (!DeletePath(szSrcPath))
                     {
                     DrgSendTransferMsg(pditem->hwndItem, DM_ENDCONVERSATION,
                        (MPARAM)pditem->ulItemID, (MPARAM)DMFL_TARGETFAIL);
                     continue;
                     }
                  }
               DrgSendTransferMsg(pditem->hwndItem, DM_ENDCONVERSATION,
                  (MPARAM)pditem->ulItemID, (MPARAM)DMFL_TARGETSUCCESSFUL);
               }
            continue;
            }
         } /* end for */

      DrgFreeDraginfo(pdinfo);
      return;
      }

   /*
      Receiving something on a record
   */

   pditem = DrgQueryDragitemPtr(pdinfo, 0);
   pcrec  = (PRECINFO)pcnrdinfo->pRecord;
   
   DrgQueryStrName(pditem->hstrContainerName, sizeof szSrcPath, szSrcPath);
   DrgQueryStrName(pditem->hstrSourceName, sizeof szSrcName, szSrcName);

   if (szSrcPath[strlen(szSrcPath) - 1] != '\\')
      strcat(szSrcPath, "\\");
   strcat(szSrcPath, szSrcName);

   sprintf(szMessage, "Assign icon %s\nto %s ?",
      szSrcName, pcrec->finfo.szTitle);
   if (WinMessageBox(HWND_DESKTOP,HWND_DESKTOP, szMessage,"",
      0, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON1 ) == MBID_YES)
      {
      if (pcrec->finfo.chType == TYPE_ABSTRACT)
         {
         if (!SetIconToObject(pcrec->finfo.hObject, szSrcPath, FALSE))
            {
            DrgSendTransferMsg(pditem->hwndItem, DM_ENDCONVERSATION,
               (MPARAM)pditem->ulItemID, (MPARAM)DMFL_TARGETFAIL);
               {
               DrgFreeDraginfo(pdinfo);
               return;
               }
            }
         DeleteIcons(&pcrec->finfo);
         pcrec->finfo.hptrIcon = GetObjectIcon(pcrec->finfo.hObject, NULL);
         }
      else
         {
         BOOL fAnimated;

         strcpy(szTarget, wd->szCurDir);
         if (szTarget[strlen(szTarget) - 1] != '\\')
            strcat(szTarget, "\\");
         strcat(szTarget, pcrec->finfo.szFileName);

         fAnimated = FALSE;
         if (pcrec->finfo.chType == TYPE_FILE &&
               access(szTarget, 06))
            {
            DebugBox("IconTool", "Permission denied !", FALSE);
            DrgFreeDraginfo(pdinfo);
            return;
            }
         else
            fAnimated = wd->bAnimIcons;

         if (!SetIconToFile(szSrcPath, szTarget, fAnimated))
            {
            DrgSendTransferMsg(pditem->hwndItem, DM_ENDCONVERSATION,
               (MPARAM)pditem->ulItemID, (MPARAM)DMFL_TARGETFAIL);
               {
               DrgFreeDraginfo(pdinfo);
               return;
               }
            }
         DeleteIcons(&pcrec->finfo);
         GetIcons(szTarget, &pcrec->finfo, wd);
         }

      if (pcrec->finfo.hptrIcon)
         {
         PMINIRECORDCORE aprc[10] ;
         aprc[0] = &(pcrec->Record);
         pcrec->Record.hptrIcon = pcrec->finfo.hptrIcon;
         WinSendMsg(wd->hwndContainer, CM_INVALIDATERECORD,
            (MPARAM)aprc, MPFROM2SHORT(1, CMA_NOREPOSITION));
         }

      DrgSendTransferMsg(pditem->hwndItem, DM_ENDCONVERSATION,
         (MPARAM)pditem->ulItemID, (MPARAM)DMFL_TARGETSUCCESSFUL);
      }
   else
      DrgSendTransferMsg(pditem->hwndItem, DM_ENDCONVERSATION,
         (MPARAM)pditem->ulItemID, (MPARAM)DMFL_TARGETFAIL);

   DrgFreeDraginfo(pdinfo);
}

/********************************************************************
* DeletePath
********************************************************************/
BOOL DeletePath(PSZ pszPath)
{
APIRET rc;
BYTE   szFullName[270];
HDIR   FindHandle;
FILEFINDBUF3 find;
FILESTATUS3 fStat;
ULONG  ulFindCount;
BYTE szMess[80];
PULONG pulFldrContent;


   rc = DosQueryPathInfo(pszPath,
      FIL_STANDARD,
      &fStat,
      sizeof fStat);

   if (rc)
      {
      sprintf(szMess, "DosQueryPathInfo for %s failed (error %d)",
         pszPath, rc);
      DebugBox("ICONTOOL", szMess, FALSE);
      return FALSE;
      }

   if (!(fStat.attrFile & FILE_DIRECTORY))
      {
      rc = DosDelete(pszPath);
      if (rc)
         {
         sprintf(szMess, "Cannot delete %s (error %d)",
            pszPath, rc);
         DebugBox("ICONTOOL", szMess, FALSE);
         return FALSE;
         }
      return TRUE;
      }

   /*
      Delete all files from the DIR
   */

   strcpy(szFullName, pszPath);
   if (szFullName[strlen(szFullName) - 1] != '\\')
      strcat(szFullName, "\\");
   strcat(szFullName, "*.*");

   FindHandle = HDIR_CREATE;
   ulFindCount = 1;
   rc =  DosFindFirst(szFullName, &FindHandle, FILE_DIRECTORY | FILE_ARCHIVED, 
                    &find, sizeof (FILEFINDBUF3), &ulFindCount, 1);

   ulFindCount = 1;
   rc =  DosFindFirst(szFullName, &FindHandle,
      FILE_DIRECTORY | FILE_ARCHIVED | FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM, 
                    &find, sizeof (FILEFINDBUF3), &ulFindCount, 1);
   while (!rc)
      {
      if (find.achName[0] != '.')
         {
         strcpy(szFullName, pszPath);
         if (szFullName[strlen(szFullName) - 1] != '\\')
            strcat(szFullName, "\\");
         strcat(szFullName, find.achName);
         if (!DeletePath(szFullName))
            return FALSE;
         }
      ulFindCount = 1;
      rc = DosFindNext(FindHandle, &find, sizeof (FILEFINDBUF3), &ulFindCount);
      }
   DosFindClose(FindHandle);

   /*
      Delete all abstract objects
   */
   pulFldrContent = NULL;
   if (GetFolderContent(pszPath, (PBYTE *)&pulFldrContent, &ulFindCount))
      {
      USHORT usIndex;
      ulFindCount /= sizeof (ULONG);

      sprintf(szMess, "%s contains %d non-file objects, delete them?",
         pszPath, ulFindCount);
      if (WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, szMess, "Warning",
         0, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2) != MBID_YES)
         {
         free(pulFldrContent);
         return FALSE;
         }

      for (usIndex = 0; usIndex < ulFindCount; usIndex++)
         {
         if (!MyDestroyObject(MakeAbstractHandle(pulFldrContent[usIndex])))
            {
            sprintf(szMess, "Unable to delete object %lX!",
               MakeAbstractHandle(pulFldrContent[usIndex]));
            DebugBox("ICONTOOL", szMess, TRUE);
            free(pulFldrContent);
            return FALSE;
            }
         }
      free(pulFldrContent);
      }

   rc = DosDeleteDir(pszPath);
   if (rc)
      {
      sprintf(szMess, "Cannot remove directory %s (error %d)",
         pszPath, rc);
      DebugBox("ICONTOOL", szMess, FALSE);
      return FALSE;
      }
   return TRUE;
}

/*******************************************************************
* Set container attr
*******************************************************************/
VOID EndConversation(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
PRECINFO pcrec;
PINSTANCE wd;

   wd = WinQueryWindowPtr(hwnd, 0);
   if (!wd->usDragCount)
      return;
   wd->ulDragItemCount--;


   pcrec = (PRECINFO)mp1;
   if (mp2 == (MPARAM)DMFL_TARGETSUCCESSFUL)
      {
      if (wd->pdinfo->usOperation == DO_MOVE)
         RemoveRecord(wd->hwndContainer, pcrec);
      }
   if (!wd->ulDragItemCount)
      {
      if (wd->pdinfo->usOperation == DO_MOVE)
         {
         WinSendMsg(wd->hwndContainer, CM_ARRANGE, 0L, 0L);
         if (wd->bFlowed)
            {
            wd->bFlowed  = FALSE;
            SetContainerAttr(wd);
            wd->bFlowed  = TRUE;
            SetContainerAttr(wd);
            }
         }
      DrgFreeDraginfo(wd->pdinfo);
      wd->pdinfo = NULL;
      wd->usDragCount--;
      }
}


/*******************************************************************
* Set container attr
*******************************************************************/
BOOL SetContainerAttr(PINSTANCE wd)
{
CNRINFO cInfo;

   if (WinSendMsg(wd->hwndContainer,
      CM_QUERYCNRINFO,
      (MPARAM)&cInfo,
      (MPARAM)sizeof cInfo))
      {
      cInfo.flWindowAttr &= ~(CV_NAME | CV_FLOW | CV_ICON| CV_MINI |
         CA_DRAWBITMAP | CA_DRAWICON);
      cInfo.slBitmapOrIcon.cx = 0L;
      cInfo.slBitmapOrIcon.cy = 0L;

//      cInfo.slBitmapOrIcon.cx = WinQuerySysValue(HWND_DESKTOP, SV_CXICON);
//      cInfo.slBitmapOrIcon.cy = WinQuerySysValue(HWND_DESKTOP, SV_CYICON);

      if (wd->bFlowed)
         cInfo.flWindowAttr |= (CV_NAME | CV_FLOW);
      else
         cInfo.flWindowAttr |= CV_ICON;


      if (wd->bSmallIcons)
         {
         cInfo.flWindowAttr |= CV_MINI;
//         cInfo.slBitmapOrIcon.cx = WinQuerySysValue(HWND_DESKTOP, SV_CYMENU);
//         cInfo.slBitmapOrIcon.cy = WinQuerySysValue(HWND_DESKTOP, SV_CYMENU);
         }
      cInfo.flWindowAttr |= CA_DRAWICON;
//      cInfo.flWindowAttr |= CA_ORDEREDTARGETEMPH;
      cInfo.flWindowAttr |= CA_MIXEDTARGETEMPH;
      cInfo.cDelta = DELTA_VALUE;

      WinSendMsg(wd->hwndContainer,
         CM_SETCNRINFO,
         (MPARAM)&cInfo,
         (MPARAM)(CMA_FLWINDOWATTR | CMA_SLBITMAPORICON | CMA_DELTA));
      return TRUE;                  
      }
   return FALSE;

}
BOOL SetIconToFile(PSZ pszIcon, PSZ pszFile, BOOL fAnimated)
{
HOBJECT hObject;


   hObject = WinQueryObject(pszFile);
   if (!hObject)
      {
      DebugBox("ICONTOOL","WinQueryObject", TRUE);
      return FALSE;
      }
   return SetIconToObject(hObject, pszIcon, fAnimated);
}

BOOL SetIconToObject(HOBJECT hObject, PSZ pszIcon, BOOL fAnimated)
{
BYTE    szOption[200];

   if (fAnimated)
      sprintf(szOption, "ICONNFILE=1,%s", pszIcon);
   else
      sprintf(szOption, "ICONFILE=%s", pszIcon);
   if (!MySetObjectData(hObject, szOption))
      {
      DebugBox("ICONTOOL","WinSetObjectData", TRUE);
      return FALSE;
      }
   return TRUE;
}

