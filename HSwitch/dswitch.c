#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <dos.h>
#include <stdlib.h>
#include <string.h>
#include "resid.h"
#include "hswitch.h"

/********************************************************************
*
********************************************************************/
#define HOTKEY RES_ALT | 0x35

/********************************************************************
* Global variables
********************************************************************/
static PBYTE    FingerPrint = "SWITCH";
static BYTE     _far *pbRows = (BYTE far *)0x00000484;
static WORD     wHotKey = HOTKEY;
static DOSPARMS DosParms;
static USHORT   usWaitTime = 500;


/********************************************************************
* Function prototypes
********************************************************************/
void      DisplayString(PBYTE s);
INT       IntToStr(PSZ pszStr);
WORD _far Switch(WORD ax, WORD bx, WORD cx, WORD dx);
USHORT    GetSessionTitle(BYTE _far * pszTitle);
USHORT    SetSessionTitle(BYTE _far * pszTitle);
void      sleep(USHORT usDur);
BOOL      fIsWindow(VOID);


/********************************************************************
* The main thing
********************************************************************/
void main(INT iArgc, PSZ pszArg[])
{
BYTE  rFunc;
INT   iArg;
INT   iHandle;
BOOL  fAltHotKey = FALSE;


   DisplayString(TITLE);
   DisplayString(" for DOS - Henk Kelder\r\n");

   rFunc = (BYTE)ResCheck(FingerPrint);
   _CheckVideo = FALSE;
   
   for (iArg = 1; iArg < iArgc; iArg++)
      {
      if (pszArg[iArg][0] == '/' || pszArg[iArg][0] == '-')
         {
         switch (pszArg[iArg][1])
            {
            case 'R':
            case 'r':
               if (rFunc)
                  {
                  if (!ResRemove())
                     DisplayString("HSWITCH cannot be removed!\r\n");
                  else
                     DisplayString("HSWITCH removed from memory!\r\n");
                  exit(1);
                  }
               break;
            case '?':
               DisplayString("USAGE: HSWITCH [/Hnn] [/Tnn] [/R]\r\n");
               exit(1);

            case 'h':
            case 'H':
               wHotKey = RES_ALT | IntToStr(&pszArg[iArg][2]);
               fAltHotKey = TRUE;
               break;
            case 't':
            case 'T':
               usWaitTime = atoi(&pszArg[iArg][2]);
               break;

            default :
               DisplayString ("Unknown option specified!\r\n");
               exit(1);
            }
         }
      else
         {
         DisplayString ("Unknown option specified!\r\n");
         exit(1);
         }
      }


   if (rFunc)
      {
      DisplayString("HSWITCH for DOS already loaded.\r\n");
      DisplayString("Use HSWITCH /R to unload.\r\n");
      }
   else
      {
      iHandle = open("\\PIPE\\HSWITCH", O_WRONLY|O_BINARY);
      if (iHandle < 0)
         {
         DisplayString("Unable to communicate with OS/2 Version of HSWITCH!\r\n");
         exit(1);
         }
      else
         close(iHandle);
      if (!fAltHotKey)
         DisplayString("HSWITCH installed - HotKey = <right-alt>/\r\n");
      else
         DisplayString("HSWITCH installed - Alternate hotkey\r\n");
      ResInstall(wHotKey, Switch, FingerPrint);
      }

}

/********************************************************************
* The Switch func
********************************************************************/
WORD _far Switch(WORD ax, WORD bx, WORD cx, WORD dx)
{
int iHandle;
BYTE bRows = *pbRows;
BYTE bCols;
BOOL fTitleFound = FALSE;

   bx = bx;
   cx = cx;
   dx = dx;

   if (ax)
      return 0;

   if (fIsWindow())
      return 0;

   memset(&DosParms, 0, sizeof DosParms);

   if (bRows < 24)
      bRows = 24;
   DosParms.bScreenHeight = (BYTE)(bRows + 1);

   _asm mov ah, 0Fh;
   _asm int 10h;
   _asm mov bCols, ah;

   DosParms.bScreenWidth = bCols;

   GetSessionTitle(DosParms.szOrigTitle);
   if (!strlen(DosParms.szOrigTitle))
      strcpy(DosParms.szOrigTitle, "Current DOS");
   else
      fTitleFound = TRUE;
   strcpy(DosParms.szCurTitle, DOSNAME);
   SetSessionTitle(DOSNAME);

   iHandle = open("\\PIPE\\HSWITCH", O_WRONLY|O_BINARY);
   if (iHandle > 0)
      {
      write(iHandle, &DosParms, sizeof DosParms);
      close(iHandle);
      sleep(usWaitTime);
      }

   if (fTitleFound)
      SetSessionTitle(DosParms.szOrigTitle);
   else
      SetSessionTitle("");
   return 0;
}


void sleep(USHORT usDur)
{
ULONG usTime;
union REGS r;

    _enable();

    usTime = usDur;
    usTime *= 1000;
    r.h.ah = 0x86; /* wait */
    r.x.cx = (WORD) (usTime / 0xffff);
    r.x.dx = (WORD) (usTime % 0xffff);
    int86(0x15, &r, &r);
}


/********************************************************************
*
********************************************************************/
void DisplayString(PBYTE s)
{
union REGS r;
short sLen= 0;

   while (*s)
      {
      r.h.ah=0x0E;
      r.h.bh=0;
      r.h.al=*s++;
      int86(0x10,&r, &r);
      sLen++;
      }
}


/********************************************************************
*
********************************************************************/
INT IntToStr(PSZ pszStr)
{
INT iInt = 0;

   while (*pszStr)
      {
      iInt = (iInt * 10) + pszStr[0] - '0';
      pszStr++;
      }
   return iInt;
}

/*******************************************************************
* Set DOS VDM session title
*******************************************************************/
USHORT SetSessionTitle(BYTE _far * pszTitle)
{
union REGS r;
struct SREGS s;

   r.h.ah = 0x64;      /* OS/2 gate       */
   r.x.cx = 0x636C;    /* magic number    */
   r.x.bx = 0x0000;    /* special request */
   r.x.dx = 0x0001;    /* request number  */
   s.es   = FP_SEG(pszTitle);
   r.x.di = FP_OFF(pszTitle);
   intdosx(&r, &r, &s);
   if (r.x.cflag)
      return r.x.ax;
   return 0;
}

/*******************************************************************
* Set DOS VDM session title
*******************************************************************/
USHORT GetSessionTitle(BYTE _far * pszTitle)
{
union REGS r;
struct SREGS s;

   r.h.ah = 0x64;      /* OS/2 gate       */
   r.x.cx = 0x636C;    /* magic number    */
   r.x.bx = 0x0000;    /* special request */
   r.x.dx = 0x0002;    /* request number  */
   s.es   = FP_SEG(pszTitle);
   r.x.di = FP_OFF(pszTitle);
   intdosx(&r, &r, &s);
   if (r.x.cflag)
      return r.x.ax;
   return 0;
}

BOOL fIsWindow(VOID)
{
static BOOL fWindow = 0;

   _asm
      {
      MOV       DX,3D6h
      MOV       AL,82h
      OUT       DX,AL
      IN        AL,DX
      XOR       AH,AH
      MOV       fWindow, AX
      }
   return !fWindow;
}
