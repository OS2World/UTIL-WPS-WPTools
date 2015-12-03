//#define TEMP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <process.h>
#include <io.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>

#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_GPI
#define INCL_WINSTDFONT  /* Window Standard Font Functions     */
#define INCL_DEV
#include <os2.h>

#include "..\os2hek\os2hek.h"
#include "eabrowse.h"

#define TITLE        "EA-Browser"
#define OPTKEY       "Options"

#define MAX_MESSAGE  512
#define CHARS_ON_ROW 16
#define MAX_MVMTLEVEL 2

#pragma pack(1)
typedef struct _IconRes
{
BYTE   bFF;
USHORT usID;
} ICONRES, *PICONRES;
#pragma pack()

typedef struct _BinData
{
BYTE   bBinBuffer[65536];
ULONG  ulBinSize;
ULONG  ulCurPos;
BOOL   fRightSide;
BOOL   fRightNibble;
} BINDATA, *PBINDATA;

typedef struct _FeaData
{
BOOL  fChanged;
FEA2  Fea;
} FEADATA, *PFEADATA;

typedef struct _Current
{
BYTE        szPathOrFile[300];
PFEA2LIST   pFea2List;
PFEADATA    pFeaData;
PBYTE       pbValue;
USHORT      usValueSize;
SWP         swpData;
} CUR, *PCUR;

/********************************************************************
* Globale variabelen
********************************************************************/
OPTIONS Options = {0};

static HAB       hab;
static HMQ       hmq;

static CUR       Cur;
static BYTE      bDena[2000];
static BYTE      szStartupDir[CCHMAXPATH];
static BYTE      szStartupFile[CCHMAXPATHCOMP];
static BYTE      szMessage[MAX_MESSAGE];
static ULONG     ulBorderCX,
                 ulBorderCY;
static ULONG     ulThinBorderCX,
                 ulThinBorderCY;
static ULONG     ulBackgroundTop,
                 ulBackgroundBottom;
static BOOL      fOptionsFound = FALSE;
static PFNWP     OldMLEProc;
static ULONG     ulPointSize = 10;
static PSZ       rgpszSpecialEA[]=
{
".CLASSINFO",
".ICONPOS",
NULL
};



/********************************************************************
* Function prototypes
********************************************************************/
MRESULT EXPENTRY WindowProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
HWND             CreateWindow(VOID);
VOID             SetMenuOptions  (HWND hwndClient);
VOID             CreateWindowControls(HWND hwnd);
VOID             HandleFontDialog(HWND hwndClient);
VOID             HandleOpenDialog(HWND hwndClient);
BOOL             ApplyFont       (HWND hwnd, PSZ pszFamily, ULONG ulPointSize);
VOID             AdjustAllPositions(HWND hwnd);
VOID             FillFileBox     (HWND hwnd);
BOOL             ChangeDir       (HWND hwnd, PSZ pszSelection);
VOID             FillEABox       (HWND hwnd);
PSZ              GetOS2Error     (APIRET rc);
PSZ              GetPMError      (VOID);
ULONG            GetTextHeight   (HWND hwnd);
PFEA2LIST        GetDirEA        (PSZ pszName);
PFEA2LIST        GetFileEA       (PSZ pszName);
PFEA2LIST        GetEAs          (ULONG ulDenaCnt, PDENA2 pDenaBlock, HFILE hfFile, PSZ pszName);
VOID             SetStatusText   (HWND hwndClient, PSZ pszText);
VOID             DrawStatusArea  (HWND hwnd, HPS hps);
VOID             ShowSelectedEA  (HWND hwndClient);
VOID             ShowBinary      (HWND hwndData, PSWP pswpArea, PBYTE pValue, USHORT usValueSize);
PSZ              FormatBinData   (PBYTE pValue, USHORT usValueSize);
VOID             ShowIcon        (HWND hwndData, PSWP pswpArea, PBYTE pValue, USHORT usValueSize);
VOID             ShowAscii       (HWND hwndData, PSWP pswpArea, PBYTE pValue, USHORT usValueSize);
BOOL             ShowMVMT        (HWND hwndParent, PSWP pswpArea, PBYTE pValue, USHORT usLevel);
VOID             ShowSelectedMVMT(HWND hwndParent, USHORT usLevel);
USHORT           GetEASize       (PBYTE pValue);
USHORT           GetMVMTSize     (PBYTE pValue);
BOOL             DeleteEA        (HWND hwndClient);
BOOL             UpdateFeaData   (HWND hwndClient, PBYTE pNewData, USHORT usNewSize);
BOOL             WriteEA         (PSZ  pszPath, PFEADATA pFeaData);

#pragma linkage(EditIconEA, optlink)
VOID            EditIconEA       (PVOID pvClient);

/*
   Some special functions for the binary MLE
*/
MRESULT EXPENTRY MyMLEProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL             ProcessBinChar(HWND hwnd, MPARAM mp1, MPARAM mp2);
VOID             ShowCharacter  (HWND hwnd);
VOID             GetMLECursorPos(HWND hwnd);
VOID             SetMLECursorPos(HWND hwnd);


/********************************************************************
* The main thing
********************************************************************/
INT main(INT iArgc, PSZ pszArgv[])
{
QMSG qmsg;
HWND hwndFrame;

   hab = WinInitialize(0);
   if (!hab)
      {
      MessageBox(TITLE, "WinInitialize\n%s", GetPMError());
      exit(1);
      }
   hmq = WinCreateMsgQueue(hab, 0);
   if (!hmq)
      {
      MessageBox(TITLE, "WinCreateMsgQueue\n%s", GetPMError());
      exit(1);
      }

   if (iArgc > 1)
      {
      APIRET rc;
      FILESTATUS3 fStat;
      PSZ p;

      strncpy(szStartupDir, pszArgv[1], sizeof szStartupDir);
      rc = DosQueryPathInfo(szStartupDir,
         FIL_STANDARD,
         &fStat,
         sizeof fStat);
      if (!rc)
         {
         p = strrchr(szStartupDir, '\\');
         if (p)
            {
            strcpy(szStartupFile, p + 1);
            *p = 0;
            if (strlen(szStartupDir) == 2)
               strcat(szStartupDir, "\\");
            }
         }
      rc = DosQueryPathInfo(szStartupDir,
         FIL_STANDARD,
         &fStat,
         sizeof fStat);
      if (rc)
         {
         MessageBox(TITLE, "DosQueryPathInfo on %s:\n%s",
            szStartupDir, GetOS2Error(rc));
         memset(szStartupDir, 0, sizeof szStartupDir);
         memset(szStartupFile, 0, sizeof szStartupFile);
         }
      }


   ulBorderCX = WinQuerySysValue(HWND_DESKTOP, SV_CXSIZEBORDER) / 2;
   ulBorderCY = WinQuerySysValue(HWND_DESKTOP, SV_CYSIZEBORDER) / 2;

   ulThinBorderCX = WinQuerySysValue(HWND_DESKTOP, SV_CXBORDER);
   ulThinBorderCY = WinQuerySysValue(HWND_DESKTOP, SV_CYBORDER);

   if (!WinRegisterClass(hab,
      "MyClass", 
      WindowProc,
      CS_SIZEREDRAW,
      sizeof (PVOID)))
      {
      MessageBox(TITLE, "WinRegisterClass\n%s", GetPMError());
      exit(1);
      }

   hwndFrame  = CreateWindow();

   while( WinGetMsg( hab, &qmsg, 0L, 0, 0 ) )
       WinDispatchMsg( hab, &qmsg );

   if (!WinStoreWindowPos(TITLE, "WINDOWPOS", hwndFrame))
      MessageBox(TITLE, "WinStoreWindowPos\n%s", GetPMError());

   PrfWriteProfileData(HINI_USERPROFILE,
      TITLE, OPTKEY, &Options, sizeof Options);

   WinSubclassWindow(WinWindowFromID(hwndFrame, ID_BINARY),
      OldMLEProc);
   WinDestroyWindow(hwndFrame);
   WinDestroyMsgQueue(hmq);
   WinTerminate(hab);
   return 0;
}


/*************************************************************
* Create the standard window
**************************************************************/
HWND CreateWindow(VOID)
{
ULONG ulStyle;
HWND hwndWindow,
     hwndClient;


   ulStyle = FCF_SYSMENU | FCF_TITLEBAR | FCF_MINMAX | FCF_ICON |
      FCF_SIZEBORDER | FCF_NOBYTEALIGN | FCF_TASKLIST | FCF_MENU;

   hwndWindow = WinCreateStdWindow(HWND_DESKTOP,      
      0L,                
      &ulStyle,          
      "MyClass",    
      TITLE,
      0L,                
      (HMODULE)0,        
      ID_WINDOW,            
      &hwndClient
      );

   if (!hwndWindow)
      {
      MessageBox(TITLE, "WinCreateStdWindow\n%s", GetPMError());
      exit(1);
      }

   if (!WinRestoreWindowPos(TITLE,"WINDOWPOS", hwndWindow))
      WinSetWindowPos(hwndWindow,
                  HWND_TOP, 50, 32, 540, 420,
                  SWP_SIZE | SWP_ACTIVATE | SWP_SHOW | SWP_MOVE);
   else
      WinSetWindowPos(hwndWindow,
                  HWND_TOP, 50, 90, 540, 300,
                  SWP_ACTIVATE | SWP_SHOW | SWP_RESTORE);

   if (fOptionsFound)
      ApplyFont(hwndClient, Options.szFaceName, Options.ulPointSize);
   AdjustAllPositions(hwndClient);

   SetMenuOptions(hwndClient);

   return hwndWindow;
}


/*************************************************************************
* Set Allow Edit options
*************************************************************************/
VOID SetMenuOptions(HWND hwndClient)
{
HWND hwndFrame = WinQueryWindow(hwndClient, QW_PARENT);
HWND hwndMenu  = WinWindowFromID(hwndFrame, FID_MENU);


   WinCheckMenuItem(hwndMenu, MID_BINARY,      Options.fShowAllBinary);
   WinCheckMenuItem(hwndMenu, MID_EAFILESONLY, Options.fShowEAFilesOnly);
   WinCheckMenuItem(hwndMenu, MID_ALLOWEDIT,   Options.fAllowEdit);

   WinEnableMenuItem(hwndMenu, MID_ADD   , Options.fAllowEdit);
   WinEnableMenuItem(hwndMenu, MID_DELETE, Options.fAllowEdit);
   WinEnableMenuItem(hwndMenu, MID_SAVE  , Options.fAllowEdit);


   WinSendMsg(WinWindowFromID(hwndClient, ID_ASCII),
      MLM_SETREADONLY, (MPARAM)!Options.fAllowEdit, 0);

   WinSendMsg(WinWindowFromID(hwndClient, ID_BINARY),
      MLM_SETREADONLY, (MPARAM)!Options.fAllowEdit, 0);

}
/*************************************************************************
* Winproc procedure for the panel
*************************************************************************/
MRESULT EXPENTRY WindowProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
MRESULT mResult;
HPS     hps;
RECTL   rc;


   switch( msg )
      {
      case WM_CREATE:
         CreateWindowControls(hwnd);
         return (MRESULT)FALSE;

      case WM_ERASEBACKGROUND:
         return (MRESULT)FALSE;

      case WM_PAINT:
         WinQueryWindowRect(hwnd, &rc);
         hps = WinBeginPaint(hwnd, NULL, &rc);
         WinFillRect(hps, &rc, SYSCLR_DIALOGBACKGROUND);
         DrawStatusArea(hwnd, hps);
         WinEndPaint(hps);
         return (MRESULT)FALSE;
        
      case WM_CLOSE     :
         WinPostMsg( hwnd, WM_QUIT, 0L, 0L );  /* Cause termination        */
         return 0L;

      case WM_CHAR :
         {
         USHORT usFlags = SHORT1FROMMP(mp1);
         USHORT usVK    = SHORT2FROMMP(mp2);
         HWND   hwndCur;
         USHORT usID;

         if (usFlags & KC_KEYUP)
               break;

         if (!(usFlags & KC_VIRTUALKEY))
            break;

         if (usVK != VK_TAB && usVK != VK_BACKTAB)
            break;

         hwndCur = WinQueryWindowULong(
            WinQueryWindow(hwnd, QW_PARENT), QWL_HWNDFOCUSSAVE);
         if (!WinIsWindow(hab, hwndCur))
            usID = ID_FIRST;
         else
            usID = WinQueryWindowUShort(hwndCur, QWS_ID);
         if (usID < ID_FIRST || usID > ID_LAST)
            usID = ID_FIRST;

         do
            {
            if (usVK == VK_TAB)
               usID++;
            else
               usID--;
            if (usID < ID_FIRST)
               usID = ID_LAST;
            else if (usID > ID_LAST)
               usID = ID_FIRST;
            }  while (!WinIsWindowVisible(WinWindowFromID(hwnd, usID)));

         WinSetFocus(HWND_DESKTOP, WinWindowFromID(hwnd, usID));
         break;
         }

      case WM_SAVEAPPLICATION:
         if (!WinStoreWindowPos(TITLE, "WINDOWPOS", WinQueryWindow(hwnd, QW_PARENT)))
            MessageBox(TITLE, "WinStoreWindowPos\n%s", GetPMError());
         PrfWriteProfileData(HINI_USERPROFILE,
            TITLE, OPTKEY, &Options, sizeof Options);
         break;

      case WM_PRESPARAMCHANGED:
         AdjustAllPositions(hwnd);
         break;

      case WM_SIZE:
         AdjustAllPositions(hwnd);
         break;

      
      case WM_COMMAND:
         if (SHORT1FROMMP(mp2) == CMDSRC_PUSHBUTTON)
            {
            switch (SHORT1FROMMP(mp1))          /* test the command value from mp1 */
               {
               case ID_ICON:
                  if (!Options.fAllowEdit)
                     break;
                  _beginthread(EditIconEA, NULL, 12000, (PVOID)hwnd);
                  break;

               default :
                  MessageBox(TITLE, "Oops !\n I knew there was something I should have done...<g>");
                  break;
               }

            }

         if (SHORT1FROMMP(mp2) == CMDSRC_MENU)
            {
            switch (SHORT1FROMMP(mp1))          /* test the command value from mp1 */
               {
               case MID_BINARY  :
                  Options.fShowAllBinary = !Options.fShowAllBinary;
                  SetMenuOptions(hwnd);
                  ShowSelectedEA(hwnd);
                  break;

               case MID_EAFILESONLY:
                  Options.fShowEAFilesOnly = !Options.fShowEAFilesOnly;
                  SetMenuOptions(hwnd);
                  FillFileBox(hwnd);
                  break;

#ifndef TEMP
               case MID_ALLOWEDIT :
                  Options.fAllowEdit = !Options.fAllowEdit;
                  SetMenuOptions(hwnd);
                  break;
#endif

               case MID_FONT    :
                  HandleFontDialog(hwnd);
                  break;

               case MID_OPEN    :
                  HandleOpenDialog(hwnd);
                  break;

               case MID_QUIT    :
                  WinPostMsg(hwnd, WM_CLOSE, 0, 0);
                  break;
#ifndef TEMP
               case MID_DELETE:
                  DeleteEA(hwnd);
                  break;
#endif
               case MID_SAVE  :
                  if (!Cur.pFeaData)
                     break;
                  WriteEA(Cur.szPathOrFile, Cur.pFeaData);
                  break;

               case MID_ABOUT:
                  if (WinDlgBox(HWND_DESKTOP,
                     hwnd,
                     WinDefDlgProc,
                     NULL,
                     ID_ABOUTDLG,
                     NULL) == DID_ERROR)
                     {
                     MessageBox(TITLE,"Error on WinDlgBox");
                     break;
                     }
                  break;


               default :
                  MessageBox(TITLE, "Oops !\n I knew there was something I should have done...<g>");
                  break;
               }
            break;
            }
         return (MRESULT)FALSE;


      case WM_CONTROL:
         switch (SHORT2FROMMP(mp1))
            {
            case LN_ENTER :
               if (SHORT1FROMMP(mp1) == ID_FILELIST)
                  {
                  BYTE  szSelText[256];
                  SHORT sRetInx =
                     WinQueryLboxSelectedItem(WinWindowFromID(hwnd, ID_FILELIST));
                  if (sRetInx == LIT_NONE)
                     break;
                  WinQueryLboxItemText(WinWindowFromID(hwnd, ID_FILELIST),
                     sRetInx,
                     szSelText,
                     sizeof szSelText);
                  if (ChangeDir(hwnd, szSelText))
                     FillFileBox(hwnd);
                  }
               break;
            case LN_SELECT:
               if (SHORT1FROMMP(mp1) == ID_FILELIST)
                  {
                  FillEABox(hwnd);
                  SetStatusText(hwnd, szMessage);
                  }
               else if (SHORT1FROMMP(mp1) == ID_EALIST)
                  ShowSelectedEA(hwnd);
               else if (SHORT1FROMMP(mp1) == ID_MVMTLIST)
                  ShowSelectedMVMT(hwnd, 0);
               else if (SHORT1FROMMP(mp1) == ID_MVMTLIST2)
                  ShowSelectedMVMT(hwnd, 1);
               break;

            case MLN_KILLFOCUS:
               if (!Options.fAllowEdit)
                  break;

               if (SHORT1FROMMP(mp1) == ID_BINARY)
                  {
                  HWND hwndMLE  = WinWindowFromID(hwnd, SHORT1FROMMP(mp1));
                  if (WinSendMsg(hwndMLE, MLM_QUERYCHANGED, 0, 0))
                     {
                     PBINDATA pBin = WinQueryWindowPtr(hwndMLE, 0);
                     UpdateFeaData(hwnd, pBin->bBinBuffer, pBin->ulBinSize);
                     WinSendMsg(hwndMLE,
                        MLM_SETCHANGED, MPFROMSHORT(FALSE), 0);
                     ShowSelectedEA(hwnd);
                     }
                  }
               else if (SHORT1FROMMP(mp1) == ID_ASCII)
                  {
                  HWND  hwndMLE   = WinWindowFromID(hwnd, SHORT1FROMMP(mp1));
                  if (WinSendMsg(hwndMLE, MLM_QUERYCHANGED, 0, 0))
                     {
                     ULONG ulLength = WinQueryWindowTextLength(hwndMLE);
                     PBYTE pValue   = malloc(ulLength + 1);
                     if (!pValue)
                        {
                        MessageBox(TITLE, "Not enough memory !");
                        DosExit(EXIT_PROCESS, 0);
                        }
                     memset(pValue, 0, ulLength);
                     WinQueryWindowText(hwndMLE, ulLength + 1, pValue);
                     UpdateFeaData(hwnd, pValue, ulLength);
                     free(pValue);
                     WinSendMsg(hwndMLE,
                        MLM_SETCHANGED, MPFROMSHORT(FALSE), 0);
                     ShowSelectedEA(hwnd);
                     }
                  }
               break;

            }
         break;

    default:
         mResult = WinDefWindowProc( hwnd, msg, mp1, mp2 );
         return mResult;
      }
   mResult = WinDefWindowProc( hwnd, msg, mp1, mp2 );
   return mResult;
}

/***************************************************************
* Handle the font dialog
***************************************************************/
VOID HandleFontDialog(HWND hwndClient)
{
HPS     hpsScreen;       /* Screen presentation space          */
FONTDLG pfdFontdlg;      /* Font dialog info structure         */
HWND    hwndFontDlg;     /* Font dialog window                 */
BYTE    szFamily[FAMILYLEN];

   strcpy(szFamily, Options.szFontFamily);

   hpsScreen = WinGetPS(hwndClient);
 
   memset(&pfdFontdlg, 0, sizeof(FONTDLG));
   pfdFontdlg.cbSize = sizeof(FONTDLG);   /* Size of structure     */
   pfdFontdlg.hpsScreen = hpsScreen;      /* Screen presentation   */
                                          /* space                 */

   pfdFontdlg.pszFamilyname  = szFamily;
   pfdFontdlg.usFamilyBufLen = sizeof szFamily - 1;
   pfdFontdlg.fxPointSize    = MAKEFIXED((USHORT)Options.ulPointSize,0); 
   pfdFontdlg.fl             = FNTS_HELPBUTTON | FNTS_CENTER; /* FNTS_* flags */
   pfdFontdlg.clrFore = CLR_BLACK;       /* Foreground color      */
   pfdFontdlg.clrBack = CLR_WHITE;       /* Background color      */
   pfdFontdlg.fAttrs.usCodePage = 437;   /* Code page to select   */
                                         /* from                  */
   strcpy(pfdFontdlg.fAttrs.szFacename, Options.szFaceName);
 
   hwndFontDlg = WinFontDlg(HWND_DESKTOP, hwndClient, &pfdFontdlg);
 
   if (hwndFontDlg && (pfdFontdlg.lReturn == DID_OK))
      {
      strcpy(Options.szFontFamily, szFamily);
      strcpy(Options.szFaceName, pfdFontdlg.fAttrs.szFacename);
      Options.ulPointSize = pfdFontdlg.fxPointSize / 65536L;

      WinInvalidateRect(hwndClient, NULL, TRUE);

      ApplyFont(hwndClient, Options.szFaceName, Options.ulPointSize);
      AdjustAllPositions(hwndClient);
      WinDestroyWindow(hwndFontDlg);
      }
   WinReleasePS(hpsScreen);

}

/***************************************************************
* Apply Font
***************************************************************/
BOOL ApplyFont(HWND hwnd, PSZ pszFaceName, ULONG ulPointSize)
{
HENUM henum;
BYTE  szFontName[FACESIZE + 4];
HWND  hwndChild;

   sprintf(szFontName, "%ld.%s", ulPointSize, pszFaceName);

   henum = WinBeginEnumWindows(hwnd);
   while ((hwndChild = WinGetNextWindow(henum)) != NULLHANDLE)
      {
      USHORT usID = WinQueryWindowUShort(hwndChild, QWS_ID);
      if (usID != ID_BINARY)
         WinSetPresParam(hwndChild, PP_FONTNAMESIZE,
            (ULONG)(strlen(szFontName) + 1), szFontName);
      }
   WinEndEnumWindows(henum);

   return TRUE;
}

/***************************************************************
* Handle the open dialog
***************************************************************/
VOID HandleOpenDialog(HWND hwndClient)
{
#if 0
HWND hwndFileDlg;
FILEDLG pfdFiledlg;

   memset(&pfdFiledlg, 0, sizeof pfdFiledlg);
   pfdFiledlg.cbSize   = sizeof pfdFiledlg;
   pfdFiledlg.fl       = FDS_OPEN_DIALOG | FDS_HELPBUTTON | FDS_CENTER;
   pfdFiledlg.pszTitle = "Select a file or directory";
   pfdFiledlg.pszOKButton = "~Select";
   pfdFiledlg.pszIDrive   = Options.szCurDir;
   strcpy(pfdFiledlg.szFullFile, Options.szCurDir);
   if (pfdFiledlg.szFullFile[strlen(pfdFiledlg.szFullFile) - 1] != '\\')
      strcat(pfdFiledlg.szFullFile, "\\");
   strcat(pfdFiledlg.szFullFile, "*.*");


   
   hwndFileDlg = WinFileDlg(HWND_DESKTOP, hwndClient, &pfdFiledlg);
 
   if (hwndFileDlg && (pfdFiledlg.lReturn == DID_OK))
      {

      WinDestroyWindow(hwndFileDlg);
      }
#endif
   if (ChangeDirDialog(hwndClient))
      FillFileBox(hwndClient);
}


/***************************************************************
* Create the frame controls
***************************************************************/
VOID CreateWindowControls(HWND hwnd)
{
HWND  hwndField;
ULONG ulColor;
ULONG ulPrfSize;
PBINDATA pBinData;
BTNCDATA ButCtl;
FATTRS   fAttrs;
HDC hdc;
LONG lVertFontRes, lHorzFontRes;

   ulPrfSize = sizeof Options;
   fOptionsFound = TRUE;
   if (!PrfQueryProfileData(HINI_USERPROFILE,
      TITLE,
      OPTKEY,
      &Options,
      &ulPrfSize) ||
      ulPrfSize != sizeof Options)
      fOptionsFound = FALSE;

   if (fOptionsFound)
      {
      if (access(Options.szCurDir, 0))
         {
         _getdcwd(0, Options.szCurDir, sizeof Options.szCurDir);
         Options.szCurDir[0] = (BYTE)(_getdrive() + '@');
         }
      }
   if (strlen(szStartupDir))
      strcpy(Options.szCurDir, szStartupDir);

   ulColor = SYSCLR_DIALOGBACKGROUND;
   /*
      Create the text above the listbox
   */
   hwndField = WinCreateWindow(hwnd,
      WC_STATIC,
      "Files",
      SS_TEXT | DT_CENTER | DT_VCENTER,
      0,0,0,0,
      hwnd,
      HWND_TOP,
      ID_FILETITLE,
      NULL,
      NULL);
   if (!hwndField)
      {
      MessageBox(TITLE, "WinCreateWindow:/n%s", GetPMError());
      exit(1);
      }


   WinSetPresParam(hwndField,
      PP_BACKGROUNDCOLORINDEX,
      sizeof ulColor,
      &ulColor);


   /*
      Create first listbox
   */
   hwndField = WinCreateWindow(hwnd,
      WC_LISTBOX,                                 /* class     */
      "",                                         /* text      */
      LS_NOADJUSTPOS | LS_HORZSCROLL,
      0, 0, 0, 0,
      hwnd,                                       /* owner     */
      HWND_TOP,                                   /* behind    */
      ID_FILELIST,                                /* win ID    */
      NULL,                                       /* ctl data  */
      NULL);                                      /* reserved  */

   if (!hwndField)
      {
      MessageBox(TITLE, "WinCreateWindow:/n%s", GetPMError());
      exit(1);
      }


   /*
      Create the text above the listbox
   */
   hwndField = WinCreateWindow(hwnd,
      WC_STATIC,
      "Extended Attributes",
      SS_TEXT | DT_CENTER | DT_VCENTER,
      0,0,0,0,
      hwnd,
      HWND_TOP,
      ID_EATITLE,
      NULL,
      NULL);

   if (!hwndField)
      {
      MessageBox(TITLE, "WinCreateWindow:/n%s", GetPMError());
      exit(1);
      }

   WinSetPresParam(hwndField,
      PP_BACKGROUNDCOLORINDEX,
      sizeof ulColor,
      &ulColor);
   

   /*
      Create second listbox
   */
   hwndField = WinCreateWindow(hwnd,
      WC_LISTBOX,                                 /* class     */
      "",                                         /* text      */
      LS_NOADJUSTPOS | LS_HORZSCROLL,
      0, 
      0,
      0,
      0,
      hwnd,                                       /* owner     */
      HWND_TOP,                                   /* behind    */
      ID_EALIST,                                  /* win ID    */
      NULL,                                       /* ctl data  */
      NULL);                                      /* reserved  */
   if (!hwndField)
      {
      MessageBox(TITLE, "WinCreateWindow:/n%s", GetPMError());
      exit(1);
      }


   /*
      The EA type
   */
   hwndField = WinCreateWindow(hwnd,
      WC_STATIC,
      "EA Type",
      SS_TEXT | DT_LEFT | SS_AUTOSIZE,
      0,0,0,0,
      hwnd,
      HWND_TOP,
      ID_EATYPE,
      NULL,
      NULL);
   if (!hwndField)
      {
      MessageBox(TITLE, "WinCreateWindow:/n%s", GetPMError());
      exit(1);
      }

   WinSetPresParam(hwndField,
      PP_BACKGROUNDCOLORINDEX,
      sizeof ulColor,
      &ulColor);


   /*
      The MLE for BINARY data
   */

   hwndField = WinCreateWindow(hwnd,
      WC_MLE,
      "",
      MLS_BORDER | MLS_VSCROLL | MLS_HSCROLL | MLS_READONLY |
         MLS_DISABLEUNDO | WS_VISIBLE | MLS_IGNORETAB,
      0,0,0,0,
      hwnd,
      HWND_TOP,
      ID_BINARY,
      NULL,
      NULL);
   if (!hwndField)
      {
      MessageBox(TITLE, "WinCreateWindow:/n%s", GetPMError());
      exit(1);
      }

   hdc = WinOpenWindowDC(hwndField);
   DevQueryCaps(hdc, CAPS_VERTICAL_FONT_RES, 1, &lVertFontRes);
   DevQueryCaps(hdc, CAPS_HORIZONTAL_FONT_RES, 1, &lHorzFontRes);


   memset(&fAttrs, 0, sizeof fAttrs);
   fAttrs.usRecordLength = sizeof fAttrs;
   strcpy(fAttrs.szFacename, "Courier");
//   fAttrs.lMatch = 7;
//   fAttrs.lMaxBaselineExt = 13;
//   fAttrs.lAveCharWidth   = 8;
   fAttrs.lMaxBaselineExt = lHorzFontRes * 10 / 72;
   fAttrs.lAveCharWidth   = lVertFontRes * 10 / 72;

   fAttrs.fsFontUse       = FATTR_FONTUSE_NOMIX;
   WinSendMsg(hwndField, MLM_SETFONT, (MPARAM)&fAttrs, 0);


#if 0
   strcpy(szFontName, "8.Courier");
   if (!WinSetPresParam(hwndField, PP_FONTNAMESIZE,
      (ULONG)(strlen(szFontName) + 1), szFontName))
      {
      MessageBox(TITLE, "WinSetPresParam:\n%s", GetPMError());
      }
#endif

   pBinData = malloc(sizeof (BINDATA));
   if (!pBinData)
      {
      MessageBox(TITLE, "Not enough memory!");
      exit(1);
      }
   if (!WinSetWindowPtr(hwndField, 0, pBinData))
      {
      MessageBox(TITLE, "WinSetWindowPtr\n%s", GetPMError());
      exit(1);
      }

   OldMLEProc = WinSubclassWindow(hwndField, MyMLEProc);
   if (!OldMLEProc)
      {
      MessageBox(TITLE, "WinSubclassWindow:/n%s", GetPMError());
      exit(1);
      }



   /*
      The ICON control
   */

   memset(&ButCtl, 0, sizeof ButCtl);
   ButCtl.cb            = sizeof ButCtl;
   ButCtl.fsCheckState  = 0;
   ButCtl.fsHiliteState = 0;
   ButCtl.hImage        =
      (LHANDLE)WinLoadPointer(HWND_DESKTOP,0L, ID_WINDOW);
   hwndField = WinCreateWindow(hwnd,
      WC_BUTTON,
      "Edit",
      BS_PUSHBUTTON | BS_ICON,
      0,0,0,0,
      hwnd,
      HWND_TOP,
      ID_ICON,
      &ButCtl,
      NULL);
   if (!hwndField)
      {
      MessageBox(TITLE, "WinCreateWindow:/n%s", GetPMError());
      exit(1);
      }



   /*
      The MLE for ASCII fields
   */
   hwndField = WinCreateWindow(hwnd,
      WC_MLE,
      "",
      MLS_BORDER | MLS_VSCROLL | MLS_HSCROLL | MLS_READONLY |
         WS_VISIBLE | MLS_IGNORETAB | MLS_DISABLEUNDO, 
      0,0,0,0,
      hwnd,
      HWND_TOP,
      ID_ASCII,
      NULL,
      NULL);
   if (!hwndField)
      {
      MessageBox(TITLE, "WinCreateWindow:/n%s", GetPMError());
      exit(1);
      }


   /*
      The Listbox for the MVMT
   */

   hwndField = WinCreateWindow(hwnd,
      WC_LISTBOX,   
      "",           
      LS_NOADJUSTPOS | LS_HORZSCROLL,
      0, 0, 0, 0,
      hwnd,
      HWND_TOP,     
      ID_MVMTLIST,  
      NULL,         
      NULL);
   if (!hwndField)
      {
      MessageBox(TITLE, "WinCreateWindow:/n%s", GetPMError());
      exit(1);
      }

   hwndField = WinCreateWindow(hwnd,
      WC_LISTBOX,   
      "",           
      LS_NOADJUSTPOS | LS_HORZSCROLL,
      0, 0, 0, 0,
      hwnd,
      HWND_TOP,     
      ID_MVMTLIST2,  
      NULL,         
      NULL);        
   if (!hwndField)
      {
      MessageBox(TITLE, "WinCreateWindow:/n%s", GetPMError());
      exit(1);
      }

   /*
      Create the status AREA
   */

   hwndField = WinCreateWindow(hwnd,
      WC_STATIC,
      "StatusLine",
      SS_TEXT | DT_LEFT | DT_TOP | SS_AUTOSIZE| DT_WORDBREAK,
      0,0,0,0,
      hwnd,
      HWND_TOP,
      ID_STATUS,
      NULL,
      NULL);
   if (!hwndField)
      {
      MessageBox(TITLE, "WinCreateWindow:/n%s", GetPMError());
      exit(1);
      }

   WinSetPresParam(hwndField,
      PP_BACKGROUNDCOLORINDEX,
      sizeof ulColor,
      &ulColor);


   FillFileBox(hwnd);


}

/*************************************************************************
* Draw status Area
*************************************************************************/
VOID DrawStatusArea(HWND hwnd, HPS hps)
{
ULONG ulCharHeight;
HWND  hwndStatus;
RECTL rctl;
SWP   swpClient;

   WinQueryWindowPos(hwnd, &swpClient);

   hwndStatus = WinWindowFromID(hwnd, ID_STATUS);
   if (!hwndStatus)
      return;
   ulCharHeight = GetTextHeight(hwndStatus);
   rctl.xLeft   = ulBorderCX;
   rctl.xRight  = swpClient.cx;
   rctl.yBottom = 0;
   rctl.yTop    = ulCharHeight * 2 + ulBorderCY * 3;

   WinDrawBorder(hps,
      &rctl,
      1,1,
      SYSCLR_WINDOWFRAME,
      SYSCLR_BACKGROUND,
      DB_PATCOPY|DB_STANDARD);
}

/*****************************************************************
* Adjust all positions
*****************************************************************/
VOID AdjustAllPositions(HWND hwndClient)
{
SWP    swpClient,
       swpField;
ULONG  ulCharHeight;
HWND   hwnd;


   /*
      Get current position of client area
   */
   WinQueryWindowPos(hwndClient, &swpClient);

   /* TITLE ABOVE FILE BOX */

   hwnd = WinWindowFromID(hwndClient, ID_FILETITLE);
   ulCharHeight = GetTextHeight(hwnd);
   WinSetWindowPos(hwnd,
      HWND_TOP,
      0,
      swpClient.cy - ulCharHeight,
      swpClient.cx / 2,
      ulCharHeight,
      SWP_SIZE | SWP_MOVE | SWP_SHOW);

   /* FILE BOX */

   WinSetWindowPos(WinWindowFromID(hwndClient, ID_FILELIST),
      HWND_TOP,
      0,
      swpClient.cy / 2,
      swpClient.cx / 2,
      swpClient.cy / 2 - ulCharHeight,
      SWP_SIZE | SWP_MOVE | SWP_SHOW);

   /* TITLE ABOVE EA's */

   hwnd = WinWindowFromID(hwndClient, ID_EATITLE);
   ulCharHeight = GetTextHeight(hwnd);
   WinSetWindowPos(hwnd,
      HWND_TOP,
      swpClient.cx / 2,
      swpClient.cy - ulCharHeight,
      swpClient.cx / 2,
      ulCharHeight,
      SWP_SIZE | SWP_MOVE | SWP_SHOW);

   /* EA-BOX */

   WinSetWindowPos(WinWindowFromID(hwndClient, ID_EALIST),
      HWND_TOP,
      swpClient.cx / 2,
      swpClient.cy / 2,
      swpClient.cx / 2,
      swpClient.cy / 2 - ulCharHeight,
      SWP_SIZE | SWP_MOVE | SWP_SHOW);

   /* The EA Type     */

   hwnd = WinWindowFromID(hwndClient, ID_EATYPE);
   ulCharHeight = GetTextHeight(hwnd);
   WinSetWindowPos(hwnd,
      HWND_TOP,
      ulBorderCX * 2,
      swpClient.cy / 2 - ( ulCharHeight + ulCharHeight /2),
      swpClient.cx - (ulBorderCX * 2),
      ulCharHeight,
      SWP_SIZE | SWP_MOVE | SWP_SHOW);

   WinQueryWindowPos(hwnd, &swpField);
   ulBackgroundTop    = swpField.y - 3;
   ulBackgroundBottom = ulCharHeight * 2 + ulBorderCY * 4;

   /* The status AREA */

   hwnd = WinWindowFromID(hwndClient, ID_STATUS);
   ulCharHeight = GetTextHeight(hwnd);
   WinSetWindowPos(hwnd, 
      HWND_TOP,
      ulBorderCX * 2,
      ulBorderCY * 2,
      swpClient.cx - ulBorderCX * 4,
      ulCharHeight * 2,
      SWP_SIZE | SWP_MOVE | SWP_SHOW);


   swpField.x  = 0;
   swpField.y  = ulBackgroundBottom;
   swpField.cx = swpClient.cx;
   swpField.cy = ulBackgroundTop - ulBackgroundBottom;

   /*
      The MVMT ListBox 1
   */
   hwnd = WinWindowFromID(hwndClient, ID_MVMTLIST);
   if (WinIsWindowVisible(hwnd))
      {
      WinSetWindowPos(hwnd, HWND_TOP,
            swpField.x,
            swpField.y,
            swpField.cx / 4,
            swpField.cy,
            SWP_MOVE|SWP_SIZE|SWP_SHOW);
      swpField.x  += swpField.cx / 4;
      swpField.cx -= swpField.cx / 4;
      }

   /*
      The MVMT ListBox 2
   */
   hwnd = WinWindowFromID(hwndClient, ID_MVMTLIST2);
   if (WinIsWindowVisible(hwnd)) 
      {
      WinSetWindowPos(hwnd, HWND_TOP,
            swpField.x,
            swpField.y,
            swpField.cx / 3,
            swpField.cy,
            SWP_MOVE|SWP_SIZE|SWP_SHOW);
      swpField.x  += swpField.cx / 3;
      swpField.cx -= swpField.cx / 3;
      }

   /*
      The ASCII MLE
   */
   hwnd = WinWindowFromID(hwndClient, ID_ASCII);
   if (WinIsWindowVisible(hwnd))
      {
      WinSetWindowPos(hwnd, HWND_TOP,
            swpField.x,
            swpField.y,
            swpField.cx,
            swpField.cy,
            SWP_MOVE|SWP_SIZE|SWP_SHOW);
      }

   /*
      The BINARY MLE
   */
   hwnd = WinWindowFromID(hwndClient, ID_BINARY);
   if (WinIsWindowVisible(hwnd))
      {
      WinSetWindowPos(hwnd, HWND_TOP,
            swpField.x,
            swpField.y,
            swpField.cx,
            swpField.cy,
            SWP_MOVE|SWP_SIZE|SWP_SHOW);
      }

   /*
      The ICON FIELD
   */
   hwnd = WinWindowFromID(hwndClient, ID_ICON);
   if (WinIsWindowVisible(hwnd))
      {
      WinSetWindowPos(hwnd, HWND_TOP,
            swpField.x + (swpField.cx - WinQuerySysValue(HWND_DESKTOP, SV_CXICON)) / 2,
            swpField.y + (swpField.cy - WinQuerySysValue(HWND_DESKTOP, SV_CYICON)) / 2,
            WinQuerySysValue(HWND_DESKTOP, SV_CXICON) + 10,
            WinQuerySysValue(HWND_DESKTOP, SV_CYICON) + 10,
            SWP_MOVE|SWP_SIZE|SWP_SHOW);
      }

}

/*******************************************************************
*  SetStatusText
*******************************************************************/
VOID SetStatusText(HWND hwndClient, PSZ pszText)
{
HWND hwnd = WinWindowFromID(hwndClient, ID_STATUS);

   WinSetWindowText(hwnd, pszText);
}

/*************************************************************************
* Get the height of the text in a window
*************************************************************************/
ULONG GetTextHeight(HWND hwndClient)
{
HPS    hps;
PSZ    pszAllChars = "ABCDE";
POINTL aptl[TXTBOX_COUNT];
ULONG  ulCharHeight;

   /*
      Get the height of the current font
   */

   hps = WinGetPS(hwndClient);
   GpiQueryTextBox(hps,
      strlen(pszAllChars),
      pszAllChars,
      TXTBOX_COUNT,
      aptl);
   WinReleasePS(hps);
   ulCharHeight = aptl[TXTBOX_TOPLEFT].y - aptl[TXTBOX_BOTTOMLEFT].y;
   return ulCharHeight;

}



/*****************************************************************
* Change a directory
*****************************************************************/
BOOL ChangeDir(HWND hwnd, PSZ pszSelection)
{
   if (pszSelection[0] == '[')
      {
      _getdcwd((INT)(pszSelection[1] - '@'), Options.szCurDir,
         sizeof Options.szCurDir);
      return TRUE;
      }
   if (pszSelection[0] == '<' && pszSelection[1] == '.')
      {
      PBYTE p = strrchr(Options.szCurDir ,'\\');
      if (!p)
         return FALSE;
      *p = 0;
      return TRUE;
      }

   if (pszSelection[0] == '<')
      {
      PBYTE p;
      if (Options.szCurDir[strlen(Options.szCurDir) - 1] != '\\')
         strcat(Options.szCurDir, "\\");
      strcat(Options.szCurDir, pszSelection + 1);
      p = strchr(Options.szCurDir, '>');
      if (p)
         *p = 0;
      return TRUE;
      }

   FillEABox(hwnd);
   SetStatusText(hwnd, szMessage);
   return FALSE;

}

/*****************************************************************
* Fill The file list box
*****************************************************************/
VOID FillFileBox(HWND hwnd)
{
HWND           hwndList;
HDIR           FindHandle;
ULONG          ulAttr;
ULONG          ulFindCount;
FILEFINDBUF4   find;
BYTE           szSearch[256];
APIRET         rc;
BYTE           szFileName[128];
ULONG          ulCurDisk,
               ulDriveMap;
USHORT         usIndex;
BYTE           szWinTitle[260];
BOOL           fSelectSet = FALSE;


   strcpy(szWinTitle, TITLE);
   strcat(szWinTitle, " - ");
   strcat(szWinTitle, Options.szCurDir);
   WinSetWindowText(WinQueryWindow(hwnd, QW_PARENT), szWinTitle);


   hwndList = WinWindowFromID(hwnd, ID_FILELIST);

   WinEnableWindowUpdate(hwndList, FALSE);
   WinSendMsg(hwndList, LM_DELETEALL, 0L, 0L);

   /*
      Get all drive letters in
   */
   DosQCurDisk(&ulCurDisk, &ulDriveMap);
   for (usIndex = 0; usIndex < 26 ; usIndex++)
      {
      ULONG Mask = 0x0001 << usIndex;

      if (!(ulDriveMap & Mask))
         continue;
      sprintf(szFileName, "[%c:]", usIndex + 'A');
      WinSendMsg(hwndList,
         LM_INSERTITEM, MPFROMSHORT(LIT_SORTASCENDING), (MPARAM)szFileName);
      }

   /*
      Get all files
   */
   strcpy(szSearch, Options.szCurDir);
   if (szSearch[strlen(szSearch) - 1] != '\\')
      strcat(szSearch, "\\");
   strcat(szSearch, "*.*");

   FindHandle = HDIR_CREATE;
   ulAttr = FILE_DIRECTORY | FILE_ARCHIVED | FILE_READONLY | FILE_HIDDEN | FILE_SYSTEM;

   ulFindCount = 1;
   rc =  DosFindFirst(szSearch, &FindHandle, ulAttr, 
      &find, sizeof find, &ulFindCount,
      FIL_QUERYEASIZE);

   while (!rc)
      {
      if (strcmp(find.achName, "."))
         {
         USHORT usItem = LIT_ERROR;

         if (find.attrFile & FILE_DIRECTORY)
            {
            strcpy(szFileName, "<");
            strcat(szFileName+1, find.achName);
            strcat(szFileName, ">");
            usItem = (USHORT)WinSendMsg(hwndList,
               LM_INSERTITEM, MPFROMSHORT(LIT_SORTASCENDING), (MPARAM)szFileName);
            }
         else if (!Options.fShowEAFilesOnly || find.cbList > 4)
            {
            strcpy(szFileName, find.achName);
            usItem = (USHORT)WinSendMsg(hwndList,
               LM_INSERTITEM, MPFROMSHORT(LIT_SORTASCENDING), (MPARAM)szFileName);
            }
         if (usItem != LIT_ERROR && strlen(szStartupFile))
            {
            if (!stricmp(szStartupFile, find.achName))
               {
               WinSendMsg(hwndList,
                  LM_SELECTITEM, MPFROMSHORT(usItem), MPFROMSHORT(TRUE));
               memset(szStartupFile, 0, sizeof szStartupFile);
               fSelectSet = TRUE;
               }
            }
         }

      ulFindCount = 1;
      rc = DosFindNext(FindHandle, &find, sizeof find, &ulFindCount);
      }
   DosFindClose(FindHandle);


   if (!fSelectSet)
      {
      WinSendMsg(hwndList,
         LM_SETTOPINDEX, MPFROMSHORT(0), 0);
      WinSendMsg(hwndList,
         LM_SELECTITEM, MPFROMSHORT(0), MPFROMSHORT(TRUE));
      }

   WinEnableWindowUpdate(hwndList, TRUE);
   
   FillEABox(hwnd);
   SetStatusText(hwnd, szMessage);
}



/*****************************************************************
* Fill The file list box
*****************************************************************/
VOID FillEABox(HWND hwnd)
{
HWND   hwndList;
BYTE   szFileName[256];
PFEA2  pFea;
SHORT  sIndex;
BYTE   szSelText[256];
FILESTATUS3 fStat;
APIRET rc;
USHORT usEACount;

   if (Cur.pFeaData && Cur.pFeaData->fChanged)
      {
      if (WinMessageBox(HWND_DESKTOP,HWND_DESKTOP,
         "Do you want to save the changes?","",
         0, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON1 ) == MBID_YES)
         {
         WriteEA(Cur.szPathOrFile, Cur.pFeaData);
         }
      }


   memset(szMessage, 0, sizeof szMessage);
   hwndList = WinWindowFromID(hwnd, ID_EALIST);

   WinEnableWindowUpdate(hwndList, FALSE);

   for (sIndex = 0; ;sIndex++)
      {
      PFEADATA pFeaData = (PFEADATA)WinSendMsg(hwndList,
            LM_QUERYITEMHANDLE, (MPARAM)sIndex, 0L);
      if (!pFeaData)
         break;
      free(pFeaData);
      }


   WinSendMsg(hwndList, LM_DELETEALL, 0L, 0L);

   sIndex = WinQueryLboxSelectedItem(WinWindowFromID(hwnd, ID_FILELIST));
   if (sIndex == LIT_NONE)
      {
      WinEnableWindowUpdate(hwndList, TRUE);
      ShowSelectedEA(hwnd);
      return;
      }
   WinQueryLboxItemText(WinWindowFromID(hwnd, ID_FILELIST),
      sIndex,
      szSelText,
      sizeof szSelText);

   if (*szSelText == '[')
      {
      memset(szMessage, 0, sizeof szMessage);
      WinEnableWindowUpdate(hwndList, TRUE);
      ShowSelectedEA(hwnd);
      return;
      }

   if (szSelText[0] == '<' && szSelText[1] == '.')
      {
      WinEnableWindowUpdate(hwndList, TRUE);
      ShowSelectedEA(hwnd);
      return;
      }

   strcpy(szFileName, Options.szCurDir);
   if (szFileName[strlen(szFileName) - 1] != '\\')
      strcat(szFileName, "\\");
   if (szSelText[0] == '<')
      {
      PBYTE p;
      strcat(szFileName, szSelText+1);
      p = strchr(szFileName, '>');
      if (p)
         *p = 0;
      }
   else
      strcat(szFileName, szSelText);

   strcpy(Cur.szPathOrFile, szFileName);
   sprintf(szMessage, "%s contains no EAs", Cur.szPathOrFile);
   if (Cur.pFea2List)
      free(Cur.pFea2List);

   rc = DosQueryPathInfo(Cur.szPathOrFile,
      FIL_STANDARD,
      &fStat,
      sizeof fStat);
   if (rc)
      {
      sprintf(szMessage, "DosQueryPathInfo: %s", GetOS2Error(rc));
      WinEnableWindowUpdate(hwndList, TRUE);
      ShowSelectedEA(hwnd);
      return;
      }
   if (fStat.attrFile & FILE_DIRECTORY)
      Cur.pFea2List = GetDirEA(Cur.szPathOrFile);
   else
      Cur.pFea2List = GetFileEA(Cur.szPathOrFile);

   if (!Cur.pFea2List)
      {
      WinEnableWindowUpdate(hwndList, TRUE);
      ShowSelectedEA(hwnd);
      return;
      }

   pFea = Cur.pFea2List->list;
   usEACount = 0;
   for (;;)
      {
      SHORT sEntry;
      PFEADATA pFeaData = malloc(sizeof (FEADATA) + pFea->cbName + pFea->cbValue);
      if (!pFeaData)
         {
         MessageBox(TITLE, "Not enough memory!");
         DosExit(EXIT_PROCESS, 0);
         }
      memset(pFeaData, 0, sizeof (FEADATA) + pFea->cbName + pFea->cbValue);
      memcpy(&pFeaData->Fea, pFea, sizeof (FEA2) + pFea->cbName + pFea->cbValue);
      pFeaData->Fea.oNextEntryOffset = 0L;

      sEntry = (SHORT)WinSendMsg(hwndList,
         LM_INSERTITEM, MPFROMSHORT(LIT_SORTASCENDING), (MPARAM)pFea->szName);
      WinSendMsg(hwndList,
         LM_SETITEMHANDLE, MPFROMSHORT(sEntry), (MPARAM)pFeaData);
      usEACount++;
      if (!pFea->oNextEntryOffset)
         break;
      pFea = (PFEA2) ((PBYTE)pFea + pFea->oNextEntryOffset);
      } 

   WinSendMsg(hwndList,
      LM_SELECTITEM,
      MPFROMSHORT(0),
      MPFROMSHORT(TRUE));

   sprintf(szMessage, "%s contains %d EAs", Cur.szPathOrFile, usEACount);
   WinEnableWindowUpdate(hwndList, TRUE);
//   ShowSelectedEA(hwnd);

}


/**********************************************************************
* Check the EA's or a directory
**********************************************************************/
PFEA2LIST GetDirEA(PSZ pszName)
{
ULONG ulOrd;
ULONG ulDenaCnt;
APIRET rc;


   ulOrd = 1;
   ulDenaCnt = (ULONG)-1;

   memset(bDena, 0, sizeof bDena);
   rc = DosEnumAttribute(ENUMEA_REFTYPE_PATH,
      pszName,
      ulOrd,
      bDena,
      sizeof bDena,
      &ulDenaCnt,
      ENUMEA_LEVEL_NO_VALUE);

   if (rc)
      {
      sprintf(szMessage, "DosEnumAttribute: %s", GetOS2Error(rc));
      return NULL;
      }
   if (!ulDenaCnt)
      return NULL;

   return GetEAs(ulDenaCnt, (PDENA2)bDena, 0, pszName);
}

/**********************************************************************
* Check the EA's of a file
**********************************************************************/
PFEA2LIST GetFileEA(PSZ pszName)
{
ULONG ulOrd;
ULONG ulDenaCnt;
APIRET rc;
ULONG  ulActionTaken;
HFILE  hfFile;
PFEA2LIST pFea2List;

   rc = DosOpen(pszName,
      &hfFile,
      &ulActionTaken,
      0L, /* file size */
      0L, /* file attributes */
      OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
      OPEN_SHARE_DENYNONE | OPEN_ACCESS_READONLY,
      NULL);
   if (rc)
      {
      sprintf(szMessage, "DosOpen: %s", GetOS2Error(rc));
      return NULL;
      }

   /*
      Now query ea's
   */

   ulOrd = 1;
   ulDenaCnt = (ULONG)-1;

   memset(bDena, 0, sizeof bDena);
   rc = DosEnumAttribute(ENUMEA_REFTYPE_FHANDLE,
      &hfFile,
      ulOrd,
      bDena,
      sizeof bDena,
      &ulDenaCnt,
      ENUMEA_LEVEL_NO_VALUE);

   if (rc)
      {
      sprintf(szMessage, "DosEnumAttribute: %s", GetOS2Error(rc));
      DosClose(hfFile);
      return NULL;
      }
   if (!ulDenaCnt)
      {
      DosClose(hfFile);
      return NULL;
      }
   pFea2List = GetEAs(ulDenaCnt, (PDENA2)bDena, hfFile, pszName);
   DosClose(hfFile);
   return pFea2List;
}

/**********************************************************************
* Check EA's
**********************************************************************/
PFEA2LIST GetEAs(ULONG ulDenaCnt, PDENA2 pDenaBlock, HFILE hfFile, PSZ pszName)
{
ULONG  ulFEASize;
ULONG  ulGEASize;
ULONG  ulIndex;
EAOP2  EAop2;
PGEA2  pGea;
PDENA2 pDena;
APIRET rc;

   /*
      Calculate size of needed buffers
   */
   ulFEASize = sizeof (ULONG);
   ulGEASize = sizeof (ULONG);
   pDena = (PDENA2)pDenaBlock;
   for (ulIndex = 0; ulIndex < ulDenaCnt; ulIndex++)
      {
      ULONG ulFSize = sizeof (FEA2) + pDena->cbName + pDena->cbValue;
      ULONG ulGSize = sizeof (GEA2) + pDena->cbName;

      ulFSize += (4 - (ulFSize % 4));
      ulFEASize += ulFSize;

      ulGSize += (4 - (ulGSize % 4));
      ulGEASize += ulGSize;

      if (!pDena->oNextEntryOffset)
         break;
      pDena = (PDENA2) ((PBYTE)pDena + pDena->oNextEntryOffset);
      }

   /*
      Allocate needed buffers
   */
   EAop2.fpGEA2List = malloc(ulGEASize);
   if (!EAop2.fpGEA2List)
      {
      strcpy(szMessage, "Not enough memory!");
      return NULL;
      }
   memset(EAop2.fpGEA2List, 0, ulGEASize);

   EAop2.fpFEA2List = malloc(ulFEASize);
   if (!EAop2.fpFEA2List)
      {
      free(EAop2.fpGEA2List);
      strcpy(szMessage, "Not enough memory!");
      return NULL;
      }
   memset(EAop2.fpFEA2List, 0, ulFEASize);


   /*
      Build the PGEALIST
   */
   EAop2.fpGEA2List->cbList = ulGEASize;
   EAop2.fpFEA2List->cbList = ulFEASize;

   pDena = (PDENA2)pDenaBlock;
   pGea  = EAop2.fpGEA2List->list;
   for (ulIndex = 0; ulIndex < ulDenaCnt; ulIndex++)
      {
      USHORT usDiff;

      pGea->cbName = pDena->cbName;
      memcpy(pGea->szName, pDena->szName, pDena->cbName + 1);

      if (ulIndex < ulDenaCnt - 1)
         pGea->oNextEntryOffset = sizeof (GEA2) + pDena->cbName;
      else
         pGea->oNextEntryOffset = 0;

      usDiff = pGea->oNextEntryOffset % 4;
      if (usDiff)
         pGea->oNextEntryOffset += 4 - usDiff;

      pGea  = (PGEA2)  ((PBYTE)pGea + pGea->oNextEntryOffset);
      pDena = (PDENA2) ((PBYTE)pDena + pDena->oNextEntryOffset);
      }

   if (hfFile)
      {
      rc = DosQueryFileInfo(hfFile,
         FIL_QUERYEASFROMLIST,
         (PBYTE) &EAop2,
         sizeof EAop2);
      }
   else
      {
      rc = DosQueryPathInfo(pszName,
         FIL_QUERYEASFROMLIST,
         (PBYTE) &EAop2,
         sizeof EAop2);
      }

   if (rc)
      {
      MessageBox(TITLE, "DosQueryXXXXInfo on %s :\n %s",
         pszName,
         GetOS2Error(rc));
      free(EAop2.fpGEA2List);
      free(EAop2.fpFEA2List);
      return NULL;
      }

   free(EAop2.fpGEA2List);
   return EAop2.fpFEA2List;
}


/*********************************************************************
* Get an OS/2 errormessage out of the message file
*********************************************************************/
PSZ GetOS2Error(APIRET rc)
{
static BYTE szErrorBuf[MAX_MESSAGE];
APIRET rc2;
ULONG  ulReplySize;
PBYTE  p;

   if (rc)
      {
      memset(szErrorBuf, 0, sizeof szErrorBuf);
      rc2 = DosGetMessage(NULL, 0, szErrorBuf, sizeof(szErrorBuf),
                          (ULONG) rc, "OSO001.MSG", &ulReplySize);
      switch (rc2)
         {
         case NO_ERROR:
            while ((p = strstr(szErrorBuf, "\r\n")) != NULL)
               {
               *p++ = ' ';
               memmove(p, p+1, strlen(p+1) + 1);
               }
            break;
         case ERROR_FILE_NOT_FOUND :
            sprintf(szErrorBuf, "SYS%04u (Message file not found!)", rc);
            break;
         default:
            sprintf(szErrorBuf, "SYS%04u (Error %d while retrieving message text!)", rc, rc2);
            break;
         }
      }
   return szErrorBuf;
}

/*********************************************************************
*  Get A PM ErrorMsg
*********************************************************************/
PSZ GetPMError(VOID)
{
static BYTE szPMError[MAX_MESSAGE];
PERRINFO pErrInfo = WinGetErrorInfo(hab);
USHORT usSeverity;
CHAR   szSev[14],
       szErrNo[6],
       szHErrNo[6];

   if (pErrInfo)
      {
      PUSHORT pus = (PUSHORT)((PBYTE)pErrInfo + pErrInfo->offaoffszMsg);
      PSZ     psz = (PBYTE)pErrInfo + *pus;
      PSZ     p;

      strcpy(szPMError, psz);
      p = psz + strlen(psz) + 1;
      strcat(szPMError, "\n");
      strcat(szPMError, p);

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
      strcat(szPMError, "\n");
      strcat(szPMError, "Error No = ");
      strcat(szPMError, szErrNo);
      strcat(szPMError, "  (0x");
      strcat(szPMError, szHErrNo);
      strcat(szPMError, ")");
      strcat(szPMError, "\n");
      strcat(szPMError, "Severity = ");
      strcat(szPMError, szSev);
      WinFreeErrorInfo(pErrInfo);
      }
   else
      strcpy(szPMError, "No error");

   return szPMError;
}

/******************************************************************
* ShowSelectedEA
******************************************************************/
VOID ShowSelectedEA(HWND hwndClient)
{
SHORT  sIndex;
PBYTE  pValue;
PSZ    pszEAType;
BYTE   szEAType[100];
USHORT usEAType,
       usEASize;
BOOL   fProcess;

   WinShowWindow(WinWindowFromID(hwndClient, ID_MVMTLIST), FALSE);
   WinShowWindow(WinWindowFromID(hwndClient, ID_MVMTLIST2),FALSE);

   Cur.pFeaData = NULL;
   Cur.pbValue  = NULL;
   Cur.usValueSize = 0;

   fProcess = TRUE;
   sIndex = WinQueryLboxSelectedItem(WinWindowFromID(hwndClient, ID_EALIST));
   if (sIndex == LIT_NONE)
      fProcess = FALSE;
   else 
      {
      Cur.pFeaData = (PFEADATA)WinSendMsg(WinWindowFromID(hwndClient, ID_EALIST),
            LM_QUERYITEMHANDLE, (MPARAM)sIndex, 0L);
      if (!Cur.pFeaData)
         fProcess = FALSE;
      }

   if (!fProcess)
      {
      WinShowWindow(WinWindowFromID(hwndClient, ID_BINARY), FALSE);
      WinShowWindow(WinWindowFromID(hwndClient, ID_ASCII)  ,FALSE);
      WinShowWindow(WinWindowFromID(hwndClient, ID_ICON), FALSE);
      WinSetWindowText(WinWindowFromID(hwndClient, ID_EATYPE), "");
      return;
      }

   pValue = Cur.pFeaData->Fea.szName + Cur.pFeaData->Fea.cbName + 1;

   /*
      Save global values
   */
   Cur.pbValue     = pValue;
   Cur.usValueSize = Cur.pFeaData->Fea.cbValue;

   usEAType = *(PUSHORT)pValue;
   pValue += sizeof (USHORT);



   WinQueryWindowPos(hwndClient, &Cur.swpData);
   Cur.swpData.x  = 0;
   Cur.swpData.y  = ulBackgroundBottom;
   Cur.swpData.cy = ulBackgroundTop - ulBackgroundBottom;

   pszEAType = "";
   usEASize  = Cur.pFeaData->Fea.cbValue;
   switch (usEAType)
      {
      case EA_LPBINARY    :
         if (Options.fShowAllBinary)
            ShowBinary(hwndClient, &Cur.swpData, Cur.pFeaData->Fea.szName + Cur.pFeaData->Fea.cbName + 1, Cur.pFeaData->Fea.cbValue);
         else
            {
            pszEAType = "Length preceeding binary data";
            usEASize = *(PUSHORT)pValue;
            ShowBinary(hwndClient, &Cur.swpData, pValue + 2, usEASize);
            }
         break;

      case EA_LPASCII     :
         if (Options.fShowAllBinary)
            ShowBinary(hwndClient, &Cur.swpData,
               Cur.pFeaData->Fea.szName + Cur.pFeaData->Fea.cbName + 1, Cur.pFeaData->Fea.cbValue);
         else
            {
            pszEAType = "Length preceeding ASCII";
            usEASize = *(PUSHORT)pValue;
            ShowAscii(hwndClient, &Cur.swpData, pValue + 2, usEASize);
            }
         break;

      case EA_LPBITMAP    :
         if (Options.fShowAllBinary)
            ShowBinary(hwndClient, &Cur.swpData,
               Cur.pFeaData->Fea.szName + Cur.pFeaData->Fea.cbName + 1, Cur.pFeaData->Fea.cbValue);
         else
            {
            pszEAType = "Length preceeding bitmap";
            usEASize = *(PUSHORT)pValue;
            ShowBinary(hwndClient, &Cur.swpData, pValue + 2, usEASize);
            }
         break;

      case EA_LPMETAFILE  :
         if (Options.fShowAllBinary)
            ShowBinary(hwndClient, &Cur.swpData,
               Cur.pFeaData->Fea.szName + Cur.pFeaData->Fea.cbName + 1, Cur.pFeaData->Fea.cbValue);
         else
            {
            pszEAType = "Length preceeding metafile";
            usEASize = *(PUSHORT)pValue;
            ShowBinary(hwndClient, &Cur.swpData, pValue + 2, usEASize);
            }
         break;

      case EA_LPICON      :
         if (Options.fShowAllBinary)
            ShowBinary(hwndClient, &Cur.swpData,
               Cur.pFeaData->Fea.szName + Cur.pFeaData->Fea.cbName + 1, Cur.pFeaData->Fea.cbValue);
         else
            {
            pszEAType = "Length preceeding icon";
            usEASize = *(PUSHORT)pValue;
            ShowIcon(hwndClient, &Cur.swpData, pValue + 2, usEASize);
            }
         break;

      case EA_ASCIIZ      :
         if (Options.fShowAllBinary)
            ShowBinary(hwndClient, &Cur.swpData,
               Cur.pFeaData->Fea.szName + Cur.pFeaData->Fea.cbName + 1, Cur.pFeaData->Fea.cbValue);
         else
            {
            pszEAType = "ASCII-Zero data";
            usEASize = strlen(pValue)+1;
            ShowAscii(hwndClient, &Cur.swpData, pValue, usEASize - 1);
            }
         break;

      case EA_ASCIIZFN    :
         if (Options.fShowAllBinary)
            ShowBinary(hwndClient, &Cur.swpData,
               Cur.pFeaData->Fea.szName + Cur.pFeaData->Fea.cbName + 1, Cur.pFeaData->Fea.cbValue);
         else
            {
            pszEAType = "ASCII-Zero filename";
            usEASize = strlen(pValue)+1;
            ShowAscii(hwndClient, &Cur.swpData, pValue, usEASize - 1);
            }
         break;

      case EA_ASCIIZEA    :
         if (Options.fShowAllBinary)
            ShowBinary(hwndClient, &Cur.swpData,
               Cur.pFeaData->Fea.szName + Cur.pFeaData->Fea.cbName + 1, Cur.pFeaData->Fea.cbValue);
         else
            {
            pszEAType = "ASCII-Zero EA";
            usEASize = strlen(pValue)+1;
            ShowAscii(hwndClient, &Cur.swpData, pValue, usEASize - 1);
            }
         break;

      case EA_MVMT        :
         pszEAType = "Multiple-Value Multi-Type";
         if (Options.fShowAllBinary ||
            !ShowMVMT(hwndClient, &Cur.swpData, Cur.pFeaData->Fea.szName + Cur.pFeaData->Fea.cbName + 1, 0))
            {
            pszEAType = "";
            ShowBinary(hwndClient, &Cur.swpData,
               Cur.pFeaData->Fea.szName + Cur.pFeaData->Fea.cbName + 1, Cur.pFeaData->Fea.cbValue);
            }
         break;

      case EA_MVST        :
         if (Options.fShowAllBinary)
            ShowBinary(hwndClient, &Cur.swpData, pValue, Cur.pFeaData->Fea.cbValue);
         else
            {
            pszEAType = "Multiple-Value Single-Type";
            usEASize = Cur.pFeaData->Fea.cbValue - 2;
            ShowBinary(hwndClient, &Cur.swpData, pValue, usEASize);
            }
         break;

      case EA_ASN1        :
         if (Options.fShowAllBinary)
            ShowBinary(hwndClient, &Cur.swpData, pValue, Cur.pFeaData->Fea.cbValue);
         else
            {
            pszEAType = "ASN.1 Field";
            usEASize = Cur.pFeaData->Fea.cbValue - 2;
            ShowBinary(hwndClient, &Cur.swpData, pValue, usEASize);
            }
         break;

      default             :
         ShowBinary(hwndClient, &Cur.swpData,
            Cur.pFeaData->Fea.szName + Cur.pFeaData->Fea.cbName + 1, Cur.pFeaData->Fea.cbValue);
         break;
      }

   sprintf(szEAType, "%s %s: %ld bytes (%s)",
      Cur.pFeaData->Fea.szName,
      pszEAType,
      usEASize,
      (Cur.pFeaData->Fea.fEA & FEA_NEEDEA ? "Critical" : "Non-Critical")
      );
   WinSetWindowText(WinWindowFromID(hwndClient, ID_EATYPE), szEAType);
   return;
}

/*****************************************************************
* Show Binary data
*****************************************************************/
VOID ShowBinary(HWND hwndClient, PSWP pswpArea, PBYTE pValue, USHORT usValueSize)
{
HWND hwndMLE;
PSZ  pszText;
PBINDATA pBinData;

   hwndMLE = WinWindowFromID(hwndClient, ID_BINARY);
   pBinData = WinQueryWindowPtr(hwndMLE, 0);
   if (!pBinData)
      {
      MessageBox(TITLE, "WinQueryWindowPtr\n%s", GetPMError());
      exit(1);
      }


   memcpy(pBinData->bBinBuffer, pValue, usValueSize);
   pBinData->ulBinSize = usValueSize;
   pBinData->ulCurPos = 0;
   pBinData->fRightSide = FALSE;

   pszText = FormatBinData(pValue, usValueSize);
   WinSetWindowText(hwndMLE, pszText);
   WinSendMsg(hwndMLE, MLM_SETCHANGED, MPFROMSHORT(FALSE), 0);
   free(pszText);

   WinSendMsg(hwndMLE, MLM_SETSEL, (MPARAM)0, (MPARAM)0);

   WinShowWindow(WinWindowFromID(hwndClient, ID_ASCII),    FALSE);
   WinShowWindow(WinWindowFromID(hwndClient, ID_ICON) ,    FALSE);

   WinSetWindowPos(hwndMLE, HWND_TOP,
         pswpArea->x,
         pswpArea->y,
         pswpArea->cx,
         pswpArea->cy,
         SWP_MOVE|SWP_SIZE|SWP_SHOW);

   return;
}

/*****************************************************************
* Format the binary data
*****************************************************************/
PSZ FormatBinData(PBYTE pValue, USHORT usValueSize)
{
PBYTE       pValueEnd;
UINT        uiBuffSize;
USHORT      usIndex;
PBYTE       pszText,
            pszTextEnd,
            p;

   pValueEnd  = pValue + usValueSize;

   uiBuffSize = (UINT)usValueSize * 6;
   if (uiBuffSize < 500)
      uiBuffSize = 500;
   pszText = malloc(uiBuffSize);
   if (!pszText)
      {
      MessageBox(TITLE, "Not enough memory for this EA!");
      return NULL;
      }
   memset(pszText, 0, uiBuffSize);
   pszTextEnd = pszText + uiBuffSize;

   p = pszText;
   while (pValue < pValueEnd)
      {
      if (p > pszTextEnd)
         {
         MessageBox(TITLE, "OVERFLOW!");
         break;
         }
      for (usIndex = 0; usIndex < CHARS_ON_ROW; usIndex++)
         {
         if (pValue + usIndex < pValueEnd)
            p += sprintf(p, "%2.2X ", (USHORT)pValue[usIndex]);
         else
            p += sprintf(p, "   ", (USHORT)pValue[usIndex]);
         }
      *p++ = ' ';

      for (usIndex = 0; usIndex < CHARS_ON_ROW; usIndex++)
         {
         if (pValue + usIndex < pValueEnd)
            {
            if (pValue[usIndex] >= 0x20)
               sprintf(p, "%c", pValue[usIndex]);
            else
               *p = '.';
            }
         else
            *p = ' ';
         p++;
         }
      *p++ = '\n';
      pValue += CHARS_ON_ROW;
      }
   return pszText;
}

/***********************************************************************
* Show the icon
***********************************************************************/
VOID ShowIcon(HWND hwndClient, PSWP pswpArea, PBYTE pValue, USHORT usValueSize)
{
static   HPOINTER hptr;
HWND hwndIcon = WinWindowFromID(hwndClient, ID_ICON);
WNDPARAMS WndParms;
BTNCDATA  ButCtl;
USHORT usIconCX;

   if (hptr)
      WinDestroyPointer(hptr);
   hptr = NULL;

   usIconCX = WinQuerySysValue(HWND_DESKTOP, SV_CXICON);

   hptr = Buffer2Icon(HWND_DESKTOP, pValue, usIconCX);
   if (!hptr)
      return; 


   memset(&ButCtl, 0, sizeof ButCtl);
   ButCtl.cb            = sizeof ButCtl;
   ButCtl.fsCheckState  = 0;
   ButCtl.fsHiliteState = 0;
   ButCtl.hImage        = hptr;

   memset(&WndParms, 0, sizeof WndParms);
   WndParms.fsStatus  = WPM_CTLDATA;
   WndParms.cbCtlData = sizeof ButCtl;
   WndParms.pCtlData  = (PVOID)&ButCtl;

   WinSendMsg(hwndIcon, WM_SETWINDOWPARAMS, &WndParms, 0);


#if 0
   WinSendMsg(hwndIcon, SM_SETHANDLE, (MPARAM)hptr, 0L);
#endif
   
   WinShowWindow(WinWindowFromID(hwndClient, ID_ASCII),    FALSE);
   WinShowWindow(WinWindowFromID(hwndClient, ID_BINARY),   FALSE);
   WinSetWindowPos(hwndIcon, HWND_TOP,
         pswpArea->x + (pswpArea->cx - WinQuerySysValue(HWND_DESKTOP, SV_CXICON)) / 2,
         pswpArea->y + (pswpArea->cy - WinQuerySysValue(HWND_DESKTOP, SV_CYICON)) / 2,
         WinQuerySysValue(HWND_DESKTOP, SV_CXICON) + 10,
         WinQuerySysValue(HWND_DESKTOP, SV_CYICON) + 10,
         SWP_MOVE|SWP_SIZE|SWP_SHOW);
}


/***********************************************************************
* ShowAscii
***********************************************************************/
VOID ShowAscii(HWND hwndClient, PSWP pswpArea, PBYTE pValue, USHORT usValueSize)
{
HWND hwndMLE = WinWindowFromID(hwndClient, ID_ASCII);
PSZ    pszText;
PBYTE  p;

   pszText = malloc(usValueSize + 1);
   memset(pszText, 0, usValueSize + 1);
   memcpy(pszText, pValue, usValueSize);
   for (p = pszText; p < pszText + usValueSize; p++)
      {
      if (!*p)
         *p = 'ú';
      }
   WinSetWindowText(hwndMLE, pszText);
   free(pszText);
   WinSendMsg(hwndMLE, MLM_SETCHANGED, MPFROMSHORT(FALSE), 0);

   WinShowWindow(WinWindowFromID(hwndClient, ID_BINARY),   FALSE);
   WinShowWindow(WinWindowFromID(hwndClient, ID_ICON) ,    FALSE);
   WinSetWindowPos(hwndMLE, HWND_TOP,
         pswpArea->x,
         pswpArea->y,
         pswpArea->cx,
         pswpArea->cy,
         SWP_MOVE|SWP_SIZE|SWP_SHOW);

}

/***********************************************************************
* Show a multi value multi type EA
***********************************************************************/
BOOL ShowMVMT(HWND hwndClient, PSWP pswpArea, PBYTE pValue, USHORT usLevel)
{
HWND hwndListBox = WinWindowFromID(hwndClient, ID_MVMTLIST+usLevel);
USHORT usEAType;
USHORT usCodePage;
USHORT usEACount;
USHORT usIndex;
PSZ    pszEAType;
SHORT  sItem,
       sSelected;
USHORT usEASize;


   sSelected = WinQueryLboxSelectedItem(hwndListBox);
   if (sSelected == LIT_NONE)
      sSelected = 0;

   WinEnableWindowUpdate(hwndListBox, FALSE);
   WinSendMsg(hwndListBox, LM_DELETEALL, 0L, 0L);

   usEAType   = *(PUSHORT)pValue;
   usCodePage = *(PUSHORT)(pValue+2);
   usEACount  = *(PUSHORT)(pValue+4);
   if (usEACount > 0x8000)
      {
      usEACount  = *(PUSHORT)(pValue+2);
      usCodePage = 0;
      pValue += 2 * sizeof (USHORT);
      }
   else
      pValue += 3 * sizeof (USHORT);

   if (sSelected >= (SHORT)usEACount)
      sSelected = 0;

   for (usIndex = 0; usIndex < usEACount; usIndex++)
      {
      switch (*(PUSHORT)pValue)
         {
         case EA_LPBINARY    :
            pszEAType = "LP Binary";
            break;
         case EA_LPASCII     :
            pszEAType = "LP Ascii";
            break;
         case EA_LPBITMAP    :
            pszEAType = "LP Bitmap";
            break;
         case EA_LPMETAFILE  :
            pszEAType = "LP Metafile";
            break;
         case EA_LPICON      :
            pszEAType = "LP Icon";
            break;
         case EA_ASCIIZ      :
            pszEAType = "Ascii Zero";
            break;
         case EA_ASCIIZFN    :
            pszEAType = "Ascii Zero filename";
            break;
         case EA_ASCIIZEA    :
            pszEAType = "Ascii Zero EA";
            break;
         case EA_ASN1        :
            pszEAType = "ASN.1";
            break;
         case EA_MVMT       :
            pszEAType = "Mvalue, Mtype";
            break;
         default             :
            return FALSE;
         }

      sItem = (SHORT)WinSendMsg(hwndListBox,
         LM_INSERTITEM, MPFROMSHORT(LIT_END), (MPARAM)pszEAType);
      WinSendMsg(hwndListBox,
         LM_SETITEMHANDLE, MPFROMSHORT(sItem), (MPARAM)pValue);

      usEASize = GetEASize(pValue);
      if (usEASize == 0xFFFF)
         return FALSE;
      pValue += usEASize;
      }

   WinEnableWindowUpdate(hwndListBox, TRUE);

   WinSetWindowPos(hwndListBox, HWND_TOP,
         pswpArea->x,
         pswpArea->y,
         (!usLevel ? pswpArea->cx / 4 : pswpArea->cx / 3),
         pswpArea->cy,
         SWP_MOVE|SWP_SIZE|SWP_SHOW);

   WinSendMsg(hwndListBox,
      LM_SELECTITEM, MPFROMSHORT(sSelected), MPFROMSHORT(TRUE));

   usCodePage = usCodePage;
   usEAType   = usEAType;

   return TRUE;
}

/***********************************************************************
* Get multi value multitype size
***********************************************************************/
USHORT GetMVMTSize(PBYTE pValue)
{
USHORT usEAType,
       usCodePage,
       usEACount;
PBYTE  pSave = pValue;
USHORT usIndex;

   usEAType   = *(PUSHORT)pValue;
   usCodePage = *(PUSHORT)(pValue+2);
   usEACount  = *(PUSHORT)(pValue+4);
   if (usEACount > 0x8000)
      {
      usEACount  = *(PUSHORT)(pValue+2);
      usCodePage = 0;
      pValue += 2 * sizeof (USHORT);
      }
   else
      pValue += 3 * sizeof (USHORT);

   for (usIndex = 0; usIndex < usEACount; usIndex++)
      {
      USHORT usEASize = GetEASize(pValue);
      if (usEASize == 0xFFFF)
         return 0xFFFF;
      pValue += usEASize;
      }

   usCodePage = usCodePage;
   usEAType   = usEAType;
   return pValue - pSave;
}


/***********************************************************************
* Get The EA's Size
***********************************************************************/
USHORT GetEASize(PBYTE pValue)
{
USHORT usEASize;

   switch (*(PUSHORT)pValue)
      {
      case EA_LPBINARY    :
         usEASize = *(PUSHORT)(pValue + 2) + 4;
         break;
      case EA_LPASCII     :
         usEASize = *(PUSHORT)(pValue + 2) + 4;
         break;
      case EA_LPBITMAP    :
         usEASize = *(PUSHORT)(pValue + 2) + 4;
         break;
      case EA_LPMETAFILE  :
         usEASize = *(PUSHORT)(pValue + 2) + 4;
         break;
      case EA_LPICON      :
         usEASize = *(PUSHORT)(pValue + 2) + 4;
         break;
      case EA_ASCIIZ      :
         usEASize = strlen(pValue + 2) + 3;
         break;
      case EA_ASCIIZFN    :
         usEASize = strlen(pValue + 2) + 3;
         break;
      case EA_ASCIIZEA    :
         usEASize = strlen(pValue + 2) + 3;
         break;
      case EA_ASN1        :
         usEASize = *(PUSHORT)(pValue + 2) + 4;
         break;
      case EA_MVMT        :
         usEASize = GetMVMTSize(pValue);
         break;
      default             :
         MessageBox(TITLE, "GetEASize: Unknown EA type %X found",
            *(PUSHORT)pValue);
         usEASize = 0xFFFF;
         break;
      }
   return usEASize;
}
/***********************************************************************
* Show MVMT EA
***********************************************************************/
VOID ShowSelectedMVMT(HWND hwndClient, USHORT usLevel)
{
HWND hwndListBox = WinWindowFromID(hwndClient, ID_MVMTLIST + usLevel);
SHORT sIndex;
PBYTE pValue;
USHORT usEASize;


   WinQueryWindowPos(hwndClient, &Cur.swpData);
   Cur.swpData.x   = (Cur.swpData.cx / 4) * (usLevel + 1);
   Cur.swpData.cx -= Cur.swpData.x;
   Cur.swpData.y   = ulBackgroundBottom;
   Cur.swpData.cy  = ulBackgroundTop - ulBackgroundBottom;

   if (!usLevel)
      WinShowWindow(WinWindowFromID(hwndClient, ID_MVMTLIST2),FALSE);

   sIndex = WinQueryLboxSelectedItem(hwndListBox);
   if (sIndex == LIT_NONE)
      return;
   else 
      {
      pValue = (PBYTE)WinSendMsg(hwndListBox,
            LM_QUERYITEMHANDLE, (MPARAM)sIndex, 0L);
      if (!pValue)
         return;
      }

   usEASize = GetEASize(pValue);
   if (usEASize == 0xFFFF)
      return;

   /*
      Save global values
   */
   Cur.pbValue     = pValue;
   Cur.usValueSize = usEASize;

   switch (*(PUSHORT)pValue)
      {
      case EA_LPBINARY    :
      case EA_LPBITMAP    :
      case EA_ASN1        :
      case EA_LPMETAFILE  :
         ShowBinary(hwndClient, &Cur.swpData, pValue, usEASize);
         break;
      case EA_LPICON      :
         Cur.usValueSize = usEASize;
         ShowIcon(hwndClient, &Cur.swpData, pValue + 4, *(PUSHORT)(pValue + 2));
         break;
      case EA_LPASCII     :
         ShowAscii(hwndClient, &Cur.swpData, pValue + 4, *(PUSHORT)(pValue + 2));
         break;
      case EA_ASCIIZ      :
      case EA_ASCIIZFN    :
      case EA_ASCIIZEA    :
         ShowAscii(hwndClient, &Cur.swpData, pValue + 4, strlen(pValue + 4));
         break;
      case EA_MVMT        :
         ShowMVMT(hwndClient, &Cur.swpData, pValue, 1);
         break;
      default             :
         ShowBinary(hwndClient, &Cur.swpData, pValue, usEASize);
         break;
      }
}


/*************************************************************************
* Winproc procedure for the BINARY MLE
*************************************************************************/
MRESULT EXPENTRY MyMLEProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
MRESULT mResult;

   switch (msg)
      {
      case WM_CHAR:
         if ((*OldMLEProc)(hwnd, MLM_QUERYREADONLY, 0, 0))
            break;

         if (ProcessBinChar(hwnd, mp1, mp2))
            {
            SetMLECursorPos(hwnd);
            return (MRESULT)TRUE;
            }
         break;

      case WM_BUTTON1DBLCLK:
      case WM_BUTTON1DOWN:
      case WM_BUTTON1UP:
      case MLM_SETSEL:
      case MLM_SETREADONLY:
         mResult = (*OldMLEProc)(hwnd, msg, mp1, mp2);
         GetMLECursorPos(hwnd);
         SetMLECursorPos(hwnd);
         return mResult;

      default :
         mResult = (*OldMLEProc)(hwnd, msg, mp1, mp2);
         return mResult;
      }
   mResult = (*OldMLEProc)(hwnd, msg, mp1, mp2);
   return mResult;
}


/*************************************************************************
* Process a character (WM_CHAR message)
************************************************************************/
BOOL ProcessBinChar(HWND hwnd, MPARAM mp1, MPARAM mp2)
{
static BYTE szHex[]="0123456789ABCDEF";

USHORT usKeyFlags  = (USHORT) SHORT1FROMMP(mp1);    /* key flags    */
BYTE   bRepeat     = (BYTE) CHAR3FROMMP(mp1);       /* repeat count */
BYTE   bScanCode   = (BYTE) CHAR4FROMMP(mp1);       /* scan code    */
USHORT usChr       = (USHORT) SHORT1FROMMP(mp2);    /* character    */
USHORT usVKey      = (USHORT) SHORT2FROMMP(mp2);    /* virtual key  */
PBINDATA pBinData = WinQueryWindowPtr(hwnd, 0);
PSZ      pszText;


   if (usKeyFlags & KC_KEYUP)
      return FALSE;

   if (usKeyFlags & KC_VIRTUALKEY)
      {
      switch (usVKey)
         {
         case VK_TAB :
            if (pBinData->fRightSide)
               return FALSE;
            pBinData->fRightSide = TRUE;
            return TRUE;

         case VK_BACKTAB:
            if (!pBinData->fRightSide)
               return FALSE;
            pBinData->fRightSide = FALSE;
            return TRUE;

         case VK_HOME :
            pBinData->ulCurPos     = 0;
            pBinData->fRightSide   = FALSE;
            pBinData->fRightNibble = FALSE;
            return TRUE;

         case VK_END:
            pBinData->ulCurPos   = pBinData->ulBinSize;
            pBinData->fRightSide = TRUE;
            return TRUE;

         case VK_UP:
            if (pBinData->ulCurPos >= CHARS_ON_ROW)
               pBinData->ulCurPos -= CHARS_ON_ROW;
            return TRUE;

         case VK_DOWN:
            if (pBinData->ulCurPos + CHARS_ON_ROW <= pBinData->ulBinSize)
               pBinData->ulCurPos += CHARS_ON_ROW;
            return TRUE;

         case VK_RIGHT:
            if (pBinData->ulCurPos < pBinData->ulBinSize)
               {
               if (pBinData->fRightSide)
                  pBinData->ulCurPos++;
               else if (pBinData->fRightNibble)
                  {
                  pBinData->ulCurPos++;
                  pBinData->fRightNibble = FALSE;
                  }
               else
                  pBinData->fRightNibble = TRUE;
               }
            return TRUE;

         case VK_LEFT :
            if (pBinData->ulCurPos > 0)
               {
               if (pBinData->fRightSide)
                  pBinData->ulCurPos--;
               else if (pBinData->fRightNibble)
                  pBinData->fRightNibble = FALSE;
               else
                  {
                  pBinData->ulCurPos--;
                  pBinData->fRightNibble = TRUE;
                  }
               }
            else
               pBinData->fRightNibble = FALSE;
            return TRUE;

         case VK_INSERT:
            if (pBinData->ulBinSize < sizeof pBinData->bBinBuffer)
               {
               memmove(pBinData->bBinBuffer + pBinData->ulCurPos + 1,
                       pBinData->bBinBuffer + pBinData->ulCurPos,
                       pBinData->ulBinSize - pBinData->ulCurPos);
               pBinData->ulBinSize++;
               pBinData->bBinBuffer[pBinData->ulCurPos] = 0;
               pszText = FormatBinData(pBinData->bBinBuffer, pBinData->ulBinSize);
               WinSetWindowText(hwnd, pszText);
               free(pszText);
               }
            return TRUE;

         case VK_DELETE:
            if (pBinData->ulBinSize)
               {
               memmove(pBinData->bBinBuffer + pBinData->ulCurPos,
                       pBinData->bBinBuffer + pBinData->ulCurPos + 1,
                       pBinData->ulBinSize - (pBinData->ulCurPos + 1));
               pBinData->ulBinSize--;
               pBinData->bBinBuffer[pBinData->ulBinSize] = 0;
               }
            pszText = FormatBinData(pBinData->bBinBuffer, pBinData->ulBinSize);
            WinSetWindowText(hwnd, pszText);
            free(pszText);
            return TRUE;

         case VK_SPACE:
            usKeyFlags |= KC_CHAR;
            usChr = 0x0020;
            break;

         case VK_PAGEDOWN :
         case VK_PAGEUP   :
            (*OldMLEProc)(hwnd, WM_CHAR, mp1, mp2);
            GetMLECursorPos(hwnd);
            SetMLECursorPos(hwnd);
            return TRUE;

         case VK_BACKSPACE:
            return TRUE;

         default :
            return FALSE;
         }
      }

   if (!(usKeyFlags & KC_CHAR))
      return FALSE;

   if (pBinData->ulCurPos >= pBinData->ulBinSize)
      {
      DosBeep(1000, 50);
      return TRUE;
      }

   if (pBinData->fRightSide)
      {
      pBinData->bBinBuffer[pBinData->ulCurPos] = (BYTE)usChr;
      ShowCharacter(hwnd);
      pBinData->ulCurPos++;
      }
   else
      {
      PBYTE p;
      USHORT usHexChr;

      usHexChr = toupper(usChr);

      p = strchr(szHex, usHexChr);
      if (!p)
         {
         DosBeep(1000, 50);
         return TRUE;
         }

      usHexChr = p - szHex;

      if (pBinData->fRightNibble)
         {
         pBinData->bBinBuffer[pBinData->ulCurPos] &= 0xF0;
         pBinData->bBinBuffer[pBinData->ulCurPos] |= (BYTE)usHexChr;
         ShowCharacter(hwnd);
         pBinData->ulCurPos++;
         pBinData->fRightNibble = FALSE;
         }
      else
         {
         pBinData->bBinBuffer[pBinData->ulCurPos] &= 0x0F;
         pBinData->bBinBuffer[pBinData->ulCurPos] |= (BYTE)(usHexChr << 4);
         ShowCharacter(hwnd);
         pBinData->fRightNibble = TRUE;
         }
      }

//   pszText = FormatBinData(pBinData->bBinBuffer, pBinData->ulBinSize);
//   WinSetWindowText(hwnd, pszText);
//   free(pszText);

   return TRUE;
}

/*******************************************************************
* Change a single character inside the Binary MLE
*******************************************************************/
VOID ShowCharacter(HWND hwnd)
{
PBINDATA pBinData = WinQueryWindowPtr(hwnd, 0);
IPT      iptAnchor,
         iptCursor;
USHORT   usRow;
USHORT   usColumn;
BYTE     szText[3];

  (*OldMLEProc)(hwnd, MLM_DISABLEREFRESH, 0, 0);

   usRow    = pBinData->ulCurPos / CHARS_ON_ROW;
   usColumn = pBinData->ulCurPos % CHARS_ON_ROW;

   /*
      Left Side
   */

   iptCursor = usRow * ((CHARS_ON_ROW * 4) + 2) + usColumn * 3;
   iptAnchor = iptCursor + 2;
   sprintf(szText, "%2.2X", (USHORT)pBinData->bBinBuffer[pBinData->ulCurPos]);
   (*OldMLEProc)(hwnd, MLM_SETSEL, (MPARAM)iptAnchor, (MPARAM)iptCursor);
   (*OldMLEProc)(hwnd, MLM_INSERT, (MPARAM)&szText, 0);

   /*
      Right side
   */
   iptCursor = usRow * ((CHARS_ON_ROW * 4) + 2) +
               (usColumn + (CHARS_ON_ROW * 3 + 1));
   iptAnchor = iptCursor + 1;
   if (pBinData->bBinBuffer[pBinData->ulCurPos] >= 0x20)
      szText[0] = pBinData->bBinBuffer[pBinData->ulCurPos];
   else
      szText[0] = '.';
   szText[1] = 0;
   (*OldMLEProc)(hwnd, MLM_SETSEL, (MPARAM)iptAnchor, (MPARAM)iptCursor);
   (*OldMLEProc)(hwnd, MLM_INSERT, (MPARAM)&szText, 0);

   SetMLECursorPos(hwnd);

   (*OldMLEProc)(hwnd, MLM_ENABLEREFRESH, 0, 0);   

}

/*******************************************************************
* Get The position
*******************************************************************/
VOID GetMLECursorPos(HWND hwnd)
{
USHORT   usRow;
USHORT   usColumn;
IPT      iptCursor;
PBINDATA pBinData = WinQueryWindowPtr(hwnd, 0);

   iptCursor = (IPT)(*OldMLEProc)(hwnd, MLM_QUERYSEL,
      MPFROMSHORT(MLFQS_CURSORSEL), 0);

   usRow    = (USHORT)(iptCursor / ((CHARS_ON_ROW * 4) + 2));
   usColumn = (USHORT)(iptCursor % ((CHARS_ON_ROW * 4) + 2));

   if (usColumn > (CHARS_ON_ROW * 3 + 1))
      {
      usColumn -= (CHARS_ON_ROW * 3 + 1);
      pBinData->ulCurPos = (usRow * CHARS_ON_ROW) + usColumn;
      pBinData->fRightSide = TRUE;
      }
   else
      {
      pBinData->ulCurPos = (usRow * CHARS_ON_ROW) + usColumn / 3;
      pBinData->fRightSide = FALSE;
      if ((usColumn % 3) == 1)
         pBinData->fRightNibble = TRUE;
      else
         pBinData->fRightNibble = FALSE;
      }
}

/*************************************************************************
* Set the cursor position
*************************************************************************/
VOID SetMLECursorPos(HWND hwnd)
{
USHORT   usRow;
USHORT   usColumn;
IPT      iptCursor,
         iptAnchor;
PBINDATA pBinData = WinQueryWindowPtr(hwnd, 0);

   usRow    = pBinData->ulCurPos / CHARS_ON_ROW;
   usColumn = pBinData->ulCurPos % CHARS_ON_ROW;

   if (pBinData->fRightSide)
      usColumn += (CHARS_ON_ROW * 3 + 1);
   else
      {
      usColumn *= 3;
      if (pBinData->fRightNibble)
         usColumn++;
      }

   iptCursor = usRow * ((CHARS_ON_ROW * 4) + 2) + usColumn;
   iptAnchor = iptCursor+1;
   (*OldMLEProc)(hwnd, MLM_SETSEL, (MPARAM)iptAnchor, (MPARAM)iptCursor);
}


/*************************************************************************
* Delete an EA
*************************************************************************/
BOOL DeleteEA(HWND hwndClient)
{
FILESTATUS3 fStat;
APIRET      rc;
SHORT       sIndex;
PFEA2LIST   pFeaList;
EAOP2       eaop2;
PSZ        *ppszSpecial;

   if (!Cur.pFeaData)
      return FALSE;

   sprintf(szMessage, "Delete \'%s\' Extended Attribute?",  Cur.pFeaData->Fea.szName);
   if (WinMessageBox(HWND_DESKTOP,HWND_DESKTOP, szMessage,"",
      0, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON1 ) != MBID_YES)
      return FALSE;

   for (ppszSpecial = &rgpszSpecialEA[0] ; *ppszSpecial; ppszSpecial++)
      {
      if (!strcmp(Cur.pFeaData->Fea.szName, *ppszSpecial))
         break;
      }

   if (*ppszSpecial)
      {
      strcpy(szMessage, "Deleting this EA can lead to misbehaviour or\n");
      strcat(szMessage, "even complete failure of your WorkPlace Shell!\n\n");
      sprintf(szMessage+strlen(szMessage),
         "Delete \'%s\' Extended Attribute anyway?",  Cur.pFeaData->Fea.szName);
      if (WinMessageBox(HWND_DESKTOP,HWND_DESKTOP, szMessage,"",
         0, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON1 ) != MBID_YES)
         return FALSE;
      }

   rc = DosQueryPathInfo(Cur.szPathOrFile,
      FIL_STANDARD,
      &fStat,
      sizeof fStat);
   if (rc)
      {
      MessageBox(TITLE, "DosQueryPathInfo: %s", GetOS2Error(rc));
      return FALSE;
      }

   pFeaList = (PFEA2LIST)malloc(200);
   if (!pFeaList)
      {
      MessageBox(TITLE, "Not enough memory!");
      return FALSE;
      }


   memset(pFeaList, 0, 200);
   pFeaList->cbList = sizeof (FEA2LIST) + strlen(Cur.pFeaData->Fea.szName);
   strcpy(pFeaList->list->szName, Cur.pFeaData->Fea.szName);
   pFeaList->list->cbName = strlen(Cur.pFeaData->Fea.szName);


   memset(&eaop2, 0, sizeof eaop2);
   eaop2.fpFEA2List = pFeaList;

   rc = DosSetPathInfo(Cur.szPathOrFile,
      FIL_QUERYEASIZE,
      (PBYTE) &eaop2,
      sizeof eaop2,
      DSPI_WRTTHRU);
   free(pFeaList);
   if (rc)
      {
      MessageBox(TITLE, "DosSetPathInfo: %s", GetOS2Error(rc));
      return FALSE;
      }
   
   free(Cur.pFeaData);
   Cur.pFeaData = NULL;

   sIndex = WinQueryLboxSelectedItem(WinWindowFromID(hwndClient, ID_EALIST));

   WinSendMsg(WinWindowFromID(hwndClient, ID_EALIST),
         LM_DELETEITEM, MPFROMSHORT(sIndex), 0L);
   return TRUE;
}

/*************************************************************************
* EditIconEA
*************************************************************************/
VOID EditIconEA(PVOID pvClient)
{
HWND  hwndClient = (HWND)pvClient;
HWND  hwndFrame;
PSZ   pszTempFile;
PBYTE pValue;
INT   iHandle;
HMQ   hmqLocal;
HAB   habLocal;
USHORT usIconSize;
FILESTATUS3 fStatOld,
            fStatNew;
APIRET rc;


   habLocal = WinInitialize((USHORT)0);
   hmqLocal = WinCreateMsgQueue(habLocal, 40);
   WinCancelShutdown(hmqLocal, TRUE);

   if (*(PUSHORT)Cur.pbValue != EA_LPICON)
      {
      MessageBox(TITLE, "ERROR: not EA_LPICON!");
      WinDestroyMsgQueue( hmqLocal );
      WinTerminate(habLocal);
      return;
      }


   pValue     = Cur.pbValue + sizeof (USHORT);
   usIconSize = *(PUSHORT)pValue;
   pValue     += sizeof (USHORT);

   pszTempFile = tempnam("\\", "EAb");
   if (!pszTempFile)
      {
      MessageBox(TITLE, "Error on tempnam!");
      WinDestroyMsgQueue( hmqLocal );
      WinTerminate(habLocal);
      return;
      }

   iHandle = open(pszTempFile, O_BINARY|O_WRONLY|O_CREAT, S_IWRITE);
   if (iHandle == -1)
      {
      MessageBox(TITLE, "Cannot create temp file!");
      free(pszTempFile);
      WinDestroyMsgQueue( hmqLocal );
      WinTerminate(habLocal);
      return;
      }

   write(iHandle, pValue, usIconSize);
   close(iHandle);

   hwndFrame = WinQueryWindow(hwndClient, QW_PARENT);
   WinShowWindow(hwndFrame, FALSE);
   WinEnableWindow(hwndClient, FALSE);

   DosQueryPathInfo(pszTempFile,
       FIL_STANDARD,
       &fStatOld,
       sizeof fStatOld);

   spawnlp(P_WAIT, "iconedit.exe", "iconedit.exe", pszTempFile, NULL);

   DosQueryPathInfo(pszTempFile,
       FIL_STANDARD,
       &fStatNew,
       sizeof fStatNew);

   fStatOld.fdateLastAccess = fStatNew.fdateLastAccess;
   fStatOld.ftimeLastAccess = fStatNew.ftimeLastAccess;
   if (memcmp(&fStatNew, &fStatOld, sizeof (FILESTATUS3)))
      {
      iHandle = open(pszTempFile, O_BINARY|O_RDONLY);
      if (iHandle != -1)
         {
         ULONG ulFileSize = filelength(iHandle);
         PBYTE pbData;
         pbData = malloc(ulFileSize);
         if (!pbData)
            {
            MessageBox(TITLE, "Not enough memory!");
            DosExit(EXIT_PROCESS, 0);
            }
         read(iHandle, pbData, ulFileSize);
         close(iHandle);

         UpdateFeaData(hwndClient, pbData, (USHORT)ulFileSize);

         }
      }
   ShowSelectedEA(hwndClient);

   unlink(pszTempFile);
   free(pszTempFile);

   WinEnableWindow(hwndClient, TRUE);
   WinShowWindow(hwndFrame, TRUE);

   WinDestroyMsgQueue( hmqLocal );
   WinTerminate(habLocal);
}

/*************************************************************************
* Update changed data into the FEA block
*************************************************************************/
BOOL UpdateFeaData(HWND hwndClient, PBYTE pNewData, USHORT usNewPartSize)
{
PFEADATA pNewFeaData;
PBYTE    p, p2;
ULONG    lDiff;
USHORT   usLeadingBytes,
         usTrailingBytes;
PBYTE    pbNewValue;
ULONG    ulOldFeaSize,
         ulNewFeaSize;
SHORT    sIndex;


   lDiff        = (LONG)usNewPartSize - (LONG)Cur.usValueSize;
   usLeadingBytes  = 0;
   usTrailingBytes = 0;
   if (!Options.fShowAllBinary)
      {
      switch (*(PUSHORT)Cur.pbValue)
         {
         case EA_LPBINARY    :
         case EA_LPBITMAP    :
         case EA_LPMETAFILE  :
         case EA_LPICON      :
         case EA_LPASCII     :
            usLeadingBytes = 4;
            break;
         case EA_ASCIIZ      :
         case EA_ASCIIZFN    :
         case EA_ASCIIZEA    :
            usTrailingBytes = 1;
            break;
         case EA_ASN1        :
         case EA_MVMT        :
            usLeadingBytes = 2;
            break;
         default             :
            break;
         }
      }
      
   ulOldFeaSize =  sizeof (FEADATA) + Cur.pFeaData->Fea.cbName +
      Cur.pFeaData->Fea.cbValue;
   ulNewFeaSize =  ulOldFeaSize + lDiff + usLeadingBytes + usTrailingBytes;

   pNewFeaData = malloc(ulNewFeaSize);
   if (!pNewFeaData)
      {
      MessageBox(TITLE, "Not enough memory!");
      DosExit(EXIT_PROCESS, 0);
      }
   memset(pNewFeaData, 0, ulNewFeaSize);

   /*
      Copy everything until the icon to the new EA,
      and adjust the cbValue field
   */
   memcpy(pNewFeaData, Cur.pFeaData, (PBYTE)Cur.pbValue - (PBYTE)Cur.pFeaData);
   pNewFeaData->Fea.cbValue += (lDiff + usLeadingBytes + usTrailingBytes);

   /*
      p points to the point where the new EA_LPICON field should start
   */
   p = (PBYTE)pNewFeaData + ((PBYTE)Cur.pbValue - (PBYTE)Cur.pFeaData);
   pbNewValue = p;

   if (!Options.fShowAllBinary)
      {
      switch (*(PUSHORT)Cur.pbValue)
         {
         case EA_LPBINARY    :
         case EA_LPBITMAP    :
         case EA_LPMETAFILE  :
         case EA_LPICON      :
         case EA_LPASCII     :
            *(PUSHORT)p = *(PUSHORT)Cur.pbValue;
            p += sizeof (USHORT);

            *(PUSHORT)p = (USHORT)usNewPartSize;
            p += sizeof (USHORT);

            break;
         case EA_ASCIIZ      :
         case EA_ASCIIZFN    :
         case EA_ASCIIZEA    :
         case EA_ASN1        :
         case EA_MVMT        :
            *(PUSHORT)p = *(PUSHORT)Cur.pbValue;
            p += sizeof (USHORT);
            break;
         default             :
            break;
         }
      }
   memcpy(p, pNewData, usNewPartSize);

   p += usNewPartSize + usTrailingBytes;

   p2 = Cur.pbValue + Cur.usValueSize;
   memcpy(p, p2, ulOldFeaSize - (p2 - (PBYTE)Cur.pFeaData));

   free(Cur.pFeaData);
   Cur.pbValue     = pbNewValue;
   Cur.usValueSize = usNewPartSize + usLeadingBytes + usTrailingBytes;

   sIndex = WinQueryLboxSelectedItem(WinWindowFromID(hwndClient, ID_EALIST));
   WinSendMsg(WinWindowFromID(hwndClient, ID_EALIST),
         LM_SETITEMHANDLE, MPFROMSHORT(sIndex), (MPARAM)pNewFeaData);

   Cur.pFeaData = pNewFeaData;
   Cur.pFeaData->fChanged = TRUE;
//   DosBeep(1000,30);

   return TRUE;
}
/*********************************************************************
* Save an extended attribute
*********************************************************************/
BOOL WriteEA(PSZ pszPath, PFEADATA pFeaData)
{
EAOP2     eaop2;
PFEA2LIST pFeaList;
ULONG     ulListSize;
APIRET    rc;

   if (!pFeaData->fChanged)
      {
      MessageBox(TITLE, "Extended attribute not changed !");
      return FALSE;
      }

   ulListSize = sizeof (FEA2LIST) +
      pFeaData->Fea.cbName +
      pFeaData->Fea.cbValue;

   pFeaList = malloc(ulListSize);
   if (!pFeaList)
      {
      MessageBox(TITLE, "Not enough memory!");
      return FALSE;
      }

   memcpy(pFeaList->list, &pFeaData->Fea, sizeof (FEA2) +
      pFeaData->Fea.cbName +
      pFeaData->Fea.cbValue);
   pFeaList->cbList = ulListSize;

   memset(&eaop2, 0, sizeof eaop2);
   eaop2.fpFEA2List = pFeaList;

   rc = DosSetPathInfo(pszPath,
      FIL_QUERYEASIZE,
      (PBYTE) &eaop2,
      sizeof eaop2,
      DSPI_WRTTHRU);

   free(pFeaList);

   if (rc)
      {
      MessageBox(TITLE, "%s\n%s",
         "DosSetPathInfo", GetOS2Error(rc));
      return FALSE;
      }

   pFeaData->fChanged = FALSE;

   return TRUE;
}

void msg_nrm(void)
{

}
