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
#include <os2.h>

#include "icon.h"
#include "icondef.h"


static DEVOPENSTRUC dop = {NULL, "DISPLAY", NULL, NULL, NULL, NULL, NULL, NULL, NULL};

 

BOOL GetIconFromPointer(HAB hab, HPOINTER hptr, PSZ pszFileName)
{
HDC hdcMem;
HPS hpsMem;
SIZEL sizel;
POINTERINFO pinfo;
BITMAPINFOHEADER bmp;
BITMAPARRAYFILEHEADER bafh;
PBITMAPINFO           pbmpPointer, pbmpColor;
PBYTE                 pbBufPointer, pbBufColor;
ULONG                 cbBufPointer, cbBufColor,
                      cbRGBPointer, cbRGBColor;
INT   iHandle;
USHORT usExtra, usExp;

   sizel.cx = 0;
   sizel.cy = 0;

   /*
      Query pointerinformation
   */

   if (!WinQueryPointerInfo(hptr, &pinfo))
      {
      DebugBox("ICONTOOL","WinQueryPointerInfo", TRUE);
      return FALSE;
      }


   /*
      Create memory device context
   */
   hdcMem = DevOpenDC(hab, OD_MEMORY, "*", 5L,(PDEVOPENDATA)&dop, NULLHANDLE);
   if (hdcMem == DEV_ERROR)
      {
      DebugBox("ICONTOOL","DevOpenDC", TRUE);
      return FALSE;
      }

   /*
      Create presentation space
   */
   hpsMem = GpiCreatePS(hab, hdcMem, &sizel, GPIA_ASSOC | PU_PELS);
   if (hpsMem == GPI_ERROR)
      {
      DebugBox("ICONTOOL","GpiCreatePS", TRUE);
      DevCloseDC(hdcMem);
      return FALSE;
      }


   /*
      Query bitmapInfoHeader
   */
   bmp.cbFix = sizeof bmp;
   if (!GpiQueryBitmapParameters(pinfo.hbmPointer, &bmp))
      {
      DebugBox("ICONTOOL","GpiQueryBitmapHeader", TRUE);
      return FALSE;
      }

   cbBufPointer = (((bmp.cBitCount * bmp.cx) + 31) / 32)
      * 4 * bmp.cy * bmp.cPlanes;
   for (usExtra = 1, usExp = 0; usExp < bmp.cBitCount; usExp++)
      usExtra *= 2;
   cbRGBPointer = sizeof(RGB) * usExtra;

   pbBufPointer = malloc(cbBufPointer);
   pbmpPointer   = (PBITMAPINFO)malloc(sizeof (BITMAPINFO) + cbRGBPointer);
   memcpy(pbmpPointer, &bmp, sizeof bmp);

   if (GpiSetBitmap(hpsMem, pinfo.hbmPointer)==HBM_ERROR)
      {
      DebugBox("ICONTOOL","GpiSetBitmap", TRUE);
      GpiDestroyPS(hpsMem);
      DevCloseDC(hdcMem);
      free(pbBufPointer);
      free(pbmpPointer);
      return FALSE;
      }

   if (GpiQueryBitmapBits(hpsMem, 0L,
      (LONG) bmp.cy, pbBufPointer, (PBITMAPINFO2)pbmpPointer) == GPI_ALTERROR)
      {
      DebugBox("ICONTOOL","GpiSetBitmap", TRUE);
      GpiDestroyPS(hpsMem);
      DevCloseDC(hdcMem);
      free(pbBufPointer);
      free(pbmpPointer);
      return FALSE;
      }





   bmp.cbFix = sizeof bmp;
   if (!GpiQueryBitmapParameters(pinfo.hbmColor, &bmp))
      {
      DebugBox("ICONTOOL","GpiQueryBitmapHeader", TRUE);
      return FALSE;
      }

   cbBufColor = (((bmp.cBitCount * bmp.cx) + 31) / 32)
      * 4 * bmp.cy * bmp.cPlanes;
   for (usExtra = 1, usExp = 0; usExp < bmp.cBitCount; usExp++)
      usExtra *= 2;
   cbRGBColor = sizeof(RGB) * usExtra;

   pbBufColor = malloc(cbBufColor);
   pbmpColor   = (PBITMAPINFO)malloc(sizeof (BITMAPINFO) + cbRGBColor);
   memcpy(pbmpColor, &bmp, sizeof bmp);

   if (GpiSetBitmap(hpsMem, pinfo.hbmColor)==HBM_ERROR)
      {
      DebugBox("ICONTOOL","GpiSetBitmap", TRUE);
      GpiDestroyPS(hpsMem);
      DevCloseDC(hdcMem);
      free(pbBufPointer);
      free(pbmpPointer);
      free(pbBufColor);
      free(pbmpColor);
      return FALSE;
      }

   if (GpiQueryBitmapBits(hpsMem, 0L,
      (LONG) bmp.cy, pbBufColor, (PBITMAPINFO2)pbmpColor) == GPI_ALTERROR)
      {
      DebugBox("ICONTOOL","GpiSetBitmap", TRUE);
      GpiDestroyPS(hpsMem);
      DevCloseDC(hdcMem);
      free(pbBufPointer);
      free(pbmpPointer);
      free(pbBufColor);
      free(pbmpColor);
      return FALSE;
      }

   iHandle = open(pszFileName, O_RDWR | O_BINARY | O_TRUNC | O_CREAT, S_IWRITE | S_IREAD);
   if (iHandle == -1)
      {
      DebugBox("ICONTOOL", "Unable to create icon file !", FALSE);
      GpiDestroyPS(hpsMem);
      DevCloseDC(hdcMem);
      free(pbBufPointer);
      free(pbmpPointer);
      free(pbBufColor);
      free(pbmpColor);
      return FALSE;
      }

   /*
      Prepare BITMAPARRAYFILEHEADER structure
   */
   memset(&bafh, 0, sizeof bafh);
   bafh.usType = BFT_BITMAPARRAY;
   bafh.cbSize = sizeof bafh;

   /*
      And the BITMAPFILEHEADER structure for the AND & XOR bitmaps
   */
   bafh.bfh.usType = BFT_COLORICON;
   bafh.bfh.cbSize = sizeof bafh.bfh;
   bafh.bfh.offBits = sizeof bafh +
                        sizeof bafh.bfh +
                        cbRGBPointer +
                        cbRGBColor;

   /*
      Copy BITMAPINFOHEADER into first BITMAPFILEHEADER and
      write BITMAPARRAYFILEHEADER which includes
      the first BITMAPFILEHEADER structure.
   */

   memcpy(&bafh.bfh.bmp, pbmpPointer, sizeof (BITMAPINFOHEADER));
   write(iHandle,(PBYTE)&bafh, sizeof bafh);

   /*
      Write RGB array
   */
   write(iHandle,(PBYTE)pbmpPointer->argbColor, cbRGBPointer);

   /*
      Prepare second BITMAPFILEHEADER structure for the COLOR bitmap
   */
   bafh.bfh.usType = BFT_COLORICON;
   bafh.bfh.cbSize = sizeof bafh.bfh;
   bafh.bfh.offBits = sizeof bafh +
                        sizeof bafh.bfh +
                        cbRGBPointer +
                        cbRGBColor +
                        cbBufPointer;

   /*
      Copy BITMAPINFOHEADER
   */
   memcpy(&bafh.bfh.bmp, pbmpColor, sizeof (BITMAPINFOHEADER));
   write(iHandle,(PBYTE)&bafh.bfh, sizeof bafh.bfh);
   /*
      Write RGB array
   */
   write(iHandle,(PBYTE)pbmpColor->argbColor, cbRGBColor);

   /*
      Write AND & XOR bits
   */
   write(iHandle, (PBYTE)pbBufPointer, cbBufPointer);

   /*
      Write COLOR bits
   */
   write(iHandle, (PBYTE)pbBufColor, cbBufColor);

   close(iHandle);


   GpiDestroyPS(hpsMem);
   DevCloseDC(hdcMem);

   free(pbBufColor);
   free(pbmpColor);
   free(pbBufPointer);
   free(pbmpPointer);

   return TRUE;

}

