#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <share.h>
#include <sys\types.h>
#include <sys\stat.h>

#define INCL_GPI
#define INCL_WIN
#include <os2.h>

#include "eabrowse.h"

static BOOL GetPointerBitmaps(HWND hwnd,
                       PBYTE pchIcon,
                       PBITMAPARRAYFILEHEADER2 pbafh2,
                       HBITMAP *phbmPointer,
                       HBITMAP *phbmColor,
                       USHORT usIconSize);

/***************************************************************
* Read an icon
***************************************************************/
HPOINTER ReadIcon(HWND hwnd, PSZ pszIcon, USHORT usIconSize)
{
INT   iHandle;
ULONG ulSize;
PBYTE  pchIcon;
HPOINTER hptrIcon;

   /*
      Read the icon file
   */

   iHandle = sopen(pszIcon, O_RDONLY|O_BINARY, SH_DENYNO);
   if (iHandle < 0)
      return (HPOINTER)NULL;

   /*
      Get the size of the icon file and allocate memory for it
   */
   ulSize = filelength(iHandle);
   pchIcon = (PBYTE)malloc((size_t)ulSize);
   if (!pchIcon)
      {
      close (iHandle);
      return (HPOINTER)NULL;
      }

   /*
      Read the icon
   */
   if (read(iHandle, pchIcon, (unsigned INT)(ulSize)) != (INT)ulSize)
      {
      close(iHandle);
      free(pchIcon);
      return (HPOINTER)NULL;
      }

   hptrIcon = Buffer2Icon(hwnd, pchIcon, usIconSize);

   free(pchIcon);

   close(iHandle);
   return hptrIcon;
}

/*******************************************************************
* Buffer to Icon
*******************************************************************/
HPOINTER Buffer2Icon(HWND hwnd, PBYTE pchIcon, USHORT usIconSize)
{
static USHORT usDeviceCX = 0;
static USHORT usDeviceCY = 0;
BOOL fContinue,
     fIconFound;
POINTERINFO PointerInfo;
PBITMAPARRAYFILEHEADER2 pbafh2;
PBYTE p;
HPOINTER hptrIcon = (HPOINTER)0;

   memset(&PointerInfo, 0, sizeof PointerInfo);
           
   if (!usDeviceCX)
      {
      usDeviceCX = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
      usDeviceCY = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
      }

   fIconFound = FALSE;
   pbafh2 = (PBITMAPARRAYFILEHEADER2)pchIcon;

   switch (pbafh2->usType)
      {
      case BFT_BITMAPARRAY    :
         break;
      case BFT_ICON           :
      case BFT_BMAP           :
      case BFT_POINTER        :
      case BFT_COLORICON      :
      case BFT_COLORPOINTER   :
         if (GetPointerBitmaps(hwnd,
            pchIcon,
            pbafh2,
            &PointerInfo.hbmPointer,
            &PointerInfo.hbmColor,
            0))
            {
            fIconFound = TRUE;
            }
         else
            return (HPOINTER)0;
         break;
      default :
         return (HPOINTER)0;
      }

   /*
      First see if the icon contains an icon for the current device
      size.
   */
   fContinue = TRUE;
   while (!fIconFound && fContinue)
      {
      if (pbafh2->cxDisplay == usDeviceCX &&
          pbafh2->cyDisplay == usDeviceCY)
         {
         if (GetPointerBitmaps(hwnd,
            pchIcon,
            pbafh2,
            &PointerInfo.hbmPointer,
            &PointerInfo.hbmColor,
            usIconSize))
            {
            fIconFound = TRUE;
            break;
            }
         }
      p = (PBYTE)pchIcon + pbafh2->offNext;
      if (!pbafh2->offNext)
            break;
      pbafh2 = (PBITMAPARRAYFILEHEADER2)p;
      }

   /*
      Now look for the independed icons
   */
   if (!fIconFound)
      {
      pbafh2 = (PBITMAPARRAYFILEHEADER2)pchIcon;
      fContinue = TRUE;
      while (fContinue)
         {
         if (pbafh2->cxDisplay == 0 &&
            pbafh2->cyDisplay == 0)
            {
            if (GetPointerBitmaps(hwnd,
               pchIcon,
               pbafh2,
               &PointerInfo.hbmPointer,
               &PointerInfo.hbmColor,
               usIconSize))
               {
               fIconFound = TRUE;
               break;
               }
            }
         p = (PBYTE)pchIcon + pbafh2->offNext;
         if (!pbafh2->offNext)
            break;
         pbafh2 = (PBITMAPARRAYFILEHEADER2)p;
         }
      }

   /*
      if we still haven't found an icon we take the first icon there is
   */
   if (!fIconFound)
      {
      pbafh2 = (PBITMAPARRAYFILEHEADER2)pchIcon;
      if (GetPointerBitmaps(hwnd,
         pchIcon,
         pbafh2,
         &PointerInfo.hbmPointer,
         &PointerInfo.hbmColor,
         0))
         fIconFound = TRUE;
      }

   if (!fIconFound)
      return (HPOINTER)0L;

   PointerInfo.fPointer  = FALSE;
   PointerInfo.xHotspot   = 0;
   PointerInfo.yHotspot   = 0;
   hptrIcon = WinCreatePointerIndirect(HWND_DESKTOP, &PointerInfo);
//   if (!hptrIcon)
//      DebugBox("ICONTOOL","Error on WinCreatePointerIndirect !", TRUE);

   if (PointerInfo.hbmPointer)
      GpiDeleteBitmap(PointerInfo.hbmPointer);
   if (PointerInfo.hbmColor)
      GpiDeleteBitmap(PointerInfo.hbmColor);

   return hptrIcon;

}

/**********************************************************************
* Get pointer bitmaps based on a icon size
**********************************************************************/
BOOL GetPointerBitmaps(HWND hwnd,
                       PBYTE pchIcon,
                       PBITMAPARRAYFILEHEADER2 pbafh2,
                       HBITMAP *phbmPointer,
                       HBITMAP *phbmColor,
                       USHORT usIconSize)
{
HPS   hps;
USHORT usBitCount, usRGB;
PBITMAPFILEHEADER2 pbfh2;
PBITMAPINFOHEADER2 pbmp2;
USHORT usExtra, usExp;
PBYTE  p;

   *phbmPointer = (HBITMAP)0;
   *phbmColor   = (HBITMAP)0;

   /*
      Is it the correct icon type ?
   */
   switch (pbafh2->usType)
      {
      case BFT_BITMAPARRAY    :
         pbfh2 = &pbafh2->bfh2;
         break;

      case BFT_ICON           :
      case BFT_BMAP           :
      case BFT_POINTER        :
      case BFT_COLORICON      :
      case BFT_COLORPOINTER   :
         pbfh2 = (PBITMAPFILEHEADER2)pbafh2;
         break;
      default :
         return FALSE;
      }
   pbmp2 = &pbfh2->bmp2;

      /*
         Is it a BITMAPINFOHEADER or BITMAPINFOHEADER2 ?
      */
   if (pbmp2->cbFix == sizeof (BITMAPINFOHEADER2))
      {
      usRGB = sizeof (RGB2);
      usBitCount = pbmp2->cBitCount;
      if (usIconSize && pbmp2->cx != usIconSize)
         return FALSE;
      }
   else if (pbmp2->cbFix == sizeof (BITMAPINFOHEADER))
      {
      PBITMAPINFOHEADER pbmp = (PBITMAPINFOHEADER)pbmp2;
      usRGB = sizeof (RGB);
      usBitCount = pbmp->cBitCount;
      if (usIconSize && pbmp->cx != usIconSize)
         return FALSE;
      }
   else  /* Unknown length found */
      return FALSE;

   /*
      Create the first pointer by getting the presentation space first
      and than call GpiCreateBitmap
   */

   hps = WinGetPS(hwnd);
   *phbmPointer = GpiCreateBitmap(hps,
      pbmp2,
      CBM_INIT,
      (PBYTE)pchIcon + pbfh2->offBits,
      (PBITMAPINFO2)pbmp2);

   if (*phbmPointer == GPI_ERROR)
      {
      WinReleasePS(hps);
      return FALSE;
      }
   WinReleasePS(hps);

   /*
      If it is a color icon than another BITMAPFILEHEADER follow after
      the color information. This color information contains of a number
      of RGB or RGB2 structures. The number depends of the number of colors
      in the bitmap. The number of colors is calculated by looking at
      the Number of bits per pel and using this number as an exponent on 2.
   */

   if (pbfh2->usType != BFT_COLORICON &&
       pbfh2->usType != BFT_COLORPOINTER)
      return TRUE;


   /*
      Calculate beginning of BITMAPFILEHEADER structure
      2^Bits_per_pel
   */
   for (usExtra = 1, usExp = 0; usExp < usBitCount; usExp++)
      usExtra *= 2;

   p = (PBYTE)(pbfh2) +
      (pbfh2->cbSize + usExtra * usRGB);
   pbfh2 = (PBITMAPFILEHEADER2)p;
   /*
      Get adress of BITMAPINFOHEADER
   */
   pbmp2 = &pbfh2->bmp2;

   if (pbmp2->cbFix == sizeof (BITMAPINFOHEADER2))
      {
      if (pbmp2->cBitCount == 1)
         return TRUE;
      }
   else if (pbmp2->cbFix == sizeof (BITMAPINFOHEADER))
      {
      PBITMAPINFOHEADER pbmp = (PBITMAPINFOHEADER)pbmp2;
      if (pbmp->cBitCount == 1)
         return TRUE;
      }
   else  /* Unknown length found */
      return TRUE;

   /*
      And create bitmap number 2
   */

   hps = WinGetPS(hwnd);
   *phbmColor = GpiCreateBitmap(hps,
      pbmp2,
      CBM_INIT,
      (PBYTE)pchIcon + pbfh2->offBits,
      (PBITMAPINFO2)pbmp2);
   if (*phbmColor == GPI_ERROR)
      {
      GpiDeleteBitmap(*phbmPointer);
      return FALSE;
      }
   WinReleasePS(hps);
   return TRUE;
}

