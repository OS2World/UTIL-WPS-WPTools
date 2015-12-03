/*******************************************************************
* START for DOS in OS/2
*
*
*
*
*
*
*
*
******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <conio.h>
#include <direct.h>
#include <signal.h>

#include <dos.h>
#define INCL_DOS
#include <os2.h>
#include "portable.h"


#define SSF_RELATED_INDEPENDENT 0
#define SSF_RELATED_CHILD       1

#define SSF_FGBG_FORE           0
#define SSF_FGBG_BACK           1

#define SSF_TRACEOPT_NONE       0
#define SSF_TRACEOPT_TRACE      1
#define SSF_TRACEOPT_TRACEALL   2

#define SSF_INHERTOPT_SHELL     0
#define SSF_INHERTOPT_PARENT    1

/* note that these types are identical to those in pmshl.h for PROG_* */
#define SSF_TYPE_DEFAULT        0
#define SSF_TYPE_FULLSCREEN     1
#define SSF_TYPE_WINDOWABLEVIO  2
#define SSF_TYPE_PM             3
#define SSF_TYPE_VDM            4
#define SSF_TYPE_GROUP          5
#define SSF_TYPE_DLL            6
#define SSF_TYPE_WINDOWEDVDM    7
#define SSF_TYPE_PDD            8
#define SSF_TYPE_VDD            9

/* note that these flags are identical to those in pmshl.h for SHE_* */
#define SSF_CONTROL_VISIBLE     0x0000
#define SSF_CONTROL_INVISIBLE   0x0001
#define SSF_CONTROL_MAXIMIZE    0x0002
#define SSF_CONTROL_MINIMIZE    0x0004
#define SSF_CONTROL_NOAUTOCLOSE 0x0008
#define SSF_CONTROL_SETPOS      0x8000

PRIVATE BYTE szQueueName[50] = "\\QUEUES\\HSTART\\PID";
PRIVATE BYTE szSemName[50]   = "\\SEM32\\HSTART.SEM";
PRIVATE BYTE fOS2            = FALSE;
PRIVATE ULONG ulSemHandle = 0;


PRIVATE USHORT usStartAppOS2(PSTARTDATA pStart);
PRIVATE USHORT usStartAppDOS(STARTDATA _far * pStart);
PRIVATE BOOL   ReadSettings(PBYTE  pszFileName, PBYTE  pbBuffer, WORD wBufLen);
PRIVATE USHORT usMyCreateEventSem(PSZ pszSemName, PULONG puHandle, WORD wAttr, BYTE fState);
PRIVATE USHORT usMyWaitEventSem(ULONG ulHandle, BYTE bTimeOut);
PRIVATE USHORT usMyCloseEventSem(ULONG ulHandle);
PRIVATE USHORT usMyOpenEventSem(PSZ pszSemName, PULONG pulHandle);
PRIVATE USHORT usMyPostEventSem(ULONG ulHandle);
PRIVATE BOOL fFillPosition(PSTARTDATA pStart, PSZ pszArg);

PRIVATE VOID handler(VOID);


#if 0
typedef struct _STARTDATA {	/* stdata */
	USHORT	Length;             2
	USHORT	Related;            4
	USHORT	FgBg;               6
	USHORT	TraceOpt;           8
	PSZ	   PgmTitle;          12
	PSZ	   PgmName;           16
	PBYTE	   PgmInputs;         20
	PBYTE	   TermQ;             24
	PBYTE	   Environment;       28
	USHORT	InheritOpt;        30
	USHORT	SessionType;       32

	PSZ	   IconFile;          36
	ULONG 	PgmHandle;         40
	USHORT	PgmControl;        42
	USHORT	InitXPos;          44
	USHORT	InitYPos;          46
	USHORT	InitXSize;         48
	USHORT	InitYSize;         50
} STARTDATA;

#endif 
/*******************************************************************
* The Main Thing
******************************************************************/
INT main(USHORT usArgc, PSZ pszArgv[])
{
static BYTE szOptions[256]   = "";
static BYTE szSettings[2048] = "";
static BYTE szSetFile[256]   = "";
static BYTE szDirectory[512] = "";
static BYTE szCurDir[512] = "";

STARTDATA StartData;
USHORT usArg;
USHORT usRetco;

BOOL   fFullScreen = FALSE;
BOOL   fDos = FALSE;
BOOL   fWindow = FALSE;
BOOL   fForeGround = FALSE;
BOOL   fBackGround = FALSE;
BOOL   fMax    = FALSE;
BOOL   fMin    = FALSE;
BOOL   fPM     = FALSE;
BOOL   fHelp   = FALSE;
BOOL   fCmdN   = FALSE;
BOOL   fCmdC   = FALSE;
BOOL   fCmdK   = FALSE;
BOOL   fSettings = FALSE;
BOOL   fWait   = FALSE;
BOOL   fPos    = FALSE;
BYTE   fSendSignal = FALSE;
PSZ    pszProg;
USHORT usEnvSel;
USHORT usCmdOffset;
USHORT rc;


   /*
      get the name of the current programm
      NOTE: pArgv[0] does not always include drive and directory
   */

   DosGetEnv(&usEnvSel, &usCmdOffset);
   pszProg = MAKEP(usEnvSel, usCmdOffset);
   pszProg--;
   while (*(pszProg -1))
      pszProg--;


   DosGetMachineMode(&fOS2);

   printf("HSTART for OS/2 32 bits - Start session from DOS and OS/2 Sessions\n");
   printf("Version 2.0 -  Made by Henk Kelder\n");

   memset(&StartData, 0, sizeof StartData);

   /*
      Getting the title, if any
   */
   usArg = 1;
   if (usArgc > 2 && pszArgv[usArg][0] != '-' && pszArgv[usArg][0] !='/')
      StartData.PgmTitle = pszArgv[usArg++];

   /*
      Getting all the arguments, if any
   */

   for (; usArg < usArgc; usArg++)
      {
      strupr(pszArgv[usArg]);
      if (pszArgv[usArg][0] == '-' || pszArgv[usArg][0] =='/')
         {
         if (!strcmp(&pszArgv[usArg][1], "KELDER"))
            fSendSignal = TRUE;
         if (!strcmp(&pszArgv[usArg][1], "FS"))
            fFullScreen = TRUE;
         else if (!strcmp(&pszArgv[usArg][1], "DOS"))
            fDos = TRUE;
         else if (!strcmp(&pszArgv[usArg][1], "WIN"))
            fWindow = TRUE;
         else if (!strcmp(&pszArgv[usArg][1], "PM"))
            fPM = TRUE;
         else if (!strcmp(&pszArgv[usArg][1], "F"))
            fForeGround = TRUE;
         else if (!strcmp(&pszArgv[usArg][1], "B"))
            fBackGround = TRUE;
         else if (!strcmp(&pszArgv[usArg][1], "MAX"))
            fMax = TRUE;
         else if (!strcmp(&pszArgv[usArg][1], "MIN"))
            fMin = TRUE;
         else if (!strcmp(&pszArgv[usArg][1], "N"))
            fCmdN = TRUE;
         else if (!strcmp(&pszArgv[usArg][1], "C"))
            fCmdC = TRUE;
         else if (!strcmp(&pszArgv[usArg][1], "K"))
            fCmdK = TRUE;
         else if (pszArgv[usArg][1] == 'D')
            strcpy(szDirectory, &pszArgv[usArg][2]);
         else if (!strcmp(&pszArgv[usArg][1], "HELP"))
            fHelp = TRUE;
         else if (!strcmp(&pszArgv[usArg][1], "WAIT"))
            fWait = TRUE;
         else if (!memcmp(&pszArgv[usArg][1], "POS=", 4))
            {
            if (fFillPosition(&StartData, pszArgv[usArg] + 5))
               fPos = TRUE;
            else
               {
               printf("Invalid format for /POS=\n");
               exit(1);
               }
            }
         else if (!strcmp(&pszArgv[usArg][1], "?"))
            fHelp = TRUE;
         else if (!memcmp(&pszArgv[usArg][1], "S:", 2))
            fSettings = ReadSettings(&pszArgv[usArg][3], szSettings, sizeof szSettings);
         else
            {
            printf("Invalid option %s\n", pszArgv[usArg]);
            exit(1);
            }
         }
      else
         break;
      }

   if (fHelp)
      {
      printf("USAGE: START [\"title\"] [options] [program] [parameters]\n");
      printf("Supported options are:\n");
      printf("\n");
      printf(" /DOS    - start a dos session\n");
      printf(" /PM     - start a presentation manager session\n");
      printf(" /FS     - start a fullscreen session\n");
      printf(" /WIN    - start a windowed session\n");
      printf(" /F      - start session in the foreground\n");
      printf("           (This is default when /PM, /FS, /DOS or /WIN is specified)\n");
      printf(" /B      - start session in the background\n");
      printf("           (This is default when /PM, /FS, /DOS and /WIN are not specified)\n");
      printf(" /MAX    - starts session maximized (if windowed)\n");
      printf(" /MIN    - starts session minimized (if windowed)\n");
      printf(" /POS=x,y,cx,cy - specify window positions if windowed\n");
      printf(" /C      - start via command processor. Terminate it when ready\n");
      printf(" /K      - start via command processor. Keep it when ready\n");
      printf("           (This is the default for non-PM programs)\n");
      printf(" /N      - start program directly (bypass command processor)\n");
      printf("           (This is the default for PM programs)\n");
      printf(" /S:file - Specify File containing settings for DOS sessions\n");
      printf(" /WAIT   - Wait for the program to terminate\n");
      printf(" /Ddir   - specify directory to start command in.\n");
      exit(1);
      }

   if (fFullScreen || fPM || fWindow || fDos)
      {
      if (!fBackGround)
         fForeGround = TRUE;
      }

   if (fForeGround && fBackGround)
      {
      printf("Either /F or /B can be specified, but not both\n");
      exit(1);
      }

   if (fFullScreen && fWindow)
      {
      printf("Either /FS or /WIN can be specified, but not both\n");
      exit(1);
      }

   if (fMax && fMin)
      {
      printf("Either /MAX or /MIN can be specified, but not both\n");
      exit(1);
      }
   if (fWindow + fFullScreen + fPM > 1)
      {
      printf("Only one of /WIN, /FS and /PM can be specified!\n");
      exit(1);
      }
   if (fDos && fPM)
      {
      printf("Either /DOS or /PM can be specified, but not both\n");
      exit(1);
      }

   if (fCmdN + fCmdK + fCmdC > 1)
      {
      printf("Only one of /K, /C or /N can be specified!\n");
      exit(1);
      }

   if (fPM)
      {
      if (fCmdK || fCmdC)
         {
         printf("/C and /K cannot be used with /PM\n");
         exit(1);
         }
      else
         fCmdN = TRUE;
      }
   else if (!fCmdN && !fCmdK && !fCmdC)
      fCmdK = TRUE;


   if (fCmdC)
      strcpy(szOptions, "/C ");
   else if (fCmdK)
      strcpy(szOptions, "/K ");

   /*
      Getting the program and the options, if any
   */
   for (; usArg < usArgc; usArg++)
      {  
      if (fCmdN && !StartData.PgmName)
         StartData.PgmName = pszArgv[usArg];
      else
         {
         if (!fDos)
            strcat(szOptions, "\"");
         strcat(szOptions, pszArgv[usArg]);
         if (!fDos)
            strcat(szOptions, "\"");
         strcat(szOptions, " ");
         }
      }
   StartData.PgmInputs = szOptions;

   if (fCmdN)
      {
      if (!StartData.PgmName)
         {
         printf("The /N option cannot be used when no program is specified!\n");
         exit(1);
         }
      if (fDos)
         {
         printf("The /N option cannot be used when /DOS is specified!\n");
         exit(1);
         }
      }

   /*
      Get DOS Parms
   */
   if (!fSettings && fDos)
      {
      PBYTE pDot;
      strcpy(szSetFile, pszProg);
      pDot = strchr(szSetFile, '.');
      if (pDot)
         strcpy(pDot, ".CFG");
      else
         strcat(szSetFile, ".CFG");
      printf("Using default DOS settings file %s\n", szSetFile);
      fSettings = ReadSettings(szSetFile, szSettings, sizeof szSettings);
      }
   else if (fSettings && !fDos)
      {
      printf("The /S: option cannot be used when /DOS is NOT specified!\n");
      exit(1);
      }

   /*
      Get program type
   */
   StartData.SessionType = SSF_TYPE_DEFAULT;
   if (fPM)
      StartData.SessionType = SSF_TYPE_PM;
   else if (fDos)
      {
      if (fWindow)
         StartData.SessionType = SSF_TYPE_WINDOWEDVDM;
      else
         StartData.SessionType = SSF_TYPE_VDM;
      }
   else /* Not DOS or PM */
      {
      if (fFullScreen)
         StartData.SessionType = SSF_TYPE_FULLSCREEN;
      else if (!StartData.PgmName) 
         StartData.SessionType = SSF_TYPE_WINDOWABLEVIO;
      }

   if (fMax || fMin || fPos)
      StartData.Length     = 50;
   else
      StartData.Length     = 32;

   if (!fWait)
   	StartData.Related    = SSF_RELATED_INDEPENDENT ;  /* Not related */
   else
	   StartData.Related    = SSF_RELATED_CHILD ; 
	StartData.FgBg       = (fForeGround ? SSF_FGBG_FORE : SSF_FGBG_BACK);
	StartData.TraceOpt   = SSF_TRACEOPT_NONE ;  /* No tracing */
	StartData.TermQ      = NULL;
	StartData.Environment= (fSettings ? szSettings : NULL);
	StartData.InheritOpt = SSF_INHERTOPT_SHELL;
	StartData.InheritOpt = SSF_INHERTOPT_PARENT;
	StartData.IconFile   = NULL;
	StartData.PgmHandle  = 0;
	StartData.PgmControl = (fMax ? SSF_CONTROL_MAXIMIZE: 0) |
                          (fMin ? SSF_CONTROL_MINIMIZE: 0) |
                          (fPos ? SSF_CONTROL_SETPOS  : 0);


   if (strlen(szDirectory))
      {
      USHORT usDrive;
      ULONG  ulDrives;
      USHORT usSize;

      DosQCurDisk(&usDrive, &ulDrives);
      usSize = sizeof szCurDir;
      DosQCurDir(0, szCurDir, &usSize);
      if (szCurDir[0] != '\\')
         {
         memmove(szCurDir + 3, szCurDir, strlen(szCurDir) + 1);
         szCurDir[0] = (BYTE)('@' + usDrive);
         szCurDir[1] = ':';
         szCurDir[2] = '\\';
         }
      else
         {
         memmove(szCurDir + 2, szCurDir, strlen(szCurDir) + 1);
         szCurDir[0] = (BYTE)('@' + usDrive);
         szCurDir[1] = ':';
         }

      strupr(szDirectory);
      if (strlen(szDirectory) > 2 && szDirectory[1] == ':')
         {
         rc = DosSelectDisk(szDirectory[0] - '@');
         if (rc)
            {
            printf("SYS%4.4u: Cannot change to drive %c:!\n",
               rc, szDirectory[0]);
            exit(1);
            }
         }
      rc = DosChDir(szDirectory, 0L);
      if (rc)
         {
         printf("SYS%4.4u: Cannot change to directory %s!\n",
            rc, szDirectory);
         exit(1);
         }
      }

   if (fCmdN)
      printf("Starting %s...\n", StartData.PgmName);
   else
      printf("Starting %s...\n", StartData.PgmInputs);


   if (fOS2)
      usRetco = usStartAppOS2(&StartData);
   else
      usRetco = usStartAppDOS(&StartData);

   if (strlen(szDirectory))
      {
      if (strlen(szCurDir) > 2 && szCurDir[1] == ':')
         {
         rc = DosSelectDisk(szCurDir[0] - '@');
         if (rc)
            {
            printf("SYS%4.4u: Cannot change to drive %c:!\n",
               rc, szCurDir[0]);
            exit(1);
            }
         }
      rc = DosChDir(szCurDir, 0L);
      if (rc)
         {
         printf("SYS%4.4u: Cannot change to directory %s!\n",
            rc, szCurDir);
         exit(1);
         }
      }

   return usRetco;
}
/*******************************************************************
* The Starting of the APP (DOS)
******************************************************************/
USHORT usStartAppDOS(STARTDATA _far * pStart)
{
WORD   wSegMent = FP_SEG(pStart),
       wOffset  = FP_OFF(pStart);
USHORT usRetco;
BOOL   fWait = FALSE;

   if (pStart->Related == SSF_RELATED_CHILD)
      fWait = TRUE;
 	pStart->Related    = SSF_RELATED_INDEPENDENT ;  /* Not related */


   signal(SIGINT, handler);
   signal(SIGBREAK, handler);

   if (fWait)
      {
      usRetco = usMyCreateEventSem(szSemName, &ulSemHandle, 0, 0);
      if (usRetco == 285)
         usRetco = usMyOpenEventSem(szSemName, &ulSemHandle);
      if (usRetco)
         {
         printf("Unable to wait for application...\n");
         fWait = FALSE;
         }
      }

   _asm
      {
         mov ah, 64h    ;
         mov bx, 0025h  ;
         mov cx, 636Ch  ;
         push ds        ;
         mov si, wOffset;
         mov dx, wSegMent;
         mov ds, dx     ;
         xor dx, dx     ;
         int   21h      ;
         pop ds         ;
         jc l_error     ;
         mov usRetco, 0 ;
         jmp l_ready    ;
      l_error :
         mov usRetco, ax;
      l_ready :
      }

   if (usRetco)
      {
      printf("DosStartApp returned error SYS%4.4u\n", usRetco);
      if (ulSemHandle)
         usMyCloseEventSem(ulSemHandle);
      return usRetco;
      }


   if (fWait)
      {
      USHORT rc;

      for (;;)
         {
         _asm int 28h;
         rc = usMyWaitEventSem(ulSemHandle, 2);
         if (rc != 640)
            break;
         printf("Waiting for HWAIT...\n");
         }

      usMyCloseEventSem(ulSemHandle);
      }
   return usRetco;
}

/***************************************************************************
* usStartAppOS2
**************************************************************************/
USHORT usStartAppOS2(PSTARTDATA pStart)
{
USHORT usSession,
       usPid;
USHORT rc;
USHORT usStartRetco;
HQUEUE hQueue;
QUEUERESULT QueueResult;
PULONG pulData;
USHORT usDataLength;
BYTE   bPriority;

   if (pStart->Related == SSF_RELATED_CHILD)
      {
      PIDINFO pidInfo;


      rc = DosGetPID(&pidInfo);
      sprintf(szQueueName + strlen(szQueueName), "%4.4X", pidInfo.pid);

      rc = DosCreateQueue(&hQueue, QUE_FIFO, szQueueName);
      if (rc)
         {
         printf("Sorry, cannot create a queue! (SYS%4.4u)\n", rc);
         exit(1);
         }
      pStart->TermQ = szQueueName;
      }

   usStartRetco = DosStartSession(pStart, &usSession, &usPid);
   if (usStartRetco && usStartRetco != 457)
      {
      printf("DosStartSession returned error SYS%4.4u\n", usStartRetco);
      return usStartRetco;
      }
   else if (usStartRetco)
      {
      printf("Session cannot be started in foreground,\n");
      printf("and is started in the background.\n");
      }


   if (pStart->Related == SSF_RELATED_CHILD)
      {
      rc = DosReadQueue(hQueue,
         &QueueResult,
         &usDataLength,
         &pulData,
         QUE_FIFO,
         DCWW_WAIT,
         &bPriority,
         NULL);

      usStartRetco = HIUSHORT(*pulData);

      rc = DosFreeSeg(SELECTOROF(pulData));

      rc = DosCloseQueue(hQueue);
      }

   return usStartRetco;
}

/***************************************************************************
* usMyCreateEventSem
**************************************************************************/
USHORT usMyCreateEventSem(PSZ pszSemName, PULONG pulHandle, WORD wAttr, BYTE fState)
{
WORD wSegName   = FP_SEG(pszSemName),
     wOffName   = FP_OFF(pszSemName);
WORD wSegHandle = FP_SEG(pulHandle),
     wOffHandle = FP_OFF(pulHandle);
USHORT usRetco;


   _asm mov cx, 636Ch;
   _asm mov bx, 0144h;

   _asm push es;
   _asm push ds;

   _asm mov  si, wOffHandle;
   _asm mov  ax, wSegHandle;
   _asm mov  ds, ax;

   _asm mov  di, wOffName;
   _asm mov  ax, wSegName;
   _asm mov  es, ax;

   _asm mov  al, fState;
   _asm mov  dx, wAttr;

   _asm mov ah, 64h;
   _asm int 21h;
   _asm pop ds;
   _asm pop es;
   _asm mov usRetco, ax;

   return usRetco;
}


/***************************************************************************
* usMyWaitEventSem
**************************************************************************/
USHORT usMyWaitEventSem(ULONG ulHandle, BYTE bTimeOut)
{
WORD wSegHandle = FP_SEG(ulHandle),
     wOffHandle = FP_OFF(ulHandle);
USHORT usRetco;


   _asm mov cx, 636Ch;
   _asm mov bx, 0149h;

   _asm push es;
   _asm push ds;

   _asm mov  dx, wSegHandle;
   _asm mov  si, wOffHandle;

   _asm mov ah, 64h;
   _asm mov al, bTimeOut;

   _asm int 21h;
   _asm pop ds;
   _asm pop es;
   _asm mov usRetco, ax;

   return usRetco;
}

/***************************************************************************
* usMyCloseEventSem
**************************************************************************/
USHORT usMyCloseEventSem(ULONG ulHandle)
{
WORD wSegHandle = FP_SEG(ulHandle),
     wOffHandle = FP_OFF(ulHandle);
USHORT usRetco;

   _asm mov cx, 636Ch;
   _asm mov bx, 0146h;

   _asm mov  dx, wSegHandle;
   _asm mov  si, wOffHandle;

   _asm mov ah, 64h;
   _asm xor al, al;
   _asm int 21h;
   _asm mov usRetco, ax;

   return usRetco;
}

/***************************************************************************
* usMyCloseEventSem
**************************************************************************/
USHORT usMyPostEventSem(ULONG ulHandle)
{
WORD wSegHandle = FP_SEG(ulHandle),
     wOffHandle = FP_OFF(ulHandle);
USHORT usRetco;

   _asm mov cx, 636Ch;
   _asm mov bx, 0148h;

   _asm mov  dx, wSegHandle;
   _asm mov  si, wOffHandle;

   _asm mov ah, 64h;
   _asm xor al, al;
   _asm int 21h;
   _asm mov usRetco, ax;

   return usRetco;
}


/***************************************************************************
* usMyOpenEventSem
**************************************************************************/
USHORT usMyOpenEventSem(PSZ pszSemName, PULONG pulHandle)
{
WORD wSegName   = FP_SEG(pszSemName),
     wOffName   = FP_OFF(pszSemName);
WORD wSegHandle = FP_SEG(pulHandle),
     wOffHandle = FP_OFF(pulHandle);
USHORT usRetco;


   _asm mov cx, 636Ch;
   _asm mov bx, 0145h;

   _asm push es;
   _asm push ds;

   _asm mov  si, wOffHandle;
   _asm mov  ax, wSegHandle;
   _asm mov  ds, ax;

   _asm mov  di, wOffName;
   _asm mov  ax, wSegName;
   _asm mov  es, ax;

   _asm mov ah, 64h;
   _asm int 21h;
   _asm pop ds;
   _asm pop es;
   _asm mov usRetco, ax;

   return usRetco;
}



/*******************************************************************
* Read a settings file
******************************************************************/
BOOL ReadSettings(PBYTE  pszFileName, PBYTE  pbBuffer, WORD wBufLen)
{
FILE *fHandle;
static BYTE szLine[500];
INT iLineNr = 0;
BOOL fErrors = FALSE;

   memset(pbBuffer, 0, wBufLen);

   fHandle = fopen(pszFileName, "r");
   if (!fHandle)
      {
      printf("WARNING: %s cannot be not found!\n", pszFileName);
      getch();
      return FALSE;
      }
   while (fgets(szLine, sizeof szLine, fHandle))
      {
      PBYTE  pszCur;
      BOOL fLineEnd = FALSE;
      PBYTE  pbSave  = pbBuffer;
      iLineNr++;

      if (szLine[0] == '*')
         continue;

      if (memicmp(szLine, "SET ", 4))
         {
         printf("Line %d from %s does not start with \'SET \'\n",
            iLineNr, pszFileName);
         printf("Line ignored!\n");
         fErrors = TRUE;
         continue;
         }
      for (pszCur = szLine+4; isspace((INT)*pszCur); pszCur++)
         ;

      for (; !fLineEnd && *pszCur && *pszCur != '\n'; pszCur++)
         {
         switch (*pszCur)
            {
            case '^':
               *pbBuffer++ = *(pszCur+1);
               pszCur++;
               break;
            case ';':
               *pbBuffer++ = 0;
               fLineEnd = TRUE;
               break;
            case ',':
               *pbBuffer++ = '\n';
               break;
            default:
               *pbBuffer++ = *pszCur;
            }
         }
      if (!fLineEnd)
         {
         printf("Line %d from %s does not end with \';\'\n",
            iLineNr, pszFileName);
         printf("Line ignored!\n");
         fErrors = TRUE;
         pbBuffer = pbSave;
         }
      }

   fclose(fHandle);
   if (fErrors)
      getch();
   return TRUE;
}

VOID handler(VOID)
{
   if (!fOS2 && ulSemHandle)
      usMyCloseEventSem(ulSemHandle);
   exit(-1);
}

BOOL fFillPosition(PSTARTDATA pStart, PSZ pszArg)
{
INT rgPos[4];
INT iIndex;

   memset(rgPos, 0, sizeof rgPos);
   iIndex = 0;
   while (*pszArg)
      {
      if (iIndex > 3)
         return FALSE;
      if (isdigit(*pszArg))
         rgPos[iIndex] = rgPos[iIndex] * 10 + (INT)((*pszArg) - '0');
      else if (*pszArg == ' ')
         ;
      else if (*pszArg == ',')
         iIndex++;
      else
         return FALSE;
      pszArg++;
      }

   if (iIndex != 3)
      return FALSE;

   pStart->InitXPos = rgPos[0];
   pStart->InitYPos = rgPos[1];
   pStart->InitXSize = rgPos[2];
   pStart->InitYSize = rgPos[3];

   return TRUE;
   
}
