#define DEBUG
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <direct.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>

#define INCL_GPI
#define INCL_WIN
#define INCL_DOS
#define INCL_DEV
#define INCL_DOSERRORS
#define INCL_WINWORKPLACE
#include <os2.h>
                   
#include "..\wptools\wptools.h"
#include "icon.h"
#include "icondef.h"

PSZ _System pszGetObjectSettings(HOBJECT hObject, PSZ pszCls, USHORT usMax, BOOL fAddComment);
PSZ _System DumpObjectData(HOBJECT hObject);

#define CRA_SOURCE 0x00004000L
#define MAX_FIND 200

#define OBJECTID_NOT_SET "< (not set) >"
#define LOCATION       "PM_Workplace:Location"

#define OPEN_UNKNOWN      -1
#define OPEN_DEFAULT       0
#define OPEN_CONTENTS      1
#define OPEN_SETTINGS      2
#define OPEN_HELP          3
#define OPEN_RUNNING       4
#define OPEN_PROMPTDLG     5
#define OPEN_PALETTE       121                                   /*SPLIT*/
#define OPEN_USER          0x6500

/*******************************************************************
* global static variables
*******************************************************************/
static HMQ hmq;
static PSZ  pszTitle = "IconTool showing ";
static HPOINTER hptrDrive;
static HWND     hwndPopup, hwndPopupSys, hwndPopupEA;
static HWND     hwndHelp;
static BOOL     fHelpEnabled;
static BYTE     szHelpFname[256];
static USHORT   usIconCX;
/*******************************************************************
* global public variables
*******************************************************************/
HAB      hab;
USHORT   usWindowCount = 0;
HWND     hwndFirst = 0L, hwndSecond = 0L;
HPOINTER hptrUnknown;

/*******************************************************************
* local function prototypes
*******************************************************************/
static HWND CreateWindow(HWND);
static MRESULT EXPENTRY WindowProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
static VOID msg_wait_remove            (PINSTANCE);
static VOID msg_wait                   (PINSTANCE);
static BOOL ClearContainer             (PINSTANCE wd);
static BOOL StartProgram(HWND hwnd, PRECINFO pcrec, USHORT usAssocIndex);
static BOOL RemoveInUse(HWND hwnd, HAPP happ);
static VOID ExtractIcon(HWND hwnd);
static VOID ConditionalCascadeMenu(HWND hwndMenu, USHORT usSubMenuID, USHORT usDefID);
static PRECINFO CreateRecord(PINSTANCE wd, PRECINFO pAfter, PFILEINFO pFinfo);
static VOID PrepareCnrPopup    (PINSTANCE wd, PRECINFO pcrec);
static VOID PrepareEAPopup  (PINSTANCE wd);
static VOID OpenRecord      (HWND hwnd, PRECINFO pcrec, USHORT usAssocIndex);
static HWND InitHelpInstance(HWND hwndMain);
static BOOL WindowsSessionActive(VOID);
static VOID ShowSetupString(HWND hwnd, PSZ pszTitle, PSZ pszSetup);
static BOOL CreateIcon2(HWND hwnd, PRECINFO pcrec, USHORT usAssocIndex);
static VOID CreateIcon(HWND hwnd, PRECINFO pcrec, USHORT usAssocIndex);
static VOID SetObjectID(HWND hwnd, HOBJECT hObject, PSZ pszTitle);
static MRESULT EXPENTRY SetObjIDProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);


APIRET16 APIENTRY16 DosGetPPID  (USHORT pidChild,
                               PUSHORT ppidParent);


#pragma linkage(FillContainerThread, optlink)
static VOID FillContainerThread(PVOID);

/*************************************************************
* The main function
**************************************************************/
int main(int argc, PSZ *pszArg)
{
QMSG  qmsg;
PINSTANCE wd;
PBYTE  p;

//   DosError(FERR_DISABLEHARDERR | FERR_DISABLEEXCEPTION);

   strcpy(szHelpFname, pszArg[0]);
   p = strrchr(szHelpFname, '\\');
   if (p)
      {
      *p = 0;
      strcat(szHelpFname, "\\icon.hlp");
      }
   else
      strcpy(szHelpFname, "icon.hlp");


   hab = WinInitialize(0);
   if (!hab)
      {
      DebugBox("ICONTOOL", "Fout bij hab !", TRUE);
      exit(1);
      }

   hmq = WinCreateMsgQueue( hab, 50 );
   if (!hmq)
      {
      DebugBox("ICONTOOL", "Fout bij hmq !", TRUE);
      exit(1);
      }

   usIconCX = WinQuerySysValue(HWND_DESKTOP, SV_CXICON);

   hptrDrive    = WinLoadPointer(HWND_DESKTOP,0L, ID_DRIVEICON);
   hptrUnknown  = WinLoadPointer(HWND_DESKTOP,0L, ID_UNKNOWNICO);

   hwndPopup    = WinLoadMenu(HWND_DESKTOP, 0L, ID_POPUPMENU);
   hwndPopupSys = WinLoadMenu(HWND_DESKTOP, 0L, ID_POPUPSYS);
   hwndPopupEA  = WinLoadMenu(HWND_DESKTOP, 0L, ID_POPUPEA);

   ConditionalCascadeMenu(hwndPopupSys, ID_SUBMENUOPEN, ID_CHOPTS);
   ConditionalCascadeMenu(hwndPopupSys, ID_HELPMENU, ID_GENERALHELP);

   ConditionalCascadeMenu(hwndPopup,    ID_SUBMENUOBJECT, ID_OPENOBJECT);
   ConditionalCascadeMenu(hwndPopup,    ID_POPUPHELPMENU, ID_GENERALHELP);

   ConditionalCascadeMenu(hwndPopupEA,  ID_SUBMENUOPEN, ID_CHOPTS);
   ConditionalCascadeMenu(hwndPopupEA,  ID_HELPMENU, ID_GENERALHELP);



   if (!WinRegisterClass(hab,
      "IconToolClass", 
      WindowProc, 
      CS_SIZEREDRAW,                    
      sizeof (PVOID)))
      {
      DebugBox("ICONTOOL", "Fout bij WinRegisterClass !", TRUE);
      exit(1);
      }

   hwndFirst = CreateWindow(HWND_DESKTOP);
   hwndHelp = InitHelpInstance(hwndFirst);


   wd = WinQueryWindowPtr(WinWindowFromID(hwndFirst, FID_CLIENT), 0);
   if (wd->bSecondWindowOpen)
      hwndSecond = CreateWindow(HWND_DESKTOP);

   while( WinGetMsg( hab, &qmsg, 0L, 0, 0 ) )
       WinDispatchMsg( hab, &qmsg );

   WinDestroyHelpInstance(hwndHelp);
   WinDestroyMsgQueue( hmq );            /* and                          */
   WinTerminate( hab );                  /* terminate the application    */

   return 0;
}

/*************************************************************
* Create the icon window
**************************************************************/
HWND CreateWindow(HWND hwndParent)
{
ULONG ulStyle;
HWND hwndWindow,
     hwndClient;
BYTE szWindowPos[15];
HWND hwndSysMenu,
     hwndNew;
PVOID pResource;
MENUITEM mItem;
USHORT usID;
PINSTANCE wd;



   ulStyle = FCF_SYSMENU | FCF_TITLEBAR | FCF_HIDEMAX | FCF_ICON |
      FCF_SIZEBORDER | FCF_NOBYTEALIGN | FCF_TASKLIST ;

   hwndWindow = WinCreateStdWindow(hwndParent,      
      WS_ANIMATE,
      &ulStyle,          
      "IconToolClass",    
      pszTitle,
      0L,                
      (HMODULE)0,        
      ID_WINDOW,            
      &hwndClient
      );

   if (!hwndWindow)
      {
      DebugBox("ICONTOOL", "Fout bij creeren window !", TRUE);
      exit(1);
      }

   wd = WinQueryWindowPtr(hwndClient, 0);

   sprintf(szWindowPos,"WINDOWPOS%d", (hwndFirst ? 2 : 1));
   if (!WinRestoreWindowPos(APPS_NAME, szWindowPos, hwndWindow))
      WinSetWindowPos(hwndWindow,
                  HWND_TOP, 100, 90, 400, 300,
                  SWP_SIZE | SWP_ACTIVATE | SWP_SHOW | SWP_MOVE);
   else
      WinSetWindowPos(hwndWindow,
                  HWND_TOP, 0, 0, 0, 0,
                  SWP_ACTIVATE | SWP_SHOW | SWP_RESTORE);


   /*
      Replace the system menu
   */

   DosGetResource(0, RT_MENU, ID_POPUPSYS, &pResource);

   hwndNew = WinCreateWindow(hwndWindow,
      WC_MENU,                                    /* class     */
      "",                                         /* text      */
      0,
      1, 1, 1, 1,
      hwndWindow,                                 /* Client    */
      HWND_TOP,                                   /* behind    */
      ID_POPUPSYS,                                 /* win ID    */
      pResource,                                  /* ctl data  */
      (PVOID) NULL);                              /* reserved  */

   if (!hwndNew)
      DebugBox("ICONTOOL", "Create Sysmenu", TRUE);

   DosFreeResource(pResource);


   /*
      Get handle of sysmenu, query the first item, which should be a submenu
      than save this submenu, and install ours
   */
   wd->hwndMenu = hwndSysMenu = WinWindowFromID(hwndWindow, FID_SYSMENU);
   usID = SHORT1FROMMR(WinSendMsg(hwndSysMenu, MM_ITEMIDFROMPOSITION, 0L, 0L));

   WinSendMsg(hwndSysMenu, MM_QUERYITEM,
      MPFROM2SHORT(usID, FALSE), (MPARAM)&mItem);
   mItem.hwndSubMenu = hwndNew;
   WinSendMsg(hwndSysMenu, MM_SETITEM,
      MPFROM2SHORT(usID, FALSE), (MPARAM)&mItem);

   /*
      Change window style of submenu 'Open'
   */
   ConditionalCascadeMenu(hwndNew, ID_SUBMENUOPEN, ID_CHOPTS);
   ConditionalCascadeMenu(hwndNew, ID_HELPMENU, ID_GENERALHELP);

   WinSendMsg(hwndWindow, WM_UPDATEFRAME, (MPARAM)FCF_SYSMENU, 0L);


   usWindowCount++;
   return hwndWindow;
}

/*************************************************************************
* Change a submenu to conditional cascade and set default option
*************************************************************************/
VOID ConditionalCascadeMenu(HWND hwndMenu, USHORT usSubMenuID, USHORT usDefID)
{
MENUITEM mItem;
ULONG    ulStyle;

   if (!WinSendMsg(hwndMenu, MM_QUERYITEM,
      MPFROM2SHORT(usSubMenuID, TRUE), (MPARAM)&mItem))
      return;

   ulStyle = WinQueryWindowULong(mItem.hwndSubMenu, QWL_STYLE);
   ulStyle |= MS_CONDITIONALCASCADE;
   WinSetWindowULong(mItem.hwndSubMenu, QWL_STYLE, ulStyle);
   WinSendMsg(hwndMenu, MM_SETITEM,
      MPFROM2SHORT(usSubMenuID, TRUE), (MPARAM)&mItem);

   WinSendMsg(mItem.hwndSubMenu, MM_SETDEFAULTITEMID, MPFROMSHORT(usDefID), 0L);
}
/*************************************************************************
* Winproc procedure for the panel
*************************************************************************/
MRESULT EXPENTRY WindowProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
PINSTANCE    wd;
SWP          swp;
PRECINFO     pcrec;
MRESULT      mResult;
PNOTIFYRECORDENTER pEnter;
PNOTIFYRECORDEMPHASIS pEmphas;
PCNREDITDATA pcEdit;


   wd = WinQueryWindowPtr(hwnd, 0);

   switch( msg )
      {
      case WM_CREATE:
         CreateContainer(hwnd);
         return (MRESULT)FALSE;

      case WM_REFILL:
         FillContainer(hwnd, wd);
         break;

       case WM_DELDLG:
         DeleteDialog(hwnd);
         break;

       case WM_ERASEBACKGROUND:
         return (MRESULT)TRUE;

       case WM_SIZE:
         {
         INT iMLEHeight;
         LONG lDiff = 15;

         WinQueryWindowPos(hwnd, &swp);

         iMLEHeight = swp.cy / 4;
         if (wd->usReadEA == READEA_NONE)
            iMLEHeight = 0;

         WinSetWindowPos(wd->hwndContainer, HWND_TOP,
            0,
            iMLEHeight,
            swp.cx,
            swp.cy - iMLEHeight,  
            SWP_SIZE | SWP_SHOW | SWP_MOVE);

         if (iMLEHeight && iMLEHeight > 15)
            {
            WinSetWindowPos(wd->hwndStatic, HWND_TOP,
               0,
               iMLEHeight - lDiff,
               swp.cx,
               lDiff,  
               SWP_SIZE | SWP_SHOW | SWP_MOVE);

            WinSetWindowPos(wd->hwndMLE, HWND_TOP,
               0,
               0,
               swp.cx,
               iMLEHeight - lDiff,  
               SWP_SIZE | SWP_SHOW | SWP_MOVE);
            }
         else
            {
            WinShowWindow(wd->hwndMLE, FALSE);
            WinShowWindow(wd->hwndStatic, FALSE);
            }
         }
         break;

   /*
      Appearantly next both message's are NOT send to the container when
      initiated by the accelerator, so we'll send them!
   */
      case WM_TEXTEDIT   :
         WinPostMsg(wd->hwndContainer, msg, mp1, mp2);
         break;

      case WM_CONTEXTMENU:
         {
         HWND     hwndFocus;
         POINTL   Pointl;

         if (SHORT1FROMMP(mp2)) /* activated by pointer */
            {
            PPOINTS pPoints = (PPOINTS)&mp1;
            Pointl.x = pPoints->x;
            Pointl.y = pPoints->y;
            hwndFocus = WinWindowFromPoint(hwnd, &Pointl, FALSE);
            if (hwndFocus == wd->hwndContainer)
               {
               WinPostMsg(wd->hwndContainer, msg, mp1, mp2);
               break;
               }
            }
         else
            {
            hwndFocus = WinQueryFocus(HWND_DESKTOP);
            if (hwndFocus == wd->hwndContainer ||
               WinIsChild(hwndFocus, wd->hwndContainer))
               {
               WinPostMsg(wd->hwndContainer, msg, mp1, mp2);
               break;
               }
            Pointl.x = 10;
            Pointl.y = 10;
            }

         PrepareEAPopup(wd);
         WinPopupMenu(wd->hwndMLE,
            hwnd,
            hwndPopupEA,
            Pointl.x,
            Pointl.y,
            0L,
            PU_HCONSTRAIN|PU_VCONSTRAIN|
            PU_MOUSEBUTTON1|PU_MOUSEBUTTON2|PU_KEYBOARD);
         }
         break;

      case WM_CLOSE       :
         if (wd->iFillThread)
            {
            APIRET rc;
            wd->bScanActive = FALSE;
            rc = DosKillThread(wd->iFillThread);
            wd->iFillThread = 0;
            if (rc)
               DebugBox("ICONTOOL", "Unable to kill thread!", FALSE);
            }

         if (wd->bSaveOnExit)
            SaveOptions(hwnd);
         if (wd->usWindowNo == 1)
            hwndFirst = 0L;
         else
            hwndSecond = 0L;
         if (usWindowCount == 1)
            WinPostMsg( hwnd, WM_QUIT, 0L, 0L );  /* Cause termination        */
         else
            {
            WinDestroyWindow(WinQueryWindow(hwnd, QW_PARENT));
            usWindowCount--;
            }
         break;

    case WM_DESTROY:
         ClearContainer(wd);
         free(wd);
         break;

    case WM_COMMAND:
         switch (SHORT1FROMMP(mp1))          /* test the command value from mp1 */
            {
            case ID_DELETE:
               DeleteObject(hwnd);
               break;
            case ID_REMOVE:
               if (wd->pcrec->finfo.chType != TYPE_DRIVE &&
                   wd->pcrec->finfo.chType != TYPE_ABSTRACT &&
                   stricmp(wd->pcrec->finfo.szFileName, ".."))
                  {
                  BYTE     szTarget[MAX_PATH];
                  PSZ      pszEA;
                  strcpy(szTarget, wd->szCurDir);
                  if (szTarget[strlen(szTarget) - 1] != '\\')
                     strcat(szTarget, "\\");
                  strcat(szTarget, wd->pcrec->finfo.szFileName);

                  if (wd->pcrec->finfo.chType == TYPE_DIRECTORY &&
                      wd->bAnimIcons)
                     pszEA = ".ICON1";
                  else
                     pszEA = ".ICON";

                  if (wd->pcrec->finfo.chType == TYPE_FILE && access(szTarget, 06))
                     DebugBox("IconTool", "Permission denied !", FALSE);
                  else
                     SetEAValue(szTarget, pszEA, EA_LPICON, "", 0);

                  DeleteIcons(&wd->pcrec->finfo);
                  GetIcons(szTarget, &wd->pcrec->finfo, wd);

                  if (wd->pcrec->finfo.hptrIcon)
                     {
                     PMINIRECORDCORE aprc[10] ;
                     aprc[0] = &(wd->pcrec->Record);
                     wd->pcrec->Record.hptrIcon = wd->pcrec->finfo.hptrIcon;
                     WinSendMsg(wd->hwndContainer, CM_INVALIDATERECORD,
                        (MPARAM)aprc, MPFROM2SHORT(1, CMA_NOREPOSITION));
                     }
                  }
               break;
            case ID_EXTRACT:
               ExtractIcon(hwnd);
               break;
            case ID_ABOUT:
            case ID_HELPPRODUCTINFO :
               AboutBoxDlg(hwnd);
               break;
            case ID_USINGHELP       :
               if (fHelpEnabled)
                  WinSendMsg(hwndHelp, HM_DISPLAY_HELP, 0, 0);
               break;
            case ID_HELPINDEX       :
               if (fHelpEnabled)
                  WinSendMsg(hwndHelp, HM_HELP_INDEX, 0, 0);
               break;
            case ID_KEYSHELP        :
               if (fHelpEnabled)
                  WinSendMsg(hwndHelp, HM_KEYS_HELP, 0, 0);
               break;
            case ID_GENERALHELP     :
               if (fHelpEnabled)
                  WinSendMsg(hwndHelp, HM_EXT_HELP, 0, 0);
               break;
            case ID_OPEN:
               ChangeDirDialog(hwnd);
               break;
            case ID_NEW :
               if (!hwndFirst)
                  hwndFirst = CreateWindow(HWND_DESKTOP);
               else if (!hwndSecond)
                  hwndSecond = CreateWindow(HWND_DESKTOP);
               else
                  DebugBox("ICONTOOL", "Sorry, only two windows allowed !", FALSE);
               break;
            case ID_REFRESH:
               WinSendMsg(hwnd, WM_REFILL, 0, 0L);
               break;
            case ID_CHOPTS:
               OptionsDialog(hwnd);
               break;
            case ID_SAVOPTS:
               SaveOptions(hwnd);
               break;
            case ID_SETTING:
               {
               HOBJECT hObject;
               hObject = wd->pcrec->finfo.hObject;
               if (hObject)
                  WinOpenObject(hObject, OPEN_SETTINGS, FALSE);
//                  MySetObjectData(hObject, "OPEN=SETTINGS");
               }
               break;
            case ID_QUERYSETTING:
            case ID_DUMPOBJDATA:
               {
               HOBJECT hObject;
               hObject = wd->pcrec->finfo.hObject;
               if (hObject)
                  {
                  if (SHORT1FROMMP(mp1) == ID_QUERYSETTING)
                     {
                     PSZ pszSet = pszGetObjectSettings(hObject, NULL, 0, TRUE);
                     if (!pszSet)
                        DebugBox("OBJECTSETTINGS", "Not found!", FALSE);
                     else
                        ShowSetupString(hwnd, wd->pcrec->finfo.szTitle, pszSet);
                     }
                  else
                     {
                     PSZ pszSet = DumpObjectData(hObject);
                     if (!pszSet)
                        DebugBox("OBJECTSETTINGS", "Not found!", FALSE);
                     else
                        ShowSetupString(hwnd, wd->pcrec->finfo.szTitle, pszSet);
                     }
                  }
               }
               break;

            case ID_SETOBJECTID:
               SetObjectID(hwnd, wd->pcrec->finfo.hObject, wd->pcrec->finfo.szTitle);
               break;

            case ID_SAVEEA:
               ReadEAValue(hwnd, NULL, 2);
               ReadEAValue(hwnd, NULL, TRUE);
               break;

            case ID_EASUBJECT    :
            case ID_EACOMMENTS   :   
            case ID_EAKEYPHRASES :   
            case ID_EAHISTORY    :
               switch (SHORT1FROMMP(mp1))          /* test the command value from mp1 */
                  {
                  case ID_EASUBJECT    :
                     wd->usReadEA = READEA_SUBJECT;
                     break;
                  case ID_EACOMMENTS   :   
                     wd->usReadEA = READEA_COMMENTS;
                     break;
                  case ID_EAKEYPHRASES :   
                     wd->usReadEA = READEA_KEYPHRASES;
                     break;
                  case ID_EAHISTORY    :
                     wd->usReadEA = READEA_HISTORY;
                     break;
                  }
               ReadEAValue(hwnd, NULL, FALSE);
               ReadEAValue(hwnd, NULL, TRUE);
               break;

            default :
               if (SHORT1FROMMP(mp1) >= ID_OPENOBJECT &&
                   SHORT1FROMMP(mp1) < ID_OPENOBJECT + MAX_ASSOC + 1)
                  OpenRecord(hwnd, wd->pcrec, SHORT1FROMMP(mp1) - ID_OPENOBJECT);
               else if (SHORT1FROMMP(mp1) >= ID_CREATEOBJECT &&
                   SHORT1FROMMP(mp1) < ID_CREATEOBJECT + MAX_ASSOC + 1)
                  CreateIcon(hwnd, wd->pcrec, SHORT1FROMMP(mp1) - ID_CREATEOBJECT);
               else
                  return WinDefWindowProc( hwnd, msg, mp1, mp2 );
               break;
            }
         break;

      case WM_CONTROL:
         if (SHORT1FROMMP(mp1) == ID_CONTAINER)
            {
            switch (SHORT2FROMMP(mp1))
               {
               case CN_HELP    :
                  if (fHelpEnabled)
                     WinSendMsg(hwndHelp, HM_EXT_HELP, 0, 0);
                  break;

               case CN_INITDRAG:
                  InitDrag(hwnd, mp1, mp2);
                  break;

               case CN_DRAGOVER:
               case CN_DRAGAFTER:
                  return DragOver(hwnd, mp1, mp2);

               case CN_DROP:
                  DragDrop(hwnd, mp1, mp2);
                  break;

               case CN_CONTEXTMENU:
                  pcrec = (PRECINFO)mp2;
                  if (wd->bScanActive)
                     break;
                  if (pcrec)
                     {
                     RECTL  rectl;
                     QUERYRECORDRECT qRect;

                     wd->pcrec = pcrec;
                     memset(&qRect, 0, sizeof qRect);
                     qRect.cb       = sizeof qRect;
                     qRect.pRecord  = (PRECORDCORE)pcrec;
                     qRect.fsExtent = CMA_ICON;

                     WinSendMsg(wd->hwndContainer, CM_QUERYRECORDRECT,
                        (MPARAM)&rectl, (MPARAM)&qRect);
                     PrepareCnrPopup(wd, pcrec);
                     WinSendMsg(wd->hwndContainer, CM_SETRECORDEMPHASIS,
                        (MPARAM)wd->pcrec, MPFROM2SHORT(TRUE, CRA_SOURCE));
                     WinPopupMenu(wd->hwndContainer,
                        hwnd,
                        hwndPopup,
                        rectl.xRight,
                        rectl.yBottom,
                        0L,
                        PU_HCONSTRAIN|PU_VCONSTRAIN|
                        PU_MOUSEBUTTON1|PU_MOUSEBUTTON2|PU_KEYBOARD);
                     }
                  else
                     {
                     POINTL ptl;

                     wd->pcrec = NULL;
                     WinSendMsg(wd->hwndContainer, CM_SETRECORDEMPHASIS,
                        (MPARAM)wd->pcrec, MPFROM2SHORT(TRUE, CRA_SOURCE));
                     WinQueryPointerPos(HWND_DESKTOP, &ptl);
                     WinPopupMenu(HWND_DESKTOP,
                        WinQueryWindow(hwnd, QW_PARENT),
                        hwndPopupSys,
                        ptl.x,
                        ptl.y,
                        0L,
                        PU_HCONSTRAIN|PU_VCONSTRAIN|
                        PU_MOUSEBUTTON1|PU_MOUSEBUTTON2|PU_KEYBOARD);
                     }
                  break;

               case CN_BEGINEDIT:
                  pcEdit = (PCNREDITDATA) mp2;
                  if (pcEdit->pRecord)
                     wd->bEdit = TRUE;
                  break;

               case CN_REALLOCPSZ:
                  pcEdit = (PCNREDITDATA) mp2;
                  if (!pcEdit->pRecord)
                     break;
                  if (pcEdit->cbText > MAX_PATH - 1)
                     return (MRESULT)FALSE;
                  else
                     return (MRESULT)TRUE;

               case CN_ENDEDIT:
                  pcEdit = (PCNREDITDATA) mp2;
                  if (pcEdit->pRecord)
                     {
                     BYTE szFullName[MAX_PATH];

                     pcrec = (PRECINFO)pcEdit->pRecord;
                     strcpy(szFullName, wd->szCurDir);
                     if (szFullName[strlen(szFullName) - 1] != '\\')
                        strcat(szFullName, "\\");
                     strcat(szFullName, pcrec->finfo.szFileName);
                     if (!SetEAValue(szFullName, ".LONGNAME", EA_LPASCII,
                        *pcEdit->ppszText, strlen(*pcEdit->ppszText) + 1))
                        break;
                     }

                  wd->bEdit = FALSE;
                  break;

               case CN_ENTER:
                  pEnter = (PNOTIFYRECORDENTER)mp2;
                  if (!pEnter->pRecord)
                     break;
                  pcrec = (PRECINFO)pEnter->pRecord;
                  OpenRecord(hwnd, pcrec, 0);
                  return (MRESULT)TRUE;

               case CN_EMPHASIS:
                  if (wd->bScanActive)
                     break;
                  pEmphas = (PNOTIFYRECORDEMPHASIS)mp2;
                  if (!pEmphas->pRecord)
                     break;
                  if (!(pEmphas->fEmphasisMask & CRA_CURSORED))
                     break;
                  pcrec = (PRECINFO)pEmphas->pRecord;
                  ReadEAValue(hwnd, pcrec,
                     (pcrec->Record.flRecordAttr & CRA_CURSORED ? TRUE : FALSE));
                  return (MRESULT)TRUE;

               default :
                  return WinDefWindowProc( hwnd, msg, mp1, mp2 );
               }
            }
         else
            return WinDefWindowProc( hwnd, msg, mp1, mp2 );
         break;

      case WM_MENUEND:
         {
         HWND hwndEnd = (HWND)mp2;
         if (hwndEnd == hwndPopup)
            {
            WinSendMsg(wd->hwndContainer, CM_SETRECORDEMPHASIS,
               (MPARAM)wd->pcrec, MPFROM2SHORT(FALSE, CRA_SOURCE));
            }
         else if (hwndEnd == hwndPopupSys)
            {
            WinSendMsg(wd->hwndContainer, CM_SETRECORDEMPHASIS,
               (MPARAM)NULL, MPFROM2SHORT(FALSE, CRA_SOURCE));
            }
         else
            return WinDefWindowProc( hwnd, msg, mp1, mp2 );
         break;
      }

      case HM_QUERY_KEYS_HELP:
         return ((MRESULT)4119);

      case DM_ENDCONVERSATION:
         EndConversation(hwnd, mp1, mp2);
         break;

      case DM_DISCARDOBJECT:
         DiscardObject(hwnd, mp1, mp2);
         return (MRESULT)DRR_SOURCE;

      case DM_DRAGOVERNOTIFY :
         break;

      case WM_CONTROLPOINTER:
         if ((SHORT)mp1 == ID_CONTAINER)
            {
            if (wd->bMsgWait)
               mResult = (MRESULT) WinQuerySysPointer(HWND_DESKTOP, SPTR_WAIT, FALSE);
            else
               mResult = (MRESULT) mp2;
            return mResult;
            }
         else
            return WinDefWindowProc( hwnd, msg, mp1, mp2 );

      case WM_APPTERMINATENOTIFY:
         RemoveInUse(hwnd, (HAPP)mp1);
         break;

      default:
         return WinDefWindowProc( hwnd, msg, mp1, mp2 );
      }
   return (MRESULT)FALSE;
}

/*************************************************************************
* Open a record
*************************************************************************/
VOID OpenRecord(HWND hwnd, PRECINFO pcrec, USHORT usAssocIndex)
{
PINSTANCE wd = WinQueryWindowPtr(hwnd, 0);
BYTE szFullName[MAX_PATH];
PBYTE pszExt;

   strcpy(szFullName, wd->szCurDir);
   if (szFullName[strlen(szFullName) - 1] != '\\')
      strcat(szFullName, "\\");
   strcat(szFullName, pcrec->finfo.szFileName);

   memset(wd->szLastDir, 0, sizeof wd->szLastDir);
   switch (pcrec->finfo.chType)
      {
      case TYPE_FILE     :
         /*
            Is the default choosen ?
         */
         if (!usAssocIndex)
            {
            pszExt = strrchr(pcrec->finfo.szFileName, '.');
            if (pszExt)
               {
               if (!stricmp(pszExt, ".EXE") || !stricmp(pszExt, ".COM") ||
                  !stricmp(pszExt, ".BAT") || !stricmp(pszExt, ".CMD"))
                  {
                  StartProgram(hwnd, pcrec, usAssocIndex);
                  break;
                  }
               }
            GetAssocFilter(szFullName, pcrec->finfo.hObjAssoc, MAX_ASSOC);
            usAssocIndex = 1;
            }
         StartProgram(hwnd, pcrec, usAssocIndex);
         break;

      case TYPE_ABSTRACT :
         StartProgram(hwnd, pcrec, usAssocIndex);
         break;

      case TYPE_DRIVE    :
         if (access(pcrec->finfo.szFileName, 0))
            break;
         if (!_getdcwd((INT)(*pcrec->finfo.szFileName - '@'),
            wd->szCurDir, sizeof wd->szCurDir))
            {
            DebugBox("ICONTOOL",_strerror("Change drive"), FALSE);
            break;
            }
         WinSendMsg(hwnd, WM_REFILL, 0, 0);
         break;
                            	
      case TYPE_DIRECTORY:
         if (!strcmp(pcrec->finfo.szFileName, ".."))
            {
            PBYTE p = strrchr(wd->szCurDir, '\\');
            if (!p)
               return;
            strcpy(wd->szLastDir, p + 1);
            if (p - wd->szCurDir < 3)
               p++;
            *p = 0;
            }
         else
            strcpy(wd->szCurDir, szFullName);

         WinSendMsg(hwnd, WM_REFILL, 0, 0);
         break;
      }
}

/*************************************************************************
* Create an icon
*************************************************************************/
VOID CreateIcon(HWND hwnd, PRECINFO pcrec, USHORT usAssocIndex)
{
PBYTE pszExt;
BOOL  fIsProgram;
   
   fIsProgram = FALSE;
   pszExt = strrchr(pcrec->finfo.szFileName, '.');
   if (pszExt)
      {
      if (!stricmp(pszExt, ".EXE") || !stricmp(pszExt, ".COM") ||
         !stricmp(pszExt, ".BAT") || !stricmp(pszExt, ".CMD"))
         {
         fIsProgram = TRUE;
         }
      }


   if (!fIsProgram)
      usAssocIndex++;
   CreateIcon2(hwnd, pcrec, usAssocIndex);
}

BOOL CreateIcon2(HWND hwnd, PRECINFO pcrec, USHORT usAssocIndex)
{
static BYTE      szParameters[MAX_PATH];
static BYTE      szProgramName[MAX_PATH];
static BYTE      szCurDir[MAX_PATH];
static BYTE      szSettings[512];
PINSTANCE wd = WinQueryWindowPtr(hwnd, 0);
PSZ  p;
HOBJECT hObject = 0L;

   memset(szParameters, 0, sizeof szParameters);
   memset(szCurDir, 0, sizeof szCurDir);
      
   /*
      Get full name of selected object
   */
   strcpy(szProgramName, wd->szCurDir);
   if (szProgramName[strlen(szProgramName) - 1] != '\\')
      strcat(szProgramName, "\\");
   strcat(szProgramName, pcrec->finfo.szFileName);

   /*
      Is a associated prog selected?
   */
   if (usAssocIndex > 0)
      {
      usAssocIndex--;
      strcpy(szParameters, szProgramName);
      hObject = pcrec->finfo.hObjAssoc[usAssocIndex];
      if (!hObject)
         {
         DebugBox("ICONTOOL", "Cannot create an icon for this file !", FALSE);
         return FALSE;
         }
      if (!GetObjectName(hObject, szProgramName, sizeof szProgramName))
         {
         DebugBox("ICONTOOL", "Unable to find associated program name!", FALSE);
         return FALSE;
         }
      strcpy(szCurDir, szProgramName);
      p = strrchr(szCurDir, '\\');
      if (p)
         *p = 0;
      }

   sprintf(szSettings, "EXENAME=%s;STARTUPDIR=%s;PARAMETERS=%s",
      szProgramName, szCurDir, szParameters);

   hObject = WinCreateObject("WPProgram",
      pcrec->finfo.szTitle,
      szSettings,
      "<WP_DESKTOP>",
      CO_REPLACEIFEXISTS);
   if (!hObject)
      DebugBox("ICONTOOL", "Unable to create icon", TRUE);
   return TRUE;
}

/*************************************************************************
* Open a record
*************************************************************************/
VOID ReadEAValue(HWND hwnd, PRECINFO pcrec, BOOL fRead)
{
PINSTANCE wd = WinQueryWindowPtr(hwnd, 0);
PSZ  pszEA, pszEAName;
USHORT usSize;

   if (!pcrec)
      {
      pcrec = WinSendMsg(wd->hwndContainer,
            CM_QUERYRECORDEMPHASIS, (MPARAM)CMA_FIRST, (MPARAM)CRA_CURSORED);
      if ((LONG)pcrec < 0)
         return;
      }



   if (fRead == TRUE)
      {
      BYTE   szTitle[80];

      if (wd->usReadEA == READEA_NONE)
         return;

      switch (wd->usReadEA)
         {
         case READEA_SUBJECT :
            pszEAName = ".SUBJECT";
            break;
         case READEA_COMMENTS:
            pszEAName = ".COMMENTS";
            break;
         case READEA_HISTORY :
            pszEAName = ".HISTORY";
            break;
         case READEA_KEYPHRASES:
            pszEAName = ".KEYPHRASES";
            break;
         case READEA_USER:
            pszEAName = wd->szEAName;
            break;
         default  :
            return;
         }

      strcpy(wd->szCurEAName, pszEAName);
      sprintf(szTitle, "%s for %s", wd->szCurEAName , pcrec->finfo.szTitle);
      WinSetWindowText(wd->hwndStatic, szTitle);

      strcpy(wd->szCurEAFile, wd->szCurDir);
      if (wd->szCurEAFile[strlen(wd->szCurEAFile) - 1] != '\\')
         strcat(wd->szCurEAFile, "\\");
      strcat(wd->szCurEAFile, pcrec->finfo.szFileName);

      if (pcrec->finfo.chType == TYPE_ABSTRACT ||
          pcrec->finfo.chType == TYPE_DRIVE ||
         !strcmp(pcrec->finfo.szFileName, ".."))
         {
         WinSetWindowText(wd->hwndMLE, "No extended attributes can be set for this icon!");
         return;
         }

      WinEnableWindow(wd->hwndMLE, TRUE);
      WinSendMsg(wd->hwndMLE, MLM_SETCHANGED, MPFROMSHORT(FALSE), 0L);
      if (GetEAValue(wd->szCurEAFile, pszEAName, &pszEA, &usSize))
         {
         WinSetWindowText(wd->hwndMLE, pszEA);
         free(pszEA);
         }
      }
   else if (strlen(wd->szCurEAFile))
      {
      BYTE szMessage[80];

      WinEnableWindow(wd->hwndMLE, FALSE);
      WinSetWindowText(wd->hwndStatic, "");
      if (WinSendMsg(wd->hwndMLE, MLM_QUERYCHANGED, 0L, 0L))
         {
         ULONG ulSize = (ULONG)WinSendMsg(wd->hwndMLE, MLM_QUERYTEXTLENGTH, 0, 0);

         if (ulSize)
            {
            ulSize += 10;
            ulSize += (ULONG)WinSendMsg(wd->hwndMLE, MLM_QUERYLINECOUNT, 0, 0);
            pszEA = malloc(ulSize);
            if (!pszEA)
               {
               DebugBox("ICONTOOL", "Not enough memory !", FALSE);
               return;
               }
            memset(pszEA, 0, ulSize);
            WinQueryWindowText(wd->hwndMLE, ulSize, pszEA);
            }
         else
            pszEA = "";

         sprintf(szMessage, "%s for %s changed! Save ?",
            wd->szCurEAName,
            wd->szCurEAFile);

         if (fRead == 2 || WinMessageBox(HWND_DESKTOP,HWND_DESKTOP, szMessage,"",
            0, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON1 ) == MBID_YES)
            {
            PSZ p;
            while ((p = strchr(pszEA, '\r')) != NULL)
               memmove(p, p+1, strlen(p+1)+1);
            SetEAValue(wd->szCurEAFile,
                    wd->szCurEAName,
                    EA_LPASCII,
                    pszEA,
                    strlen(pszEA));
            }
         free(pszEA);
         }
      WinSetWindowText(wd->hwndMLE, "");
      memset(wd->szCurEAFile, 0, sizeof wd->szCurEAFile);
      }
}


/*************************************************************************
* Prepare the popup menu
*************************************************************************/
VOID PrepareEAPopup(PINSTANCE wd)
{
   WinCheckMenuItem(hwndPopupEA, ID_EASUBJECT   , FALSE);
   WinCheckMenuItem(hwndPopupEA, ID_EACOMMENTS  , FALSE);
   WinCheckMenuItem(hwndPopupEA, ID_EAKEYPHRASES, FALSE);
   WinCheckMenuItem(hwndPopupEA, ID_EAHISTORY   , FALSE);

   switch (wd->usReadEA)
      {
      case READEA_NONE:
         break;
      case READEA_SUBJECT   :
         WinCheckMenuItem(hwndPopupEA, ID_EASUBJECT, TRUE);
         break;
      case READEA_COMMENTS  :
         WinCheckMenuItem(hwndPopupEA,ID_EACOMMENTS, TRUE);
         break;
      case READEA_HISTORY   :
         WinCheckMenuItem(hwndPopupEA,ID_EAHISTORY, TRUE);
         break;
      case READEA_KEYPHRASES:
         WinCheckMenuItem(hwndPopupEA,ID_EAKEYPHRASES, TRUE);
         break;
      }

   if (!strlen(wd->szCurEAFile) ||
       !WinSendMsg(wd->hwndMLE, MLM_QUERYCHANGED, 0L, 0L))
      WinEnableMenuItem(hwndPopupEA, ID_SAVEEA, FALSE);
   else
      WinEnableMenuItem(hwndPopupEA, ID_SAVEEA, TRUE);
}

/*************************************************************************
* Prepare the popup menu
*************************************************************************/
VOID PrepareCnrPopup(PINSTANCE wd, PRECINFO pcrec)
{
MENUITEM mItemOpen, mItemCreate;
USHORT usItem, usIndex, usDefaultItem = 0;
PSZ    pszText,
       pszExt;
HWND   hwndMenuOpen;
HWND   hwndMenuCreate;
USHORT usPos = 1; /* After Setting */
BYTE szFullName[MAX_PATH];
HOBJECT hObject;
PSZ    pszSettings;

   WinSendMsg(hwndPopup, MM_QUERYITEM,
      MPFROM2SHORT(ID_SUBMENUOBJECT, TRUE), (MPARAM)&mItemOpen);
   hwndMenuOpen = mItemOpen.hwndSubMenu;

   if (!WinSendMsg(hwndPopup, MM_QUERYITEM,
      MPFROM2SHORT(ID_SUBMENUCREATE, TRUE), (MPARAM)&mItemCreate))
      DebugBox("ICONTOOL", "MM_QUERYITEM returned FALSE", TRUE);
   hwndMenuCreate = mItemCreate.hwndSubMenu;


   for (usItem = 0; usItem < MAX_ASSOC + 1; usItem++)
      {
      WinSendMsg(hwndMenuOpen,   MM_DELETEITEM,
         MPFROM2SHORT(usItem+ID_OPENOBJECT, TRUE), 0);
      WinSendMsg(hwndMenuCreate, MM_DELETEITEM,
         MPFROM2SHORT(usItem+ID_CREATEOBJECT, TRUE), 0);
      }

   memset(&mItemOpen, 0, sizeof mItemOpen);
   mItemOpen.iPosition   = usPos;
   mItemOpen.id          = ID_OPENOBJECT;
   mItemOpen.hwndSubMenu = NULL;
   mItemOpen.hItem       = (ULONG)NULL;
   mItemOpen.afStyle     = MIS_TEXT;

   memset(&mItemCreate, 0, sizeof mItemCreate);
   mItemCreate.iPosition   = usPos - 1;
   mItemCreate.id          = ID_CREATEOBJECT;
   mItemCreate.hwndSubMenu = NULL;
   mItemCreate.hItem       = (ULONG)NULL;
   mItemCreate.afStyle     = MIS_TEXT;

   switch(pcrec->finfo.chType)
      {
      case TYPE_DRIVE    :
         pszText = "~Drive";
         WinSendMsg(hwndMenuOpen, MM_INSERTITEM,
            (MPARAM)&mItemOpen, (MPARAM)pszText);
         WinEnableMenuItem(hwndPopup, ID_SETTING,      TRUE);
         WinEnableMenuItem(hwndPopup, ID_DELETE,       FALSE);
         WinEnableMenuItem(hwndPopup, ID_REMOVE,       FALSE);
         WinEnableMenuItem(hwndPopup, ID_EXTRACT,      FALSE);
         WinEnableMenuItem(hwndPopup, ID_QUERYSETTING, TRUE);
         WinEnableMenuItem(hwndPopup, ID_DUMPOBJDATA,  TRUE);
         WinEnableMenuItem(hwndPopup, ID_SUBMENUCREATE, FALSE);
         WinEnableMenuItem(hwndPopup, ID_SETOBJECTID, TRUE);
         usDefaultItem = ID_OPENOBJECT;
         break;
      case TYPE_DIRECTORY:
         pszText = "~Folder";
         WinSendMsg(hwndMenuOpen, MM_INSERTITEM,
            (MPARAM)&mItemOpen, (MPARAM)pszText);
         if (stricmp(pcrec->finfo.szFileName,".."))
            {
            WinEnableMenuItem(hwndPopup, ID_DELETE,  TRUE);
            WinEnableMenuItem(hwndPopup, ID_REMOVE,  TRUE);
            WinEnableMenuItem(hwndPopup, ID_EXTRACT, TRUE);
            WinEnableMenuItem(hwndPopup, ID_SETTING, TRUE);
            WinEnableMenuItem(hwndPopup, ID_QUERYSETTING,TRUE);
            WinEnableMenuItem(hwndPopup, ID_DUMPOBJDATA,  TRUE);
            WinEnableMenuItem(hwndPopup, ID_SETOBJECTID, TRUE);
            }
         else
            {
            WinEnableMenuItem(hwndPopup, ID_SETTING,FALSE);
            WinEnableMenuItem(hwndPopup, ID_QUERYSETTING,FALSE);
            WinEnableMenuItem(hwndPopup, ID_DUMPOBJDATA,  FALSE);
            WinEnableMenuItem(hwndPopup, ID_DELETE, FALSE);
            WinEnableMenuItem(hwndPopup, ID_REMOVE, FALSE);
            WinEnableMenuItem(hwndPopup, ID_EXTRACT, FALSE);
            WinEnableMenuItem(hwndPopup, ID_SETOBJECTID, FALSE);
            }
         WinEnableMenuItem(hwndPopup, ID_SUBMENUCREATE, FALSE);
         usDefaultItem = ID_OPENOBJECT;
         break;

      case TYPE_ABSTRACT :
         WinEnableMenuItem(hwndPopup, ID_DELETE,  TRUE);
         WinEnableMenuItem(hwndPopup, ID_REMOVE,  FALSE);
         WinEnableMenuItem(hwndPopup, ID_EXTRACT, TRUE);
         WinEnableMenuItem(hwndPopup, ID_SETTING, TRUE);
         WinEnableMenuItem(hwndPopup, ID_QUERYSETTING, TRUE);
         WinEnableMenuItem(hwndPopup, ID_DUMPOBJDATA,  TRUE);
         WinEnableMenuItem(hwndPopup, ID_SETOBJECTID, TRUE);
         pszText = pcrec->finfo.szTitle;
         WinSendMsg(hwndMenuOpen, MM_INSERTITEM,
            (MPARAM)&mItemOpen, (MPARAM)pszText);
         usDefaultItem = ID_OPENOBJECT;
         WinEnableMenuItem(hwndPopup, ID_SUBMENUCREATE, FALSE);
         break;
      case TYPE_FILE     :
         WinEnableMenuItem(hwndPopup, ID_DELETE, TRUE);
         WinEnableMenuItem(hwndPopup, ID_REMOVE, TRUE);
         WinEnableMenuItem(hwndPopup, ID_EXTRACT, TRUE);
         WinEnableMenuItem(hwndPopup, ID_SETTING, TRUE);
         WinEnableMenuItem(hwndPopup, ID_QUERYSETTING, TRUE);
         WinEnableMenuItem(hwndPopup, ID_DUMPOBJDATA,  TRUE);
         WinEnableMenuItem(hwndPopup, ID_SUBMENUCREATE, TRUE);
         WinEnableMenuItem(hwndPopup, ID_SETOBJECTID, TRUE);

         strcpy(szFullName, wd->szCurDir);
         if (szFullName[strlen(szFullName) - 1] != '\\')
            strcat(szFullName, "\\");
         strcat(szFullName, pcrec->finfo.szFileName);

         GetAssocFilter(szFullName, pcrec->finfo.hObjAssoc, MAX_ASSOC);
         pszExt = strrchr(pcrec->finfo.szFileName, '.');
         if (pszExt)
            {
            if (!stricmp(pszExt, ".EXE") || !stricmp(pszExt, ".COM") ||
               !stricmp(pszExt, ".BAT") || !stricmp(pszExt, ".CMD"))
               {
               pszText = "~Program";
               WinSendMsg(hwndMenuOpen, MM_INSERTITEM,
                  (MPARAM)&mItemOpen, (MPARAM)pszText);
               WinSendMsg(hwndMenuCreate, MM_INSERTITEM,
                  (MPARAM)&mItemCreate, (MPARAM)pszText);

               usDefaultItem = ID_OPENOBJECT;
               usPos++;
               }
            }
         usItem = 1;
         for (usIndex = 0; usIndex < MAX_ASSOC; usIndex++)
            {
            BYTE szAssocName[512];

            if (!pcrec->finfo.hObjAssoc[usIndex])
               break;

            if (!usDefaultItem)
               usDefaultItem = usItem + ID_OPENOBJECT;


            if (!GetObjectTitle(pcrec->finfo.hObjAssoc[usIndex], szAssocName, sizeof szAssocName))
               {
               if (!GetObjectName(pcrec->finfo.hObjAssoc[usIndex], szAssocName, sizeof szAssocName))
                  strcpy(szAssocName, "Unknown");
               }
            pszText = strrchr(szAssocName, '\\');
            if (!pszText)
               pszText = szAssocName;
            else
               pszText++;

            memset(&mItemOpen, 0, sizeof mItemOpen);
            mItemOpen.iPosition   = usPos;
            mItemOpen.id          = ID_OPENOBJECT + usItem;
            mItemOpen.hwndSubMenu = NULL;
            mItemOpen.hItem       = (ULONG)NULL;
            mItemOpen.afStyle     = MIS_TEXT;

            memset(&mItemCreate, 0, sizeof mItemCreate);
            mItemCreate.iPosition   = usPos-1;
            mItemCreate.id          = ID_CREATEOBJECT + usItem++ - 1;
            mItemCreate.hwndSubMenu = NULL;
            mItemCreate.hItem       = (ULONG)NULL;
            mItemCreate.afStyle     = MIS_TEXT;
            usPos++;


            WinSendMsg(hwndMenuOpen, MM_INSERTITEM,
               (MPARAM)&mItemOpen, (MPARAM)pszText);

            if ((SHORT)WinSendMsg(hwndMenuCreate, MM_INSERTITEM,
               (MPARAM)&mItemCreate, (MPARAM)pszText) == MIT_ERROR)
               DebugBox("ICONTOOL", "Fout bij aanmaken menu !", TRUE);
            }
         break;
      }
   if (usItem  > 0)
      ConditionalCascadeMenu(hwndPopup,    ID_SUBMENUOBJECT, usDefaultItem);

   if (stricmp(pcrec->finfo.szFileName,".."))
      {
      if (wd->pcrec->finfo.chType == TYPE_ABSTRACT)
         hObject = wd->pcrec->finfo.hObject;
      else 
         {
         memset(szFullName, 0, sizeof szFullName);
         if (wd->pcrec->finfo.chType != TYPE_DRIVE)
            {
            strcpy(szFullName, wd->szCurDir);
            if (szFullName[strlen(szFullName) - 1] != '\\')
               strcat(szFullName, "\\");
            }
         strcat(szFullName, wd->pcrec->finfo.szFileName);
         hObject = WinQueryObject(szFullName);
         wd->pcrec->finfo.hObject = hObject;
         }
      strcpy(szFullName, OBJECTID_NOT_SET);
      pszSettings = pszGetObjectSettings(hObject, NULL, 0, TRUE);
      if (pszSettings)
         {
         pszText = strstr(pszSettings, "OBJECTID=");
         if (pszText)
            {
            pszText += strlen("OBJECTID=");
            strncpy(szFullName, pszText, sizeof szFullName);
            pszText = strchr(szFullName, ';');
            if (pszText)
               *pszText = 0;
            }
         }
      }
   else
      strcpy(szFullName, OBJECTID_NOT_SET);

   WinSetMenuItemText(hwndPopup, ID_OBJECTID, szFullName);

}


/****************************************************************
* Fill the container
****************************************************************/
void FillContainer(HWND hwnd, PINSTANCE wd)
{
PVOID *pArg = malloc(sizeof (PVOID));
BYTE         szWinTitle[MAX_PATH];
PBYTE        p;


   /*
      Set the window title
   */
   WinQueryWindowText(WinQueryWindow(hwnd, QW_PARENT),
      sizeof szWinTitle, szWinTitle);
   p = strchr(szWinTitle, '-');
   if (p)
      *p = 0;

   sprintf(szWinTitle + strlen(szWinTitle), "- %s",
      wd->szCurDir);
   WinSetWindowText(WinQueryWindow(hwnd, QW_PARENT), szWinTitle);

   /*
      Start the thread
   */
   pArg[0] = (PVOID)wd;
   wd->iFillThread = _beginthread(FillContainerThread, NULL, 12000, pArg);
}

/****************************************************************
* Fill the container (Separate Thread)
****************************************************************/
void FillContainerThread(PVOID pArg)
{
PVOID        *pArray = (PVOID *)pArg;
PINSTANCE    wd =          pArray[0];
APIRET       rc;
ULONG        ulFindCount;
FILEFINDBUF3 * pfind;
HDIR         FindHandle;
FILEINFO     Finfo;
HMQ          hmqLocal;
HAB          habLocal;
BYTE         szFullName[MAX_PATH];
ULONG        ulCurDisk, ulDriveMap, ulAttr;
USHORT       usIndex;
PRECINFO     pCurSel = NULL,
             pcrec = NULL,
             pcFirst = NULL;
ULONG        ulRecCount;
PULONG       pulFldrContent;
ULONG        ulFldrCount;


   habLocal = WinInitialize((USHORT)0);
   hmqLocal = WinCreateMsgQueue(habLocal, 40);
   WinCancelShutdown(hmqLocal, TRUE);

   rc = DosSetPriority(PRTYS_THREAD,
      PRTYC_TIMECRITICAL,
      PRTYD_MAXIMUM,
      0L);
   if (rc)
      DebugBox("ICONTOOL", "Error on DosSetPriority!", FALSE);

   /*
      Set signal for possible active thread
   */
   wd->bScanActive = FALSE;

   /*
      Request ownership of the mutex semaphore
   */
//   if (DosRequestMutexSem(wd->hmtx, 120000L))
//      {
//      DebugBox("ICONTOOL","Cannot get semaphore !", FALSE);
//      WinDestroyMsgQueue( hmqLocal );
//      WinTerminate(habLocal);
//      return;
//      }


   wd->bScanActive = TRUE;
   msg_wait(wd);

   ClearContainer(wd);
   ulRecCount = 0;


   /*
      Getting all existing drives
   */

   DosQCurDisk(&ulCurDisk, &ulDriveMap);
   for (usIndex = 0; usIndex < 26 && wd->bScanActive; usIndex++)
      {
      ULONG Mask = 0x0001 << usIndex;

      if (!(ulDriveMap & Mask))
         continue;
      Finfo.szFileName[0] = (BYTE)(usIndex + 'A');
      Finfo.szFileName[1] = ':';
      Finfo.szFileName[2] = '\\';
      Finfo.szFileName[3] = 0;

      if (*Finfo.szFileName == *wd->szCurDir)
         continue;

      strcpy(Finfo.szTitle, "Drive A");
      Finfo.szTitle[strlen(Finfo.szTitle) - 1] = (BYTE)(usIndex + 'A');
      Finfo.chType   = TYPE_DRIVE;
      GetIcons(Finfo.szTitle, &Finfo, wd);
      pcrec = CreateRecord(wd, pcrec, &Finfo);
      ulRecCount++;
      if (!pCurSel)
         pCurSel = pcrec;
      if (!pcFirst)
         pcFirst = pcrec;
      }

   pfind = calloc(MAX_FIND, sizeof (FILEFINDBUF3));

   /*
      Construct full searchname and get all directories
   */

   strcpy(szFullName, wd->szCurDir);
   if (szFullName[strlen(szFullName) - 1] != '\\')
      strcat(szFullName, "\\");
   strcat(szFullName, "*.*");


   FindHandle = HDIR_CREATE;
   ulAttr = FILE_DIRECTORY | FILE_ARCHIVED | FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM | FILE_SYSTEM;

   /*
      Little bug, helps to do it 2 times
   */
   ulFindCount = 1;
   rc =  DosFindFirst(szFullName, &FindHandle, ulAttr, 
                    pfind, ulFindCount * sizeof (FILEFINDBUF3), &ulFindCount, 1);

   ulFindCount = MAX_FIND;
   rc =  DosFindFirst(szFullName, &FindHandle, ulAttr, 
                    pfind, ulFindCount * sizeof (FILEFINDBUF3), &ulFindCount, 1);
   while (wd->bScanActive && !rc)
      {
      FILEFINDBUF3 * pf = pfind;

      while (wd->bScanActive && !rc)
         {
         /*
            Little bug, files without DIR bit are also returned
         */
         if (strcmp(pf->achName, "."))
            {
            memset(&Finfo, 0, sizeof Finfo);
            strcpy(Finfo.szFileName, pf->achName);
            strcpy(szFullName, wd->szCurDir);
            if (szFullName[strlen(szFullName) - 1] != '\\')
               strcat(szFullName, "\\");
            strcat(szFullName, pf->achName);

            if (pf->attrFile & FILE_DIRECTORY) 
               Finfo.chType = TYPE_DIRECTORY;
            else
               Finfo.chType = TYPE_FILE;

            GetIcons(szFullName, &Finfo, wd);

            if (pf->achName[0] == '.')
               strcpy(Finfo.szTitle, "Parent Dir");
            else if (wd->bRealNames || !GetLongName(szFullName, Finfo.szTitle, sizeof Finfo.szTitle))
               strcpy(Finfo.szTitle, Finfo.szFileName);

            pcrec = CreateRecord(wd, pcrec, &Finfo);
            ulRecCount++;

            if (Finfo.chType == TYPE_DIRECTORY &&
               !stricmp(wd->szLastDir, pf->achName))
               pCurSel = pcrec;
            if (!pCurSel)
               pCurSel = pcrec;
            if (!pcFirst)
               pcFirst = pcrec;
            }
         if (!pf->oNextEntryOffset)
            break;
         pf = (FILEFINDBUF3 *)((PBYTE)pf + pf->oNextEntryOffset);
         }
      if (wd->bScanActive)
         {
         ulFindCount = MAX_FIND;
         rc = DosFindNext(FindHandle, pfind, MAX_FIND * sizeof (FILEFINDBUF3), &ulFindCount);
         }
      else
         rc = 1;
      }

   DosFindClose(FindHandle);

   if (wd->bScanActive && rc != ERROR_NO_MORE_FILES)
      {
      BYTE szMessage[80];
      sprintf(szMessage, "ICONTOOL, error %d", rc);
      DebugBox(szMessage, _strerror("DosFindFirst"), FALSE);
      }

   DosFindClose(FindHandle);

   free(pfind);

   if (wd->bScanActive)
      {
      pulFldrContent = NULL;
      if (GetFolderContent(wd->szCurDir, (PBYTE *)&pulFldrContent, &ulFldrCount))
         {
         ulFldrCount /= sizeof (ULONG);
         for (usIndex = 0; wd->bScanActive && usIndex < ulFldrCount; usIndex++)
            {
            if (GetAbstractObject(MakeAbstractHandle(pulFldrContent[usIndex]), &Finfo))
               {
               pcrec = CreateRecord(wd, pcrec, &Finfo);
               ulRecCount++;
               if (!pCurSel)
                  pCurSel = pcrec;
               if (!pcFirst)
                  pcFirst = pcrec;
               }
            else
               {
               BYTE szMessage[80];
               sprintf(szMessage, "Cannot find abstract object %X",
                  pulFldrContent[usIndex]);
               DebugBox("ICONTOOL", szMessage, FALSE);
               }
            }
         }
      free((PBYTE)pulFldrContent);
      }


   if (wd->bScanActive)
      {
      RECORDINSERT rci;
      ULONG        ulRetco;

      rci.cb = sizeof(RECORDINSERT);
      rci.pRecordOrder = (PRECORDCORE)CMA_END;
      rci.pRecordParent = NULL;
      rci.zOrder = CMA_TOP;
      rci.cRecordsInsert = ulRecCount;
      rci.fInvalidateRecord = FALSE;
      ulRetco = (ULONG)WinSendMsg(wd->hwndContainer, CM_INSERTRECORD, (MPARAM)pcFirst, (MPARAM)&rci);
      if (ulRetco != ulRecCount)
         {
         DebugBox("ICONTOOL", "Error while inserting records in container", TRUE);
         exit(1);
         }
      }

   /*     
      Sort all the records, this will also invalidate all records
      so they will be repainted
   */
   if (wd->bScanActive)
      WinSendMsg(wd->hwndContainer, CM_FILTER,
         (MPARAM)FilterContainerFunc, (MPARAM) wd);

   if (wd->bScanActive)
      WinSendMsg(wd->hwndContainer,
         CM_SORTRECORD, (MPARAM)SortContainerFunc, wd);

   if (wd->bScanActive)
      if (pCurSel)
         WinSendMsg(wd->hwndContainer, CM_SETRECORDEMPHASIS,
            (MPARAM)pCurSel, MPFROM2SHORT(TRUE, CRA_CURSORED));

   if (wd->bScanActive)
      WinSetFocus(HWND_DESKTOP, wd->hwndContainer);

   msg_wait_remove(wd);

   ReadEAValue(WinQueryWindow(wd->hwndContainer, QW_PARENT), NULL, TRUE);

   WinDestroyMsgQueue( hmqLocal );
   WinTerminate(habLocal);
   free(pArg);
   wd->bScanActive = FALSE;
//   DosReleaseMutexSem(wd->hmtx);
   wd->iFillThread = 0;
}

/*********************************************************************
* Create a record
*********************************************************************/
PRECINFO CreateRecord(PINSTANCE wd, PRECINFO pAfter, PFILEINFO pFinfo)
{
PRECINFO     pcrec;

   pcrec = (PRECINFO)WinSendMsg(wd->hwndContainer, CM_ALLOCRECORD, (MPARAM)sizeof (FILEINFO), (MPARAM)1);
   if (!pcrec)
      {
      DebugBox("ICONTOOL", "The container has no memory available", TRUE);
      exit(1);
      }

   if (pAfter)
      pAfter->Record.preccNextRecord = (PMINIRECORDCORE)pcrec;

   memcpy(&pcrec->finfo, pFinfo, sizeof (FILEINFO));
   pcrec->Record.ptlIcon.x = 10;
   pcrec->Record.ptlIcon.y = 10;
   pcrec->Record.preccNextRecord = NULL;
   pcrec->Record.pszIcon = pcrec->finfo.szTitle;
   pcrec->Record.hptrIcon = pcrec->finfo.hptrIcon;

   if (wd->bRealNames ||
       pFinfo->chType == TYPE_DRIVE ||
       pFinfo->chType == TYPE_ABSTRACT ||
       !strcmp(pcrec->finfo.szFileName, ".."))
      pcrec->Record.flRecordAttr = CRA_RECORDREADONLY;
   else
      pcrec->Record.flRecordAttr = 0L;
          
   pcrec->Record.flRecordAttr |= CRA_DROPONABLE;

   return pcrec;
}

/*********************************************************************
* Add a record
*********************************************************************/
PRECINFO AddRecord(PINSTANCE wd, PRECINFO pAfter, PFILEINFO pFinfo, BOOL bInvalidate)
{
PRECINFO     pcrec;
RECORDINSERT rci;

   pcrec = CreateRecord(wd, NULL, pFinfo);

   rci.cb = sizeof(RECORDINSERT);
   rci.pRecordOrder = (PRECORDCORE)pAfter;
   rci.pRecordParent = NULL;
   rci.zOrder = CMA_TOP;
   rci.cRecordsInsert = 1;
   rci.fInvalidateRecord = bInvalidate;

   WinSendMsg(wd->hwndContainer, CM_INSERTRECORD, (MPARAM)pcrec, (MPARAM)&rci);

   return pcrec;
}

/**************************************************************************
*  Sort function for CM_SORTRECORD function
**************************************************************************/
SHORT SortContainerFunc(PRECINFO p1, PRECINFO p2, PINSTANCE wd)
{
SHORT sRetco = 0;
PBYTE e1, e2;


   if (p1->finfo.chType != p2->finfo.chType)
      sRetco = (SHORT)(p1->finfo.chType - p2->finfo.chType);
   else
      {
      if (p1->finfo.chType == TYPE_DRIVE)
         {
         sRetco = stricmp(p1->finfo.szFileName, p2->finfo.szFileName);
         return sRetco;
         }
      if (p1->finfo.chType == TYPE_DIRECTORY)
         {
         if (!stricmp(p1->finfo.szFileName, ".."))
            return -1;
         if (!stricmp(p2->finfo.szFileName, ".."))
            return 1;
         }
      switch (wd->usSort)
         {
         case SORT_EXT:
            e1 = strrchr(p1->finfo.szFileName, '.');
            e2 = strrchr(p2->finfo.szFileName, '.');
            if (e1 || e2)
               {
               if (!e1)
                  return -1;
               if (!e2)
                  return 1;
               sRetco = stricmp(e1+1, e2+1);
               }
            if (!sRetco)
               sRetco = stricmp(p1->finfo.szFileName, p2->finfo.szFileName);
            break;
         case SORT_NAME    :
            sRetco = stricmp(p1->finfo.szTitle, p2->finfo.szTitle);
            break;

         case SORT_REALNAME:
         default           :
            sRetco = stricmp(p1->finfo.szFileName, p2->finfo.szFileName);
            break;
         }

      }      
   return sRetco;
}


/******************************************************************
* Filter the container records
******************************************************************/
BOOL FilterContainerFunc(PRECINFO p1, PINSTANCE wd)
{
BYTE szResult[MAX_PATH];
APIRET rc;

   switch(p1->finfo.chType)
      {
      case TYPE_DRIVE    :
         if (!wd->bDrives)
            return FALSE;
         break;
      case TYPE_DIRECTORY:
         if (!wd->bDirectories)
            return FALSE;
         break;
      case TYPE_ABSTRACT :
         break;
      case TYPE_FILE     :
      default            :
         if (!strlen(wd->szFilter))
            return TRUE;
         rc = DosEditName(1, p1->finfo.szFileName, wd->szFilter,
            szResult, sizeof szResult);
         if (!rc && stricmp(p1->finfo.szFileName, szResult))
            return FALSE;
         break;
      }
   return TRUE;
}
/******************************************************************
* Message wait
******************************************************************/
VOID msg_wait(PINSTANCE wd)
{
HPOINTER hptr;

   hptr = WinQuerySysPointer(HWND_DESKTOP, SPTR_WAIT, FALSE);
   if (!hptr)
      return;
   WinSetPointer(HWND_DESKTOP, hptr);
   wd->bMsgWait = TRUE;
}

/******************************************************************
* Message wait remove
******************************************************************/
VOID msg_wait_remove(PINSTANCE wd)
{
   /*
HPOINTER hptr;

   hptr = WinQuerySysPointer(HWND_DESKTOP, SPTR_ARROW, FALSE);
   if (!hptr)
      return;

   WinSetPointer(HWND_DESKTOP, hptr);
   */
   wd->bMsgWait = FALSE;
}


/***********************************************************
* Clear the container
***********************************************************/
BOOL ClearContainer(PINSTANCE wd)
{
PRECINFO pcrec;
BOOL     bContinue = TRUE;

   ReadEAValue(WinQueryWindow(wd->hwndContainer, QW_PARENT), NULL, FALSE);

   WinEnableWindowUpdate(wd->hwndContainer, FALSE);
   while (bContinue)
      {
      pcrec = (PRECINFO)WinSendMsg(wd->hwndContainer,
         CM_QUERYRECORD,
         0L,
         MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));
      if (pcrec)
         {
         if (!RemoveRecord(wd->hwndContainer, pcrec))
            break;
         }
      else
         bContinue = FALSE;
      }

   WinEnableWindowUpdate(wd->hwndContainer, TRUE);
   return !bContinue;
}

/******************************************************************
* Remove a single record
******************************************************************/
BOOL RemoveRecord(HWND hwndContainer, PRECINFO pcrec)
{
PMINIRECORDCORE aprc[10];

aprc[0] = &(pcrec->Record);

   DeleteIcons(&pcrec->finfo);

   if (!WinSendMsg(hwndContainer, CM_ERASERECORD,
      (MPARAM)&pcrec->Record, 0L))
      return FALSE;


   WinSendMsg(hwndContainer, CM_REMOVERECORD,
      (MPARAM)aprc, MPFROM2SHORT(1, CMA_FREE));

   return TRUE;
}

/*********************************************************************
*  Debug Box
*********************************************************************/
BOOL DebugBox(PSZ pszTitle, PSZ pszMes, BOOL bShowInfo)
{
BYTE szMessage[MAX_PATH];
PERRINFO pErrInfo = WinGetErrorInfo(hab);
USHORT usSeverity;
CHAR   szSev[14],
       szErrNo[6],
       szHErrNo[6];
HWND   hwndParent;

   strcpy(szMessage, pszMes);

   if (bShowInfo && pErrInfo)
      {
      PUSHORT pus = (PUSHORT)((PBYTE)pErrInfo + pErrInfo->offaoffszMsg);
      PSZ     psz = (PBYTE)pErrInfo + *pus;
      PSZ     p;

      strcat(szMessage, "\n");
      strcat(szMessage, psz);
      p = psz + strlen(psz) + 1;
      strcat(szMessage, "\n");
      strcat(szMessage, p);

      usSeverity = ERRORIDSEV(pErrInfo->idError);          
      itoa(ERRORIDERROR(pErrInfo->idError), szErrNo, 10);
      itoa(ERRORIDERROR(pErrInfo->idError), szHErrNo, 16);
      switch (usSeverity)
         {
         case 4:
            strcpy(szSev, "Warning");
            break;
  
         case 8:
            strcpy(szSev, "Error");
            break;
  
         case 12:
            strcpy(szSev, "Severe");
            break;
  
         case 16:
            strcpy(szSev, "Unrecoverable");
            break;
  
         default:
            strcpy(szSev, "None");
            break;
         }
      strcat(szMessage, "\n");
      strcat(szMessage, "Error No = ");
      strcat(szMessage, szErrNo);
      strcat(szMessage, "  (0x");
      strcat(szMessage, szHErrNo);
      strcat(szMessage, ")");
      strcat(szMessage, "\n");
      strcat(szMessage, "Severity = ");
      strcat(szMessage, szSev);

      }
   if (pErrInfo)
      WinFreeErrorInfo(pErrInfo);


   hwndParent = HWND_DESKTOP;
   if (hwndFirst)
      hwndParent = hwndFirst;
   else if (hwndSecond)
      hwndParent = hwndSecond;

   WinMessageBox(HWND_DESKTOP,hwndParent, 
       (PSZ) szMessage , (PSZ) pszTitle, 0, 
       MB_OK | MB_INFORMATION | MB_SYSTEMMODAL);



   return TRUE;
}


/*************************************************************************
*  Get Long filename
*************************************************************************/
BOOL GetLongName(PSZ pszFile, PSZ pszTarget, USHORT usMax)
{
PSZ pszLongName;
USHORT usSize;
PBYTE  p;

   if (!GetEAValue(pszFile, ".LONGNAME", &pszLongName, &usSize))
      return FALSE;

   memset(pszTarget, 0, usMax);
   if (usSize < usMax)
      memcpy(pszTarget, pszLongName, usSize);
   else
      memcpy(pszTarget, pszLongName, usMax - 1);

   while ((p = strchr(pszLongName, '^')) != NULL)
      *p = '\n';

   free(pszLongName);
   return TRUE;
}

/*************************************************************
* Get Icons
*************************************************************/
VOID GetIcons(PSZ pszFileName, PFILEINFO pInfo, PINSTANCE wd)
{
PBYTE pchIcon=NULL ;
USHORT usIconSize;
BOOL fPrivate = FALSE;

   pInfo->hptrMiniIcon = 0L;

   pInfo->hptrIcon = 0L;
   pInfo->bIconCreated     = FALSE;
   pInfo->bMiniIconCreated = FALSE;

   switch(pInfo->chType)
      {
      case TYPE_DRIVE:
         pInfo->hptrIcon = hptrDrive;
         break;
      case TYPE_DIRECTORY:
         if (wd->bAnimIcons)
            {
//            pInfo->hptrIcon     = WinLoadFileIconN(pszFileName, fPrivate, 1);
            if (GetEAValue(pszFileName, ".ICON1", &pchIcon, &usIconSize))
               {
               pInfo->hptrIcon = Buffer2Icon(HWND_DESKTOP, pchIcon, usIconCX);
               free(pchIcon);
               pInfo->bIconCreated = 2;
               break;
               }
            }
         else
            pInfo->hptrIcon     = WinLoadFileIcon(pszFileName, fPrivate);
         break;
      case TYPE_FILE:
         pInfo->bIconCreated = TRUE;
         pInfo->hptrIcon = WinLoadFileIcon(pszFileName, fPrivate);
         break;
      case TYPE_ABSTRACT:
         break;
      }

   if (pchIcon)
      free(pchIcon);
   if (!pInfo->hptrIcon)
      {
      pInfo->hptrIcon = hptrUnknown;
      pInfo->bIconCreated = FALSE;
      }
   if (!pInfo->hptrMiniIcon)
      pInfo->hptrMiniIcon = pInfo->hptrIcon;
}

/*************************************************************
* Delete Icons
*************************************************************/
VOID DeleteIcons(PFILEINFO pInfo)
{
   if (pInfo->hptrIcon)
      {
      if (pInfo->bIconCreated == TRUE)
         {
         if (!WinFreeFileIcon(pInfo->hptrIcon))
            WinDestroyPointer(pInfo->hptrIcon);
         }
      else if (pInfo->bIconCreated == 2)
         {
         WinDestroyPointer(pInfo->hptrIcon);
         }
      }
     
   if (pInfo->bMiniIconCreated && pInfo->hptrMiniIcon)
      WinDestroyPointer(pInfo->hptrMiniIcon);
}

/********************************************************************
* Start program
********************************************************************/
BOOL StartProgram(HWND hwnd, PRECINFO pcrec, USHORT usAssocIndex)
{
static PROGDETAILS ProgDet;
static BYTE      szParameters[MAX_PATH];
static BYTE      szProgramName[MAX_PATH];
static BYTE      szCurDir[MAX_PATH];
static BYTE      szDosSettings[2048];

PINSTANCE wd = WinQueryWindowPtr(hwnd, 0);
ULONG     ulAppType;
APIRET    rc;
PSZ       pszExt;
PBYTE     pClassInfo;
USHORT    usSize;
#ifdef DEBUG
PBYTE     p;
#endif


   memset(szDosSettings, 0, sizeof szDosSettings);
   memset(szParameters, 0, sizeof szParameters);
   memset(szCurDir, 0, sizeof szCurDir);
   memset(&ProgDet, 0, sizeof ProgDet);
   ProgDet.Length = sizeof ProgDet;
   ProgDet.progt.fbVisible = SHE_VISIBLE;
   ProgDet.swpInitial.fl   = SWP_ACTIVATE;
   ProgDet.swpInitial.hwndInsertBehind = HWND_TOP;
//   ProgDet.swpInitial.hwnd             = hwnd;


   if (pcrec->finfo.chType == TYPE_ABSTRACT)
      {
      PBYTE pObjectData;
      BYTE  szObjID[10];
      ULONG ulProfileSize;

      sprintf(szObjID, "%lX", LOUSHORT(pcrec->finfo.hObject));
      pObjectData = GetProfileData("PM_Abstract:Objects", szObjID,
         HINI_USERPROFILE, &ulProfileSize);
      if (!pObjectData)
         {
         DebugBox("ICONTOOL", "Unable to get profiledata!", FALSE);
         return FALSE;
         }
      if (!strcmp(pObjectData + 4, "WPProgram"))
         {
         free(pObjectData);
         if (!GetObjectName(pcrec->finfo.hObject, szProgramName, sizeof szProgramName))
            return FALSE;
         ProgDet.progt.progc = GetObjectProgType(pcrec->finfo.hObject);
         GetObjectCurDir(pcrec->finfo.hObject, szCurDir, sizeof szCurDir);
         if (!szCurDir[0] && *szProgramName != '*')
            {
            strcpy(szCurDir, szProgramName);
            p = strrchr(szCurDir, '\\');
            if (p)
               *p = 0;
            }
         GetObjectParameters(pcrec->finfo.hObject, szParameters, sizeof szParameters);
         GetDosSettings(pcrec->finfo.hObject, szDosSettings, sizeof szDosSettings);
         }
//      else if (!strcmp(pObjectData + 4, "WPSchemePalette"))
//         {
//         free(pObjectData);
//         DebugBox("ICONTOOL","Cannot open WPSchemePalette objects!", FALSE);
//         return FALSE;
//         }
      else
         {
         free(pObjectData);
//         WinOpenObject(pcrec->finfo.hObject, OPEN_DEFAULT, FALSE);
         MySetObjectData(pcrec->finfo.hObject, "OPEN=DEFAULT");
         return TRUE;
         }
      }
   else if (pcrec->finfo.chType == TYPE_FILE)
      {
      HOBJECT hObject = 0L;
      
      /*
         Get full name of selected object
      */
      strcpy(szProgramName, wd->szCurDir);
      if (szProgramName[strlen(szProgramName) - 1] != '\\')
         strcat(szProgramName, "\\");
      strcat(szProgramName, pcrec->finfo.szFileName);

      /*
         Is a associated prog selected?
      */
      if (usAssocIndex > 0)
         {
         usAssocIndex--;
         szParameters[0] = ' ';
//         strcpy(szParameters+1, szProgramName);
         strcpy(szParameters, szProgramName);
         hObject = pcrec->finfo.hObjAssoc[usAssocIndex];
         if (!hObject)
            {
            DebugBox("ICONTOOL", "Don't know how to start this file !", FALSE);
            return FALSE;
            }
         if (!GetObjectName(hObject, szProgramName, sizeof szProgramName))
            {
            DebugBox("ICONTOOL", "Unable to find associated program name!", FALSE);
            return FALSE;
            }
         strcpy(szCurDir, szProgramName);
         p = strrchr(szCurDir, '\\');
         if (p)
            *p = 0;
         }

      /*
         If Object is abstract
      */
      if (hObject && IsObjectAbstract(hObject))
         {
         ProgDet.progt.progc = GetObjectProgType(hObject);
         GetDosSettings(hObject, szDosSettings, sizeof szDosSettings);
         }
      else
         {
         if (GetEAValue(szProgramName, ".CLASSINFO", &pClassInfo, &usSize))
            {
            ULONG ulProgType = 0;
            WPPGMFILEDATA wpPgm;

            *(PULONG)pClassInfo = (ULONG)usSize;
            GetObjectData(pClassInfo+4, "WPProgramFile", WPPROGFILE_DOSSET, szDosSettings, sizeof szDosSettings);
            if (!GetObjectData(pClassInfo+4, "WPProgramFile", WPPROGFILE_DATA, &wpPgm, sizeof wpPgm))
               {
               if (GetObjectData(pClassInfo+4, "WPProgramFile", WPPROGFILE_PROGTYPE, &ulProgType, sizeof ulProgType))
                  ProgDet.progt.progc = ulProgType;
               }
            else
               ProgDet.progt.progc = wpPgm.ulProgType;
            free(pClassInfo);
            }
         }
      }

   if (!ProgDet.progt.progc)
      {
      pszExt = strrchr(szProgramName, '.');
      if (!pszExt)                   
         pszExt = szProgramName;

      if (!stricmp(pszExt, ".EXE") || !stricmp(pszExt, ".COM"))
         {
         rc = DosQueryAppType(szProgramName, &ulAppType);
         if (rc)
            {
            DebugBox("ICONTOOL", "Error on DosQueryAppType", FALSE);
            return FALSE;
            }
         ProgDet.progt.progc = 0xFFFF;
         switch (ulAppType & 7)  /* only last tree bits */
            {
            case FAPPTYP_NOTWINDOWCOMPAT :
               ProgDet.progt.progc = PROG_FULLSCREEN;
               break;
            case FAPPTYP_WINDOWCOMPAT    :
               ProgDet.progt.progc = PROG_WINDOWABLEVIO;
               break;
            case FAPPTYP_WINDOWAPI       :
               ProgDet.progt.progc = PROG_PM;
               break;
            case FAPPTYP_NOTSPEC          :
               if (ulAppType & FAPPTYP_WINDOWSREAL)
                  ProgDet.progt.progc = PROG_31_ENHSEAMLESSCOMMON; /* PROG_WINDOW_REAL;*/
               else if (ulAppType & FAPPTYP_WINDOWSPROT)
                  ProgDet.progt.progc = PROG_31_ENHSEAMLESSCOMMON; /* PROG_WINDOW_PROT; */
               else if (ulAppType & FAPPTYP_WINDOWSPROT31)
                  ProgDet.progt.progc = PROG_31_ENHSEAMLESSCOMMON; /* PROG_WINDOW_PROT; */
               else if (ulAppType & FAPPTYP_DOS)
                  ProgDet.progt.progc = PROG_VDM;
               else if (ulAppType == FAPPTYP_NOTSPEC)
                  ProgDet.progt.progc = PROG_FULLSCREEN;
               break;
            }
         if (ProgDet.progt.progc == 0xFFFF)
            {
            DebugBox("ICONTOOL", "Unknown application type !", FALSE);
            return FALSE;                 
            }
         }
      else if (!stricmp(pszExt, ".CMD"))
         ProgDet.progt.progc = PROG_WINDOWABLEVIO;
      else if (!stricmp(pszExt, ".BAT"))
         ProgDet.progt.progc = PROG_WINDOWEDVDM;
      else
         {
         DebugBox("ICONTOOL", "Don't know how to start this file !", FALSE);
         return FALSE;
         }
      }
     
   if (!szCurDir[0])
      strcpy(szCurDir, wd->szCurDir);

   if (ProgDet.progt.progc == PROG_SEAMLESSCOMMON ||
       ProgDet.progt.progc == PROG_31_ENHSEAMLESSCOMMON ||
       ProgDet.progt.progc == PROG_31_STDSEAMLESSCOMMON)
      {
      PSZ pszWinSet;
      ULONG ulWinSize;

      pszWinSet = GetProfileData("WINOS2",
         "PM_GlobalWindows31Settings",
         HINI_PROFILE,
          &ulWinSize);
      if (pszWinSet)
         {
         memmove(szDosSettings + ulWinSize - 1,  szDosSettings, strlen(szDosSettings) + 1);
         memcpy(szDosSettings, pszWinSet, ulWinSize - 1);
         free(pszWinSet);
         }

      if (szProgramName[0] == '*')
         strcpy(szProgramName, "WINOS2.COM");
      else
      if (!WindowsSessionActive())
         {
         memmove(szParameters + strlen(szProgramName) + 4, szParameters,
            strlen(szParameters) + 1);
         memset(szParameters, 0x20, strlen(szProgramName) + 4);
         memcpy(szParameters, " /K ", 4);
         memcpy(szParameters + 4, szProgramName, strlen(szProgramName));
         strcpy(szProgramName, "WINOS2.COM");
         }
      }
   else if (ProgDet.progt.progc == PROG_WINDOW_REAL       ||
            ProgDet.progt.progc == PROG_WINDOW_PROT       ||
            ProgDet.progt.progc == PROG_30_STD            ||
            ProgDet.progt.progc == PROG_WINDOW_AUTO       ||
            ProgDet.progt.progc == PROG_SEAMLESSVDM       ||
            ProgDet.progt.progc == PROG_30_STDSEAMLESSVDM ||
            ProgDet.progt.progc == PROG_31_STDSEAMLESSVDM ||
            ProgDet.progt.progc == PROG_31_ENHSEAMLESSVDM ||
            ProgDet.progt.progc == PROG_31_ENH            ||
            ProgDet.progt.progc == PROG_31_STD            )
      {
      PSZ pszWinSet;
      ULONG ulWinSize;

      pszWinSet = GetProfileData("WINOS2",
         "PM_GlobalWindows31Settings",
         HINI_PROFILE,
          &ulWinSize);

      if (pszWinSet)
         {
         memmove(szDosSettings + ulWinSize - 1,  szDosSettings, strlen(szDosSettings) + 1);
         memcpy(szDosSettings, pszWinSet, ulWinSize - 1);
         free(pszWinSet);
         }

      if (szProgramName[0] == '*')
         {
         ProgDet.progt.progc = PROG_VDM;
         strcpy(szProgramName, "WINOS2.COM");
         }
      }


   ProgDet.pszTitle = pcrec->finfo.szTitle;
   if (szProgramName[0] != '*')
      ProgDet.pszExecutable = szProgramName;
   else
      ProgDet.pszExecutable = NULL;
   if (*szCurDir)
      ProgDet.pszStartupDir = szCurDir;
   if (*szParameters)
      ProgDet.pszParameters = szParameters;
   if (*szDosSettings)
      ProgDet.pszEnvironment = szDosSettings;

   if (strchr(szParameters, ' '))
      {
      memmove(szParameters + 1, szParameters, strlen(szParameters)+1);
      szParameters[0] = '"';
      strcat(szParameters, "\"");
      }

   printf("Starting \'%s\' Catagory %d\nCurDir: \'%s\'\nParameters \'%s\'\nEnvironment:",
      szProgramName, ProgDet.progt.progc,
      ProgDet.pszStartupDir,
      ProgDet.pszParameters);
   p = szDosSettings;
   while (*p)
      {
      printf("\'%s\'\n", p);
      p +=strlen(p) + 1;
      }

   pcrec->finfo.happ = WinStartApp(hwnd, &ProgDet,
//      szParameters, NULL, 0);
      NULL, NULL, SAF_INSTALLEDCMDLINE);

   if (!pcrec->finfo.happ)
      DebugBox("ICONTOOL", szProgramName, TRUE);
   else
      WinSendMsg(wd->hwndContainer, CM_SETRECORDEMPHASIS,
         (MPARAM)pcrec, MPFROM2SHORT(TRUE, CRA_INUSE));

   return TRUE;
}

/***********************************************************
* Clear the container
***********************************************************/
BOOL RemoveInUse(HWND hwnd, HAPP happ)
{
PRECINFO pcrec;
PINSTANCE wd = WinQueryWindowPtr(hwnd, 0);

   pcrec = (PRECINFO)WinSendMsg(wd->hwndContainer,
      CM_QUERYRECORD,
      0L,
      MPFROM2SHORT(CMA_FIRST, CMA_ITEMORDER));

   while ((ULONG)pcrec > 0L)
      {
      if (happ == pcrec->finfo.happ)
         {
         WinSendMsg(wd->hwndContainer, CM_SETRECORDEMPHASIS,
            (MPARAM)pcrec, MPFROM2SHORT(FALSE, CRA_INUSE));
         pcrec->finfo.happ = NULL;
         return TRUE;
         }
      pcrec = (PRECINFO)WinSendMsg(wd->hwndContainer,
         CM_QUERYRECORD,
         (PMINIRECORDCORE)pcrec,
         MPFROM2SHORT(CMA_NEXT, CMA_ITEMORDER));
      }

//   DebugBox("ICONTOOL", "APPTERMINATE received for unknown app", FALSE);



   return FALSE;
}

/******************************************************************
* Extract icon
*******************************************************************/
VOID ExtractIcon(HWND hwnd)
{
PINSTANCE wd = WinQueryWindowPtr(hwnd, 0);
PRECINFO  pcrec,
          pTemp;
BYTE      szFullName[MAX_PATH];
INT       iHandle;
PBYTE     p;
FILEINFO  Finfo;
BOOL      bEAExist;
PBYTE     pchIcon = NULL;
USHORT    usIconSize;
BOOL      bMultiple,
          bIconExist;
BYTE      szMessage[80];

   pcrec = wd->pcrec;
   if (!pcrec)
      return;

   if (pcrec->Record.flRecordAttr & CRA_SELECTED)
      {
      bMultiple = TRUE;
      pTemp = (PRECINFO)CMA_FIRST;
      }
   else
      {
      bMultiple = FALSE;
      pTemp = pcrec;
      }

   do {
      if (bMultiple)
         {
         pTemp = WinSendMsg(wd->hwndContainer,
            CM_QUERYRECORDEMPHASIS, (MPARAM)pTemp, (MPARAM)CRA_SELECTED);
         if ((LONG)pTemp <= 0)
            break;
         }

      sprintf(szMessage, "Extract icon from %s?", pTemp->finfo.szTitle);
      if (WinMessageBox(HWND_DESKTOP,HWND_DESKTOP, szMessage,"",
         0, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON1 ) != MBID_YES)
         continue;

      /*
         Construct filename for icon
      */

      if (pTemp->finfo.chType != TYPE_ABSTRACT)
         {
         PSZ pszEA;

         if (wd->szExtractPath[0])
            strcpy(szFullName, wd->szExtractPath);
         else
            strcpy(szFullName, wd->szCurDir);
         if (szFullName[strlen(szFullName) - 1] != '\\')
            strcat(szFullName, "\\");
         strcat(szFullName, pTemp->finfo.szFileName);

         if (pTemp->finfo.chType == TYPE_DIRECTORY && wd->bAnimIcons)
            pszEA = ".ICON1";
         else
            pszEA = ".ICON";

         if (!GetEAValue(szFullName, pszEA, &pchIcon, &usIconSize))
            bEAExist = FALSE;
         else
            bEAExist = TRUE;
         p = strrchr(szFullName, '.');
         if (!p)
            p = szFullName + strlen(szFullName);
         strcpy(p, ".ICO");
         }
      else
         {
         BYTE szName[9];
         bEAExist = FALSE;
         if (wd->szExtractPath[0])
            strcpy(szFullName, wd->szExtractPath);
         else
            strcpy(szFullName, wd->szCurDir);
         if (szFullName[strlen(szFullName) - 1] != '\\')
            strcat(szFullName, "\\");
         memset(szName, 0, sizeof szName);
         strncpy(szName, pTemp->finfo.szTitle, sizeof szName - 1);
         p = strpbrk(szName, "\n.^,");
         if (p)
            *p = 0;

         while ((p = strpbrk(szName, "\\/")) != NULL)
            *p = '!';

         while ((p = strpbrk(szName, " ")) != NULL)
            *p = '_';

         if (!strlen(szName))
            strcpy(szName, "ABSTRACT");
         strcat(szFullName, szName);
         strcat(szFullName, ".ICO");
         }

      /*
         Test if iconfile already exist
      */
      if (!access(szFullName, 0))
         {
         bIconExist = TRUE;
         if (pchIcon)
            {
            free(pchIcon);
            pchIcon = NULL;
            }
         sprintf(szMessage, "%s already exist!\nOverwrite it?", strrchr(szFullName, '\\') + 1);
         if (WinMessageBox(HWND_DESKTOP,HWND_DESKTOP, szMessage,"",
            0, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON1 ) != MBID_YES)
            continue;
         }
      else
         bIconExist = FALSE;


      if (!bEAExist)
         {
         if (!GetIconFromPointer(hab, pTemp->finfo.hptrIcon, szFullName))
            continue;
         }
      else
         {
         iHandle = open(szFullName, O_RDWR | O_BINARY | O_TRUNC | O_CREAT, S_IWRITE | S_IREAD);
         if (iHandle == -1)
            {
            DebugBox("ICONTOOL", "Unable to create icon file !", FALSE);
            if (pchIcon)
               {
               free(pchIcon);
               pchIcon = NULL;
               }
            continue;
            }

         write(iHandle, pchIcon, usIconSize);
         close(iHandle);
         if (pchIcon)
            {
            free(pchIcon);
            pchIcon = NULL;
            }
         }

      /*
         Add record into container
      */

      if (!wd->szExtractPath[0] && !bIconExist)
         {
         memset(&Finfo, 0, sizeof Finfo);
         p = strrchr(szFullName, '\\');
         strcpy(Finfo.szFileName, p + 1);
         Finfo.chType = TYPE_FILE;
         GetIcons(szFullName, &Finfo, wd);
         strcpy(Finfo.szTitle, Finfo.szFileName);
         AddRecord(wd, (PRECINFO)pTemp, &Finfo, TRUE);
         }
      } while (bMultiple);
}

/****************************************************************
* Save the options
****************************************************************/
VOID SaveOptions(HWND hwnd)
{
PINSTANCE wd = WinQueryWindowPtr(hwnd, 0);
BYTE  szOptions[15];

   if (hwndSecond)
      wd->bSecondWindowOpen = TRUE;
   else
      wd->bSecondWindowOpen = FALSE;

   sprintf(szOptions, "WINDOWPOS%d", wd->usWindowNo);
   WinStoreWindowPos(APPS_NAME,szOptions,
      WinQueryWindow(hwnd, QW_PARENT));

   sprintf(szOptions, "OPTIONS%d", wd->usWindowNo);
   PrfWriteProfileData(HINI_USERPROFILE,
      APPS_NAME, szOptions, wd, sizeof (INSTANCE));
}
/****************************************************************
* Init the helpinstance
****************************************************************/
HWND InitHelpInstance(HWND hwndMain)
{
HELPINIT hini;
HWND     hwndHelpInstance;

   fHelpEnabled = FALSE;

   memset(&hini, 0, sizeof hini);
   hini.cb = sizeof(HELPINIT);
   hini.pszHelpWindowTitle = "IconTool Help";
   hini.pszHelpLibraryName = szHelpFname;
 
   hini.phtHelpTable = (PHELPTABLE)MAKELONG(EAS_HELP_TABLE, 0xFFFF);
                                
   hwndHelpInstance = WinCreateHelpInstance(hab, &hini);

   if (!hwndHelpInstance || hini.ulReturnCode)
      {
      DebugBox("WARNING", "Cannot find helpfile, no help available!", FALSE);
      return NULL;
      }
      
   if (!WinAssociateHelpInstance(hwndHelpInstance, hwndMain))
      {
      DebugBox("Error","Error on WinAssoctiateHelpInstance", TRUE);
      return NULL;
      }

   fHelpEnabled = TRUE;
   return hwndHelpInstance;
}

/******************************************************************
* Is A seamlesscommon session already active ?
******************************************************************/
BOOL WindowsSessionActive(VOID)
{
   return TRUE;

#if 0
ULONG ulEntries;
ULONG ulBlockSize;
PSWBLOCK pSwBlock;
ULONG ulIndex;
APIRET rc;
PTIB ptib;
PPIB ppib;

   rc = DosGetInfoBlocks(&ptib, &ppib);

   ulEntries = WinQuerySwitchList(hab, NULL, 0);
   ulBlockSize = sizeof (SWBLOCK) +
                 sizeof (HSWITCH) +
                 (ulEntries + 4) * (ULONG) sizeof(SWENTRY);
   pSwBlock = malloc(ulBlockSize);
   if (!pSwBlock)
      return FALSE;
   ulEntries = WinQuerySwitchList(hab, pSwBlock, ulBlockSize);
   /*
      Look for a DOS process with the same process ID is
      my own parent
   */
   for (ulIndex = 0; ulIndex < ulEntries; ulIndex++)
      {
      if (pSwBlock->aswentry[ulIndex].swctl.bProgType == PROG_WINDOWEDVDM &&
          pSwBlock->aswentry[ulIndex].swctl.idProcess == ppib->pib_ulppid)
         {
         free(pSwBlock);
         return TRUE;
         }
      }

   free(pSwBlock);
   return FALSE;
#endif
}

static MRESULT EXPENTRY DialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
typedef struct _Parms
{
PSZ pszTitle;
PSZ pszSet;
} PARMS, *PPARMS;

/***********************************************************************
* Handle the deletion dialog
***********************************************************************/
VOID ShowSetupString(HWND hwnd, PSZ pszTitle, PSZ pszSetup)
{
PARMS Parms;
USHORT usLen;
PSZ    pszEnd,
       pszTarget;

   Parms.pszTitle = pszTitle;

   usLen = strlen(pszSetup);
   usLen += 500;
   Parms.pszSet = malloc(usLen);
   if (!Parms.pszSet)
      {
      DebugBox("ICONTOOL", "Not enough memory!", FALSE);
      return;
      }

   memset(Parms.pszSet, 0, usLen);
   pszTarget = Parms.pszSet;

   while (*pszSetup)
      {
      pszEnd = pszSetup;
      for (;;)
         {
         pszEnd = strchr(pszEnd, ';');
         if (!pszEnd)
            {
            pszEnd = pszSetup + strlen(pszSetup);
            break;
            }
         if (pszEnd > pszSetup && *(pszEnd - 1) == '^')
            {
            pszEnd = pszEnd + 1;
            continue;
            }
         break;
         }
      sprintf(pszTarget, "%.*s;\n", pszEnd - pszSetup, pszSetup);
      pszTarget += strlen(pszTarget);
      pszSetup = pszEnd;
      if (*pszSetup == ';')
         pszSetup++;
      }


   WinDlgBox(HWND_DESKTOP,
   hwnd,
   DialogProc,
   0L,
   ID_SSTRINGDLG,
   &Parms);
}


/*************************************************************************
* Winproc procedure for the dialog
*************************************************************************/
static MRESULT EXPENTRY DialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
MRESULT mResult;
PPARMS  pParms;
PSWP    pswpNew;
HWND    hwndMLE;

   switch( msg )
      {
      case WM_ADJUSTWINDOWPOS:
      case WM_MINMAXFRAME:
         pswpNew = (PSWP)mp1;
         if (!(pswpNew->fl & SWP_SIZE))
            break;

         hwndMLE = WinWindowFromID(hwnd, ID_MLESETTING);
         if (!hwndMLE)
            break;

         WinSetWindowPos(hwndMLE, HWND_TOP,
                  0,
                  0,
                  pswpNew->cx - WinQuerySysValue(HWND_DESKTOP, SV_CXSIZEBORDER) * 2,
                  pswpNew->cy - WinQuerySysValue(HWND_DESKTOP, SV_CYSIZEBORDER) * 2 -
                                WinQuerySysValue(HWND_DESKTOP, SV_CYTITLEBAR),
                  SWP_SIZE | SWP_NOREDRAW);
         break;

      case WM_INITDLG :
         pParms = (PPARMS)mp2;
         WinSetWindowText (hwnd, pParms->pszTitle);
         WinSetDlgItemText(hwnd, ID_MLESETTING, pParms->pszSet);
         break;

      case WM_COMMAND :
         switch(SHORT1FROMMP(mp1))
            {
            case DID_OK :
               WinDismissDlg(hwnd, DID_OK);
               return (MRESULT)FALSE;
            }
         break;

      default:
         break;

      }
   mResult = WinDefDlgProc( hwnd, msg, mp1, mp2 );
   return mResult;
}

typedef struct _SetObjParms
{
HOBJECT hObject;
PSZ     pszTitle;
BYTE    szObjectID[65];
BYTE    szOldObjectID[65];
} SETOBJPARMS, *PSETOBJPARMS;

VOID SetObjectID(HWND hwnd, HOBJECT hObject, PSZ pszTitle)
{
SETOBJPARMS Parms;

   memset(&Parms, 0, sizeof Parms);
   Parms.hObject = hObject;
   Parms.pszTitle = pszTitle;
 
   WinSendMsg(hwndPopup, MM_QUERYITEMTEXT,
      MPFROM2SHORT(ID_OBJECTID, sizeof Parms.szObjectID), Parms.szObjectID);
   if (!strcmp(Parms.szObjectID, OBJECTID_NOT_SET))
      memset(Parms.szObjectID, 0, sizeof Parms.szObjectID);
   else
      strcpy(Parms.szOldObjectID, Parms.szObjectID);

   WinDlgBox(HWND_DESKTOP,
      hwnd,
      SetObjIDProc,
      0L,
      ID_DLGSETOBJID,
      &Parms);
}


static MRESULT EXPENTRY SetObjIDProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
PSETOBJPARMS pParms = (PSETOBJPARMS)WinQueryWindowPtr(hwnd, 0);    // retrieve instance data pointer
HOBJECT hObjNew;
static BYTE szSetup[128];

   switch (msg)
      {
      case WM_INITDLG :
         pParms = (PSETOBJPARMS)mp2;
         WinSendDlgItemMsg(hwnd, ID_NEWOBJECTID, EM_SETTEXTLIMIT,
            MRFROMSHORT(sizeof pParms->szObjectID - 1), 0L);
         WinSetDlgItemText(hwnd, ID_NEWOBJECTID,
            pParms->szObjectID);

         WinSetWindowPtr(hwnd, 0, (PVOID)pParms);
         break;

      case WM_COMMAND :
         switch(SHORT1FROMMP(mp1))
            {
            case DID_OK     :
               WinQueryDlgItemText(hwnd, ID_NEWOBJECTID,
                  sizeof pParms->szObjectID, pParms->szObjectID);
               if (!strlen(pParms->szObjectID))
                  {
                  DebugBox("IconTool", "An OBJECTID must be entered!", FALSE);
                  WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwnd, ID_NEWOBJECTID));
                  return (MRESULT)FALSE;
                  }
               if (pParms->szObjectID[0] != '<' ||
                  pParms->szObjectID[strlen(pParms->szObjectID) -1] != '>')
                  {
                  DebugBox("IconTool", "An OBJECT must start with < and end with >!", FALSE);
                  WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwnd, ID_NEWOBJECTID));
                  return (MRESULT)FALSE;
                  }
               if (strlen(pParms->szObjectID) < 3)
                  {
                  DebugBox("IconTool", "An OBJECTID may not be <>!", FALSE);
                  WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwnd, ID_NEWOBJECTID));
                  return (MRESULT)FALSE;
                  }
               hObjNew = WinQueryObject(pParms->szObjectID);
               if (hObjNew && hObjNew != pParms->hObject)
                  {
                  DebugBox("IconTool", "This OBJECTID is in use by another object!", FALSE);
                  WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwnd, ID_NEWOBJECTID));
                  return (MRESULT)FALSE;
                  }
               if (strcmp(pParms->szObjectID, pParms->szOldObjectID))
                  {
                  hObjNew = WinQueryObject(pParms->szOldObjectID);
                  if (hObjNew == pParms->hObject)
                     {
                     PrfWriteProfileData(HINI_USERPROFILE,
                        LOCATION,
                        pParms->szOldObjectID,
                        NULL,
                        0L);
                     }
                  }
               sprintf(szSetup, "OBJECTID=%s", pParms->szObjectID);
               if (!WinSetObjectData(pParms->hObject, szSetup))
                  {
                  DebugBox("IconTool", "WinSetObjectData failed", TRUE);
                  WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwnd, ID_NEWOBJECTID));
                  return (MRESULT)FALSE;
                  }
               WinSaveObject(pParms->hObject, FALSE);




               WinDismissDlg(hwnd, DID_OK);
               return (MRESULT)FALSE;
            case DID_CANCEL :
               WinDismissDlg(hwnd, DID_CANCEL);
               return (MRESULT)FALSE;
            }
         break;

      default:
         break;

      
      }
   return WinDefDlgProc( hwnd, msg, mp1, mp2 );
}
