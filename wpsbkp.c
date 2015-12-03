/***********************************************************************
WPSBKP.C Workplace shell backup


***********************************************************************/
#define WRITEICONS
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
#include "wptools\wptools.h"

#include "msg.h"

#define MAX_FOLDERS 1024
#define OPTIONS_SIZE 4096


typedef struct _ObjLoc
{
HOBJECT hFolder;
HOBJECT hObject;
} OBJLOC, *POBJLOC;

typedef struct _FolderInfo
{
HOBJECT  hFolder;
BYTE     szLocation[256];
} FOLDERINFO, *PFOLDERINFO;

/***********************************************************************
* Global variables
***********************************************************************/
HINI hIniUser   = HINI_USERPROFILE;
HINI hIniSystem = HINI_SYSTEMPROFILE;

HOBJECT hobjDesktop;
FOLDERINFO FldrArray[MAX_FOLDERS];
ULONG   ulFldrCount = 0L;
ULONG   ulIconNr    = 0L;
FILE   *fOutput;
ULONG   ulDesktopCX=640,
        ulDesktopCY=480;
BYTE    szDesktopPath[512];
PBYTE   pDesktopICONPOS = NULL;
USHORT  usDesktopICONPOSSize;
BYTE    szOutputDir[256];
BOOL    fSilent = FALSE;


BYTE   szMessage[]=
"\nThis program will backup your workplace shell desktop.\n\n\
It will do so by creating a text file that contains information\n\
about all the programs or shadows that are on your desktop or in other\n\
folders as well as the folders these objects are in.\n\
Also, any icons you have set for these objects will be extracted from\n\
the ini-files into separate .ICO files.\n\n\
This program will NOT backup any other object types!\n\
                   =====================\n\
NOTE 1: If you have added or changed object properties and have not\n\
        done a shutdown some properties might not backup properly!\n\
\n\
NOTE 2: Objects that are backup'd by this program need an unique OBJECTID\n\
        You will be asked a confirmation for each OBJECTID this program \n\
        assigns to objects without such an id.\n\
                   =====================\n";



/***********************************************************************
* Function prototypes
***********************************************************************/
BOOL   GetAllObjects       (POBJLOC *ppLoc, PULONG pulObjCnt);
INT    Compare             (const void *p1, const void *p2);
BOOL   BackupObjects       (POBJLOC pObj, ULONG ulObjCnt, PSZ pszClass);
BOOL   BackupFolder        (HOBJECT hFolder, BOOL fForce);
PBYTE  WriteIconFile       (PSZ pszTitle, ULONG ulIconNr, PBYTE pchIcon, ULONG ulIconSize, BOOL fAnimated);
BOOL   GetUniqueID         (PSZ pszObjectID, PSZ pszPath);
BOOL   GetPosOnDesktop     (PSZ pszClass, HOBJECT hObject, PSZ pszOptions);
BOOL   CorrectObjectID     (HOBJECT hObject, PSZ pszSettings, PSZ pszTitle);
BOOL   CheckWPS            (VOID);
BOOL   SetObjectID         (HOBJECT hObject, PSZ pszID, PSZ pszTitle);
BOOL   WriteBackupFile     (FILE *fh, PSZ pszClass, PSZ pszTitle, PSZ pszLocation, PSZ pszOptions);
BOOL   fShadowOnLP(HOBJECT hObject);


/***********************************************************************
* The main function
***********************************************************************/
INT main(INT iArgc, PSZ pszArg[])
{
POBJLOC pLoc;
ULONG   ulObjectCnt;
BYTE    szBackupFileName[256];
BYTE    szOldBackupFileName[256];
BYTE    chChar;
INT     iArg;

   printf("WPSBKP - Workplace shell backup, Version 1.40 (%s)\n", __DATE__);
   printf("Made by Henk Kelder\n\n");
   printf("USAGE: WPSBKP [outputdirectory] [options]\n");
   printf("Use /? for help\n");

   for (iArg = 1; iArg < iArgc; iArg++)
      {
      strupr(pszArg[iArg]);
      if (pszArg[iArg][0] == '-' || pszArg[iArg][0] == '/')
         {
         switch (pszArg[iArg][1])
            {
            case  '?'   :
               printf("Valid options are:\n");
               printf("/Y  - No questions asked, all questions are answered Yes\n");
               exit(1);
               break;

            case 'Y'    :
               fSilent = TRUE;
               break;

            default     :
               printf("Invalid switch %s used\n", pszArg[iArg]);
               break;
            }
         }
      else
         {
         if (!strlen(szOutputDir))
            strcpy(szOutputDir, pszArg[iArg]);
         else
            {
            printf("Invalid argument \'%s\'\n",
               pszArg[iArg]);
            exit(1);
            }
         }
      }

   if (!strlen(szOutputDir))
      getcwd(szOutputDir, sizeof szOutputDir);

   if (access(szOutputDir, 0))
      {
      printf("Unable to access directory: %s\n", szOutputDir);
      exit(1);
      }

   printf(szMessage);
   printf("Backup will be made to: %s\n", szOutputDir);
   if (!fSilent)
      {
      printf("Press [Y] to proceed\n");
      chChar = getch();
      if (chChar != 'y' && chChar != 'Y')
         exit(1);
      }

   if (szOutputDir[strlen(szOutputDir) - 1] != '\\')
      strcat(szOutputDir, "\\");


   ulDesktopCX = WinQuerySysValue(HWND_DESKTOP, SV_CXSCREEN);
   ulDesktopCY = WinQuerySysValue(HWND_DESKTOP, SV_CYSCREEN);

   if (!CheckWPS())
      exit(1);


   if (PathFromObject(HINI_SYSTEM, hobjDesktop, szDesktopPath, sizeof szDesktopPath, NULL))
      {
      GetEAValue(szDesktopPath, ".ICONPOS",
         (PBYTE *)&pDesktopICONPOS, &usDesktopICONPOSSize);
      }

   strcpy(szBackupFileName, szOutputDir);
   strcat(szBackupFileName, "WPSBKP.DAT");
   strcpy(szOldBackupFileName, szOutputDir);
   strcat(szOldBackupFileName, "WPSBKP.OLD");
   unlink(szOldBackupFileName);
   rename(szBackupFileName, szOldBackupFileName);


   fOutput = fopen(szBackupFileName, "w");
   if (!fOutput)
      {
      printf("Unable to create wpsbkp.dat");
      exit(1);
      }

   if (GetAllObjects(&pLoc, &ulObjectCnt))
      {
      BackupObjects(pLoc, ulObjectCnt, "WPProgram");
      BackupObjects(pLoc, ulObjectCnt, "WPHost");
      BackupObjects(pLoc, ulObjectCnt, "WPUrl");
      BackupObjects(pLoc, ulObjectCnt, "WPLaunchpad");
      BackupObjects(pLoc, ulObjectCnt, "SmartCenter");
      BackupObjects(pLoc, ulObjectCnt, "WPShadow");
      BackupFolder(hobjDesktop, TRUE);
      }

   fclose(fOutput);
   return 0;
}


/*****************************************************************
*  GetAllObjects
*****************************************************************/
BOOL GetAllObjects(POBJLOC *ppLoc, PULONG pulObjCnt)
{
PBYTE   pBuffer, p;
ULONG   ulProfileSize;
ULONG   ulIndex, ulIndex2, ulExtra;
PULONG  pulFldrContent;
HOBJECT hFolder;

   /*
      Get all objects
   */
   pBuffer = GetAllProfileNames(OBJECTS, hIniUser, &ulProfileSize);
   if (!pBuffer)
      {
      printf("Cannot find %s in userprofile\n", OBJECTS);
      exit(1);
      }

   /*
      Count total number of objects
   */
   for ((*pulObjCnt) = 0, p = pBuffer; *p; p+= strlen(p) + 1)
      (*pulObjCnt)++;

   /*
      Keep space for 100 extra, undefined ones
   */
   ulExtra = 100;
   (*ppLoc) = calloc((*pulObjCnt) + ulExtra, sizeof (OBJLOC));
   if (!(*ppLoc))
      {
      printf("Not enough memory for object array\n");
      exit(1);
      }
   memset((*ppLoc), 0, ((*pulObjCnt) + ulExtra) * sizeof (OBJLOC));


   /*
      Place all objects in array
   */
   for (ulIndex = 0, p = pBuffer;
      ulIndex < (*pulObjCnt);
         p+= strlen(p) + 1, ulIndex++)
      {
      PBYTE pStop;
      (*ppLoc)[ulIndex].hObject = MakeAbstractHandle(strtol(p, &pStop, 16));
      }
   free(pBuffer);

   /*
      Get all folder contents record
   */
   pBuffer = GetAllProfileNames(FOLDERCONTENT, hIniUser, &ulProfileSize);
   if (!pBuffer)
      {
      printf("Cannot find %s in userprofile\n", FOLDERCONTENT);
      exit(1);
      }

   p = pBuffer;
   while (*p)
      {
      PBYTE pStop;

      hFolder = MakeDiskHandle(strtol(p, &pStop, 16));

      /*
         Get a specific foldercontents record
      */
      pulFldrContent = (PULONG)GetProfileData(FOLDERCONTENT,
         p,
         hIniUser,
         &ulProfileSize);

      ulProfileSize /= sizeof (ULONG);  /* total number of objects */

      for (ulIndex = 0; ulIndex < ulProfileSize; ulIndex++)
         {
         HOBJECT hObject = MakeAbstractHandle(pulFldrContent[ulIndex]);
         for (ulIndex2 = 0; ulIndex2 < (*pulObjCnt); ulIndex2++)
            {
            if (hObject == (*ppLoc)[ulIndex2].hObject)
               {
               if (!(*ppLoc)[ulIndex2].hFolder)
                  {
                  (*ppLoc)[ulIndex2].hFolder = hFolder;
                  break;
                  }
               }
            }
         }
      if (pulFldrContent)
         free((PBYTE)pulFldrContent);
      p += strlen(p) + 1;
      }
   free(pBuffer);

   qsort((*ppLoc), (*pulObjCnt), sizeof (OBJLOC), Compare);
   return TRUE;
}

/*****************************************************************
* Compare func for the qsort
*****************************************************************/
INT Compare(const void *p1, const void *p2)
{
INT iResult;

   iResult = ((POBJLOC)p1)->hFolder - ((POBJLOC)p2)->hFolder;
   if (!iResult)
      iResult = ((POBJLOC)p1)->hObject - ((POBJLOC)p2)->hObject;
   return iResult;
}


/*****************************************************************
* Backup all objects
*****************************************************************/
BOOL BackupObjects(POBJLOC pObjLoc, ULONG ulObjCnt, PSZ pszCls)
{
static  BYTE    szTitle   [256];
static  BYTE    szOptions [OPTIONS_SIZE];
PBYTE   pchIcon;
ULONG   ulIndex,
        ulProfileSize;
HOBJECT hFolder = 0xFFFFFFFFL;
BYTE    szObjID[10];
PFOLDERINFO pFolder;
BYTE    szClass[50];
PBYTE   p;
HOBJECT hObjectCheck;
BYTE    szObjectID[100];

   hFolder = 0xFFFFFFFF;
   for (ulIndex = 0; ulIndex < ulObjCnt; ulIndex++)
      {
      PSZ pszResult;

      if (!fGetObjectClass(pObjLoc[ulIndex].hObject,
         szClass, sizeof szClass))
         continue;
      if (stricmp(szClass, pszCls))
         continue;
      if (!strcmp(szClass, "WPShadow") && fShadowOnLP(pObjLoc[ulIndex].hObject))
         continue;
      
      pszResult = pszGetObjectSettings(pObjLoc[ulIndex].hObject,
         szClass, sizeof szClass, FALSE);
      if (!pszResult)
         continue;
      strcpy(szOptions, pszResult);
      if (strstr(szOptions, "TEMPLATE=YES"))
         continue;

      if (hFolder != pObjLoc[ulIndex].hFolder)
         {
         ULONG ulIndex2;
         hFolder = pObjLoc[ulIndex].hFolder;

         BackupFolder(hFolder, FALSE);

         pFolder = NULL;
         for (ulIndex2 = 0; ulIndex2 < ulFldrCount; ulIndex2++)
            {
            if (FldrArray[ulIndex2].hFolder == hFolder)
               {
               pFolder = &FldrArray[ulIndex2];
               break;
               }
            }
         if (!pFolder)
            {
            printf("Internal error: Trying to backup an object when the folder has not been backup'd\n");
            exit(1);
            }
         }

      /*
         Get title
      */
      memset(szTitle,    0, sizeof szTitle);
      p = strstr(szOptions, "TITLE=");
      if (p)
         {
         BYTE szTemp[256];
         p+=6;
         strncpy(szTemp, p, sizeof szTemp);
         p = strchr(szTemp, ';');
         if (p)
            *p = 0;
         ConvertTitle(szTitle, sizeof szTitle, szTemp, strlen(szTemp));
         }

      if (!strcmp(szClass, "WPShadow") ||
          !strcmp(szClass, "WPNetLink"))
         {
         /*
            Remove TITLE=xxxx since it will create probs on re-creating!
         */
         p = strstr(szOptions, "TITLE=");
         if (p)
            {
            PBYTE pEnd = strchr(p, ';');
            if (pEnd)
               {
               pEnd++;
               memmove(p, pEnd, strlen(pEnd) + 1);
               }
            else
               *p = 0;
            }
         }

      /*
         Get iconpos for objects on the desktop
      */
      if (hFolder == hobjDesktop)
         {
         if (!GetPosOnDesktop(szClass,
            pObjLoc[ulIndex].hObject, 
            szOptions))
            printf("Unable to find ICONPOS for %s!\n", szTitle);
         }

      /*
         Check and/or set objectID
      */
      memset(szObjectID, 0, sizeof szObjectID);
      p = strstr(szOptions, "OBJECTID=");
      if (p)
         {
         memset(szObjectID, 0, sizeof szObjectID);
         strncpy(szObjectID, p + strlen("OBJECTID="), sizeof szObjectID);
         p = strchr(szObjectID, ';');
         if (p)
            *p = 0;
         }
      else
         {
         GetUniqueID(szObjectID, szTitle);
         strcat(szOptions, "OBJECTID=");
         strcat(szOptions, szObjectID);
         strcat(szOptions, ";");
         }
      hObjectCheck = WinQueryObject(szObjectID);
      if (!hObjectCheck)
         {
         DosSleep(300);
         hObjectCheck = WinQueryObject(szObjectID);
         }
      if (!hObjectCheck)
         SetObjectID(pObjLoc[ulIndex].hObject, szObjectID, szTitle);
      else if (LOUSHORT(hObjectCheck) != LOUSHORT(pObjLoc[ulIndex].hObject))
         {
         printf("PROBLEM: OBJECTID %s is also in use by object %lX\n",
            szObjectID, hObjectCheck);
         CorrectObjectID(pObjLoc[ulIndex].hObject, szOptions, szTitle);
         }

      /*
         Get abstract icon
      */
#ifdef WRITEICONS
      sprintf(szObjID, "%X", LOUSHORT(pObjLoc[ulIndex].hObject));
      pchIcon = GetProfileData(ICONS, szObjID, hIniUser, &ulProfileSize);
      if (pchIcon)
         {
         PBYTE pszIconFile = WriteIconFile(szTitle, ulIconNr++, pchIcon, ulProfileSize, FALSE);
         if (pszIconFile)
            {
            strcat(szOptions, "ICONFILE=");
            strcat(szOptions, pszIconFile);
            strcat(szOptions, ";");
            }
         free(pchIcon);
         }
#endif
      WriteBackupFile(fOutput, szClass, szTitle, pFolder->szLocation, szOptions);
      printf("Writing %s\n", szTitle);
      }

   return TRUE;
}


/*******************************************************************
* Backup Folder;
*******************************************************************/
BOOL BackupFolder(HOBJECT hFolder, BOOL fForce)
{
static  BYTE    szTitle[256];
static  BYTE    szOptions[OPTIONS_SIZE];
BYTE    szLocation[256];
BYTE    szPath[256];
BYTE    szClass[50];
BYTE    szObjectID[100];
PBYTE   pObjectData;
USHORT  usEASize;
PNODE   pNode;
ULONG   ulIndex;
HOBJECT hObjectCheck;
HOBJECT hObjParent;
PSZ     pszReturn;
PSZ     p;

   for (ulIndex = 0; !fForce && ulIndex < ulFldrCount; ulIndex++)
      {
      if (FldrArray[ulIndex].hFolder == hFolder)
         return TRUE;
      }

   memset(szLocation, 0, sizeof szLocation);
   pNode = PathFromObject(HINI_SYSTEM, hFolder, szLocation, sizeof szLocation, NULL);
   if (!pNode)
      {
      printf("Unable to get pathname for folder %lX\n", hFolder);
      exit(1);
      }
   if (strlen(szLocation) == 2)
      {
      strcat(szLocation, "\\");
      FldrArray[ulFldrCount].hFolder = hFolder;
      strcpy(FldrArray[ulFldrCount].szLocation, szLocation);
      ulFldrCount++;
      return TRUE;
      }
   strcpy(szPath, szLocation);
   if (pNode->usParentID && hFolder != hobjDesktop)
      BackupFolder(MakeDiskHandle(pNode->usParentID), FALSE);

   FldrArray[ulFldrCount].hFolder = hFolder;
   strcpy(FldrArray[ulFldrCount].szLocation, szLocation);

   strcpy(szClass, "WPFolder");
   pszReturn = pszGetObjectSettings(hFolder,
      szClass, sizeof szClass, FALSE);
   if (pszReturn)
      strcpy(szOptions, pszReturn);

   /*
      Get title
   */

   p = strstr(szOptions, "TITLE=");
   if (p)
      {
      BYTE szTemp[256];
      p+=6;
      strncpy(szTemp, p, sizeof szTemp);
      p = strchr(szTemp, ';');
      if (p)
         *p = 0;
      ConvertTitle(szTitle, sizeof szTitle, szTemp, strlen(szTemp));
      }
   else
      {
      PSZ pszLongName;
      /*
         Get title of object
      */
      if (GetEAValue(szPath, ".LONGNAME", &pszLongName, &usEASize))
         {
         ConvertTitle(szTitle, sizeof szTitle, pszLongName, usEASize);
         free(pszLongName);
         }
      else
         {
         pszLongName = strrchr(szLocation, '\\');
         if (pszLongName)
            strcpy(szTitle, ++pszLongName);
         }
      strcat(szOptions, "TITLE=");
      strcat(szOptions, szTitle);
      strcat(szOptions, ";");
      }

   /*
      Get ObjectID
   */
   memset(szObjectID, 0, sizeof szObjectID);
   p = strstr(szOptions, "OBJECTID=");
   if (p)
      {
      memset(szObjectID, 0, sizeof szObjectID);
      strncpy(szObjectID, p + strlen("OBJECTID="), sizeof szObjectID);
      p = strchr(szObjectID, ';');
      if (p)
         *p = 0;
      }
   else
      {
      GetUniqueID(szObjectID, szTitle);
      strcat(szOptions, "OBJECTID=");
      strcat(szOptions, szObjectID);
      strcat(szOptions, ";");
      }

   strcpy(FldrArray[ulFldrCount].szLocation, szObjectID);
   if (!fForce && hFolder == hobjDesktop)
      {
      ulFldrCount++;
      return TRUE;
      }



   hObjectCheck = WinQueryObject(szObjectID);
   if (!hObjectCheck)
      {
      DosSleep(300);
      hObjectCheck = WinQueryObject(szObjectID);
      }
   if (!hObjectCheck)
      {
      SetObjectID(hFolder, szObjectID, szTitle);
      }
   else if (LOUSHORT(hObjectCheck) != LOUSHORT(hFolder))
      {
      printf("PROBLEM: OBJECTID %s is also in use by object %lX\n",
         szObjectID, hObjectCheck);
      CorrectObjectID(hFolder, szOptions, szTitle);
      }

   /*
      Get object location
   */
   GetObjectLocation(hFolder, szLocation, sizeof szLocation);
   hObjParent = WinQueryObject(szLocation);


   /*
      For objects on the desktop,
      Get icon position
   */

   if (hObjParent == hobjDesktop)
      {
      if (!GetPosOnDesktop(szClass, hFolder, szOptions))
         printf("Unable to find ICONPOS for %s!\n", szTitle);
      }


   /*
      Get icon for the folder
   */
#ifdef WRITEICONS
   if (GetEAValue(szPath, ".ICON", &pObjectData, &usEASize))
      {
      PBYTE pszIconFile = WriteIconFile(szTitle, ulIconNr++, pObjectData, usEASize, FALSE);
      if (pszIconFile)
         {
         strcat(szOptions, "ICONFILE=");
         strcat(szOptions, pszIconFile);
         strcat(szOptions, ";");
         }
      free(pObjectData);
      }

   if (GetEAValue(szPath, ".ICON1", &pObjectData, &usEASize))
      {
      PBYTE pszIconFile = WriteIconFile(szTitle, ulIconNr++, pObjectData, usEASize, TRUE);
      if (pszIconFile)
         {
         strcat(szOptions, "ICONNFILE=1,");
         strcat(szOptions, pszIconFile);
         strcat(szOptions, ";");
         }
      free(pObjectData);
      }

#endif
   printf("Writing %s\n", szTitle);
   WriteBackupFile(fOutput, szClass, szTitle, szLocation, szOptions);

   ulFldrCount++;

   return TRUE;
}

/*******************************************************************
* Get Unique ID
*******************************************************************/
BOOL GetUniqueID(PSZ pszObjectID, PSZ pszPath)
{
PBYTE pName;
ULONG ulIndex=1;
BYTE  szRoot[30];
PBYTE p;

   pName = strrchr(pszPath, '\\');
   if (pName)
      pName++;
   else if (pszPath[1] == ':')
      {
      sprintf(szRoot, "ROOT%c", *pszPath);
      pName = szRoot;
      }
   else
      pName = pszPath;

   sprintf(pszObjectID, "<WP_%.10s>", pName);
   strupr(pszObjectID);
   for (p = pszObjectID; *p; p++)
      {
      if (*p == ' ')
         *p = '_';
      }

   while (WinQueryObject(pszObjectID))
      {
      sprintf(pszObjectID, "<WP_%.10s%d>", pName, ulIndex++);
      for (p = pszObjectID; *p; p++)
         {
         if (*p == ' ')
            *p = '_';
         }
      strupr(pszObjectID);
      }

   return TRUE;
}


/******************************************************************
* Write icon file
*******************************************************************/
PBYTE WriteIconFile(PSZ pszTitle, ULONG ulNr, PBYTE pchIcon, ULONG ulIconSize, BOOL fAnimated)
{
static BYTE szIconFile[256];
BYTE   szLongName[256];
INT    iHandle;
PBYTE  p;

   sprintf(szIconFile, "%sWPSB%4.4ld.ICO", szOutputDir, ulNr);

   iHandle = open(szIconFile,
      O_RDWR | O_BINARY | O_TRUNC | O_CREAT, S_IWRITE | S_IREAD);
   if (iHandle == -1)
      {
      printf("Unable to create iconfile!\n");
      return NULL;
      }

   write(iHandle, pchIcon, ulIconSize);
   close(iHandle);

   if (fAnimated)
      sprintf(szLongName, "Animated icon for\n%s", pszTitle);
   else
      sprintf(szLongName, "Icon for\n%s", pszTitle);
   while ((p = strchr(szLongName, '^')) != NULL)
      *p = '\n';
   SetEAValue(szIconFile, ".LONGNAME", EA_LPASCII, szLongName, strlen(szLongName));
   return szIconFile;
}


/*****************************************************************
* Get Icon position on <WP_DESKTOP>
*****************************************************************/
BOOL GetPosOnDesktop(PSZ pszClass, HOBJECT hObject, PSZ pszOptions)
{
PICONPOS pPos;
BYTE     szKey[100];
BYTE     szPath[256];
FILESTATUS3 fStat;
ULONG    ulX, ulY;
BYTE     szOptionText[20];
USHORT   usStartPos;

   if (Is21() > 0)
      usStartPos = 21;
   else
      usStartPos = 0;

   if (IsObjectAbstract(hObject))
      sprintf(szKey, "%s:A%lX", pszClass, LOUSHORT(hObject));
   else
      {
      PBYTE p;
      if (!PathFromObject(HINI_SYSTEM, hObject, szPath, sizeof szPath, NULL))
         return FALSE;
      DosQueryPathInfo(szPath,
         FIL_STANDARD,
         &fStat,
         sizeof fStat);
      p = strrchr(szPath, '\\');
      if (!p)
         return FALSE;
      p++;
      sprintf(szKey, "%s:%c%s", pszClass,
         (fStat.attrFile & FILE_DIRECTORY ? 'D' : 'F'),
         p);
      }

   for (pPos = (PICONPOS)(pDesktopICONPOS + usStartPos);
      (PBYTE)pPos < pDesktopICONPOS + usDesktopICONPOSSize; )
         {
         if (!stricmp(pPos->szIdentity, szKey))
            {
            ulX = pPos->ptlIcon.x * 100 / ulDesktopCX;
            ulY = pPos->ptlIcon.y * 100 / ulDesktopCY;
            sprintf(szOptionText, "%ld,%ld", ulX, ulY);
            strcat(pszOptions, "ICONPOS=");
            strcat(pszOptions, szOptionText);
            strcat(pszOptions, ";");
            return TRUE;
            }
         pPos = (PICONPOS)
            ((PBYTE)pPos + sizeof(POINTL) + strlen(pPos->szIdentity) + 1);
         }

   return FALSE;
}



/***************************************************************
* Correct ObjectID if OBJECTID points to another object
***************************************************************/
BOOL CorrectObjectID(HOBJECT hObject, PSZ pszSettings, PSZ pszTitle)
{
BYTE szObjectID[40];
PSZ  p;

   /*
      Get ObjectID in settings
   */
   p = strstr(pszSettings, "OBJECTID=");
   if (!p)
      return FALSE;

   strncpy(szObjectID, p + strlen("OBJECTID="), sizeof szObjectID);
   p = strchr(szObjectID, ';');
   if (p)
      *p = 0;

   if (!GetUniqueID(szObjectID, pszTitle))
      return FALSE;

   /*
      Remove Old ObjectID
   */
   p = strstr(pszSettings, "OBJECTID=");
   if (p)
      {
      PBYTE pEnd = strchr(p, ';');
      if (pEnd)
         {
         pEnd++;
         memmove(p, pEnd, strlen(pEnd) + 1);
         }
      }
   else
      *p = 0;

   /*
      And add new one
   */
   strcat(pszSettings, "OBJECTID=");
   strcat(pszSettings, szObjectID);
   strcat(pszSettings, ";");
   printf("Assigning alternate OBJECTID %s to %s!\n",
      szObjectID, pszTitle);

   SetObjectID(hObject, szObjectID, pszTitle);
   return TRUE;
}

/*******************************************************************
* Check Workplace shells compatibility
*******************************************************************/
BOOL CheckWPS(VOID)
{
PSZ   pszReturn;
BYTE  szClass[50];
PBYTE p;
BOOL  fError = FALSE;

   hobjDesktop = WinQueryObject("<WP_DESKTOP>");
   if (!hobjDesktop)
      {
      DosSleep(300);
      hobjDesktop = WinQueryObject("<WP_DESKTOP>");
      }
   if (!hobjDesktop)
      {
      printf("FATAL ERROR: Unable to locate the desktop in the ini-files!\n");
      printf("(<WP_DESKTOP> not found in OS2.INI)\n");
      return FALSE;
      }

   pszReturn = pszGetObjectSettings(hobjDesktop, szClass, sizeof szClass, FALSE);
   if (!pszReturn)
      {
      printf("FATAL ERROR: Unable to get the Desktops object data!\n");
      printf("             Error while gettings its extended attributes!\n");
      fError = TRUE;
      }
   else
      {
      p = strstr(pszReturn, "OBJECTID=");
      if (!p || memcmp(p + strlen("OBJECTID="), "<WP_DESKTOP>", strlen("<WP_DESKTOP>")))
         {
         printf("FATAL ERROR: The format of the workplace shells data\n");
         printf("             seems incompatible with this program!\n");
         fError = TRUE;
         }
      }

   if (fError)
      {
      printf("\nYou might try CheckIni to repair this.\n");
      printf("\nTo do so, run CheckIni /C !\n");
      }

   return TRUE;
}

/*******************************************************************
* Set Object ID
*******************************************************************/
BOOL SetObjectID(HOBJECT hObject, PSZ pszID, PSZ pszTitle)
{
BYTE szOptions[100];
BYTE chChar;

   if (hObject == hobjDesktop)
      {
      printf("FATAL ERROR: The format of the workplace shells data\n");
      printf("             seems incompatible with this program!\n");
      printf("             (Program tried to set OBJECTID of DESKTOP!\n");
      exit(1);
      }

   if (!fSilent)
      {
      printf("Ok to assign objectID %s to %s? [YN]\n", pszID, pszTitle);
      DosBeep(1000,100);
      do
         {
         chChar = getch();
         if (chChar == 'y')
            chChar = 'Y';
         else if (chChar == 'n')
            chChar = 'N';
         } while (chChar != 'N' && chChar != 'Y');

      if (chChar == 'N')
         return FALSE;
      }


   sprintf(szOptions, "OBJECTID=%s", pszID);
   MySetObjectData(hObject, szOptions);
   printf("Assigning OBJECTID %s to %s!\n", pszID, pszTitle);

   return TRUE;
}

/*******************************************************************
* Write Backup File
*******************************************************************/
BOOL WriteBackupFile(FILE *fh, PSZ pszClass, PSZ pszTitle, PSZ pszLocation, PSZ pszOptions)
{
PBYTE pStart, pEnd;
PSZ   pszSpace = "             ";
BOOL  fFirstLine = TRUE;

   fprintf(fh, "OBJECT \"%s\" \"%s\" \"%s\"\n",
      pszClass,
      pszTitle,
      pszLocation);

   pStart = pszOptions;
   while (*pStart)
      {
      pEnd = pStart;
      for (;;)
         {
         pEnd = strchr(pEnd, ';');
         if (!pEnd)
            {
            pEnd = pStart + strlen(pStart);
            break;
            }
         if (pEnd > pStart && *(pEnd - 1) == '^')
            {
            pEnd = pEnd + 1;
            continue;
            }
         break;
         }
      if (fFirstLine)
         fprintf(fh, "%s\"", pszSpace);
      else
         fprintf(fh, "\n%s ", pszSpace);
      fFirstLine = FALSE;

      fprintf(fh, "%.*s;", pEnd - pStart, pStart);
      pStart = pEnd;
      if (*pStart == ';')
         pStart++;
      }
   fprintf(fh, "\"\n\n");
   return TRUE;
}

BOOL fShadowOnLP(HOBJECT hObject)
{
USHORT usIndex;

   for (usIndex = 0; usIndex < 200; usIndex++)
      {
      if (_rgLaunchPadObjects[usIndex] == 0)
         return FALSE;
      if (_rgLaunchPadObjects[usIndex] == hObject)
         return TRUE;
      }
   return FALSE;
}
