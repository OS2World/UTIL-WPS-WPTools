#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <direct.h>

#define INCL_VIO
#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_GPI
#include <os2.h>
#include <wpfolder.h>

#include "wptools.h"

BOOL GetFolderPos(HOBJECT hFolder, PSZ pszOptions);



/*******************************************************************
* Get Folderoptions
*******************************************************************/
BOOL _System GetWPFolderOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
BYTE szOptionText[256];
ULONG ulIconView = FOLDER_DEFATTRS;
ULONG ulTreeView = FOLDER_DEFTREEATTRS;
USHORT usFontTag,
       usNewFont;
ULONG  ulFolderFlag;
WPFOLDATA wpData;
FOLDERSORT fSort;
FLDBKGND   fBackGround;
BOOL   bNewFormat = FALSE;
USHORT usIndex;
//USHORT usDataSize;
ULONG  ulAllInTreeView;

   if (GetObjectValue(pObjectData, WPFOLDER_DATA, &wpData, sizeof wpData))
      bNewFormat = TRUE;
//   usDataSize = GetObjectValueSize(pObjectData, WPFOLDER_DATA);


   /*
      Get ICONVIEW setting
   */
   usFontTag = 0;
   ulIconView = 0;
   memset(szOptionText, 0, sizeof szOptionText);
   if (bNewFormat)
      ulIconView = wpData.ulIconView;
   else
      GetObjectValue(pObjectData, 
         IDKEY_FDRCONTENTATTR, &ulIconView, sizeof ulIconView);

   if (ulIconView)
      {
      if (ulIconView & CV_ICON)
         {
         strcpy(szOptionText, "NONGRID");
         usFontTag = IDKEY_FDRCVIFONT;
         usNewFont = 2;
         }
      else
         {
         if (ulIconView & CV_FLOW)
            strcpy(szOptionText, "FLOWED");
         else
            strcpy(szOptionText, "NONFLOWED");
         /*
            Icons visible or invisible
         */
         if (ulIconView & CV_TEXT)
            {
            usFontTag = IDKEY_FDRCVLFONT;
            usNewFont = 0;
            strcat(szOptionText, ",INVISIBLE");
            }
         else
            {
            strcat(szOptionText, ",VISIBLE");
            usFontTag = IDKEY_FDRCVNFONT;
            usNewFont = 1;
            }
         }

      /*
         Small or normal icons
      */
      if (ulIconView & CV_MINI)
         strcat(szOptionText, ",MINI");
      else
         strcat(szOptionText, ",NORMAL");

      if (ulIconView != FOLDER_DEFATTRS && *szOptionText)
         {
         strcat(pszOptions, "ICONVIEW=");
         strcat(pszOptions, szOptionText);
         strcat(pszOptions, ";");
         }
      }

   /*
      Get IconView font
   */
   if (usFontTag)
      {
      memset(szOptionText, 0, sizeof szOptionText);
      if (bNewFormat)
         GetObjectValueSubValue(pObjectData, WPFOLDER_FONTS, usNewFont,
            szOptionText, sizeof szOptionText);
      else
         GetObjectValue(pObjectData, 
           usFontTag, szOptionText, sizeof szOptionText);
      if (*szOptionText)
         {
         strcat(pszOptions, "ICONFONT=");
         strcat(pszOptions, szOptionText);
         strcat(pszOptions, ";");
         }
      }


   /*
      Get TREEVIEW setting
   */
   memset(szOptionText, 0, sizeof szOptionText);
   ulTreeView = 0;
   if (bNewFormat)
      ulTreeView = wpData.ulTreeView;
   else
      GetObjectValue(pObjectData,
         IDKEY_FDRTREEATTR, &ulTreeView, sizeof ulTreeView);
   if (ulTreeView)
      {
      if (ulTreeView & CA_TREELINE)
         strcpy(szOptionText, "LINES");
      else
         strcpy(szOptionText, "NOLINES");

      /*
         Icons visible or invisible
      */
      if (ulTreeView & CV_TEXT)
         strcat(szOptionText, ",INVISIBLE");
      else
         {
         strcat(szOptionText, ",VISIBLE");
         /*
            Small or normal icons
         */
         if (ulTreeView & CV_MINI)
            strcat(szOptionText, ",MINI");
         else
            strcat(szOptionText, ",NORMAL");
         }

      if (ulTreeView != FOLDER_DEFTREEATTRS && *szOptionText)
         {
         strcat(pszOptions, "TREEVIEW=");
         strcat(pszOptions, szOptionText);
         strcat(pszOptions, ";");
         }
      }

   /*
      Get TreeView font
   */
   memset(szOptionText, 0, sizeof szOptionText);
   if (bNewFormat)
      GetObjectValueSubValue(pObjectData,  WPFOLDER_FONTS, 3,
         szOptionText, sizeof szOptionText);
   else
      GetObjectValue(pObjectData, 
         IDKEY_FDRTVLFONT, szOptionText, sizeof szOptionText);
   if (*szOptionText)
      {
      strcat(pszOptions, "TREEFONT=");
      strcat(pszOptions, szOptionText);
      strcat(pszOptions, ";");
      }

   /*
      Get DetailsView font
   */
   memset(szOptionText, 0, sizeof szOptionText);
   if (bNewFormat)
      GetObjectValueSubValue(pObjectData, WPFOLDER_FONTS, 5,
         szOptionText, sizeof szOptionText);
   else
      GetObjectValue(pObjectData,
         IDKEY_FDRDVFONT, szOptionText, sizeof szOptionText);
   if (*szOptionText)
      {
      strcat(pszOptions, "DETAILSFONT=");
      strcat(pszOptions, szOptionText);
      strcat(pszOptions, ";");
      }

   /*
      Get WorkArea
   */
   ulFolderFlag = 0;
   if (bNewFormat)
      ulFolderFlag = wpData.ulFolderFlag;
   else
      GetObjectValue(pObjectData,
         WPFOLDER_FOLDERFLAG, &ulFolderFlag, sizeof ulFolderFlag);
   if (ulFolderFlag)
      {
      if (ulFolderFlag & FOI_WORKAREA)
         strcat(pszOptions, "WORKAREA=YES;");
      }


   memset(szOptionText, 0, sizeof szOptionText);
   GetObjectValueSubValue(pObjectData, IDKEY_FDRSTRARRAY, 6,
      szOptionText, sizeof szOptionText);
   if (*szOptionText)
      {
      strcat(pszOptions, "DETAILSCLASS=");
      strcat(pszOptions, szOptionText);
      strcat(pszOptions, ";");
      }

   memset(szOptionText, 0, sizeof szOptionText);
   GetObjectValueSubValue(pObjectData, IDKEY_FDRSTRARRAY, 7,
      szOptionText, sizeof szOptionText);
   if (*szOptionText)
      {
      BYTE szText[5];
      BOOL fFirst = TRUE;
      strcat(pszOptions, "DETAILSTODISPLAY=");
      for (usIndex = 0; usIndex < strlen(szOptionText); usIndex++)
         {
         if (szOptionText[usIndex] == 0x20)
            {
            if (!fFirst)
               strcat(pszOptions, ",");
            fFirst = FALSE;
            sprintf(szText, "%d", usIndex);
            strcat(pszOptions, szText);
            }
         }
      strcat(pszOptions, ";");
      }

   /*
      GetSortOptions
   */

   memset(szOptionText, 0, sizeof szOptionText);
   GetObjectValueSubValue(pObjectData, IDKEY_FDRSTRARRAY, 8,
      szOptionText, sizeof szOptionText);
   if (*szOptionText)
      {
      strcat(pszOptions, "SORTCLASS=");
      strcat(pszOptions, szOptionText);
      strcat(pszOptions, ";");
      }

   memset(szOptionText, 0, sizeof szOptionText);
   GetObjectValueSubValue(pObjectData, IDKEY_FDRSTRARRAY, 9,
      szOptionText, sizeof szOptionText);
   if (*szOptionText)
      {
      BYTE szText[5];
      BOOL fFirst = TRUE;
      strcat(pszOptions, "SORTBYATTR=");
      for (usIndex = 0; usIndex < 12; usIndex++)
         {
         if (szOptionText[usIndex] == 0x20)
            {
            if (!fFirst)
               strcat(pszOptions, ",");
            fFirst = FALSE;
            sprintf(szText, "%d", usIndex);
            strcat(pszOptions, szText);
            }
         }
      strcat(pszOptions, ";");
      }




   if (GetObjectValue(pObjectData, 
      IDKEY_FDRSORTINFO, &fSort, sizeof fSort))
      {
      /*
         Sort Attribute
      */
      sprintf(szOptionText, "DEFAULTSORT=%ld;", fSort.lDefaultSortIndex);
      strcat(pszOptions, szOptionText);

      sprintf(szOptionText, "ALWAYSSORT=%s;",
         (fSort.fAlwaysSort ? "YES" : "NO"));
      strcat(pszOptions, szOptionText);
      }

   /*
      Get background
   */

   if (GetObjectValue(pObjectData, 
      IDKEY_FDRBACKGROUND, &fBackGround, sizeof fBackGround))
      {
      BYTE szImageFile[255];
      if (!GetObjectValue(pObjectData, 
         IDKEY_FDRBKGNDIMAGEFILE, szImageFile, sizeof szImageFile - 1))
         strcpy(szImageFile, "(none)");

      sprintf(szOptionText, "BACKGROUND=%s,", szImageFile);
      switch (fBackGround.bImageType)
         {
         case '3':
            strcat(szOptionText, "T,");
            break;
         case '4':
            strcat(szOptionText, "S,");
            break;
         default:
            strcat(szOptionText, "N,");
            break;
         }
      if (!fBackGround.bExtra)
         {
         sprintf(szOptionText + strlen(szOptionText),
            "%u,%c,%u %u %u;",
            fBackGround.bScaleFactor,
            (fBackGround.bColorOnly == 0x27 ? 'C' : 'I'),
            fBackGround.bRed,
            fBackGround.bGreen, 
            fBackGround.bBlue);
         }
      else
         {
         sprintf(szOptionText + strlen(szOptionText),
            "%u,%c;",
            fBackGround.bScaleFactor,
            (fBackGround.bColorOnly == 0x27 ? 'C' : 'I'));
         }

      strcat(pszOptions, szOptionText);
      }


   /* WARP 4 Stuff */

   if (bNewFormat && IsWarp4())
      {
      if (!wpData.ulMenuFlag)
         strcat(pszOptions, "MENUBAR=NO;");
      if (wpData.ulMenuFlag & 1)
         strcat(pszOptions, "MENUBAR=YES;");
      if (wpData.ulMenuFlag & 2)
         strcat(pszOptions, "MENUBAR=DEFAULT;");

      if (!(wpData.fIconViewFlags & 0x4000))
         {
         if (!wpData.fIconTextVisible)
            strcat(pszOptions, "ICONTEXTVISIBLE=NO;");
         }
      if (memcmp(wpData.rgbIconTextBkgnd, "\x00\x00\x00\x40", 4))
         {
         if (memcmp(wpData.rgbIconTextBkgnd+1, "\xFF\xFF\xFF", 3))
            {
            sprintf(pszOptions + strlen(pszOptions),
               "ICONTEXTBACKGROUNDCOLOR=%u %u %u;",
               wpData.rgbIconTextBkgnd[2],
               wpData.rgbIconTextBkgnd[1],
               wpData.rgbIconTextBkgnd[0]);
            }
         }
      if (memcmp(wpData.rgbIconTextColor, "\xEF\xFF\xFF\xFF", 4))
         {
         sprintf(pszOptions + strlen(pszOptions),
               "ICONTEXTCOLOR=%u %u %u;",
               wpData.rgbIconTextColor[2],
               wpData.rgbIconTextColor[1],
               wpData.rgbIconTextColor[0]);
         }
      if (memcmp(wpData.rgbIconShadowColor, "\xD0\xFF\xFF\xFF" ,4))
         {
         sprintf(pszOptions + strlen(pszOptions),
               "ICONSHADOWCOLOR=%u %u %u;",
               wpData.rgbIconShadowColor[2],
               wpData.rgbIconShadowColor[1],
               wpData.rgbIconShadowColor[0]);
         }

      if (!(wpData.fTreeViewFlags & 0x4000))
         {
         if (!wpData.fTreeTextVisible)
            strcat(pszOptions, "TREETEXTVISIBLE=NO;");
         }

      if (memcmp(wpData.rgbTreeTextColor, "\xEF\xFF\xFF\xFF", 4))
         {
         sprintf(pszOptions + strlen(pszOptions),
               "TREETEXTCOLOR=%u %u %u;",
               wpData.rgbTreeTextColor[2],
               wpData.rgbTreeTextColor[1],
               wpData.rgbTreeTextColor[0]);
         }
      if (memcmp(wpData.rgbTreeShadowColor, "\xD0\xFF\xFF\xFF" ,4))
         {
         sprintf(pszOptions + strlen(pszOptions),
               "TREESHADOWCOLOR=%u %u %u;",
               wpData.rgbTreeShadowColor[2],
               wpData.rgbTreeShadowColor[1],
               wpData.rgbTreeShadowColor[0]);
         }

      if (GetObjectValue(pObjectData, 2939, &ulAllInTreeView, sizeof ulAllInTreeView))
         {
         if (ulAllInTreeView)
            strcat(pszOptions, "SHOWALLINTREEVIEW=YES;");
         }

      if (memcmp(wpData.rgbDetailsTextColor, "\xDD\xFF\xFF\xFF", 4))
         {
         sprintf(pszOptions + strlen(pszOptions),
               "DETAILSTEXTCOLOR=%u %u %u;",
               wpData.rgbDetailsTextColor[2],
               wpData.rgbDetailsTextColor[1],
               wpData.rgbDetailsTextColor[0]);
         }

      if (memcmp(wpData.rgbDetailsShadowColor, "\xD0\xFF\xFF\xFF" ,4))
         {
         sprintf(pszOptions + strlen(pszOptions),
               "DETAILSSHADOWCOLOR=%u %u %u;",
               wpData.rgbDetailsShadowColor[2],
               wpData.rgbDetailsShadowColor[1],
               wpData.rgbDetailsShadowColor[0]);
         }

      }


   vGetFSysTitle(hObject, pszOptions);

   GetFolderPos(hObject, pszOptions);

   return TRUE;
}

/******************************************************************
*  GetFolderPos
******************************************************************/
BOOL GetFolderPos(HOBJECT hFolder, PSZ pszOptions)
{
BYTE  szObjectID[20];
ULONG ulProfileSize;
PBYTE pBuffer;
BYTE  szOptionText[30];
SHORT sX, sY, sCX, sCY;
ULONG   ulDesktopCX,
        ulDesktopCY;

   ulDesktopCX = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
   ulDesktopCY = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);

   sprintf(szObjectID, "%ld@10", hFolder);

   pBuffer = GetProfileData(FOLDERPOS, szObjectID, HINI_PROFILE, &ulProfileSize);
   if (pBuffer)
      {
      sX = *(PSHORT)(pBuffer + 18);
      if (sX < 0)
         sX = 0;
      sY = *(PSHORT)(pBuffer + 20);
      if (sY < 0)
         sY = 0;

      sCX = *(PSHORT)(pBuffer + 22);
      sCY = *(PSHORT)(pBuffer + 24);

      sX = sX * 100   / ulDesktopCX;
      sY = sY * 100   / ulDesktopCY;

      sCX = sCX * 100 / ulDesktopCX;
      sCY = sCY * 100 / ulDesktopCY;

      if (sCX > 100)
         sCX = 100;

      if (sCY > 100)
         sCY = 100;

      sprintf(szOptionText, "ICONVIEWPOS=%d,%d,%d,%d",
         sX, sY, sCX, sCY);

      strcat(pszOptions, szOptionText);
      strcat(pszOptions, ";");
      free(pBuffer);
      return TRUE;
      }
   return FALSE;
}

