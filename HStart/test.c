#include <stdio.h>
#include <dos.h>

#include "portable.h"

USHORT usCreateSem(PSZ pszSemName, PULONG puHandle, WORD wAttr, BYTE fState);


int main(void)
{

   usCreateSem("\\SEM32\\HEKSEM", &Handle, 0, 0);

}

USHORT usCreateSem(PSZ pszSemName, PULONG puHandle, WORD wAttr, BYTE fState)
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

   _asm mov  si, wOffName;
   _asm mov  ax, wSegName;
   _asm mov  ds, ax;

   _asm mov  di, wOffHandle;
   _asm mov  ax, wOffSeg;
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
