//#define NO_POPUP
#define INCL_DOSSEMAPHORES
#define INCL_DOSPROCESS
#define INCL_SUB
#define INCL_DOSMONITORS
#define INCL_DOSINFOSEG
#define INCL_DOSSESMGR
#define INCL_DOSERRORS
#define INCL_WIN
#define INCL_WINSWITCHLIST
#define INCL_DOSNMPIPES
#define INCL_DOSERRORS
#define INCL_DOS
#define INCL_VIO
#include <os2.h>

#include <stdio.h>
#include <conio.h> /*SJD Wed  08-12-1992  19:52:45 */
#include <string.h>
#include <stdlib.h>
#include <process.h>
#include <ctype.h>
#include <stddef.h>
#include <stdarg.h>

#include "display.h"
#include "keyb.h"
#include "msg.h"
#include "language.h"
#include "hswitch.h"

#define KEY_ALT_X       0x2D00
#define KEY_ALT_B       0x3000
#define KEY_ALT_F       0x2100
#define KEY_ALT_A       0x1E00
#define KEY_ALT_SLASH   0x3500

#define MONITOR_DEFAULT 0x0000
#define MONITOR_BEGIN   0x0001
#define MONITOR_END     0x0002
#define RIGHTALT        0x0800
#define LEFTALT         0x0200
#define KEY_RELEASE_MASK 0x40
#define FIRST_SES        4
#define LAST_SES         15
#define SHAREMEM     "\\SHAREMEM\\HSWITCH"
#define DEADSEM      "\\SEM32\\HSWITCH\\DEADSEM"

typedef SHANDLE   HMONITOR;	/* hmon */
typedef HMONITOR  * PHMONITOR;

#pragma pack(2)
typedef struct _MONIN { 	/* mnin */
	USHORT cb;
	BYTE abReserved[18];
	BYTE abBuffer[108];
} MONIN;
typedef MONIN * PMONIN;

typedef struct _MONOUT {	/* mnout */
	USHORT cb;
	UCHAR buffer[18];
	BYTE abBuf[108];
} MONOUT;
typedef MONOUT * PMONOUT;

/* KBD monitor data record */

typedef struct _KeyPacket
{
USHORT mnflags;
KBDKEYINFO cp;
USHORT ddflags;
} KEYPACKET;
#pragma pack(2)


typedef struct _SwitchList
{
BYTE      szTitle[MAXNAMEL+4];
PSWENTRY  pSwEntry;
BOOL      fMessage;
} SLIST, *PSLIST;

typedef struct _Options
{
CHAR    chPopupScan;
BYTE    bColor;
BOOL    fShowAllTasks;
BOOL    fOpaque;
} OPTIONS, *POPTIONS;

#define DosGetPPID         DOS16GETPPID
#define DosMonOpen         DOS16MONOPEN
#define DosMonClose        DOS16MONCLOSE
#define DosMonReg          DOS16MONREG
#define DosMonRead         DOS16MONREAD
#define DosMonWrite        DOS16MONWRITE

APIRET16 APIENTRY16 DosGetPPID  (USHORT pidChild,
                               PUSHORT ppidParent);

APIRET16 APIENTRY16 DosMonOpen  (PSZ pszDevName,
                               PHMONITOR phmon);
APIRET16 APIENTRY16 DosMonClose (HMONITOR hmon);
APIRET16 APIENTRY16 DosMonReg   (HMONITOR hmon,
                               PVOID pbInBuf,
                               PVOID pbOutBuf,
			                      USHORT fPosition,
                               USHORT usIndex);
APIRET16 APIENTRY16 DosMonRead  (PVOID pbInBuf,
                               USHORT fWait,
                               PVOID pbDataBuf,
                               PUSHORT pcbData);
APIRET16 APIENTRY16 DosMonWrite (PVOID pbOutBuf,
                                 PVOID pbDataBuf,
                                 USHORT cbData);




/**********************************************************************
* Function prototypes
**********************************************************************/
static VOID    DoPopup       (VOID);
static VOID    APIENTRY MonitorThread( ULONG session );
static VOID    MyPrintf(PSZ pszFormat, ...);
static VOID    vSetColors(BYTE bColor);



/**********************************************************************
* Global variables
**********************************************************************/
static HMONITOR KBDHandle;
static BOOL     fTerminate = FALSE,
                fUsePrintf;
static HMUX     hmux;
static HEV      hevDead;
static HEV      hevPopup;
static HEV      hevPipe;
static ULONG    ulPopupSession;
static BYTE     szPipeInput[sizeof (DOSPARMS)+100];
static PDOSPARMS pDosParms = NULL;
static INT     iFirstSession = FIRST_SES;
static INT     iLastSession  = LAST_SES;
static POPTIONS pOptions = NULL;

IMPORT BOOL    _fListboxClearScreen;
IMPORT BOOL    _fMsgWrapOnSlash;
/**********************************************************************
* The main function
**********************************************************************/
int cdecl main (INT iArgc, PSZ pszArgv[])
{
static    SEMRECORD rgSemRec[3];

ULONG     ulSem; 
USHORT    usIndex;
HPIPE     hPipe;
ULONG     ulBytesRead;
APIRET    rc,
          rc2;
ULONG     ulCount;
TID       Tid;

ULONG     ulLVB;
USHORT    uscbLVB;
BOOL      fActive;

   /*
      Disable altkey handling in keyb_read()
   */
   _AltKey = FALSE;
   _display_init();

   SetLanguage(LANG_UK);

   _fMsgWrapOnSlash = FALSE;

   /*
      Determine if we run in the foreground
   */
   rc = VioGetBuf(&ulLVB, &uscbLVB, (HVIO)0);
   if (rc)
      fUsePrintf = FALSE;
   else
      fUsePrintf = TRUE;

   fActive = FALSE;
   rc = DosGetNamedSharedMem((PVOID *)&pOptions, SHAREMEM, PAG_READ|PAG_WRITE);
   if (!rc)
      fActive = TRUE;
   else
      {
      rc = DosAllocSharedMem((PVOID *)&pOptions, SHAREMEM, sizeof (OPTIONS), PAG_COMMIT|PAG_READ|PAG_WRITE);
      if (rc)
         {
         MyPrintf("DosAllocSharedMem, error %d\n", rc);
         DosExit(EXIT_PROCESS, 1);
         }
      memset(pOptions, 0, sizeof (OPTIONS));
      pOptions->chPopupScan   = 0x35;
      pOptions->bColor        = 0x70;
      pOptions->fShowAllTasks = FALSE;
      pOptions->fOpaque       = FALSE;
      }

   /*
      Parsing the arguments
   */

   MyPrintf("%s for OS/2 2.n - Henk Kelder\n", TITLE);
   for (ulCount = 1; ulCount < iArgc; ulCount++)
      {
      strupr(pszArgv[ulCount]);
      if (pszArgv[ulCount][0] == '-' ||
          pszArgv[ulCount][0] == '/')
         {
         ULONG ulNumber;
         switch (pszArgv[ulCount][1])
            {
            case '?':
               if (!fActive)
                  {
                  MyPrintf("/Fnn    - Specify nn as first session id to monitor\n");
                  MyPrintf("/Lnn    - Specify nn as last session id to monitor\n");
                  }
               MyPrintf("/Hnn    - Specify nn as alterate hotkey\n");
               MyPrintf("/O[+|-] - Switch to 25x80 when poping up [on|off]\n");
               MyPrintf("/Cfb    - Specify fb as foreground-background color\n");
               MyPrintf("/K      - Kills a resident version of HSWITCH\n");
               continue;

            case 'F':
               if (fActive)
                  {
                  MyPrintf("%s : cannot be used when HSWITCH is already active\n", pszArgv[ulCount]);
                  break;
                  }
               ulNumber = atol(&pszArgv[ulCount][2]);
               if (ulNumber >= iLastSession)
                  MyPrintf("%s : invalid value\n", pszArgv[ulCount]);
               else
                  iFirstSession = ulNumber;
               continue;

            case 'L':
               if (fActive)
                  {
                  MyPrintf("%s : cannot be used when HSWITCH is already active\n", pszArgv[ulCount]);
                  break;
                  }
               ulNumber = atol(&pszArgv[ulCount][2]);
               if (ulNumber <= iFirstSession)
                  MyPrintf("%s : invalid value\n", pszArgv[ulCount]);
               else
                  iLastSession = ulNumber;
               continue;

            case 'H':
               pOptions->chPopupScan = atoi(&pszArgv[ulCount][2]);
               MyPrintf("Alternate scan key %d will be used\n",
                  pOptions->chPopupScan);
               continue;

            case 'O':
               if (!pszArgv[ulCount][2] || pszArgv[ulCount][2] == '+')
                  pOptions->fOpaque = TRUE;
               else
                  pOptions->fOpaque = FALSE;
               continue;

            case 'C':
               {
               PSZ pEnd;
               pOptions->bColor   = strtol(&pszArgv[ulCount][2], &pEnd, 16);
               vSetColors(pOptions->bColor);
               continue;
               }
            case 'K':
               if (!fActive)
                  {
                  MyPrintf("HSWITCH not loaded, /K option ignored!\n");
                  continue;
                  }
               rc = DosOpenEventSem(DEADSEM, &hevDead);
               if (rc)
                  {
                  MyPrintf("DosOpenEventSem, error %d\n", rc);
                  DosExit(EXIT_PROCESS, 1);
                  }
               rc = DosPostEventSem(hevDead);
               DosCloseEventSem(hevDead);
               if (rc)
                  {
                  MyPrintf("DosPostEventSem, error %d\n", rc);
                  DosExit(EXIT_PROCESS, 1);
                  }
               else
                  MyPrintf("Resident version of HSWITCH killed!\n\n");
               continue;
            }
         }
      MyPrintf("Unknown argument %s ignored\n", pszArgv[ulCount]);
      }

   if (fActive)
      {
      MyPrintf("HSWITCH monitors sessions %d - %d\n",
         iFirstSession, iLastSession);
      MyPrintf("        %s to 80x25 on popup\n",
         (pOptions->fOpaque ? "switches" : "does not switch"));
      MyPrintf("        uses scancode %d to popup\n", pOptions->chPopupScan);
      MyPrintf("        uses color %2.2X\n", pOptions->bColor);

      MyPrintf("Options processed\n");
      DosFreeMem(pOptions);
      DosExit(EXIT_PROCESS, 1);
      }

   MyPrintf("HSWITCH will monitor sessions %d - %d\n",
      iFirstSession, iLastSession);

   /*
      Get a handle for registering buffers
   */
   rc = DosMonOpen ( "KBD$", &KBDHandle );
   if (rc)
      {
      MyPrintf("DosMonOpen, error %d\n", rc);
      DosExit(EXIT_PROCESS, 1);
      }

   /*
      Bump up the process priority so that Ctrl-Break (for instance) is      
      seen immediately
   */

   (VOID)DosSetPrty( PRTYS_PROCESSTREE, PRTYC_TIMECRITICAL, 0, 0 );

   /*
      set semaphore (which will be cleared when the user presses hotkey)
   */
   rc = DosCreateEventSem(NULL, &hevPopup, 0L, FALSE);
   if (rc)
      {
      MyPrintf("DosCreateEventSem, error %d\n", rc);
      DosExit(EXIT_PROCESS, 1);
      }
   rgSemRec[0].hsemCur = (HSEM)hevPopup;
   rgSemRec[0].ulUser = 0;

   rc = DosCreateEventSem(DEADSEM, &hevDead, 0L, FALSE);
   if (rc)
      {
      MyPrintf("DosCreateEventSem, error %d\n", rc);
      DosExit(EXIT_PROCESS, 1);
      }
   rgSemRec[1].hsemCur = (HSEM)hevDead;
   rgSemRec[1].ulUser = 1;


   rc = DosCreateEventSem(NULL, &hevPipe, DC_SEM_SHARED, FALSE);
   if (rc)
      {
      MyPrintf("DosCreateEventSem, error %d\n", rc);
      DosExit(EXIT_PROCESS, 1);
      }

   rgSemRec[2].hsemCur = (HSEM)hevPipe;
   rgSemRec[2].ulUser = 2;

   rc = DosCreateMuxWaitSem(NULL,
      &hmux,
      3L,
      rgSemRec,
      DCMW_WAIT_ANY);

   if (rc)
      {
      MyPrintf("DosCreateMuxWaitSem, error %d\n", rc);
      DosExit(EXIT_PROCESS, 1);
      }

   /* For each session, start a thread which installs itself as a keyboard
      Monitor to watch for hot-keys */


   rc = DosCreateNPipe("\\PIPE\\HSWITCH",
      &hPipe,
      NP_ACCESS_INBOUND,
      NP_NBLK | 0x01,
      4096,
      2048,
      0L);
   if (rc)
      {
      MyPrintf("Unable to make named pipe! (Error %d)\n", rc);
      DosExit(EXIT_PROCESS, 1);
      }
   rc = DosSetNPipeSem(hPipe, (HSEM)hevPipe, 2);
   if (rc)
      {
      MyPrintf("DosSetNPipeSem! (Error %d)\n", rc);
      DosExit(EXIT_PROCESS, 1);
      }
   rc = DosConnectNPipe(hPipe);
   if (rc && rc != ERROR_PIPE_NOT_CONNECTED)
      {
      MyPrintf("DosConnectNPipe! (Error %d)\n", rc);
      DosExit(EXIT_PROCESS, 1);
      }

   for (usIndex=iFirstSession; usIndex<=iLastSession; usIndex++)
      {
      rc = DosCreateThread(&Tid, MonitorThread, usIndex, 2, 12000);
      if (rc)
         {
         MyPrintf("DosCreateThread for Session %d, error %d\n",
            usIndex, rc);
         DosExit(EXIT_PROCESS, 1);
         }
      }

   DosSleep(500);
   do
      {
      /* Wait until one of the specified hevaphores is cleared */
      
      rc2 = DosConnectNPipe(hPipe);
      if (rc2 && rc2 != ERROR_PIPE_NOT_CONNECTED)
         MyPrintf("DosConnectNPipe, Retco %d\n", rc2);

      DosResetEventSem(hevPopup, &ulCount);
      DosResetEventSem(hevPipe, &ulCount);
      pDosParms = NULL;

      MyPrintf("Waiting for popup request..\n");
      memset(szPipeInput, 0, sizeof szPipeInput);
      rc = DosWaitMuxWaitSem(hmux, SEM_INDEFINITE_WAIT, &ulSem);
      if ( rc)
         {
         MyPrintf("DosWaitMuxWaitSem, retco %d\n", rc);
         ulSem=1;
         }

      if (ulSem==0)
         {
         rc2 = DosDisConnectNPipe(hPipe);
         if (rc2)
            MyPrintf("DosDisConnectNPipe, Retco %d\n", rc2);
         DoPopup();
         }
      else if (ulSem == 1)
         ulSem = 1;
      else if (ulSem == 2)
         {
         DosSleep(10);
         rc = DosRead(hPipe, szPipeInput,  sizeof szPipeInput,  &ulBytesRead);
         rc2 = DosDisConnectNPipe(hPipe);
         if (rc2)
            MyPrintf("DosDisConnectNPipe, Retco %d\n", rc2);

         ulPopupSession = -1L;
         pDosParms = (PDOSPARMS)szPipeInput;
         MyPrintf("From DOS %d lines, title %s\n",
            (USHORT)pDosParms->bScreenHeight,
            pDosParms->szCurTitle);


         if (!rc && ulBytesRead)
            DoPopup();
         }
      else
         DosBeep(2000,100);

      DosSleep(1L); /* magic */
      } while (ulSem != 1); /* loop until we get a hevDead */

   /*
      Close connection with keyboard
   */
   rc = DosMonClose ( KBDHandle );
   if (rc)
      MyPrintf("DosMonClose, error %d\n", rc);

   /*
      Close named pipe
   */
   rc = DosClose ( hPipe);
   if (rc)
      MyPrintf("DosClose(pipe), error %d\n", rc);



   fTerminate = TRUE;
   DosSleep(1000);
   DosBeep(1000,100);

   /*
      Exit - kill all threads
   */
   DosFreeMem(pOptions);
   DosExit ( EXIT_PROCESS, 0 );


   return 0; 
}

VOID vSetColors(BYTE bColor)
{
PPATCH pPatch = GetPatchBuffer();

   pPatch->tab_attr[A_LISTBOX]   = bColor;
   pPatch->tab_attr[A_PAN_TITLE] = bColor;
   pPatch->tab_attr[A_MSG]       = bColor;
   pPatch->tab_attr[A_MSG_YN]    = bColor;
   pPatch->tab_attr[A_INPUT_EMP] = bColor;
   bColor = bColor / 16 + (bColor % 16) * 16;
   pPatch->tab_attr[A_CUR_CHOICE] = bColor;
}

/*************************************************************************
*  The Popup Function
*************************************************************************/
static VOID DoPopup(VOID)
{
static VIOINTENSITY vioIntens;
static VIOMODEINFO ModeInfo;
PSWBLOCK prgSwapBlock;
PSLIST   pSwitchList, pSw;
ULONG    ulcEntries;
ULONG    usSize;
USHORT   usWaitFlags;
LISTBOX  ListBox;
SHORT    sIndex;
BOOL     fContinue;
BOOL     fScanTasks;
USHORT   usSelection;
USHORT   usCurSel;
APIRET   rc;
BOOL     fDosFound = FALSE;
USHORT   usMaxRow, usMaxCol;

  /* Get a pop-up screen.  If we can't make it transparent, it means that   *
   * some Vio program is in graphics mode.  In that case, try to make       *
   * it opaque.                                                             */

   usWaitFlags=VP_WAIT | VP_TRANSPARENT;
   if (pOptions->fOpaque)
      usWaitFlags=VP_WAIT | VP_OPAQUE;
   else if (pDosParms && 
      (pDosParms->bScreenHeight != 25 || pDosParms->bScreenWidth != 80))
      usWaitFlags=VP_WAIT | VP_OPAQUE;

#ifndef NO_POPUP
   rc = VioPopUp(&usWaitFlags, 0);
   if (rc)
      {
      MyPrintf("VioPopUp, retco %d\n", rc);
      usWaitFlags=VP_NOWAIT | VP_OPAQUE;
      if (VioPopUp(&usWaitFlags, 0) != 0)
         {
         MyPrintf("VioPopUp, retco %d\n", rc);
         return;
         }
      }
#endif

   ModeInfo.cb = sizeof ModeInfo;
   VioGetMode(&ModeInfo, (HVIO)0);
   if (!pDosParms)
      {
      usMaxRow = ModeInfo.row;
      usMaxCol = ModeInfo.col;
      }
   else
      {
      usMaxRow = 25;
      usMaxCol = 80;
      }

   vioIntens.cb = 6;
   vioIntens.type = 2;
   vioIntens.fs = 1;
   VioSetState(&vioIntens, (HVIO)0);
   vSetColors(pOptions->bColor);
   cursor(OFF);


   fScanTasks = TRUE;
   while (fScanTasks)
      {
      fScanTasks = FALSE;

      /* Get the switch list information */

      ulcEntries=WinQuerySwitchList(0, NULL, 0);
      usSize=sizeof (SWBLOCK) +
            sizeof (HSWITCH) +
            (ulcEntries + 4) * (long) sizeof(SWENTRY);
      MyPrintf("There are %d entries in the task list\n", ulcEntries);

   /* Allocate memory for list */

      prgSwapBlock=malloc(usSize);
      if (!prgSwapBlock)
         {
         MyPrintf("Not enough memory for swapblock\n");

         VioEndPopUp(0);
         pDosParms = NULL;
         return;
         }
      pSwitchList = (PSLIST)calloc(ulcEntries, sizeof (SLIST));
      if (!pSwitchList)
         {
         free(prgSwapBlock);
         pDosParms = NULL;
         VioEndPopUp(0);
         return;
         }
      memset(pSwitchList, 0, ulcEntries * sizeof (SLIST));


         /* Put the info in the list */

      ulcEntries = WinQuerySwitchList(0,
         prgSwapBlock,
         (USHORT)(usSize-sizeof(SWENTRY)));

      /* Set the default entry on our task list */


      memset(&ListBox, 0, sizeof ListBox);

      usCurSel = 0xFFFF;
      for (pSw = pSwitchList, sIndex = prgSwapBlock->cswentry - 1;
         sIndex >= 0;
         sIndex--)
         {
         USHORT usMaxWidth;
         PSZ p;

         if (!pOptions->fShowAllTasks &&
            prgSwapBlock->aswentry[sIndex].swctl.uchVisibility != SWL_VISIBLE)
            continue;

         pSw->pSwEntry = &prgSwapBlock->aswentry[sIndex];

         p = pSw->szTitle;
         if (pSw->pSwEntry->swctl.uchVisibility != SWL_VISIBLE)
            *p++ = '[';
         strncpy(p, prgSwapBlock->aswentry[sIndex].swctl.szSwtitle,
            sizeof pSw->szTitle  - 1);
         if (pSw->pSwEntry->swctl.uchVisibility != SWL_VISIBLE)
            strcat(pSw->szTitle, "]");

         for (p = pSw->szTitle; *p ; p++)
            {
            if (p[0] < 0x20)
               p[0] = 0x20;
            }

         if (pDosParms)
            {
            if (!stricmp(pSw->pSwEntry->swctl.szSwtitle, pDosParms->szCurTitle))
               {
               fDosFound = TRUE;
               usCurSel = ListBox.Count;

               strncpy(pSw->szTitle, pDosParms->szOrigTitle, sizeof pSw->szTitle);
               }
            }
         else  if (pSw->pSwEntry->swctl.idSession == ulPopupSession)
            usCurSel = ListBox.Count;

         usMaxWidth = strlen(pSw->szTitle);
         if (usMaxWidth > ListBox.DispWidth)
            ListBox.DispWidth = usMaxWidth;

         ListBox.Count++;
         pSw++;
         }

      for (sIndex = 1; sIndex < ListBox.Count; sIndex++)
         {
         USHORT sIndex1;
         if (pSwitchList[sIndex].pSwEntry->swctl.bProgType != PROG_PM)
            continue;

         for (sIndex1 = 0; sIndex1 < sIndex; sIndex1++)
            {
            if (pSwitchList[sIndex1].pSwEntry->swctl.bProgType != PROG_PM)
               continue;
            if (pSwitchList[sIndex].pSwEntry->swctl.idSession ==
                   pSwitchList[sIndex1].pSwEntry->swctl.idSession
                  &&
                pSwitchList[sIndex].pSwEntry->swctl.idProcess ==
                   pSwitchList[sIndex1].pSwEntry->swctl.idProcess)
                  {
                  pSwitchList[sIndex].fMessage = TRUE;
                  pSwitchList[sIndex1].fMessage = TRUE;
                  }
            }
         }

      if (pDosParms && !fDosFound)
         MyPrintf("Dos Session %s not found\n", pDosParms->szCurTitle);

      if (ListBox.DispWidth < strlen(TITLE))
         ListBox.DispWidth = strlen(TITLE);
      ListBox.ulr       = 10;
      ListBox.ulc       = (usMaxCol - ListBox.DispWidth - 2) / 2;
      ListBox.Width     = sizeof (SLIST);      
      ListBox.Offset    = offsetof (SLIST, szTitle);
      ListBox.Title     = TITLE;
      ListBox.Tabel     = (PBYTE)pSwitchList;
      ListBox.fAbortOnUnknownFkey = TRUE;


      fContinue = TRUE;
      while (fContinue)
         {
         BYTE szMess[80];
         sprintf(szMess,
            "ENTER, ESC, AltX=End Hswitch, Del=Kill task, AltB=BkndCol, AltF=FgndCol COL=%2.2X",
            pOptions->bColor);
         display_attr(usMaxRow - 1, 0, szMess,
            A_INPUT_EMP, 80);
         ListBox.CurSel = usCurSel;
         SelectionBox(&ListBox);
         usSelection = 0xFFFF;
         switch (ListBox.wFkey)
            {
            case KEY_ALT_X:
               DosPostEventSem(hevDead);
               fContinue = FALSE;
               break;
            case KEY_ENTER:
               usSelection = ListBox.CurSel;
               if (pSwitchList[usSelection].pSwEntry->swctl.uchVisibility != SWL_VISIBLE)
                  break;
               fContinue = FALSE;
               break;
            case KEY_ESC   :
               fContinue = FALSE;
               usSelection = usCurSel;
               break;
            case KEY_DELETE:
               usSelection = ListBox.CurSel;
               if (pSwitchList[usSelection].fMessage)
                  {
                  if (!msg_jn("Close %s?", 15, FALSE,
                     pSwitchList[usSelection].szTitle))
                     break;

                  WinPostMsg(pSwitchList[usSelection].pSwEntry->swctl.hwnd,
                     WM_CLOSE, 0L, 0L);
                  }
               else
                  {
                  if (!msg_jn("Terminate %s?", 15, FALSE,
                     pSwitchList[usSelection].szTitle))
                     break;

                  rc = DosKillProcess(DKP_PROCESS,
                     pSwitchList[usSelection].pSwEntry->swctl.idProcess);
                  if (rc)
                     msg_nrm("Unable to kill process!");
                  }
               if (usCurSel != usSelection)
                  {
                  DosSleep(500);
                  fScanTasks = TRUE;
                  }
               fContinue = FALSE;
               break;

            case KEY_ALT_A:
               pOptions->fShowAllTasks = !pOptions->fShowAllTasks;
               fScanTasks = TRUE;
               fContinue = FALSE;
               break;

            case KEY_ALT_B:
               pOptions->bColor = ((pOptions->bColor / 16) + 1) * 16 + pOptions->bColor % 16;
               vSetColors(pOptions->bColor);
               break;

            case KEY_ALT_F:
               pOptions->bColor = (pOptions->bColor / 16) * 16 + (pOptions->bColor % 16 + 1) % 16;
               vSetColors(pOptions->bColor);
               break;

            default     :
               fScanTasks = TRUE;
               fContinue = FALSE;
               MyPrintf("Unknown key %X\n", ListBox.wFkey);
               break;
            }
         }
      free(pSwitchList);
      free(prgSwapBlock);
      }
   pDosParms = NULL;

#ifndef NO_POPUP
   VioEndPopUp(0);
#endif

   if (usSelection != 0xFFFF)
      WinSwitchToProgram(pSwitchList[usSelection].pSwEntry->hswitch);

}


/*******************************************************************
* The Monitor Thread
*******************************************************************/
static VOID APIENTRY MonitorThread( ULONG ulSession )
{
KEYPACKET keybufr;
MONIN     InBuff ;  
MONOUT    OutBuff;
USHORT    count;  /* number of chars in Monitor read/write buffer */

   MyPrintf("Starting up thread for session %ld\n", ulSession);

   keybufr.cp.chChar = 0;
   InBuff.cb  = sizeof(InBuff);
   OutBuff.cb = sizeof(OutBuff);

   /* register the buffers to be used for Monitoring */

   if (DosMonReg( KBDHandle, &InBuff, &OutBuff, MONITOR_BEGIN,
                   (USHORT)ulSession ))
      {
      MyPrintf("Unable to create thread for Session %ld\n", ulSession);
      DosExit(EXIT_THREAD, 1);
      }

   /*
       Main loop: read key into Monitor buffer, examine it and take action
       if it is one of our hot keys, otherwise pass it on to device driver
   */
   while (!fTerminate)
      {
      count = sizeof(keybufr);
      if (DosMonRead(&InBuff, IO_WAIT, &keybufr, (PUSHORT)&count ))
         break;

      if (!(keybufr.ddflags & KEY_RELEASE_MASK) &&
           keybufr.cp.chScan == pOptions->chPopupScan &&
           (keybufr.cp.fsState & RIGHTALT || keybufr.cp.fsState & LEFTALT))
         {
         ulPopupSession = ulSession;
         DosPostEventSem(hevPopup);
         }
      else
         {
         if (DosMonWrite(&OutBuff,&keybufr, count ))
                break;
         }
      }
   MyPrintf("Thread for Session %ld ended\n", ulSession);
}

/*******************************************************************
* My Printf
*******************************************************************/
VOID MyPrintf(PSZ pszFormat, ...)
{
va_list va;

   if (!fUsePrintf)
      return;

   va_start(va, pszFormat);
   vfprintf(stdout, pszFormat, va);
   fflush(stdout);
   return;
}


