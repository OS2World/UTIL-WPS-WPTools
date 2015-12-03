#include <stdio.h>
#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define INCL_DOS
#include <os2.h>

#include "portable.h"
#include "semfuncs.h"

#define MAX_NAME 8

PRIVATE BYTE  fOS2;
PRIVATE ULONG ulSemHandle = 0L;
PRIVATE BYTE szSemName[256] = "";


PRIVATE USHORT usMyCreateEventSem(PSZ pszSemName, PULONG puHandle, WORD wAttr, BYTE fState);
PRIVATE USHORT usMyWaitEventSem(ULONG ulSemHandle, BYTE bTimeOut);
PRIVATE USHORT usMyCloseEventSem(ULONG ulSemHandle);
PRIVATE USHORT usMyOpenEventSem(PSZ pszSemName, PULONG pulSemHandle);
PRIVATE USHORT usMyPostEventSem(ULONG ulSemHandle);
PRIVATE VOID handler(VOID);

PRIVATE BYTE szHelp[] =
"USAGE : HWAIT [options]\n\
\n\
Without options HWAIT signals to the DOS version of HSTART to stop waiting.\n\
You can however also use HWAIT to synchronize other events in different\n\
sessions.\n\
\n\
Valid options are:\n\
/W:xxxxxxxx    - Wait for signal by name\n\
/S:xxxxxxxx    - Send signal by name\n";


/***************************************************************************
* The main function
**************************************************************************/
INT main(INT iArgc, PSZ rgpszArgv[])
{
USHORT usRetco;
INT  iArg;
PSZ  pszSignalName = "HSTART";
BOOL fError = FALSE;
BOOL fWait  = FALSE;
BOOL fSend  = FALSE;


   DosGetMachineMode(&fOS2);
   printf("HWAIT for OS/2 2.1 - Send signal to, or wait for signal from another session\n");
   printf("Version 0.9 -  Made by Henk Kelder\n");

   signal(SIGINT, handler);
   signal(SIGBREAK, handler);

   for (iArg = 1 ; iArg < iArgc; iArg++)
      {
      strupr(rgpszArgv[iArg]);
      if (rgpszArgv[iArg][0] == '-' || rgpszArgv[iArg][0] =='/')
         {
         switch (rgpszArgv[iArg][1])
            {
            case 'w':
            case 'W':
               fWait = TRUE;
               if (rgpszArgv[iArg][2] == ':')
                  pszSignalName = &rgpszArgv[iArg][3];
               else
                  {
                  printf("/W argument should be followed by ':name'\n");
                  fError = TRUE;
                  }
               break;
            case 'S':
            case 's':
               fSend = TRUE;
               if (rgpszArgv[iArg][2] == ':')
                  pszSignalName = &rgpszArgv[iArg][3];
               else
                  {
                  printf("/W argument should be followed by ':name'\n");
                  fError = TRUE;
                  }
               break;
            case '?':
               printf(szHelp);
               fError = TRUE;
               break;
            default :
               printf("Invalid option \'%s\' specified\n", rgpszArgv[iArg]);
               break;
            }
         }
      else
         printf("Invalid option \'%s\' specified\n", rgpszArgv[iArg]);
      }


   if (fWait && fSend)
      {
      printf("/W: and /S: cannot be used together!\n");
      exit(1);
      }
   if (fError)
      exit(1);

   if (!fSend && !fWait)
      {
      fSend = TRUE;
      strcpy(szSemName, "\\SEM32\\HSTART.SEM");
      }
   else
      {
      if (strlen(pszSignalName) > MAX_NAME)
         {
         printf("The name cannot exceed %d characters", MAX_NAME);
         exit(1);
         }
      strcpy(szSemName, "\\SEM32\\HSTART\\");
      strcat(szSemName, pszSignalName);
      strcat(szSemName, ".SEM");
      }

   if (fSend)
      {
      if (fOS2)
         usRetco = Dos16OpenEventSem(szSemName, &ulSemHandle);
      else
         usRetco = usMyOpenEventSem(szSemName, &ulSemHandle);
      switch (usRetco)
         {
         case 0   :
            break;
         case 123 :
            printf("\'%s\' is an invalid name\n", pszSignalName);
            exit(1);
         case 187 :
            printf("No one is waiting for \'%s\'\n", pszSignalName);
            exit(1);
         default  :
            printf("SYS%4.4u on DosOpenEventSem\n",  usRetco);
            exit(1);
         }

      if (fOS2)
         Dos16PostEventSem(ulSemHandle);
      else
         usMyPostEventSem(ulSemHandle);
      }
   else
      {
      if (fOS2)
         usRetco = Dos16CreateEventSem(szSemName, &ulSemHandle, 0L, 0);
      else
         usRetco = usMyCreateEventSem(szSemName, &ulSemHandle, 0, 0);
      switch (usRetco)
         {
         case 0   :
            break;
         case 123 :
            printf("\'%s\' is an invalid name\n", pszSignalName);
            exit(1);
         case 285 :
            printf("Another process is already waiting for \'%s\'\n", pszSignalName);
            exit(1);
         default  :
            printf("SYS%4.4u on DosOpenEventSem\n",  usRetco);
            exit(1);
         }

      do
         {
         printf("Waiting for signal \'%s\'\n", pszSignalName);
         if (fOS2)
            usRetco = Dos16WaitEventSem(ulSemHandle, 2000);
         else
            usRetco = usMyWaitEventSem(ulSemHandle, 2);
         }  while (usRetco == 640);
      if (usRetco)
         printf("SYS%4.4u on DosWaitEventSem\n",  usRetco);
      }

   if (fOS2)
      Dos16CloseEventSem(ulSemHandle);
   else
      usMyCloseEventSem(ulSemHandle);
   return 0;
}

/***************************************************************************
* usMyCreateEventSem
**************************************************************************/
USHORT usMyCreateEventSem(PSZ pszSemName, PULONG pulSemHandle, WORD wAttr, BYTE fState)
{
WORD wSegName   = FP_SEG(pszSemName),
     wOffName   = FP_OFF(pszSemName);
WORD wSegHandle = FP_SEG(pulSemHandle),
     wOffHandle = FP_OFF(pulSemHandle);
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
USHORT usMyWaitEventSem(ULONG ulSemHandle, BYTE bTimeOut)
{
WORD wSegHandle = FP_SEG(ulSemHandle),
     wOffHandle = FP_OFF(ulSemHandle);
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
USHORT usMyCloseEventSem(ULONG ulSemHandle)
{
WORD wSegHandle = FP_SEG(ulSemHandle),
     wOffHandle = FP_OFF(ulSemHandle);
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
USHORT usMyPostEventSem(ULONG ulSemHandle)
{
WORD wSegHandle = FP_SEG(ulSemHandle),
     wOffHandle = FP_OFF(ulSemHandle);
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
USHORT usMyOpenEventSem(PSZ pszSemName, PULONG pulSemHandle)
{
WORD wSegName   = FP_SEG(pszSemName),
     wOffName   = FP_OFF(pszSemName);
WORD wSegHandle = FP_SEG(pulSemHandle),
     wOffHandle = FP_OFF(pulSemHandle);
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

VOID handler(VOID)
{
   if (ulSemHandle)
      {
      if (fOS2)
         Dos16CloseEventSem(ulSemHandle);
      else
         usMyCloseEventSem(ulSemHandle);
      }
   exit(-1);
}

