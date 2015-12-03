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
#include "os2hek.h"

#define MAX_ICONS 20
typedef struct _IconInfo
{
USHORT usDeviceCX;
USHORT usDeviceCY;
USHORT usType;
USHORT usCX;
USHORT usCY;
USHORT usColors;
PBITMAPARRAYFILEHEADER2 pbafh2;
} ICONDATA, *PICONDATA;


static BOOL GetPointerBitmaps(HWND hwnd,
                       PBYTE pchIcon,
                       PBITMAPARRAYFILEHEADER2 pbafh2,
                       HBITMAP *phbmPointer,
                       HBITMAP *phbmColor);

static PBITMAPFILEHEADER2 pGetNextBitmap(PBITMAPFILEHEADER2 pbfh2);
static USHORT usGetColorsInBitmap(PBITMAPINFOHEADER2 pbmp2);


/***************************************************************
* Read an icon
***************************************************************/
HPOINTER _System ReadIcon(HWND hwnd, PSZ pszIcon, USHORT usIconSize)
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

#define PRINT

/*******************************************************************
* Buffer to Icon
*******************************************************************/
HPOINTER _System Buffer2Icon(HWND hwnd, PBYTE pchIcon, USHORT usIconSize)
{
static USHORT usDeviceCX = 0;
static USHORT usDeviceCY = 0;
static ICONDATA rgIcons[MAX_ICONS];
USHORT usIconCount,
       usIndex,
       usTry,
       usWantedType1,
       usWantedType2,
       usWantedColors;
BOOL   fIconFound;
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
            &PointerInfo.hbmColor))
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
      Build an array of all icons present in array
   */

   for (usIconCount = 0; !fIconFound && usIconCount < MAX_ICONS;)
      {
      PBITMAPFILEHEADER2 pbfh2;
      PBITMAPINFOHEADER2 pbmp2= &pbafh2->bfh2.bmp2;
      PBITMAPINFOHEADER pbmp = (PBITMAPINFOHEADER)&pbafh2->bfh2.bmp2;

      rgIcons[usIconCount].pbafh2     = pbafh2;
      rgIcons[usIconCount].usDeviceCX = pbafh2->cxDisplay;
      rgIcons[usIconCount].usDeviceCY = pbafh2->cyDisplay;
      rgIcons[usIconCount].usType     = pbafh2->bfh2.usType;

      pbfh2 = pGetNextBitmap(&pbafh2->bfh2);
      if (pbfh2)
         rgIcons[usIconCount].usColors = usGetColorsInBitmap(&pbfh2->bmp2);
      else
         rgIcons[usIconCount].usColors = usGetColorsInBitmap(pbmp2);

      if (pbmp2->cbFix == sizeof (BITMAPINFOHEADER2))
         {
         rgIcons[usIconCount].usCX = pbmp2->cx;
         rgIcons[usIconCount].usCY = pbmp2->cy;
         }
      else if (pbmp->cbFix == sizeof (BITMAPINFOHEADER))
         {
         rgIcons[usIconCount].usCX = pbmp->cx;
         rgIcons[usIconCount].usCY = pbmp->cy;
         }
      else
         continue;
#ifdef PRINT
      printf("Icon for device %d x %d - Icon %d x %d %2.2s, %d colors\n",
            (INT)rgIcons[usIconCount].usDeviceCX,
            (INT)rgIcons[usIconCount].usDeviceCY,
            (INT)rgIcons[usIconCount].usCX,
            (INT)rgIcons[usIconCount].usCY,
            &rgIcons[usIconCount].usType,
            (INT)rgIcons[usIconCount].usColors);

#endif
      usIconCount++;

      p = (PBYTE)pchIcon + pbafh2->offNext;
      if (!pbafh2->offNext)
            break;
      pbafh2 = (PBITMAPARRAYFILEHEADER2)p;
      }

   if (!fIconFound)
      {
      if (!usIconCount)
         return (HPOINTER)0;

      /*
         First see if the icon contains a color icon for the current device
         with the proper size.
      */
      usTry = 0;
      usWantedType1 = BFT_COLORICON;
      usWantedType2 = BFT_COLORPOINTER;
      usWantedColors = 16;
#ifdef PRINT
      printf("Looking for icon with CX = %d\n",
         (INT)usIconSize);
#endif
      }

   while (!fIconFound && usTry < 7)
      {
      for (usIndex = 0; !fIconFound && usIndex < usIconCount; usIndex++)
         {
         if (rgIcons[usIndex].usDeviceCX != usDeviceCX)
            continue;
         if (rgIcons[usIndex].usDeviceCY != usDeviceCY)
            continue;
         if (rgIcons[usIndex].usType != usWantedType1 &&
            rgIcons[usIndex].usType  != usWantedType2)
            continue;
         if (usWantedColors && rgIcons[usIndex].usColors != usWantedColors)
            continue;
         if (usIconSize && rgIcons[usIndex].usCX != usIconSize)
            continue;

         if (GetPointerBitmaps(hwnd,
            pchIcon,
            rgIcons[usIndex].pbafh2,
            &PointerInfo.hbmPointer,
            &PointerInfo.hbmColor))
            {
#ifdef PRINT
            printf("Try #%d:using #%d\n",
               (INT)usTry,
               (INT)usIndex + 1);
#endif
            fIconFound = TRUE;
            break;
            }
         else
            {
#ifdef PRINT
            printf("Try #%d:using #%d FAILED\n",
               (INT)usTry,
               (INT)usIndex + 1);
#endif
            }
         }

      if (!fIconFound)
         {
         switch (usTry)
            {
            case 0: /* No device specific icon found */
               usDeviceCX = usDeviceCY = 0;
               break;
            case 1: /* No icon found with 16 colors, try 2 colors */
               usWantedColors = 0;
               break;
            case 2: /* No color pointer found try black and white */
               usWantedType1 = BFT_POINTER;
               usWantedType2 = BFT_ICON;
               break;
            case 3: /* No device independ B&W icon found, try no size */
               usDeviceCX = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
               usDeviceCY = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);
               usWantedType1 = BFT_COLORICON;
               usWantedType2 = BFT_COLORPOINTER;
               usWantedColors = 16;
               usIconSize = 0;
               break;
            case 4:
               usWantedColors = 0;
               break;
            case 5: /* No device specific icon found */
               usDeviceCX = usDeviceCY = 0;
               break;
            case 6: /* No color pointer found try black and white */
               usWantedType1 = BFT_POINTER;
               usWantedType2 = BFT_ICON;
               break;
            }
         usTry++;
         }

      }

   if (!fIconFound)
      return (HPOINTER)0L;

   PointerInfo.fPointer  = FALSE;
   PointerInfo.xHotspot   = 0;
   PointerInfo.yHotspot   = 0;
   if (PointerInfo.hbmColor)
      hptrIcon = WinCreatePointerIndirect(HWND_DESKTOP, &PointerInfo);
   else
      hptrIcon = WinCreatePointer(HWND_DESKTOP,
         PointerInfo.hbmPointer,
         FALSE, 0, 0);

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
                       HBITMAP *phbmColor)
{
HPS   hps;
PBITMAPFILEHEADER2 pbfh2;
PBITMAPINFOHEADER2 pbmp2;

   *phbmPointer = NULLHANDLE;
   *phbmColor   = NULLHANDLE;

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


   if (pbfh2->usType != BFT_COLORICON &&
       pbfh2->usType != BFT_COLORPOINTER)
      {
      return TRUE;
      }


   pbfh2 = pGetNextBitmap(pbfh2);
   if (!pbfh2)
      return TRUE;
   pbmp2 = &pbfh2->bmp2;

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


/*
   If it is a color icon than another BITMAPFILEHEADER follow after
   the color information. This color information contains of a number
   of RGB or RGB2 structures. The number depends of the number of colors
   in the bitmap. The number of colors is calculated by looking at
   the Number of bits per pel and using this number as an exponent on 2.
*/
PBITMAPFILEHEADER2 pGetNextBitmap(PBITMAPFILEHEADER2 pbfh2)
{
PBITMAPINFOHEADER2 pbmp2;
PBYTE    p;
USHORT   usRGB;

   if (pbfh2->usType != BFT_COLORICON &&
       pbfh2->usType != BFT_COLORPOINTER)
      return NULL;

   pbmp2 = &pbfh2->bmp2;
   if (pbmp2->cbFix == sizeof (BITMAPINFOHEADER2))
      usRGB = sizeof (RGB2);
   else if (pbmp2->cbFix == sizeof (BITMAPINFOHEADER))
      usRGB = sizeof (RGB);
   else  /* Unknown length found */
      return NULL;


   p = (PBYTE)(pbfh2) +
      (pbfh2->cbSize + (usGetColorsInBitmap(pbmp2) * usRGB));

   return (PBITMAPFILEHEADER2)p;
}


USHORT usGetColorsInBitmap(PBITMAPINFOHEADER2 pbmp2)
{
PBITMAPINFOHEADER  pbmp;
USHORT   usColors,
         usExp,
         usBitsPerPel;

   if (pbmp2->cbFix == sizeof (BITMAPINFOHEADER2))
      usBitsPerPel = pbmp2->cBitCount * pbmp2->cPlanes;
   else if (pbmp2->cbFix == sizeof (BITMAPINFOHEADER))
      {
      pbmp = (PBITMAPINFOHEADER)pbmp2;
      usBitsPerPel = pbmp->cBitCount * pbmp->cPlanes;
      }
   else  /* Unknown length found */
      return 0;

   /*
      Calculate beginning of BITMAPFILEHEADER structure
      2^Bits_per_pel
   */
   for (usColors = 1, usExp = 0; usExp < usBitsPerPel; usExp++)
      usColors *= 2;

   return usColors;
}


