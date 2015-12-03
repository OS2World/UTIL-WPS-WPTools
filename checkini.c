#include <locale.h>  
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <stdarg.h>
#include <share.h>
#include <sys\types.h>
#include <sys\stat.h>

#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_DOSDEVICES
#define INCL_DOSDEVIOCTL
#define INCL_PM
#define INCL_WPERRORS
#include <os2.h>

#include "display.h"
#include "msg.h"
#include "box.h"
#include "language.h"
#include "mouse.h"
#include "files.h"
#include "keyb.h"
#include "wptools\wptools.h"


#define MROW      8
#define TYPE_TEMPLATE       32

#pragma pack(1)
typedef struct _TrayDef
{
BYTE bTrayNumber;
BYTE bInUse;
BYTE bUnknown[3];
BYTE szTitle[33];
ULONG ulIconCount;
ULONG rgulObjects[1];
} TRAYDEF, *PTRAYDEF;
#pragma pack()


typedef struct _ObjLoc
{
HOBJECT hFolder;
HOBJECT hObject;
} OBJLOC, *POBJLOC;

#define ERROR_DUPLICATE_DRIV 1
#define ERROR_INVALID_NODE   2

typedef struct _DRIVE
{
PDRIV pDriv;
BYTE  szDrivName[64];
BYTE  szNodeName[64];
ULONG ulNodeCount;
ULONG ulNodesInUse;
BOOL  fContainsDesktop;
USHORT usError;
} DRIVE, * PDRIVE;

/*********************************************************************
* Global variables
*********************************************************************/
HINI  hIniUser   = HINI_USERPROFILE;
HINI  hIniSystem = HINI_SYSTEMPROFILE;
BOOL  bCorrect   = FALSE;
BOOL  bAltIni    = FALSE;
BYTE  szAltIniPath[512];
INT   iMaxScreenRow;
FILE *fError;

BOOL  bWriteAll      = FALSE;
BOOL  bSilent        = FALSE;
BOOL  bNoRemote      = FALSE;
BOOL  bAlwaysDefault = FALSE;
BOOL  fNoScroll      = FALSE;
BOOL  fDisplayInit   = FALSE;
BOOL  fScanDisks     = FALSE;
BOOL  fHandlesOnly   = FALSE;

BOOL  bRestartNeeded = FALSE;
BOOL  fDupDrivsFound = FALSE;

#define MAX_STARTUPCLASSES 20
PSZ   rgStartup[MAX_STARTUPCLASSES]  = {"WPStartup"};
USHORT usStartupCount = 1;
BYTE  szCurDir[CCHMAXPATH];
BYTE  szStartupFile[CCHMAXPATH];
BYTE  szArchivePath[CCHMAXPATH] = "ANameThatMayNotBeFound";

#define MAX_SC_OBJECTS 256
HOBJECT rgSCObjects[MAX_SC_OBJECTS];
USHORT  usSCCount = 0;

HOBJECT hobjDesktop;
POBJCLASS pObjClass;
BYTE  szDesktopPath[255] = "<WP_DESKTOP>";
static BYTE szMess[1000] = "";

/* For CheckDrivIntegrity */
PDRIVE rgCheckDrive;
PBYTE pAllBlocks = NULL;
ULONG ulAllBlockSize;
USHORT usDriveCount;
BYTE   fHandleUsed[65536];
BYTE   fHandleFound[65536];


BYTE  szConfirmY[]=
"WARNING: CHECKINI will modify INI files without intervention./\
DID YOU MAKE A BACKUP COPY of OS2.INI and OS2SYS.INI?/\
/\
If not, please press ESCAPE now!";

BYTE  szMsgMove[]=
"Move objects in this non-existing folder to a new folder?/\
NOTE: The objects will appear after reboot!";

BYTE  szMsgRecreate[]=
"Should CHECKINI try to recreate the missing directory?";

BYTE  szWarningHandles[]=
"The workplace shell uses the following information to get/\
a physical pathname based on an object handle./\
Unfortunately LOTS of pathnames are kept in here that are no longer/\
valid or have never been valid./\
This routines checks the consistency of this information.//\
NOTE!/\
Do not delete handles of remote folders of which you have local shadows!//\
Press <ESCAPE> to skip this test!";

BYTE szWarningObjects[]=
"The following routine checks all abstract objects on your desktop./\
It also checks the folder these objects are located in./\
When an object has a link to a another object (e.g. program-/\
and shadow objects) these link are also checked.//\
Press <ESCAPE> to skip this test!";

BYTE szWarningIcons[]=
"The workplace shell stores icons for some abstract objects in /\
separate record in the ini-file. This routine checks if the ojects/\
these icons belong to still exists!//\
Press <ESCAPE> to skip this test!";

BYTE szWarningAssocFilter[]=
"The following routine checks the associations that are set/\
based on file templates (e.g. *.TXT).//\
Press <ESCAPE> to skip this test!";

BYTE szWarningAssocType[]=
"The following routine checks the associations that are set/\
based on file types (e.g. Plain Text).//\
Press <ESCAPE> to skip this test!";

BYTE szWarningCheckSum[]=
"This routine checks Checksum records that are in the ini-files. /\
These records refer to desktop-objects which could be non-existing.//\
Press <ESCAPE> to skip this test!";

BYTE szWarningFolderPos[]=
"The workplace shell continuously stores presentation parameters /\
(position, fonts, etc.) for desktop-objects in the ini-files but/\
apparantly never removes them. This routine checks the existence/\
of these objects. Removing them by mistake results in loosing their/\
position and some other values.//\
Press <ESCAPE> to skip this test!";

BYTE szWarningPalettePos[]=
"The workplace shell stores positions for the various palettes in /\
the ini-files but apparantly never removes them when the palettes/\
are deleted. This routine checks the existence of these objects.//\
Press <ESCAPE> to skip this test!";

BYTE szWarningStatusPos[]=
"The workplace shell stores positions for Power objects in /\
the ini-files but apparantly never removes them when the power objects/\
are deleted. This routine checks the existence of these objects.//\
Press <ESCAPE> to skip this test!";

BYTE szWarningPrintObjects[] =
"The workplace shell stores a list of print objects in /\
the ini-files but apparantly sometimes fails to remove them when these/\
objects are deleted. This routine checks the existence of these objects.//\
Press <ESCAPE> to skip this test!";


BYTE szWarningStartup[]=
"The workplace shell keeps a list of all startup folders in the ini/\
files. This routine checks the existence of the locations these/\
references point to.//\
Press <ESCAPE> to skip this test!";

BYTE szWarningTemplates[]=
"For some existing object classes template-objects can be created./\
The workplace shell keeps a list of these templates in the ini-files./\
Sometimes this list keeps a template when the object it refers to no/\
longer exist.//\
Press <ESCAPE> to skip this test!";

BYTE szWarningLocation[]=
"The workplace shell keeps logical names for several locations./\
These are names as <WP_DESKTOP>. These names are not always/\
removed. This routine checks the existence of the locations these/\
logical names point to.//\
Press <ESCAPE> to skip this test!";

BYTE szWarningWorkarea[]=
"The workplace shell keeps a list of all open workarea's in the ini/\
files. This routine checks the existence of the location these/\
references point to.//\
Press <ESCAPE> to skip this test!";

BYTE szWarningJobCnrPos[]=
"The workplace shell continuously stores presentation parameters /\
(position, fonts, etc.) for printer queue windows but apparantly/\
never removes them. This routine checks the existence of these objects.//\
Press <ESCAPE> to skip this test!";

BYTE szWarningPMObjects[]=
"The workplace shell keeps a list of all registered classes. /\
This routine checks if all these classes can be loaded.//\
Press <ESCAPE> to skip this test!";

BYTE szWarningScanDisks[]=
"This routine scans all local disks for possible lost startup/\
or for folders of improper classes.//\
NOTE: This test should be used with CARE if you have multiple/\
of OS/2 on your computer since older versions of OS/2 do not/\
recoqnize classes of newer versions!//\
Press <ESCAPE> to skip this test!";

BYTE szStartupClass[]=
"In the list of startup folders/%s/\
was found which is not of a known startup class, but of class %s./\
It might be a valid startup folder, but it could also be an error./\
If you think this is a valid startup folder select YES,/\
otherwise select NO!";

BYTE szBackupped[]=
"Your INI files were backup'd as %s and %s.//\
If this action fails, reboot your system and press ALT-F1 during/\
boot. Then copy these files to the \\OS2 directory as OS2.INI and/\
OS2SYS.INI.";

BYTE szConfirmWrite[]=
"Write all changes made in this check?//\
CHECKINI will terminate after this. /\
You should restart the WPS, and check if the WPS still functions,/\
Then run CHECKINI again./\
PLEASE NOTE: many object handles will be trashed. Better run WPSBKP before doing this.";

BYTE szWaitForRestart[]=
"Please wait for the workplace shell to restart./\
If it is restarted without problems answer YES./\
else select NO and your original INI-files will be restored.";

/*********************************************************************
* Function prototypes
*********************************************************************/
BOOL   CheckHandlesIntegrity(VOID);
VOID   CheckWPSClasses      (VOID);
VOID   CheckObjectHandles  (VOID);
VOID   CheckObjectsAndFolders (VOID);
VOID   CheckAssocFilters   (VOID);
VOID   CheckAssocTypes     (VOID);
VOID   CheckFolderPos      (VOID);
VOID   CheckPalettePos     (VOID);
VOID   CheckStatusPos      (VOID);
VOID   CheckJobCnrPos      (VOID);
VOID   CheckPrintObjects   (VOID);
VOID   CheckTemplates      (VOID);
VOID   CheckAssocCheckSum  (VOID);
VOID   CheckAbstractIcons  (VOID);
BOOL   CheckLocation       (PSZ pszObjectID, HOBJECT hObj, PSZ pszTitle);
VOID   CheckStartup        (VOID);
VOID   CheckWorkarea       (VOID);
BOOL   fCheckClass         (PSZ pszClassName);
BOOL   fCheckFolders(VOID);
VOID   CheckFoldersOnPath(PSZ pszPath);


BOOL   GetAbstractTitle    (HOBJECT hObject, PSZ pszBuffer, USHORT usMax);
BOOL   GetObjectName       (HOBJECT hObject, PSZ pszFname, USHORT usMax);
BOOL   GetReffedProgName   (HOBJECT hObject, PSZ pszExe, USHORT usMax);
INT    Compare             (const void *, const void *);
BOOL   WriteProfileData    (PSZ pszApp, PSZ pszKey, HINI hini, PBYTE pBuffer, ULONG ulProfileSize);
BOOL   GetObjectClass      (PBYTE pBuffer, PSZ pszClass, USHORT usMax);
ULONG  DeleteNode          (PBYTE pBuffer, PNODE pNode, ULONG ulSize);
ULONG  DeleteDrive         (PBYTE pBuffer, PDRIV pDriv, ULONG ulSize, BOOL fDeleteNodes);
BOOL   DeleteObject        (POBJLOC pObj);
BOOL   RemoveObjectFromFolder(POBJLOC pObj);
BOOL   AddObjectToFolder   (POBJLOC pObj);
BOOL   GetExeName          (PBYTE pObjectData, PSZ pszExeName, USHORT usMax);
BOOL   GetCurDir           (PBYTE pObjectData, PSZ pszCurDir, USHORT usMax);
VOID   CtrlCHandler        (int signal);
BOOL   GetUniqueID         (PSZ pszObjectID, PSZ pszPath);
BOOL   CheckWPS            (BOOL fRepair);
BOOL   RepairDesktop       (HOBJECT hObject);
BOOL   RecreateDir         (PSZ pszDirToCreate);
BOOL   IsDriveLocalAttach        (BYTE bDrive);
BOOL   IsDriveLocalType       (BYTE bDrive);
BOOL   fWriteAllBlocks(HINI hini, PSZ pszHandles, PBYTE pBuffer, ULONG ulSize);
INT    MyAccess(PSZ pszFile, INT iMode);
BOOL   IsDriveRemovable(BYTE bDrive);
BOOL   DoesDriveExist(BYTE bDrive);

BOOL   IsStartupClass(PSZ pszClass);
BOOL   AddToStartupClasses(PSZ szClassName, BOOL fWrite);
BOOL   MyMsgJN(PSZ pszMess, short sRow, BOOL fDefault, ...);
BOOL   ReadStartupClasses(VOID);
BOOL   fRepairFolder(PSZ pszFolder, PSZ pszClass);
BOOL   fGetArchivePath(PSZ pszPath, USHORT usMax);
BOOL   _System GetSCenterObjects(VOID);
BOOL   CopyInis(VOID);
BOOL   RestoreInis(VOID);
PNODE _System GetObjectPath(BOOL fAddToInuse, HINI hIniSystem, HOBJECT hObject, PSZ pszFname, USHORT usMax);
BOOL   fGetWarpFunctions(VOID);
BOOL   ResetWPS(VOID);
PDRIVE BuildDrivList(PBYTE pBuffer, ULONG ulProfileSize, PUSHORT pusCount);
INT    CompareDrive(const void *p1, const void *p2);
BOOL   IsDrivInUse(PBYTE pStart, PBYTE pEnd);
PSZ GetPMError(VOID);


HOBJECT (* APIENTRY MyMoveObject) (HOBJECT hObjectofObject,
                                 HOBJECT hObjectofDest,
                                 ULONG   ulReserved);

INT _cdecl pprintf(BOOL bError, PSZ pszFormat, ...);


INT main(INT iArgc, PSZ *pszArg)
{
USHORT iIndex;
BYTE   szIniFname[512];
BYTE   szLogPath[256];
PSZ    p;
BOOL   fBackupped = FALSE;

   p = GetWPToolsVersion();
   if (strcmp(p, VERSION) < 0)
      {
      printf("This program needs WPTOOLS.DLL version %s\n", VERSION);
      printf("Current WPTOOLS.DLL version is %s\n", p);
      exit(1);
      }

   setlocale(LC_ALL, "");

   signal(SIGABRT, CtrlCHandler);
   signal(SIGINT, CtrlCHandler);
   signal(SIGTERM, CtrlCHandler);
   signal(SIGBREAK, CtrlCHandler);

   printf("CHECKINI version 2.46 (%s) - by Henk Kelder\n", __DATE__);
   printf("Checks the information the Workplace Shell stores in the ini files.\n");

   printf("Use /? to query options\n\n");

   if (!strchr(pszArg[0], '\\'))
      _fullpath(szCurDir,  pszArg[0], sizeof szCurDir);
   else
      strcpy(szCurDir, pszArg[0]);
   p = strrchr(szCurDir, '\\');
   if (p)
      {
      *p = 0;
      strcat(szCurDir, "\\");
      }
   else
      memset(szCurDir, 0, sizeof szCurDir);

   strcpy(szLogPath, "CHECKINI.LOG");

   strcpy(szStartupFile, szCurDir);
   strcat(szStartupFile, "STARTUP.CLS");


   for (iIndex = 1; iIndex < iArgc; iIndex++)
      {
      WinUpper(0, 0, 0,pszArg[iIndex]);
      if (*pszArg[iIndex] == '-' || *pszArg[iIndex] == '/')
         {
         switch(pszArg[iIndex][1])
            {
            case 'C' :
               bCorrect = TRUE;
               break;
            case 'A' :
               bAltIni = TRUE;
               strcpy(szAltIniPath, pszArg[iIndex] + 2);
               break;
            case 'L' :
               strcpy(szLogPath, pszArg[iIndex] + 2);
               break;
            case 'W' :
               bWriteAll = TRUE;
               if (pszArg[iIndex][2] == ':')
                  {
                  if (pszArg[iIndex][3] == '2')
                     bWriteAll = 2;
                  if (pszArg[iIndex][3] == '3')
                     bWriteAll = 3;
                  }
               break;
            case 'S' :
               bSilent = TRUE;
               break;
            case 'R' :
               bNoRemote = TRUE;
               break;
            case 'Y':
               bAlwaysDefault = TRUE;
               bNoRemote = TRUE;
               if (pszArg[iIndex][2] == ':')
                  if (pszArg[iIndex][3] == '2')
                     bAlwaysDefault = 2;

               printf("/Y Option specified: No checks on removable or non-existing drives!\n");
               break;
            case 'D' :
               strcpy(szDesktopPath, &pszArg[iIndex][2]);
               break;
            case 'T' :
               fScanDisks = TRUE;
               break;
            case 'H':
               fHandlesOnly = TRUE;
               break;
            case '?' :
               printf("Options: /C     - Write corrections to ini-files\n");
               printf("         /APath - Specify different location for ini-files to be checked\n");
               printf("         /Llogfilename - specify name of logfile (default CHECKINI.LOG)\n");
               printf("         /W     - Write all output to logfile\n");
               printf("         /S     - Silent run, only write logfile\n");
               printf("         /R     - Only warn about missing files on \n");
               printf("                  existing non-removable drives\n");
               printf("         /Dpath - Specify location of DESKTOP directory\n");
               printf("         /Y[:2] - Answer all correcting questions with YES\n");
               printf("                  if /Y:2: no confirmation of individual tests.\n");
               printf("         /T     - Test classes of all folders on local disks.\n");
               printf("         /H     - Only check PM_Workplace:Handles0/1\n");
               printf("         /?     - Show info\n");
               exit(1);
               break;
            default  :
               printf("Unknown command line switch %s\n",pszArg[iIndex]);
               exit(1);
               break;
            }
         }
      else
         {
         printf("Unknown option %s\n", pszArg[iIndex]);
         exit(1);
         }
      }

//   if (bSilent && bCorrect)
//      {
//      printf("/Silent and /Correct cannot be used together!\n");
//      exit(1);
//      }

   if (bAltIni && bCorrect)
      {
      printf("/Axxxx and /Correct cannot be used together!\n");
      printf("(Cannot correct alternate ini-files!)\n");
      exit(1);
      }

   if (bAlwaysDefault && !bCorrect)
      {
      printf("/Y option is useless without /C!\n");
      exit(1);
      }

   fError = fopen(szLogPath, "w");
   if (!fError)
      {
      printf("Unable to create logfile %s\n", szLogPath);
      exit(1);
      }

   printf("Hit any key...\n");
   keyb_read();

   display_init();
   fDisplayInit = TRUE;
   inst_handler();
   clr_eos_attr(0, 0, A_TXT_NRM);
   SetLanguage(LANG_UK);
   MouseInstalled();
   iMaxScreenRow = GetMaxScreenRow();
   box_draw(0, 0, iMaxScreenRow, MAX_COL - 1, 2);


   fGetWarpFunctions();
   ReadStartupClasses();

   if (bCorrect)
      {
      if (!CheckWPS(TRUE))
         {
         pprintf(TRUE, "Press any key.\n");
         keyb_read();
         fclose(fError);
         display_exit();
         exit(1);
         }
      }

   fBackupped = FALSE;

main_restart:

   bRestartNeeded = FALSE;
   memset(fHandleFound, 0, sizeof fHandleFound);
   memset(fHandleUsed, 0, sizeof fHandleUsed);

   if (bCorrect)
      {
      if (!fBackupped && CopyInis())
         fBackupped = TRUE;
      if (!fBackupped)
         {
         if (msg_nrm_var(szConfirmY, MROW) == KEY_ESC)
            {
            display_exit();
            return 0;
            }
         }
      }


   if (bAltIni)
      {
      strcpy(szIniFname, szAltIniPath);
      if (szIniFname[strlen(szIniFname) - 1] != '\\')
         strcat(szIniFname, "\\");
      strcat(szIniFname, "OS2.INI");

      if (access(szIniFname, 0))
         errmsg("%s does not exist!", szIniFname);

      hIniUser = PrfOpenProfile(0, szIniFname);
      if (!hIniUser)
         {
         errmsg("Unable to open %s:/%s",
            szIniFname,
            GetPMError());
         }

      strcpy(szIniFname, szAltIniPath);
      if (szIniFname[strlen(szIniFname) - 1] != '\\')
         strcat(szIniFname, "\\");
      strcat(szIniFname, "OS2SYS.INI");

      if (access(szIniFname, 0))
         errmsg("%s does not exist!", szIniFname);

      hIniSystem = PrfOpenProfile(0, szIniFname);
      if (!hIniSystem)
         {
         errmsg("Unable to open %s:/%s",
            szIniFname,
            GetPMError());
         }
      pprintf(TRUE, "INI Files used in: %s\n", szAltIniPath);
      SetInis(hIniUser, hIniSystem);
      }
   else
      fGetArchivePath(szArchivePath, sizeof szArchivePath);


   if (!CheckHandlesIntegrity())
      goto Main_Done;


   if (!fHandlesOnly)
      {
      if (!bAltIni)
         CheckWPSClasses();

      CheckObjectsAndFolders();
      CheckAbstractIcons();

      CheckAssocFilters();
      CheckAssocTypes();
      CheckAssocCheckSum();

      CheckFolderPos();
      CheckJobCnrPos();
      CheckPalettePos();
      CheckStatusPos();
      CheckPrintObjects();
      CheckStartup();
      CheckTemplates();
      CheckLocation(NULL, 0, NULL);
      CheckWorkarea();

      if (!bAltIni && fScanDisks)
         fCheckFolders();

      /*
         Make sure all handles are reread again
         by freeing the block and setting the
         pointer to null
      */
      ResetBlockBuffer();
      }
   if (bCorrect)
      fDupDrivsFound = TRUE;
   if (fDupDrivsFound && !CheckHandlesIntegrity())
      goto Main_Done;

   CheckObjectHandles();


Main_Done:

   if (bAltIni)
      {
      PrfCloseProfile(hIniUser);
      PrfCloseProfile(hIniSystem);
      }
   else if (bCorrect && bRestartNeeded)
      {
      if (msg_jn("WARNING!//The workplace shell needs to be reset NOW/"
         "to make sure the changes aren't lost./"
         "Reset NOW?", MROW, TRUE))
         {
         ResetWPS();
#if 0
         if (ResetWPS())
            {
            while (keyb_ready())
               keyb_read();
            if (msg_jn(szWaitForRestart, MROW, TRUE))
               {
               if (CheckWPS(FALSE))
                  goto main_restart;
               }
            if (fBackupped && RestoreInis())
               {
//               ResetWPS();
               fclose(fError);
               msg_nrm_var("Sorry, CHECKINI failed!", MROW);
               display_exit();
               exit(1);
               }
            msg_nrm_var("Please restory a backup of your INI files now and press C-A-D!", MROW);
            fclose(fError);
            display_exit();
            exit(1);
            }
#endif
         }
      else
         msg_nrm_var("Please PRESS Control-Alt-Delete NOW!", MROW);
      }

   msg_nrm_var("See Checkini.log for the report!/Press any key to exit", MROW);
   display_exit();
   fclose(fError);
   return 0;
}

/*********************************************************************
* Check physical object handles
*********************************************************************/
VOID CheckObjectHandles()
{
PBYTE pBuffer;
ULONG ulProfileSize;
PDRIV pDriv;
PNODE pNode;
PBYTE p;
static BYTE szExeName[1024];
BYTE szHandles[100];
BOOL bWrite = FALSE;
INT  iShowSize;
BOOL fDeleteDrive;
HOBJECT hObject;
HOBJECT hObject2;
BOOL fCheckDrive;
USHORT usIndex;
PDRIVE pDrive;


   GetActiveHandles(hIniSystem, szHandles, sizeof szHandles);

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", szHandles);
   pprintf(TRUE, "=================================================\n");
   /*
      Get all objectshandles
   */

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningHandles, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }

   if (!fReadAllBlocks(hIniSystem, szHandles, &pBuffer, &ulProfileSize))
      {
      errmsg("FATAL ERROR: %s not found in profile!\n", szHandles);
      return;
      }

   fDeleteDrive = FALSE;
   p = pBuffer + 4;
   while (p < pBuffer + ulProfileSize)
      {
      if (!memicmp(p, "DRIV", 4))
         {
         BOOL bError = FALSE;
         BOOL fInUse = IsDrivInUse(p, pBuffer + ulProfileSize);

         fDeleteDrive = FALSE;

         pDriv = (PDRIV)p;
         pDrive = NULL;
         for (usIndex = 0; usIndex < usDriveCount; usIndex++)
            {
            if (!strcmp(rgCheckDrive[usIndex].pDriv->szName, pDriv->szName) &&
                !memcmp(rgCheckDrive[usIndex].pDriv, pDriv, sizeof (DRIV)))
               {
               pDrive = &rgCheckDrive[usIndex];
               break;
               }
            }

         strcpy(szExeName, pDriv->szName);
         fCheckDrive = 2;
         if (bNoRemote)
            {
            if (strlen(szExeName) > 2 && !memcmp(szExeName, "\\\\", 2))
               fCheckDrive = FALSE;
            if (!IsDriveLocalType(szExeName[0]) ||
                 IsDriveRemovable(szExeName[0]))
               fCheckDrive = FALSE;
            }
         if (bSilent && fCheckDrive == 2)
            fCheckDrive = TRUE;
         if (bAltIni)
            fCheckDrive = FALSE;

         if ((!IsDriveLocalType(szExeName[0]) || IsDriveRemovable(szExeName[0])) &&
            fCheckDrive == 2 &&
            !MyMsgJN("Check %s for obsolete handles?", MROW, TRUE, pDriv->szName))
            fCheckDrive = FALSE;

         if (strlen(szExeName) == 2 && szExeName[1] == ':')
            strcat(szExeName, "\\");

         if (fCheckDrive && MyAccess(szExeName, 0))
            {
            strcat(szExeName, "<-UNABLE TO ACCESS");
            bError = TRUE;
            }

         pprintf(TRUE, "\n%cDRIV: %s  (%u handles) %s\n",
            (fInUse ? '*' : ' '), szExeName,
            (pDrive ? pDrive->ulNodeCount : 0),
            (pDrive && pDrive->usError & ERROR_DUPLICATE_DRIV ? " (DUPLICATE) " : ""));

         if (!fInUse && bError && bCorrect &&
            MyMsgJN("Remove all references to non-locateable drive %s?",
            MROW, TRUE, pDriv->szName))
            {
            ulProfileSize = DeleteDrive(pBuffer, pDriv, ulProfileSize, TRUE);
            fDeleteDrive = TRUE;
            bWrite = TRUE;
            continue;
            }
         else if (!bCorrect && bError && !bSilent)
            msg_nrm_var("Object handles to non-locateable drive found!", MROW);

         p += sizeof(DRIV) + strlen(pDriv->szName);
         }
      else if (!memicmp(p, "NODE", 4))
         {
         BOOL bError = FALSE;
         BOOL bDupHandle = FALSE;
         pNode = (PNODE)p;

         if (fDeleteDrive)
            {
            ulProfileSize = DeleteNode(pBuffer, pNode, ulProfileSize);
            bWrite = TRUE;
            continue;
            }

         memset(szExeName, 0, sizeof szExeName);
         if (pNode->usParentID)
            {
            if (!GetPartName(((PBYTE)pDriv) - 4,
               ulProfileSize - ((PBYTE)pDriv - pBuffer),
               pNode->usParentID, szExeName, sizeof szExeName, NULL) && fCheckDrive)
               {
               strcpy(szExeName, "UNABLE TO CONSTRUCT FILENAME!");
               bError = TRUE;
               }
            else
               strcat(szExeName, "\\");
            }
         if (!bError)
            {
            strcat(szExeName, pNode->szName);
            if (strlen(szExeName) == 2)
               strcat(szExeName, "\\");

            if (fCheckDrive && MyAccess(szExeName, 0))
               {
               strcat(szExeName, "<-UNABLE TO ACCESS");
               bError = TRUE;
               }
            }

         if (fHandleFound[pNode->usID])
            {
            BYTE szOtherName[CCHMAXPATH];
            PNODE pNode2;

            memset(szOtherName, 0, sizeof szOtherName);

            pNode2 = GetPartName(pBuffer, ulProfileSize, 
               pNode->usID, szOtherName, sizeof szOtherName, NULL);
            if (!pNode2)
               strcpy(szOtherName, "UNABLE TO CONTSTRUCT NAME");
            else if (strlen(szOtherName) == 2)
               strcat(szOtherName, "\\");

            strcat(szExeName, "<- HANDLE ALREADY ASSIGNED TO ");
            strcat(szExeName, szOtherName);
            bDupHandle = bError = TRUE;
            }
         else
            fHandleFound[pNode->usID] = TRUE;

         if (pNode->usNameSize > 15)
            iShowSize = 0;
         else
            iShowSize = 15 - pNode->usNameSize;
         hObject = MakeDiskHandle(pNode->usID);
#if 0
         if (1)
            {
            static BYTE szPath[CCHMAXPATH];
            if (!WinQueryObjectPath(hObject, szPath, sizeof szPath))
               {
               bError = TRUE;
               strcat(szExeName, "<-UNKNOWN BY WPS");
               }
            else if (stricmp(szExeName, szPath))
               {
               bError = TRUE;
               sprintf(szExeName+strlen(szExeName), "<-WPS returns %s", szPath);
               }
            }
#endif
         if (fCheckDrive && !bError && !bAltIni)
            {
            hObject2 = WinQueryObject(szExeName);
            if (hObject2 && hObject != hObject2)
               {
               sprintf(szExeName + strlen(szExeName),
                  "<-HAS MULTIPLE OBJECTHANDLES ASSIGNED! %lX", hObject2);
               bDupHandle = bError = TRUE;
               }
            }

         pprintf(bError, "%c%lX:%-.*s%.*s=>%s\n",
            (fHandleUsed[LOUSHORT(hObject)] ? '*' : ' '),
            hObject,
            pNode->usNameSize,
            pNode->szName,
            iShowSize,
            "                            ",
            szExeName);

         if (bError && bCorrect && (!fHandleUsed[pNode->usID] || bDupHandle))
            {
            BOOL fRetco = MyMsgJN("Remove handle to faulty path/and all handles for lower paths?",
               MROW, TRUE);
            if (fRetco)
               {
               ulProfileSize = DeleteNode(pBuffer, pNode, ulProfileSize);
               bWrite = TRUE;
               continue;
               }
            }
         else if (!bCorrect && bError && !bSilent)
            msg_nrm_var("Object handle to non-locateable path found!", MROW);

         p += sizeof (NODE) + pNode->usNameSize;
         }
      else
         {
         pprintf(TRUE, "%s:%s appears to be corrupted\n", szHandles, HANDLEBLOCK);
         if (!bSilent)
            msg_nrm("%s:%s appears to be corrupted !",  szHandles ,HANDLEBLOCK);
         return;
         }
      }

   while (keyb_ready())
      {
      keyb_read();
      flash(1);
      }

   if (bCorrect && bWrite && MyMsgJN("Write all changes to/ %s:%s",
      MROW, TRUE, szHandles, HANDLEBLOCK))
      {
      fWriteAllBlocks(hIniSystem, "PM_Workplace:Handles0", pBuffer, ulProfileSize);
      fWriteAllBlocks(hIniSystem, "PM_Workplace:Handles1", pBuffer, ulProfileSize);
//      fWriteAllBlocks(hIniSystem, szHandles, pBuffer, ulProfileSize);
      free(pBuffer);
      bRestartNeeded = TRUE;
      }
}




/*********************************************************************
* Rewrite the blocks back to the ini.
*********************************************************************/
BOOL fWriteAllBlocks(HINI hini, PSZ pszHandles, PBYTE pBuffer, ULONG ulSize)
{
PSZ p,
    pEnd;
BYTE  szBlockName[10];
INT   iCurBlock;
ULONG ulCurSize;
PBYTE pStart;
PDRIV pDriv;
PNODE pNode;

   /*
      First delete all existing blocks
   */
   PrfWriteProfileData(hini, pszHandles,
      NULL, 
      NULL, 
      0);

   pStart    = pBuffer;
   ulCurSize = 4;
   p    = pBuffer + 4;
   pEnd = pBuffer + ulSize;
   iCurBlock = 1;
   while (p < pEnd)
      {
      while (p < pEnd)
         {
         ULONG ulPartSize;
         if (!memicmp(p, "DRIV", 4))
            {
            pDriv = (PDRIV)p;
            ulPartSize = sizeof(DRIV) + strlen(pDriv->szName);
            }
         else if (!memicmp(p, "NODE", 4))
            {
            pNode = (PNODE)p;
            ulPartSize = sizeof (NODE) + pNode->usNameSize;
            }

         if (ulCurSize + ulPartSize > 0x0000FFFF)
            break;

         ulCurSize += ulPartSize;
         p         += ulPartSize;
         }
      sprintf(szBlockName, "BLOCK%d", iCurBlock++);
      WriteProfileData(pszHandles, szBlockName, hini,
         pStart, ulCurSize);
      pStart    = p;
      ulCurSize = 0;
      }

   while (iCurBlock < 256)
      {
      ULONG ulBlockSize;

      sprintf(szBlockName, "BLOCK%d", iCurBlock++);

      if (PrfQueryProfileSize(hini,
         pszHandles,
         szBlockName,
         &ulBlockSize) && ulBlockSize > 0)
         WriteProfileData(pszHandles, szBlockName, hini, NULL, 0);
      }

   return TRUE;
}
/*********************************************************************
* Delete a record in PM_Workplace:Handles:BLOCK1
*********************************************************************/
ULONG DeleteNode(PBYTE pBuffer, PNODE pNode, ULONG ulSize)
{
ULONG ulDelSize = sizeof (NODE) + pNode->usNameSize;
USHORT usID = pNode->usID;
ULONG ulMoveSize;


   if (memcmp(pNode->chName, "NODE", 4))
      return ulSize;

   ulMoveSize = (pBuffer + ulSize) - ((PBYTE)pNode + ulDelSize);
   ulSize -= ulDelSize;

   memmove(pNode, (PBYTE)pNode + ulDelSize, ulMoveSize);

   while ((PBYTE)pNode < pBuffer + ulSize &&
      !memcmp(pNode->chName, "NODE", 4) &&
      pNode->usParentID == usID)
      ulSize = DeleteNode(pBuffer, pNode, ulSize);
   
   return ulSize;
}

/*********************************************************************
* Delete all information about a drive in PM_Workplace:Handles:BLOCK1
*********************************************************************/
ULONG DeleteDrive(PBYTE pBuffer, PDRIV pDriv, ULONG ulSize, BOOL fDeleteNodes)
{
ULONG ulDelSize;
ULONG ulMoveSize;

   if (memcmp(pDriv->chName, "DRIV", 4))
      return ulSize;

   if (fDeleteNodes)
      {
      PBYTE pEnd = pBuffer + ulSize;
      PNODE pNode = (PNODE)((PBYTE)pDriv + sizeof (DRIV) + strlen(pDriv->szName));
      while ((PBYTE)pNode < pEnd && 
             !memicmp(pNode->chName, "NODE", 4))
         pNode = (PNODE)((PBYTE)pNode + sizeof (NODE) + pNode->usNameSize);
      ulDelSize = (PBYTE)pNode - (PBYTE)pDriv;
      }
   else
      ulDelSize = sizeof (DRIV) + strlen(pDriv->szName);

   ulMoveSize = (pBuffer + ulSize) - ((PBYTE)pDriv + ulDelSize);
   ulSize -= ulDelSize;

   memmove(pDriv, (PBYTE)pDriv + ulDelSize, ulMoveSize);

   return ulSize;
}


/*********************************************************************
* Check all the objects and foldercontents records
*********************************************************************/
VOID CheckObjectsAndFolders(VOID)
{
PBYTE pBuffer, p;
ULONG ulProfileSize;
POBJLOC pLoc;
ULONG  ulObjectCnt, ulIndex, ulIndex2, ulExtra;
PULONG pulFldrContent;
HOBJECT hFolder;
BOOL    bError, bReport;
HOBJECT hLostFolder = 0L;
USHORT  usLostCount=0;
BOOL    fDefault;


   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " PM_Abstract:Objects & PM_Abstract:FldrContents\n");
   pprintf(TRUE, "=================================================\n");

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningObjects, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }

   GetSCenterObjects();

   /*
      Get all objects
   */
   pBuffer = GetAllProfileNames(OBJECTS, hIniUser, &ulProfileSize);
   if (!pBuffer)
      {
      errmsg("Cannot find %s in userprofile", OBJECTS);
      }

   /*
      Count total number of objects
   */
   for (ulObjectCnt = 0, p = pBuffer; *p; p+= strlen(p) + 1)
      ulObjectCnt++;

   /*
      Keep space for 100 extra, undefined ones
   */
   ulExtra = 100;
   pLoc = calloc(ulObjectCnt + ulExtra, sizeof (OBJLOC));
   if (!pLoc)
      errmsg("Not enough memory for object array\n");
   memset(pLoc, 0, (ulObjectCnt + ulExtra) * sizeof (OBJLOC));


   /*
      Place all objects in array
   */
   for (ulIndex = 0, p = pBuffer;
      ulIndex < ulObjectCnt;
         p+= strlen(p) + 1, ulIndex++)
      {
      PBYTE pStop;
      pLoc[ulIndex].hObject = MakeAbstractHandle(strtol(p, &pStop, 16));
      }
   free(pBuffer);

   /*
      Get all folder contents record
   */
   pBuffer = GetAllProfileNames(FOLDERCONTENT, hIniUser, &ulProfileSize);
   if (!pBuffer)
      errmsg("Cannot find %s in userprofile\n", FOLDERCONTENT);

   /*
      For each folder contents, check all objects in it by looking at
      the earlier filled array. If an object is found in the array complete
      the information by adding it's folder handle to it. If an unknown object
      is found, add it in the extra space.
   */

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

         for (ulIndex2 = 0; ulIndex2 < ulObjectCnt; ulIndex2++)
            {
            if (hObject == pLoc[ulIndex2].hObject)
               {
               if (!pLoc[ulIndex2].hFolder)
                  {
                  pLoc[ulIndex2].hFolder = hFolder;
                  break;
                  }
               pprintf(TRUE, "Object %X is in two locations\n", hObject);
               }
            }
         if (ulIndex2 == ulObjectCnt && ulExtra > 0)
            {
            pLoc[ulObjectCnt].hObject = hObject;
            pLoc[ulObjectCnt].hFolder = hFolder;
            ulObjectCnt++;
            ulExtra--;
            }
         }
      if (pulFldrContent)
         free((PBYTE)pulFldrContent);
      p += strlen(p) + 1;
      }
   free(pBuffer);

   qsort(pLoc, ulObjectCnt, sizeof (OBJLOC), Compare);

   hFolder = 0xFFFFFFFFL;

   for (ulIndex = 0; ulIndex < ulObjectCnt; ulIndex++)
      {
      BYTE szDirPath[CCHMAXPATH];
      BYTE szTitle[CCHMAXPATH];
      BYTE szClass[50];
      BYTE szExeName[CCHMAXPATH];
      PBYTE pObjectData;
      BYTE szCurDir[CCHMAXPATH];
      BOOL bMoveObject;
      BOOL bDiscardObject;
      BYTE szObjectID[150];
      BOOL fFolderPrinted;

      bReport = bError = FALSE;

      memset(szTitle, 0, sizeof szTitle);
      memset(szObjectID, 0, sizeof szObjectID);

      /***********************************************************
         INIT PER FOLDER
      ***********************************************************/
      if (pLoc[ulIndex].hFolder != hFolder)
         {
         USHORT usEASize;

         fFolderPrinted = FALSE;

         bMoveObject = FALSE;
         bDiscardObject = FALSE;
         hFolder = pLoc[ulIndex].hFolder;
         if (!GetObjectName(hFolder, szTitle, sizeof szTitle))
            {
            sprintf(szTitle, "CANNOT TRANSLATE OBJECTHANDLE %X TO A FOLDERNAME!", hFolder);
            bError = TRUE;
            pprintf(TRUE, "\nFolder : %lX %s - %s\n", hFolder,
               szObjectID, szTitle);
            fFolderPrinted = TRUE;
            }
         else if (MyAccess(szTitle, 0))
            {
            pprintf(TRUE, "\nFolder : %lX %s - %s%s", hFolder,
               szObjectID, szTitle, "<-UNABLE TO ACCESS\n");
            fFolderPrinted = TRUE;
            if (bCorrect)
               {
               if (!RecreateDir(szTitle))
                  bError = TRUE;
               }
            }
         else if (GetEAValue(szTitle, ".CLASSINFO", &pObjectData, &usEASize))
            {
            GetObjectID(pObjectData + 4, szObjectID, sizeof szObjectID);
            if (!GetObjectClass(pObjectData+4, szClass, sizeof szClass))
               {
               strcpy(szClass, "UNKNOWN CLASS!");
               bError = TRUE;
               }
            pprintf(FALSE, "\nFolder : %lX Class %s %s - %s\n", hFolder,
               szClass,
               szObjectID, szTitle);
            if (bWriteAll)
               fFolderPrinted = TRUE;
            if (!fCheckClass(szClass))
               {
               pprintf(TRUE, "FOLDER %s: CLASS %s IS NOT A REGISTERED CLASS!\n",
                  szTitle, szClass);
               }
            else if (IsStartupClass(szClass))
               {
               BYTE szKeyName[10];
               PBYTE pTest;
               ULONG ulTestSize;
               sprintf(szKeyName, "%lX", LOUSHORT(hFolder));
               pTest = GetProfileData(STARTUP, szKeyName, hIniUser, &ulTestSize);
               if (!pTest)
                  {
                  pprintf(TRUE, "STARTUP FOLDER %s NOT FOUND AT %s\n",
                     szTitle, STARTUP);
                  if (bCorrect &&
                     MyMsgJN("Recreate entry for this startup folder in %s?", MROW, TRUE, STARTUP))
                     {
                     WriteProfileData(STARTUP, szKeyName, hIniUser,
                        "Y", 2);
                     }
                  }
               else
                  free(pTest);
               }

            free(pObjectData);
            }
         else
            {
            if (bWriteAll)
               fFolderPrinted = TRUE;
            pprintf(FALSE, "\nFolder : %lX Class WPFolder %s - %s\n", hFolder,
               szObjectID, szTitle);
            }

         if (*szObjectID)
            CheckLocation(szObjectID, hFolder, szTitle);

         if (bCorrect && bError)
            {
            bMoveObject = MyMsgJN(szMsgMove, MROW, TRUE);
            if (bMoveObject)
               {
               BYTE szFolderName[40];
               sprintf(szFolderName,"CheckIni\nMoveObjects%d", usLostCount++);
               hLostFolder = WinCreateObject("WPFolder",
                  szFolderName,
                  NULL,
                  "<WP_DESKTOP>",
                  CO_UPDATEIFEXISTS);
               if (!hLostFolder)
                  errmsg("Unable to create folder!");
               }
            else
               bDiscardObject = MyMsgJN("Delete all objects in this non-existing folder?", MROW, TRUE);
            }
         if (bError && !bCorrect && !bSilent)
            msg_nrm_var("Objects found in non-existing folder!", MROW);
         strcpy(szDirPath, szTitle);
         }

      bError = FALSE;
      bReport = FALSE;

      /***********************************************************
         PROCESSING PER OBJECT
      ***********************************************************/

      memset(szTitle, 0, sizeof szTitle);
      memset(szObjectID, 0, sizeof szObjectID);
      memset(szClass, 0, sizeof szClass);
      memset(szExeName, 0, sizeof szExeName);
      memset(szCurDir, 0, sizeof szCurDir);

      if (bAltIni && IsObjectDisk(pLoc[ulIndex].hObject))
         pObjectData = NULL;
      else
         pObjectData = GetClassInfo(hIniUser, pLoc[ulIndex].hObject, &ulProfileSize);

      if (!pObjectData)
         {
         if (bAltIni && IsObjectDisk(pLoc[ulIndex].hObject))
            strcpy(szClass, "UNKNOWN: Not checked for alternate ini's");
         else
            {
            strcpy(szClass, "UNKNOWN: OBJECT DOES NOT EXIST!");
            bError = TRUE;
            }
         if (bError && !fFolderPrinted)
            {
            pprintf(bError, "\nFolder : %s\n", szDirPath);
            fFolderPrinted = TRUE;
            }
         pprintf(bError, "  Object %lX, Class %s\n",
               pLoc[ulIndex].hObject, szClass);
         if (bCorrect)
            {
            if (MyMsgJN("Remove this non-existing reference from %s?", MROW, TRUE, FOLDERCONTENT))
               RemoveObjectFromFolder(&pLoc[ulIndex]);
            }
         else if (!bSilent)
            msg_nrm_var("Errors found in object definitions !", MROW);
         }
      else
         {
         fDefault = FALSE;

         if (bMoveObject)
            {
            if (!fFolderPrinted)
               {
               pprintf(bError, "\nFolder : %s\n", szDirPath);
               fFolderPrinted = TRUE;
               }
            bReport = TRUE;
            pprintf(TRUE, "  NEXT OBJECT MOVED!\n");
            if (!MyMoveObject || !MyMoveObject(pLoc[ulIndex].hObject, hLostFolder, 0))
               {
               /* Only for 2.x ! */

               RemoveObjectFromFolder(&pLoc[ulIndex]);
               pLoc[ulIndex].hFolder = LOUSHORT(hLostFolder);
               AddObjectToFolder(&pLoc[ulIndex]);
               bRestartNeeded = TRUE;
               }
            }
         else if (bDiscardObject)
            bError = TRUE;

         if (*(PULONG)pObjectData == 0)
            {
            msg_nrm("OBJECT %X - ObjectData damaged !", pLoc[ulIndex].hObject);
            if (!fFolderPrinted)
               {
               pprintf(bError, "\nFolder : %s\n", szDirPath);
               fFolderPrinted = TRUE;
               }
            pprintf(TRUE, "  NEXT OBJECT DATA DAMAGED!\n");
            bError = TRUE;
            *(PULONG)pObjectData = ulProfileSize - 6;
            }

         GetObjectID(pObjectData, szObjectID, sizeof szObjectID);

         if (!GetGenObjectValue(pObjectData, "WPAbstract", WPABSTRACT_TITLE, szTitle, sizeof szTitle))
            {
            strcpy(szTitle, "UNKNOWN TITLE");
            bError = TRUE;
            }
         while ((p = strpbrk(szTitle, "\n\r")) != NULL)
            *p = ' ';

         if (!GetObjectClass(pObjectData, szClass, sizeof szClass))
            {
            strcpy(szClass, "UNKNOWN CLASS!");
            bError = TRUE;
            }
         else if (!fCheckClass(szClass))
            {
            if (!fFolderPrinted)
               {
               pprintf(bError, "\nFolder : %s\n", szDirPath);
               fFolderPrinted = TRUE;
               }
            pprintf(TRUE, "NEXT OBJECT %s : CLASS %s IS NOT A REGISTERED CLASS!\n",
               szTitle, szClass);
            bError = TRUE;
            fDefault = TRUE;
            }

         if (!stricmp(szClass, "WPProgram"))
            {
            if (!GetExeName(pObjectData, szExeName, sizeof szExeName))
               {
               if (!strlen(szExeName))
                  strcpy(szExeName, "PROGRAM NOT FOUND!");
               bError = TRUE;
               }
            if (!GetCurDir(pObjectData, szCurDir, sizeof szCurDir))
               {
               if (!strlen(szCurDir))
                  strcpy(szCurDir, "CURDIR NOT FOUND");
               }

            if (bError && !fFolderPrinted)
               {
               pprintf(bError, "\nFolder : %s\n", szDirPath);
               fFolderPrinted = TRUE;
               }
            pprintf(bError, "  Object %lX %s, Class %s : %s\n   Exename: %s Curdir: %s\n",
               pLoc[ulIndex].hObject,
               szObjectID,
               szClass, 
               szTitle,
               szExeName,
               szCurDir);
            }
         else if (!stricmp(szClass, "WPNetLink") || !stricmp(szClass, "WPShadow") || !stricmp(szClass, "MMShadow") || !stricmp(szClass, "SCShadow"))
            {
            HOBJECT hObj;
            if (!GetGenObjectValue(pObjectData, "WPShadow", WPSHADOW_LINK, &hObj, sizeof hObj))
               {
               ULONG ulType;

               /* Could it be a template ? */

               if (!GetGenObjectValue(pObjectData, "WPObject", WPOBJECT_STYLE, &ulType, sizeof ulType) ||
                  !(ulType & 0x20))
                  {
                  strcpy(szExeName, "NO LINKED OBJECT DEFINED!");
                  fDefault = TRUE;
                  bError = TRUE;
                  }
               }
            else if (GetAbstractTitle(hObj, szExeName, sizeof szExeName))
               {
               while ((p = strpbrk(szExeName, "\n\r")) != NULL)
                  *p = ' ';
               }
            else if (!GetObjectName(hObj, szExeName, sizeof szExeName))
               {
               sprintf(szExeName, "LINKED OBJECT %X NOT FOUND!", hObj);
               bError = TRUE;
               fDefault = TRUE;
               }
            else if (MyAccess(szExeName, 0))
               {
               strcat(szExeName, "<-UNABLE TO ACCESS!");
               bError = TRUE;
               }
            if (bError && !fFolderPrinted)
               {
               pprintf(bError, "\nFolder : %s\n", szDirPath);
               fFolderPrinted = TRUE;
               }
            pprintf(bError, "  Object %lX %s, Class %s : %s\n   Linked to %5.5X: %s\n",
               pLoc[ulIndex].hObject,
               szObjectID,
               szClass, 
               szTitle,
               hObj,
               szExeName);

            if (!bAltIni && !stricmp(szClass, "SCShadow"))
               {
               USHORT usSCIndex;

               for (usSCIndex = 0; usSCIndex < usSCCount; usSCIndex++)
                  if (pLoc[ulIndex].hObject == rgSCObjects[usSCIndex])
                     break;

               if (usSCIndex == usSCCount)
                  {
                  bError = TRUE;
                  if (bError && !fFolderPrinted)
                     {
                     pprintf(bError, "\nFolder : %s\n", szDirPath);
                     fFolderPrinted = TRUE;
                     }
                  pprintf(bError, "  Object %lX %s, Class %s : %s\n IS NOT USED BY WARPCENTER\n",
                     pLoc[ulIndex].hObject,
                     szObjectID,
                     szClass, 
                     szTitle);
                  fDefault = TRUE;
                  }

               }
            }
         else
            {
            if ((bError || bReport) && !fFolderPrinted)
               {
               pprintf(bError, "\nFolder : %s\n", szDirPath);
               fFolderPrinted = TRUE;
               }
            pprintf(bError || bReport, "  Object %lX %s, Class %s : %s\n",
               pLoc[ulIndex].hObject,
               szObjectID,
               szClass, 
               szTitle);
            }

         if (!bDiscardObject && *szObjectID)
            CheckLocation(szObjectID, pLoc[ulIndex].hObject, szTitle);
         free(pObjectData);
         if (bError && bCorrect)
            {
            if (bDiscardObject || MyMsgJN("Remove this object which contains errors?", MROW, fDefault))
               DeleteObject(&pLoc[ulIndex]);
            }
         else if (bError && !bSilent && !bDiscardObject)
            msg_nrm_var("Errors found in object definitions !", MROW);
         }
      }

   free(pLoc);
}

/*****************************************************************
* Check the assoc filters
*****************************************************************/
VOID CheckAssocFilters(VOID)
{
PBYTE pFilterArray,
      pFilter;
PBYTE pAssocArray,
      pAssoc;
HINI  hini = hIniUser;
ULONG ulProfileSize;
HOBJECT hObject;
BYTE  szObjectName[512];
BOOL  bFound;
ULONG ulAssocArray[100];
ULONG ulAssCnt, ulIndex;
BOOL  bError;


   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", ASSOC_FILTER);
   pprintf(TRUE, "=================================================\n");

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningAssocFilter, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }

   pFilterArray = GetAllProfileNames(ASSOC_FILTER, hini, &ulProfileSize);
   if (!pFilterArray)
      {
      pprintf(FALSE, "No %s records found !\n", ASSOC_FILTER);
      return;
      }

   pFilter = pFilterArray;
   while (*pFilter)
      {
      pAssocArray = GetProfileData(ASSOC_FILTER, pFilter, hini, &ulProfileSize);
      if (pAssocArray)
         {
         pprintf(FALSE, "%s is associated with :\n", pFilter);
         bFound = FALSE;
         bError = FALSE;
         pAssoc = pAssocArray;
         ulAssCnt = 0L;

         while (pAssoc < pAssocArray + ulProfileSize)
            {
            hObject = atol(pAssoc);
            ulAssocArray[ulAssCnt] = hObject;
            if (hObject)
               {
               bFound = TRUE;
               if (IsObjectAbstract(hObject))
                  {
                  if (GetAbstractTitle(hObject, szObjectName, sizeof szObjectName))
                     {
                     PBYTE p;
                     while ((p = strpbrk(szObjectName, "\n\r")) != NULL)
                        *p = ' ';
                     pprintf(FALSE, "Object %5lX - %s\n", hObject, szObjectName);
                     ulAssCnt++;
                     }
                  else
                     {
                     pprintf(TRUE, "Object %5lX - DOES NOT EXIST!\n", hObject);
                     bError = TRUE;
                     }
                  }
               else /* non-abstract object */
                  {
                  if (GetObjectName(hObject, szObjectName, sizeof szObjectName))
                     {
                     if (szObjectName[1] == ':' && MyAccess(szObjectName, 0))
                        {
                        strcat(szObjectName, "<-UNABLE TO ACCESS");
                        pprintf(TRUE, "Object %5lX - %s\n", hObject, szObjectName);
                        if (!bCorrect ||
                           !MyMsgJN("Keep %s associated with (non-locateable) %s ?",
                              MROW, FALSE, pFilter, szObjectName))
                           bError = TRUE;
                        else                       
                           ulAssCnt++;
                        }
                     else
                        {
                        pprintf(FALSE, "Object %5lX - %s\n", hObject, szObjectName);
                        ulAssCnt++;
                        }
                     }
                  else
                     {
                     pprintf(TRUE, "Object %5lX - DOES NOT EXIST!\n", hObject);
                     bError = TRUE;
                     }
                  }
               }
            pAssoc += strlen(pAssoc) + 1;
            }
         if (!bFound)
            pprintf(FALSE, " - NOTHING\n");

         if (bCorrect && bError)
            {
            memset(pAssocArray, 0, ulProfileSize);
            ulProfileSize = 0;
            pAssoc = pAssocArray;

            for (ulIndex = 0; ulIndex < ulAssCnt; ulIndex++)
               {
               sprintf(pAssoc,"%ld", ulAssocArray[ulIndex]);
               ulProfileSize += strlen(pAssoc) + 1;
               pAssoc += strlen(pAssoc) + 1;
               }
            if (!ulProfileSize)
               ulProfileSize = 1;
            if (MyMsgJN("Remove the not-existing reference(s) in / %s:%s?", MROW, TRUE,
               ASSOC_FILTER, pFilter))
               WriteProfileData(ASSOC_FILTER, pFilter, hini, pAssocArray, ulProfileSize);
            }
         else if (bError && !bSilent)
            msg_nrm_var("Error(s) in association(s) for %s found!", MROW, pFilter);
         free(pAssocArray);
         }
      else
         errmsg("Error while reading %s:%s\n", ASSOC_FILTER, pFilter);

      pFilter += strlen(pFilter) + 1;
      }
   free(pFilterArray);
}

/*****************************************************************
* Check the assoc types
*****************************************************************/
VOID CheckAssocTypes(VOID)
{
PBYTE pTypeArray,
      pType;
PBYTE pAssocArray,
      pAssoc;
HINI  hini = hIniUser;
ULONG ulProfileSize;
HOBJECT hObject;
BYTE  szObjectName[512];
BOOL  bFound;
ULONG ulAssocArray[100];
ULONG ulAssCnt, ulIndex;
BOOL  bError;


   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", ASSOC_TYPE);
   pprintf(TRUE, "=================================================\n");

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningAssocType, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }

   pTypeArray = GetAllProfileNames(ASSOC_TYPE, hini, &ulProfileSize);
   if (!pTypeArray)
      {
      pprintf(FALSE, "No %s records found !\n", ASSOC_TYPE);
      return;
      }

   pType = pTypeArray;
   while (*pType)
      {

      pAssocArray = GetProfileData(ASSOC_TYPE, pType, hini, &ulProfileSize);
      if (pAssocArray)
         {
         pprintf(FALSE, "%s is associated with :\n", pType);
         bFound = FALSE;
         bError = FALSE;
         pAssoc = pAssocArray;
         ulAssCnt = 0L;

         while (pAssoc < pAssocArray + ulProfileSize)
            {
            hObject = atol(pAssoc);
            ulAssocArray[ulAssCnt] = hObject;
            if (hObject)
               {
               bFound = TRUE;
               if (IsObjectAbstract(hObject))
                  {
                  if (GetAbstractTitle(hObject, szObjectName, sizeof szObjectName))
                     {
                     PBYTE p;
                     while ((p = strpbrk(szObjectName, "\n\r")) != NULL)
                        *p = ' ';
                     pprintf(FALSE, "Object %5lX - %s\n", hObject, szObjectName);
                     ulAssCnt++;
                     }
                  else
                     {
                     pprintf(TRUE, "Object %5lX - DOES NOT EXIST!\n", hObject);
                     bError = TRUE;
                     }
                  }
               else /* non-abstract object */
                  {
                  if (GetObjectName(hObject, szObjectName, sizeof szObjectName))
                     {
                     if (szObjectName[1] == ':' && MyAccess(szObjectName, 0))
                        {
                        strcat(szObjectName, "<-UNABLE TO ACCESS");
                        pprintf(TRUE, "Object %5lX - %s\n", hObject, szObjectName);
                        if (!bCorrect ||
                           !MyMsgJN("Keep %s associated with (non-locateable) %s ?", MROW, FALSE,
                              pType, szObjectName))
                           bError = TRUE;
                        else
                           ulAssCnt++;
                        }
                     else
                        {
                        pprintf(FALSE, "Object %5lX - %s\n", hObject, szObjectName);
                        ulAssCnt++;
                        }
                     }
                  else
                     {
                     pprintf(TRUE, "Object %5lX - DOES NOT EXIST!\n", hObject);
                     bError = TRUE;
                     }
                  }
               }
            pAssoc += strlen(pAssoc) + 1;
            }
         if (!bFound)
            pprintf(FALSE, " - NOTHING\n");

         if (bCorrect && bError)
            {
            memset(pAssocArray, 0, ulProfileSize);
            ulProfileSize = 0;
            pAssoc = pAssocArray;

            for (ulIndex = 0; ulIndex < ulAssCnt; ulIndex++)
               {
               sprintf(pAssoc,"%ld", ulAssocArray[ulIndex]);
               ulProfileSize += strlen(pAssoc) + 1;
               pAssoc += strlen(pAssoc) + 1;
               }
            if (!ulProfileSize)
               ulProfileSize = 1;
            if (MyMsgJN("Remove the not-existing reference(s) in / %s:%s?", MROW, TRUE, 
               ASSOC_TYPE, pType))
               WriteProfileData(ASSOC_TYPE, pType, hini, pAssocArray, ulProfileSize);
            }
         else if (bError && !bSilent)
            msg_nrm_var("Error(s) in association(s) for %s found!", MROW, pType);
         free(pAssocArray);
         }
      else
         errmsg("Error while reading %s:%s\n", ASSOC_TYPE, pType);

      pType += strlen(pType) + 1;
      }
   free(pTypeArray);
}

/*****************************************************************
* Check folderpos records
*****************************************************************/
VOID CheckFolderPos(VOID)
{
HINI  hini = hIniUser;
PBYTE pFolderPos, pFolder;
ULONG ulProfileSize;
BYTE  szObjectName[512];
HOBJECT hObject;
BOOL  bError, bErrorsFound = FALSE;
BOOL  bAsk = FALSE;

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", FOLDERPOS);
   pprintf(TRUE, "=================================================\n");

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningFolderPos, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }

   if (bCorrect)
      {
      bAsk = MyMsgJN("Confirm all removals for non-existing %s records?",
         MROW, FALSE, FOLDERPOS);
      }

   pFolderPos = GetAllProfileNames(FOLDERPOS, hini, &ulProfileSize);
   if (!pFolderPos)
      {
      pprintf(FALSE, "No %s records found !\n", FOLDERPOS);
      return;
      }

   pFolder = pFolderPos;
   while (*pFolder)
      {
      bError = FALSE;
      hObject = atol(pFolder);
      if (IsObjectAbstract(hObject))
         {
         if (GetAbstractTitle(hObject, szObjectName, sizeof szObjectName))
            {
            PBYTE p;
            while ((p = strpbrk(szObjectName, "\n\r")) != NULL)
               *p = ' ';

            pprintf(FALSE, "%s:%s\n  points to %lX - %s\n",
               FOLDERPOS, pFolder, hObject, szObjectName);
            }
         else
            {
            pprintf(TRUE, "%s:%s\n  POINTS TO %lX - NON-EXISTING OBJECT\n",
               FOLDERPOS, pFolder, hObject);
            bError = TRUE;
            }
         }
      else
         {
         if (GetObjectPath(FALSE, hIniSystem, hObject, szObjectName, sizeof szObjectName))
            {
            if (strlen(szObjectName) == 2)
               strcat(szObjectName, "\\");
            if (MyAccess(szObjectName, 0))
               {
               strcat(szObjectName, "<-UNABLE TO ACCESS");
               bError = TRUE;
               }
            pprintf(bError, "%s:%s\n  points to %lX - %s\n",
               FOLDERPOS, pFolder, hObject, szObjectName);
            }
         else
            {
            pprintf(TRUE, "%s:%s\n  POINTS TO %lX - NON-EXISTING OBJECT\n",
               FOLDERPOS, pFolder, hObject);
            bError = TRUE;
            }
         }

      if (bError)
         {
         if (bCorrect &&
            (!bAsk || MyMsgJN("Remove stored windowposition for non-existing object ?", MROW, TRUE)))
            WriteProfileData(FOLDERPOS, pFolder, hini, NULL, 0L);
         bErrorsFound = TRUE;
         }
      pFolder += strlen(pFolder) + 1;
      }
   free(pFolderPos);
   if (bErrorsFound && !bCorrect && !bSilent)
      msg_nrm_var("Stored windowpositions found for non-locateable/or non-existing objects!", MROW);
   if (bErrorsFound && bCorrect && !bAsk && !bAlwaysDefault)
      msg_nrm_var("Error(s) found and corrected!", MROW);
}

/*****************************************************************
* Check folderpos records
*****************************************************************/
VOID CheckJobCnrPos(VOID)
{
HINI  hini = hIniUser;
PBYTE pJobCnrPos, pJobCnr;
ULONG ulProfileSize;
BYTE  szObjectName[512];
HOBJECT hObject;
BOOL  bError;

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", JOBCNRPOS);
   pprintf(TRUE, "=================================================\n");

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningJobCnrPos, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }

   pJobCnrPos = GetAllProfileNames(JOBCNRPOS, hini, &ulProfileSize);
   if (!pJobCnrPos)
      {
      pprintf(FALSE, "No %s records found !\n", JOBCNRPOS);
      return;
      }

   pJobCnr = pJobCnrPos;
   while (*pJobCnr)
      {
      bError = FALSE;
      hObject= MakeAbstractHandle(atoi(pJobCnr));

      if (GetAbstractTitle(hObject, szObjectName, sizeof szObjectName))
         {
         PBYTE p;
         while ((p = strpbrk(szObjectName, "\n\r")) != NULL)
            *p = ' ';

         pprintf(FALSE, "%s:%s\n  points to %lX - %s\n",
            JOBCNRPOS, pJobCnr, hObject, szObjectName);
         }
      else
         {
         pprintf(TRUE, "%s:%s\n  POINTS TO %lX - NON-EXISTING OBJECT\n",
            JOBCNRPOS, pJobCnr, hObject);
         bError = TRUE;
         }

      if (bError)
         {
         if (bCorrect &&
            (MyMsgJN("Remove stored JobCnrPos for non-existing object ?", MROW, TRUE)))
            WriteProfileData(JOBCNRPOS, pJobCnr, hini, NULL, 0L);
         }
      pJobCnr += strlen(pJobCnr) + 1;
      }
   free(pJobCnrPos);
}



/*****************************************************************
* Check Palettepos records
*****************************************************************/
VOID CheckPalettePos(VOID)
{
HINI  hini = hIniUser;
PBYTE pPalettePos, pPalette;
ULONG ulProfileSize;
BYTE  szObjectName[512];
HOBJECT hObject;
BOOL  bError, bErrorsFound = FALSE;

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", PALETTEPOS);
   pprintf(TRUE, "=================================================\n");

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningPalettePos, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }

   pPalettePos = GetAllProfileNames(PALETTEPOS, hini, &ulProfileSize);
   if (!pPalettePos)
      {
      pprintf(FALSE, "No PalettePos records found !\n");
      return;
      }

   pPalette = pPalettePos;
   while (*pPalette)
      {
      bError = FALSE;
      hObject= MakeAbstractHandle(atoi(pPalette));

      if (GetAbstractTitle(hObject, szObjectName, sizeof szObjectName))
         {
         PBYTE p;
         while ((p = strpbrk(szObjectName, "\n\r")) != NULL)
            *p = ' ';

         pprintf(FALSE, "%s:%s\n  points to %lX - %s\n",
            PALETTEPOS, pPalette, hObject, szObjectName);
         }
      else
         {
         pprintf(TRUE, "%s:%s\n  POINTS TO %lX - NON-EXISTING OBJECT\n",
            PALETTEPOS, pPalette, hObject);
         bError = TRUE;
         }

      if (bError)
         {
         if (bCorrect &&
            MyMsgJN("Remove stored palettepos for non-existing object ?", MROW, TRUE))
            WriteProfileData(PALETTEPOS, pPalette, hini, NULL, 0L);
         bErrorsFound = TRUE;
         }
      pPalette += strlen(pPalette) + 1;
      }
   free(pPalettePos);
   if (bErrorsFound && !bCorrect && !bSilent)
      msg_nrm_var("Stored palettepos records found for non-locateable/or non-existing objects!", MROW);
}

/*****************************************************************
* Check Palettepos records
*****************************************************************/
VOID CheckStatusPos(VOID)
{
HINI  hini = hIniUser;
PBYTE pStatusPos, pStatus;
ULONG ulProfileSize;
BYTE  szObjectName[512];
HOBJECT hObject;
BOOL  bError, bErrorsFound = FALSE;

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", STATUSPOS);
   pprintf(TRUE, "=================================================\n");

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningStatusPos, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }

   pStatusPos = GetAllProfileNames(STATUSPOS, hini, &ulProfileSize);
   if (!pStatusPos)
      {
      pprintf(FALSE, "No StatusPos records found !\n");
      return;
      }

   pStatus = pStatusPos;
   while (*pStatus)
      {
      bError = FALSE;
      hObject= MakeAbstractHandle(atoi(pStatus));

      if (GetAbstractTitle(hObject, szObjectName, sizeof szObjectName))
         {
         PBYTE p;
         while ((p = strpbrk(szObjectName, "\n\r")) != NULL)
            *p = ' ';

         pprintf(FALSE, "%s:%s\n  points to %lX - %s\n",
            STATUSPOS, pStatus, hObject, szObjectName);
         }
      else
         {
         pprintf(TRUE, "%s:%s\n  POINTS TO %lX - NON-EXISTING OBJECT\n",
            STATUSPOS, pStatus, hObject);
         bError = TRUE;
         }

      if (bError)
         {
         if (bCorrect &&
            MyMsgJN("Remove stored StatusPos for non-existing object ?", MROW, TRUE))
            WriteProfileData(STATUSPOS, pStatus, hini, NULL, 0L);
         bErrorsFound = TRUE;
         }
      pStatus += strlen(pStatus) + 1;
      }
   free(pStatusPos);
   if (bErrorsFound && !bCorrect && !bSilent)
      msg_nrm_var("Stored statuspos records found for non-locateable/or non-existing objects!", MROW);
}


/*****************************************************************
* Check Palettepos records
*****************************************************************/
VOID CheckPrintObjects(VOID)
{
HINI  hini = hIniUser;
PBYTE pPrintObjects, pPrint;
ULONG ulProfileSize;
BYTE  szObjectName[512];
HOBJECT hObject;
BOOL  bError, bErrorsFound = FALSE;

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", PRINTOBJECTS);
   pprintf(TRUE, "=================================================\n");

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningPrintObjects, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }

   pPrintObjects = GetAllProfileNames(PRINTOBJECTS, hini, &ulProfileSize);
   if (!pPrintObjects)
      {
      pprintf(FALSE, "No %s records found !\n", PRINTOBJECTS);
      return;
      }

   pPrint = pPrintObjects;
   while (*pPrint)
      {
      bError = FALSE;
      hObject= atol(pPrint);
      if (hObject == 0x20000)
         hObject = hObject; // dummy
      else if (IsObjectAbstract(hObject))
         {
         if (GetAbstractTitle(hObject, szObjectName, sizeof szObjectName))
            {
            PBYTE p;
            while ((p = strpbrk(szObjectName, "\n\r")) != NULL)
               *p = ' ';

            pprintf(FALSE, "%s:%s\n  points to %lX - %s\n",
               PRINTOBJECTS, pPrint, hObject, szObjectName);
            }
         else
            {
            pprintf(TRUE, "%s:%s\n  POINTS TO %lX - NON-EXISTING OBJECT\n",
               PRINTOBJECTS, pPrint, hObject);
            bError = TRUE;
            }
         }
      else
         {
         if (GetObjectName(hObject, szObjectName, sizeof szObjectName))
            {
            if (strlen(szObjectName) == 2)
               strcat(szObjectName, "\\");
            if (MyAccess(szObjectName, 0))
               {
               strcat(szObjectName, "<-UNABLE TO ACCESS");
               bError = TRUE;
               }
            pprintf(bError, "%s:%s\n  points to %lX - %s\n",
               PRINTOBJECTS, pPrint, hObject, szObjectName);
            }
         else
            {
            pprintf(TRUE, "%s:%s\n  POINTS TO %lX - NON-EXISTING OBJECT\n",
               PRINTOBJECTS, pPrint, hObject);
            bError = TRUE;
            }
         }

      if (bError)
         {
         if (bCorrect &&
            MyMsgJN("Remove stored printobject record for non-existing object ?", MROW, TRUE))
            WriteProfileData(PRINTOBJECTS, pPrint, hini, NULL, 0L);
         bErrorsFound = TRUE;
         }
      pPrint += strlen(pPrint) + 1;
      }
   free(pPrintObjects);
   if (bErrorsFound && !bCorrect && !bSilent)
      msg_nrm_var("Stored printobjects records found for non-locateable/or non-existing objects!", MROW);
}

/*****************************************************************
* Check Startup records
*****************************************************************/
VOID CheckStartup(VOID)
{
HINI  hini = hIniUser;
PBYTE pStartupPos, pStartup;
ULONG ulProfileSize;
BYTE  szObjectName[512];
HOBJECT hObject;
BOOL  bError, bErrorsFound = FALSE;
PBYTE pClassInfo;
USHORT usEASize;

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", STARTUP);
   pprintf(TRUE, "=================================================\n");
   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningStartup, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }

   pStartupPos = GetAllProfileNames(STARTUP, hini, &ulProfileSize);
   if (!pStartupPos)
      {
      pprintf(FALSE, "No %s records found !\n", STARTUP);
      return;
      }

   pStartup = pStartupPos;
   while (*pStartup)
      {
      PBYTE pStop;
      bError = FALSE;

      hObject = MakeDiskHandle(strtol(pStartup, &pStop, 16));

      if (GetObjectName(hObject, szObjectName, sizeof szObjectName))
         {
         if (strlen(szObjectName) == 2)
            strcat(szObjectName, "\\");
         if (MyAccess(szObjectName, 0))
            {
            strcat(szObjectName, "<-UNABLE TO ACCESS");
            bError = TRUE;
            }
         pprintf(bError, "%s:%s\n  points to %lX - %s\n",
            STARTUP, pStartup, hObject, szObjectName);
         }
      else
         {
         pprintf(TRUE, "%s:%s\n  POINTS TO %lX - NON-EXISTING OBJECT\n",
            STARTUP, pStartup, hObject);
         bError = TRUE;
         }

      if (!bError && !bAltIni)
         {
         if (strstr(szObjectName, szArchivePath))
            {
            pprintf(TRUE, "%s is an ARCHIVED folder\n", szObjectName);
            bError = TRUE;
            }
         }

      if (!bError && !bAltIni && !bAlwaysDefault)
         {
         BYTE szClassName[50];
         strcpy(szClassName, "WPFolder");
         if (GetEAValue(szObjectName, ".CLASSINFO", &pClassInfo, &usEASize))
            {
            strcpy(szClassName, pClassInfo + 8);
            free(pClassInfo);
            }
         if (!IsStartupClass(szClassName))
            {
            bError = TRUE;
            pprintf(TRUE, "%s:%s\nIS NOT A STARTUP CLASS, but of class %s\n",
               STARTUP, pStartup, szClassName);
            if (!bSilent && msg_jn(szStartupClass, MROW, TRUE, szObjectName, szClassName))
               bError = AddToStartupClasses(szClassName, TRUE);
            else
               bError = TRUE;
            if (bError && bCorrect)
               {
               if (MyMsgJN("Try to change to folder to WPStartup class?", MROW, TRUE))
                  if (fRepairFolder(szObjectName, "WPStartup"))
                     bError = FALSE;
               }
            }
         }

      if (bError)
         {
         if (bCorrect &&
            MyMsgJN("Remove stored reference to non-existing or faulty startup folder ?/NOTE: Objects in this folder will no longer be autostarted!", MROW, TRUE))
            WriteProfileData(STARTUP, pStartup, hini, NULL, 0L);
         bErrorsFound = TRUE;
         }
      pStartup += strlen(pStartup) + 1;
      }
   free(pStartupPos);
   if (bErrorsFound && !bCorrect && !bSilent)
      msg_nrm_var("References found to faulty startup folder(s)!", MROW);
   if (bErrorsFound && bCorrect && !bAlwaysDefault)
      msg_nrm_var("Error(s) found and corrected!", MROW);
}



/*****************************************************************
* Check AssocChecksum
*****************************************************************/
VOID CheckAssocCheckSum(VOID)
{
HINI  hini = hIniUser;
PBYTE pAssocChkArray, pCheckSum;
ULONG ulProfileSize;
BYTE  szObjectName[512];
HOBJECT hObject;
BOOL  bError;

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", ASSOC_CHECKSUM);
   pprintf(TRUE, "=================================================\n");


   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningCheckSum, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }

   pAssocChkArray = GetAllProfileNames(ASSOC_CHECKSUM, hini, &ulProfileSize);
   if (!pAssocChkArray)
      {
      pprintf(FALSE, "No %s records found !\n", ASSOC_CHECKSUM);
      return;
      }

   pCheckSum = pAssocChkArray;
   while (*pCheckSum)
      {
      bError = FALSE;
      hObject = atol(pCheckSum);
      if (IsObjectAbstract(hObject))
         {
         if (GetAbstractTitle(hObject, szObjectName, sizeof szObjectName))
            {
            PBYTE p;
            while ((p = strpbrk(szObjectName, "\n\r")) != NULL)
               *p = ' ';

            pprintf(FALSE, "%s:%s\n  points to %lX - %s\n",
               ASSOC_CHECKSUM, pCheckSum, hObject, szObjectName);
            }
         else
            {
            pprintf(TRUE, "%s:%s\n  POINTS TO %lX - NON-EXISTING OBJECT\n",
               ASSOC_CHECKSUM, pCheckSum, hObject);
            bError = TRUE;
            }
         }
      else
         {
         if (GetObjectName(hObject, szObjectName, sizeof szObjectName))
            {
            if (strlen(szObjectName) == 2)
               strcat(szObjectName, "\\");
            if (MyAccess(szObjectName, 0))
               {
               strcat(szObjectName, "<-UNABLE TO ACCESS");
               bError = TRUE;
               }
            pprintf(bError, "%s:%s\n  points to %lX - %s\n",
               ASSOC_CHECKSUM, pCheckSum, hObject, szObjectName);
            }
         else
            {
            pprintf(TRUE, "%s:%s\n  POINTS TO %lX - NON-EXISTING OBJECT\n",
               ASSOC_CHECKSUM, pCheckSum, hObject);
            bError = TRUE;
            }
         }

      if (bError && bCorrect)
         {
         if (MyMsgJN("Remove checksum for a non-existing object?", MROW, TRUE))
            WriteProfileData(ASSOC_CHECKSUM, pCheckSum, hini, NULL, 0L);
         }
      else if (bError && !bSilent)
         msg_nrm_var("Checksums for non-existing objects found!", MROW);

      pCheckSum += strlen(pCheckSum) + 1;
      }
   free(pAssocChkArray);
}

/*****************************************************************
* Check AbstractIcons
*****************************************************************/
VOID CheckAbstractIcons(VOID)
{
HINI  hini = hIniUser;
PBYTE pIconArray, pIcon;
ULONG ulProfileSize;
BYTE  szObjectName[512];
HOBJECT hObject;
BOOL  bError;

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", ICONS);
   pprintf(TRUE, "=================================================\n");

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningIcons, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }


   pIconArray = GetAllProfileNames(ICONS, hini, &ulProfileSize);
   if (!pIconArray)
      {
      pprintf(FALSE, "No %s records found !\n, ICONS");
      return;
      }

   pIcon = pIconArray;
   while (*pIcon)
      {
      PBYTE pStop;
      bError = FALSE;
      hObject = MakeAbstractHandle(strtol(pIcon, &pStop, 16));

      if (GetAbstractTitle(hObject, szObjectName, sizeof szObjectName))
         {
         PBYTE p;
         while ((p = strpbrk(szObjectName, "\n\r")) != NULL)
            *p = ' ';

         pprintf(FALSE, "%s:%s\n  belongs to %lX - %s\n",
            ICONS, pIcon, hObject, szObjectName);
         }
      else
         {
         pprintf(TRUE, "%s:%s\n  belongs TO %lX - NON-EXISTING OBJECT\n",
            ICONS, pIcon, hObject);
         bError = TRUE;
         }
      if (bError && bCorrect)
         {
         if (MyMsgJN("Remove this abstract icon ?", MROW, TRUE))
            WriteProfileData(ICONS, pIcon, hini, NULL, 0L);
         }
      else if (bError && !bSilent)
         msg_nrm_var("Icons found for non-existing abstract objects!", MROW);

      pIcon += strlen(pIcon) + 1;
      }
   free(pIconArray);
}


/*****************************************************************
* Check Templates records
*****************************************************************/
VOID CheckTemplates(VOID)
{
HINI  hini = hIniUser;
PBYTE pAllTemplates, pTemplates;
ULONG ulProfileSize;
BYTE  szObjectName[512];
HOBJECT hObject;

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", TEMPLATES);
   pprintf(TRUE, "=================================================\n");

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningTemplates, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }

   pAllTemplates = GetAllProfileNames(TEMPLATES, hini, &ulProfileSize);
   if (!pAllTemplates)
      {
      pprintf(FALSE, "No %s records found !\n", TEMPLATES);
      return;
      }

   pTemplates = pAllTemplates;
   while (*pTemplates)
      {
      PBYTE p;

      p = strchr(pTemplates, ':');
      if (p)
         {
         BOOL bError = FALSE;

         p++;
         hObject = atol(p);
         if (GetAbstractTitle(hObject, szObjectName, sizeof szObjectName) ||
            GetObjectName(hObject, szObjectName, sizeof szObjectName))
            {

            while ((p = strpbrk(szObjectName, "\n\r")) != NULL)
               *p = ' ';

            if (IsObjectDisk(hObject))
               {
               if (MyAccess(szObjectName, 0))
                  {
                  strcat(szObjectName, "<-UNABLE TO ACCESS");
                  bError = TRUE;
                  }
               }
            pprintf(bError, "%s:%s\n  points to %lX - %s\n",
               TEMPLATES, pTemplates, hObject, szObjectName);
            }
         else if (!IsObjectDisk(hObject) && !IsObjectAbstract(hObject))
            {
            pprintf(bError, "%s:%s\n  POINTS TO %lX - OBJECT OF BASECLASS %s\n",
               TEMPLATES, pTemplates, hObject, GetBaseClassString(hObject));
            }
         else
            {
            pprintf(TRUE, "%s:%s\n  POINTS TO %lX - NON-EXISTING OBJECT\n",
               TEMPLATES, pTemplates, hObject);
            bError = TRUE;
            }
         if (bError)
            {
            if (bCorrect &&
               MyMsgJN("Remove this template that is based on a non-existing object ?", MROW, TRUE))
               WriteProfileData(TEMPLATES, pTemplates, hini, NULL, 0L);
            else if (!bCorrect && !bSilent)
               msg_nrm_var("Template found based on non-existing objects!", MROW);
            }
         }
      else
         pprintf(TRUE, "Corrupted record found: %s\n", pTemplates);
      pTemplates += strlen(pTemplates) + 1;
      }

   free(pAllTemplates);
}
/**********************************************************************
* Checking location records
**********************************************************************/
BOOL CheckLocation(PSZ pszObjectID, HOBJECT hObj, PSZ pszTitle)
{
PBYTE pLocationArray,
      pLocation;
HINI  hini = hIniUser;
ULONG ulProfileSize;
HOBJECT hObject;
BYTE   szObjectName[512];
PULONG pulLocation;
BOOL   bError;
PBYTE  pObjectData;


   if (pszObjectID)
      {
      if (bAltIni)
         return FALSE;

      bError = FALSE;
      hObject = WinQueryObject(pszObjectID);
      if (hObject == hObj)
         return TRUE;

      pulLocation = (PULONG)GetProfileData(LOCATION, pszObjectID, hini, &ulProfileSize);
      if (!pulLocation)
         {
         bError = TRUE;
         pprintf(TRUE, "  OBJECTID %s (from %s) NOT FOUND AT %s\n",
            pszObjectID, pszTitle, LOCATION);
         if (bCorrect)
            {
            if (MyMsgJN("Assign %s to %s?", MROW, TRUE, pszObjectID, pszTitle))
               {
               BYTE szOpt[100];
               sprintf(szOpt, "OBJECTID=%s", pszObjectID);
               MySetObjectData(hObj, szOpt);
               pprintf(TRUE, "  Setting ID %s for this object\n", pszObjectID );
               }
            }
         }
      else if (*pulLocation != hObj)
         {
         if (GetAbstractTitle(*pulLocation, szObjectName, sizeof szObjectName) ||
             GetObjectName(*pulLocation, szObjectName, sizeof szObjectName))
            {
            PBYTE p;
            while ((p = strpbrk(szObjectName, "\n\r")) != NULL)
               *p = ' ';
            pprintf(TRUE, "  OBJECTID %s (from %5lX - %s) POINTS TO ANOTHER OBJECT %5lX - %s\n",
               pszObjectID,
               hObj, pszTitle,
               *pulLocation, szObjectName);
            }
         else
            pprintf(TRUE, "  OBJECTID %s (from %5lX - %s) POINTS TO A ANOTHER, NON-EXISTING OBJECT %5lX!\n",
                pszObjectID,
               hObj, pszTitle,
               *pulLocation);

         if (bCorrect && pszTitle)
            {
            if (MyMsgJN("Assign a new OBJECTID to %s?", MROW, TRUE, pszTitle))
               {
               BYTE szObjectID[20];
               BYTE szSetting[50];
               if (GetUniqueID(szObjectID, pszTitle))
                  {
                  sprintf(szSetting, "OBJECTID=%s", szObjectID);
                  if (MySetObjectData(hObj, szSetting))
                     {
                     pprintf(TRUE, "Setting OBJECTID %s for %5lX - %s\n",
                        szObjectID, hObj, pszTitle);
                     bError = FALSE;
                     }
                  }
               if (bError)
                  pprintf(TRUE, "Unable to assign new OBJECTID to %s\n",
                     pszTitle);
               }
            }
         }
      if (bError && !bSilent && !bCorrect)
         msg_nrm_var("Error found in OBJECTID setting!", MROW);
      free(pulLocation);
      return bError;
      }


   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", LOCATION);
   pprintf(TRUE, "=================================================\n");

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningLocation, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return FALSE;
      }

   pLocationArray = GetAllProfileNames(LOCATION, hini, &ulProfileSize);
   if (!pLocationArray)
      {
      pprintf(TRUE, "No %s records found !\n", LOCATION);
      pprintf(TRUE, "There should be %s records\n",LOCATION);
      return TRUE;
      }

   pLocation = pLocationArray;
   while (*pLocation)
      {
      pulLocation = (PULONG)GetProfileData(LOCATION, pLocation, hini, &ulProfileSize);
      if (pulLocation && ulProfileSize == 4)
         {
         BYTE szObjectID[150];

         memset(szObjectID, 0, sizeof szObjectID);

         bError = FALSE;
         hObject = (HOBJECT)*pulLocation;

         if (IsObjectAbstract(hObject))
            {
            BYTE szObject[10];
            sprintf(szObject, "%lX", LOUSHORT(hObject));
            pObjectData = GetProfileData(OBJECTS, szObject, hIniUser, &ulProfileSize);
            if (!pObjectData)
               {
               sprintf(szObjectName, "Object %5lX - DOES NOT EXIST!", hObject);
               bError = TRUE;
               }
            else
               {
               if (*(PULONG)pObjectData >= ulProfileSize)
                  *(PULONG)pObjectData = ulProfileSize - 6;

               if (!GetGenObjectValue(pObjectData, "WPAbstract", WPABSTRACT_TITLE,
                  szObjectName, sizeof szObjectName))
                  {
                  sprintf(szObjectName, "Object %5lX - no title specified!", hObject);
                  }
               else
                  {
                  PBYTE p;
                  while ((p = strpbrk(szObjectName, "\n\r")) != NULL)
                     *p = ' ';
                  }

               GetObjectID(pObjectData, 
                  szObjectID, sizeof szObjectID);

               free(pObjectData);
               }
            }
         else if (IsObjectDisk(hObject))
            {
            if (GetObjectName(hObject, szObjectName, sizeof szObjectName))
               {
               if (*szObjectName == '*')
                  strcpy(szObjectName, "Command prompt");
               else if (szObjectName[1] == ':' && MyAccess(szObjectName, 0))
                  {
                  strcat(szObjectName, "<-UNABLE TO ACCESS");
                  bError = TRUE;
                  }
               else
                  {
                  ULONG ulObjectSize;
                  pObjectData = GetClassInfo(hIniSystem, hObject, &ulObjectSize);
                  if (pObjectData)
                     {
                     GetObjectID(pObjectData, szObjectID, sizeof szObjectID);
                     free(pObjectData);
                     }
                  }
               }
            else
               {
               sprintf(szObjectName, "Object %5lX - DOES NOT EXIST!", hObject);
               bError = TRUE;
               }
            }
         else
            {
            sprintf(szObjectName, "OBJECT %5lX - Object of baseclass %s\n",
               hObject, GetBaseClassString(hObject));
            strcpy(szObjectID, pLocation);
            }

         pprintf(bError, "%-15s points to %5lX - %s\n", pLocation,
            hObject,
            szObjectName);

         if (!bError && !bAltIni)
            {
            if (stricmp(pLocation, szObjectID))
               {
               if (*szObjectID)
                  {
                  pprintf(TRUE, "OBJECT %s (%lX - %s) CLAIMS TO HAVE ID \'%s\'!\n",
                     pLocation,
                     hObject,
                     szObjectName, szObjectID);
                  if (bCorrect)
                     {
                     if (MyMsgJN("Remove this incorrect location record?", MROW, TRUE))
                        {
                        BYTE szOpt[100];
                        WriteProfileData(LOCATION, pLocation, hini, NULL, 0L);
                        sprintf(szOpt, "OBJECTID=%s", szObjectID);
                        MySetObjectData(hObject, szOpt);
                        }
                     }
                  else if (!bSilent)
                     msg_nrm_var("OBJECTID Conflict found!", MROW);
                  }
               else
                  {
                  pprintf(TRUE, "OBJECT %s (%s) CLAIMS TO HAVE NO ID!\n",
                     pLocation, szObjectName);
                  if (bCorrect)
                     {
                     if (MyMsgJN("Assign %s to this object?", MROW, TRUE, pLocation))
                        {
                        BYTE szOpt[100];
                        sprintf(szOpt, "OBJECTID=%s", pLocation);
                        MySetObjectData(hObject, szOpt);
                        pprintf(TRUE, "Setting ID %s for %s\n",
                           pLocation, szObjectName);
                        }
                     }
                  else if (!bSilent)
                     msg_nrm_var("OBJECTID Conflict found!", MROW);
                  }
               }
            }
         else if (bError && bCorrect)
            {
            if (MyMsgJN("Remove non-existing location record?", MROW, TRUE))
               WriteProfileData(LOCATION, pLocation, hini, NULL, 0L);
            }
         else if (bError && !bSilent)
            msg_nrm_var("Non-existing location found!", MROW);
         }
      else if (pulLocation)
         {
         pprintf(TRUE, "%s CONTAINS AN INVALID VALUE!\n",
            pLocation);
         if (bCorrect && MyMsgJN("Remove invalid location record?", MROW, TRUE))
            WriteProfileData(LOCATION, pLocation, hini, NULL, 0L);
         }
      else
         errmsg("Error while reading %s:%s\n", LOCATION, pLocation);
      if (pulLocation)
         free(pulLocation);
      pLocation += strlen(pLocation) + 1;
      }
   free(pLocationArray);
   return FALSE;
}

/*****************************************************************
* Check FolderWorkareaRunningObjects
*****************************************************************/
VOID CheckWorkarea(VOID)
{
HINI  hini = hIniSystem;
PBYTE pAllWorkareas, pWorkareas;
ULONG ulProfileSize;

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s\n", WORKAREARUNNING);
   pprintf(TRUE, "=================================================\n");

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningWorkarea, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return;
      }

   pAllWorkareas = GetAllProfileNames(WORKAREARUNNING, hini, &ulProfileSize);
   if (!pAllWorkareas)
      {
      pprintf(FALSE, "No %s records found !\n", WORKAREARUNNING);
      return;
      }

   pWorkareas = pAllWorkareas;
   while (*pWorkareas)
      {
      if (MyAccess(pWorkareas, 0))
         {
         pprintf(TRUE, "%s:%s <-UNABLE TO ACCESS\n",
            WORKAREARUNNING, pWorkareas);
         if (bCorrect &&
            MyMsgJN("Remove non-existing workarea?", MROW, TRUE))
               WriteProfileData(WORKAREARUNNING, pWorkareas, hini, NULL, 0L);
         else if (!bSilent)
            msg_nrm_var("Non-existing workarea found!", MROW);
         }
      else
         {
         pprintf(FALSE, "%s:%s\n",
            WORKAREARUNNING, pWorkareas);
         }

      pWorkareas += strlen(pWorkareas) + 1;
      }

   free(pAllWorkareas);
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

/*******************************************************************
* Get the objects title
*******************************************************************/
BOOL GetAbstractTitle(HOBJECT hObject, PSZ pszTitle, USHORT usMax)
{
USHORT usObjID;
PBYTE  pBuffer;
ULONG  ulProfileSize;
BYTE  szObjID[10];

   if (!IsObjectAbstract(hObject))
      return FALSE;

   usObjID = LOUSHORT(hObject);

   sprintf(szObjID, "%X", usObjID);
   pBuffer = GetProfileData(OBJECTS, szObjID, hIniUser, &ulProfileSize);
   if (!pBuffer)
      return FALSE;
   if (*(PULONG)pBuffer >= ulProfileSize)
      *(PULONG)pBuffer = ulProfileSize - 6;
   if (!GetGenObjectValue(pBuffer, "WPAbstract", WPABSTRACT_TITLE, pszTitle, usMax))
      {
      free(pBuffer);
      return FALSE;
      }
   free(pBuffer);
   return TRUE;
}

/*************************************************************
* Get the object name as specified in hObject
*************************************************************/
BOOL GetObjectName(HOBJECT hObject, PSZ pszFname, USHORT usMax)
{
   if (IsObjectAbstract(hObject))
      return GetReffedProgName(hObject, pszFname, usMax);

   if (!IsObjectDisk(hObject))
      return FALSE;
  
   if (!GetObjectPath(TRUE, hIniSystem, hObject, pszFname, usMax))
      return FALSE;
   if (strlen(pszFname) == 2)
      strcat(pszFname, "\\");
   return TRUE;
}
      
/*************************************************************
* Get object className
*************************************************************/
BOOL GetObjectClass(PBYTE pBuffer, PSZ pszClass, USHORT usMax)
{
   memset(pszClass, 0, usMax);
   strcpy(pszClass, pBuffer + 4);
   return TRUE;
}

/*************************************************************
* Get Abstract name
*************************************************************/
BOOL GetReffedProgName(HOBJECT hObject, PSZ pszExe, USHORT usMax)
{
USHORT  usObjID = LOUSHORT(hObject);
ULONG   ulProfileSize;
BYTE    szObjID[10];
HOBJECT hObj;
PBYTE   pBuffer;

   memset(pszExe, 0, usMax);
   sprintf(szObjID, "%X", usObjID);
   pBuffer = GetProfileData(OBJECTS, szObjID, hIniUser, &ulProfileSize);
   if (!pBuffer)
      return FALSE;
   if (*(PULONG)pBuffer >= ulProfileSize)
      *(PULONG)pBuffer = ulProfileSize - 6;

   if (GetGenObjectValue(pBuffer, "WPProgramRef", WPPROGRAM_EXENAME, pszExe, usMax))
      {
      free(pBuffer);
      return TRUE;
      }

   if (!GetGenObjectValue(pBuffer, "WPProgramRef", WPPROGRAM_EXEHANDLE, &hObj, sizeof hObj))
      {
      free(pBuffer);
      return FALSE;
      }
   /*
      Special situation, name is cmd or command
   */
   if (hObj == 0xFFFF)
      {
      strcpy(pszExe, "*");
      free(pBuffer);
      return TRUE;
      }

   free(pBuffer);

   if (!GetObjectPath(TRUE, hIniSystem, MakeDiskHandle(hObj), pszExe, usMax))
      return FALSE;
   if (strlen(pszExe) == 2)
      strcat(pszExe, "\\");
   return TRUE;
}

/*************************************************************
* Get the data from the profile
*************************************************************/
BOOL WriteProfileData(PSZ pszApp, PSZ pszKey, HINI hini, PBYTE pBuffer, ULONG ulProfileSize)
{
ULONG ulCurSize;

   if (!ulProfileSize)
      pBuffer = NULL;

   if (!pBuffer)
      pprintf(TRUE, "  DELETING %s:%s from the profile\n", pszApp, pszKey);
   else
      pprintf(TRUE, "  CORRECTING %s:%s\n", pszApp, pszKey);
   disp_attr(iMaxScreenRow - 1, 1, A_INPUT_EMP, 78);

   if (!pBuffer && !PrfQueryProfileSize(hini,
      pszApp,
      pszKey,
      &ulCurSize))
      return TRUE;

   if (!PrfWriteProfileData(hini,
      pszApp,
      pszKey,
      pBuffer,
      ulProfileSize))
      {
      pprintf(TRUE, "PrfWriteProfile Failed %s", GetPMError());
      errmsg("Error when writing %s:%s into the profile!",
         pszApp, pszKey);
      }
   DosSleep(100);
   return TRUE;
}

/***************************************************************
* PPRINTF
***************************************************************/
INT _cdecl pprintf(BOOL bWrite, PSZ pszFormat, ...)
{
PBYTE p, pOld;
va_list va;
INT   iLen;


   if (!bWrite && !bWriteAll)
      return 0;

   va_start(va, pszFormat);
   if (!fDisplayInit)
      {
      return vprintf(pszFormat, va);
      }

   vsprintf(szMess, pszFormat, va);
   if (bWriteAll == TRUE || (bWrite && bWriteAll != 3))
      {
      p = szMess;
      while (*p)
         {
         pOld = p;
         p = strchr(pOld, '\n');
         if (p)
            iLen = (p - pOld);
         else
            iLen = strlen(pOld);
         if (iLen > 78)
            iLen = 78;
         if (!fNoScroll)
            box_scroll_attr(0, 0, iMaxScreenRow, MAX_COL - 1, 1, A_TXT_NRM);
         fNoScroll = FALSE;
         display(iMaxScreenRow - 1, 1, pOld, iLen);
         p = pOld + iLen;
         if (*p == '\n')
            p++;
         }
      }

   if (bWrite || bWriteAll)
      fputs(szMess, fError);
   return strlen(szMess);
}

/*********************************************************************
* REMOVE the OBJECT
*********************************************************************/
BOOL DeleteObject(POBJLOC pObj)
{
BYTE   szObjID[10];
PBYTE  pObjectData;
ULONG  ulProfileSize;

   if (WinDestroyObject(pObj->hObject))
      {
      pprintf(TRUE, "  OBJECT %X DELETED!\n", pObj->hObject);
      DosSleep(100);
      return TRUE;
      }

   bRestartNeeded = TRUE;

   sprintf(szObjID,   "%X", LOUSHORT(pObj->hObject));

   /*
      Read and delete the object itself
   */

   pObjectData = GetProfileData(OBJECTS, szObjID, hIniUser, &ulProfileSize);
   if (pObjectData)
      {
      free(pObjectData);
      if (!WriteProfileData(OBJECTS, szObjID, hIniUser, NULL, 0L))
         return FALSE;
      }
   else
      return FALSE;

   if (!RemoveObjectFromFolder(pObj))
      return FALSE;

      pprintf(TRUE, "  OBJECT %X DELETED!\n", pObj->hObject);
   return TRUE;
}

/*******************************************************************
* Remove object from folder
*******************************************************************/
BOOL RemoveObjectFromFolder(POBJLOC pObj)
{
BYTE   szFolderID[10];
PULONG pulFldrContent;
ULONG  ulProfileSize;
ULONG ulIndex;

   sprintf(szFolderID,"%X", LOUSHORT(pObj->hFolder));

   /*
      Read and adjust the FldrContent record
   */
   pulFldrContent = (PULONG)GetProfileData(FOLDERCONTENT, szFolderID, hIniUser, &ulProfileSize);
   if (pulFldrContent)
      {
      ulProfileSize /= sizeof (ULONG);
      for (ulIndex = 0 ; ulIndex < ulProfileSize; ulIndex++)
         {
         if (pulFldrContent[ulIndex] == LOUSHORT(pObj->hObject))
            {
            ulProfileSize--;
            break;
            }
         }

      while (ulIndex < ulProfileSize)
         {
         pulFldrContent[ulIndex] = pulFldrContent[ulIndex + 1];
         ulIndex++;
         }

      ulProfileSize *= sizeof (ULONG);
      WriteProfileData(FOLDERCONTENT, szFolderID, hIniUser,
         (PBYTE)pulFldrContent, ulProfileSize);
      free((PBYTE)pulFldrContent);
      return TRUE;
      }
   return FALSE;
}

/***************************************************************
* AddObjectToFolder
***************************************************************/
BOOL AddObjectToFolder(POBJLOC pObj)
{

BYTE   szFolderID[10];
PULONG pulFldrContent, pulNew;
ULONG  ulProfileSize;
ULONG ulIndex;

   sprintf(szFolderID,"%X", LOUSHORT(pObj->hFolder));
   /*
      Read and adjust the FldrContent record
   */
   pulFldrContent = (PULONG)GetProfileData(FOLDERCONTENT, szFolderID, hIniUser, &ulProfileSize);
   if (pulFldrContent)
      {
      ulProfileSize /= sizeof (ULONG);
      for (ulIndex = 0 ; ulIndex < ulProfileSize; ulIndex++)
         {
         if (pulFldrContent[ulIndex] == LOUSHORT(pObj->hObject))
            break;
         }
      if (ulIndex < ulProfileSize)
         {
         free(pulFldrContent);
         return TRUE;
         }
      ulProfileSize++;
      }
   else
      ulProfileSize = 1;

   pulNew = calloc(ulProfileSize, sizeof (ULONG));
   if (!pulNew)
      {
      if (pulFldrContent)
         free(pulFldrContent);
      return FALSE;
      }
   if (pulFldrContent)
      {
      memcpy(pulNew, pulFldrContent, ulProfileSize * sizeof (ULONG));
      free(pulFldrContent);
      }

   pulNew[ulProfileSize-1] = LOUSHORT(pObj->hObject);
   ulProfileSize *= sizeof (ULONG);
   WriteProfileData(FOLDERCONTENT, szFolderID, hIniUser,
      (PBYTE)pulNew, ulProfileSize);
   free((PBYTE)pulNew);
   return TRUE;
}

/***********************************************************************
* Get exename
***********************************************************************/
BOOL GetExeName(PBYTE pObjectData, PSZ pszExeName, USHORT usMax)
{
WPPGMDATA wpData;
WPOBJDATA wpOData;
ULONG     ulObjectStyle;
HOBJECT   hObjectPath = 0;

   memset(pszExeName, 0, usMax);
   if (GetGenObjectValue(pObjectData, "WPProgramRef", WPPROGRAM_DATA, &wpData, sizeof wpData))
      {
      if (wpData.hExeHandle)
         {
         if (wpData.hExeHandle == 0xFFFF)
            {
            strcpy(pszExeName, "Command prompt");
            return TRUE;
            }
         else
            {
            if (!GetObjectPath(TRUE, hIniSystem,
               MakeDiskHandle(wpData.hExeHandle),
               pszExeName, usMax))
               {
               sprintf(pszExeName, "CANNOT RESOLVE HANDLE %X", MakeDiskHandle(wpData.hExeHandle));
               return FALSE;
               }
            }
         }
      else
         {
         if (GetGenObjectValueSubValue(pObjectData, "WPProgramRef", WPPROGRAM_STRINGS,
            WPPGM_STR_EXENAME, pszExeName, usMax))
            return TRUE;
         if (GetGenObjectValue(pObjectData, "WPObject", WPOBJECT_DATA, &wpOData, sizeof wpOData))
            {
            if (wpOData.ulObjectStyle & TYPE_TEMPLATE)
               {
               strcpy(pszExeName, "Template Program");
               return TRUE;
               }
            }
         strcpy(pszExeName, "No program specified");
         return TRUE;
         }
      }
   else
      {
      if (!GetGenObjectValue(pObjectData, "WPProgramRef",
         WPPROGRAM_EXENAME, pszExeName, usMax))
         {
         if (GetGenObjectValue(pObjectData, "WPProgramRef",
            WPPROGRAM_EXEHANDLE, (PBYTE)&hObjectPath, sizeof hObjectPath))
            {
            if (hObjectPath == 0xFFFF)
               {
               strcpy(pszExeName, "*");
               return TRUE;
               }
            else if (!hObjectPath)
               {
               strcpy(pszExeName, "no program specified");
               return TRUE;
               }
            else
               {
               if (!GetObjectPath(TRUE, hIniSystem,
                  MakeDiskHandle(hObjectPath),
                  pszExeName, usMax))
                  {
                  sprintf(pszExeName, "CANNOT RESOLVE HANDLE %X", MakeDiskHandle(hObjectPath));
                  return FALSE;
                  }

               }
            }
         else
            {
            if (GetGenObjectValue(pObjectData, "WPObject", WPOBJECT_STYLE, &ulObjectStyle, sizeof ulObjectStyle))
               {
               if (ulObjectStyle & TYPE_TEMPLATE)
                  {
                  strcpy(pszExeName, "Template Program");
                  return TRUE;
                  }
               }
            return FALSE;
            }
         }
      else
         return TRUE;
      }

   if (strlen(pszExeName) == 2)
      strcat(pszExeName, "\\");

   if (MyAccess(pszExeName, 0))
      {
      strcat(pszExeName, "<-UNABLE TO ACCESS!");
      return FALSE;
      }
   return TRUE;
}

/***********************************************************************
* Get CurDir
***********************************************************************/
BOOL GetCurDir(PBYTE pObjectData, PSZ pszCurDir, USHORT usMax)
{
WPPGMDATA wpData;
HOBJECT   hObjectPath = 0;

   memset(pszCurDir, 0, usMax);
   if (GetGenObjectValue(pObjectData, "WPProgramRef", WPPROGRAM_DATA, &wpData, sizeof wpData))
      {
      if (wpData.hCurDirHandle)
         {
         if (!GetObjectPath(TRUE, hIniSystem,
            MakeDiskHandle(wpData.hCurDirHandle),
            pszCurDir, usMax))
            {
            sprintf(pszCurDir, "CANNOT RESOLVE HANDLE %X", MakeDiskHandle(wpData.hCurDirHandle));
            return FALSE;
            }
         }
      else
         {
         strcpy(pszCurDir, "Not specified!");
         return TRUE;
         }
      }
   else
      {
      if (GetGenObjectValue(pObjectData, "WPProgramRef",
         WPPROGRAM_DIRHANDLE, &hObjectPath, sizeof hObjectPath))
         {
         if (!GetObjectPath(TRUE, hIniSystem,
            MakeDiskHandle(hObjectPath),
            pszCurDir, usMax))
            {
            sprintf(pszCurDir, "CANNOT RESOLVE HANDLE %X", MakeDiskHandle(wpData.hCurDirHandle));
            return FALSE;
            }
         }
      else
         {
         strcpy(pszCurDir, "Not specified!");
         return TRUE;
         }
      }
   if (strlen(pszCurDir) == 2)
      strcat(pszCurDir, "\\");

   if (MyAccess(pszCurDir, 0))
      {
      strcat(pszCurDir, "<-UNABLE TO ACCESS!");
      return FALSE;
      }
   return TRUE;
}

void CtrlCHandler(int signal)
{
   if (bAltIni)
      {
      PrfCloseProfile(hIniUser);
      PrfCloseProfile(hIniSystem);
      }
   signal = signal;
   printf("\nprogram aborted!\n");
   exit(1);
}

PNODE _System GetObjectPath(BOOL fAddToInUse, HINI hIniSystem, HOBJECT hObject, PSZ pszFname, USHORT usMax)
{
PNODE pNode;
PBYTE p;

   if (fAddToInUse)
      p = (PBYTE)&fHandleUsed;
   else
      p = NULL;

   if (!IsObjectDisk(hObject))
      return NULL;

   pNode = PathFromObject(hIniSystem, hObject, pszFname, usMax, p);
   if (pNode && fAddToInUse)
      fHandleUsed[LOUSHORT(hObject)] = TRUE;

   return pNode;
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
   WinUpper(0, 0, 0,pszObjectID);
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
      WinUpper(0, 0, 0,pszObjectID);
      }

   return TRUE;
}

/*******************************************************************
* Check Workplace shells compatibility
*******************************************************************/
BOOL CheckWPS(BOOL fRepair)
{
PBYTE pObjectData;
ULONG ulClassInfoSize;
BYTE  szObjectID[150];
BYTE  szClassName[50];
BYTE  szDesktopDir[CCHMAXPATH];

   pprintf(TRUE, "Querying WP_DESKTOP\n");
   hobjDesktop = WinQueryObject(szDesktopPath);
   if (!hobjDesktop)
      {
      DosSleep(300);
      hobjDesktop = WinQueryObject(szDesktopPath);
      }
   if (!hobjDesktop)
      {
      if (!bCorrect || !fRepair)
         {
         pprintf(TRUE, "WARNING: Unable to retrieve the object handle for %s!\n", szDesktopPath);
         return FALSE;
         }
      else
         {
         pprintf(TRUE, "FATAL ERROR: Unable to locate the desktop!\n");
         pprintf(TRUE, "(%s not found)\n", szDesktopPath);
         pprintf(TRUE, "TRY Starting CHECKINI with the /D option.\n");
         pprintf(TRUE, "Example: CHECKINI /C /DC:\\DESKTOP.\n");
         return FALSE;
         }
      }
   else if (bCorrect && fRepair)
      MySetObjectData(hobjDesktop, "OBJECTID=<WP_DESKTOP>");

   pprintf(TRUE, "Desktop found (Desktop handle is 0x%lX)\n", hobjDesktop);
   if (!IsObjectDisk(hobjDesktop))
      {
      pprintf(TRUE, "ERROR: THIS IS AN OBJECT OF AN INCORRECT TYPE!\n");
      return FALSE;
      }
   else
      {
      if (!GetObjectPath(TRUE, HINI_PROFILE, hobjDesktop, szDesktopDir, sizeof szDesktopDir))
         {
         pprintf(TRUE, "ERROR: CANNOT RESOLVE THE HANDLE TO A PATH!\n");
         return FALSE;
         }
      else
         {
         USHORT usEASize;
         PBYTE  pEAValue;
         pprintf(TRUE, "Desktop location: %s\n", szDesktopDir);
         if (!GetEAValue(szDesktopDir, ".CLASSINFO", &pEAValue, &usEASize))
            {
            pprintf(TRUE, "ERROR: CANNOT FIND THE .CLASSINFO EXTENDED ATTRIBUTE!\n");
            if (!fRepair)
               return FALSE;
            }
         else
            free(pEAValue);
         }
      }

   if (!bAltIni && bCorrect)
      {
      pObjectData = GetClassInfo(NULL, hobjDesktop, &ulClassInfoSize);
      if (!pObjectData)
         {
         pprintf(TRUE, "FATAL ERROR: Unable to get the Desktop's object data!\n");
         pprintf(TRUE, "             Its extended attributes might have been damaged\n");

         if (fRepair)
            RepairDesktop(hobjDesktop);
         return FALSE;
         }

      if (!GetObjectID(pObjectData, szObjectID, sizeof szObjectID) ||
         memcmp(szObjectID, "<WP_DESKTOP>", strlen("<WP_DESKTOP>")))
         {
         pprintf(TRUE, "FATAL ERROR: The Desktop doesn't have <WP_DESKTOP> as OBJECTID!\n");
         pprintf(TRUE, "             Its extended attributes might have been damaged\n");
         if (fRepair && RepairDesktop(hobjDesktop))
            {
            free(pObjectData);
            return FALSE;
            }
         free(pObjectData);
         return FALSE;
         }

      GetObjectClass(pObjectData, szClassName, sizeof szClassName);
      if (strcmp(szClassName, "WPDesktop"))
         {
         pprintf(TRUE, "WARNING : The desktop is not of class WPDesktop,\n");
         pprintf(TRUE, "but of class %s\n", szClassName);
         pprintf(TRUE, "You will probably have problems like missing menu-items\n");
         }
      free(pObjectData);
      }

   return TRUE;
}



BYTE szMessageRepair[]=
"\a\n\
\n\
CheckIni found problems with the DESKTOP.\n\n\
This could be because the way the Workplace Shell stores its information \n\
has been changed and this version of CheckIni is incompatible with the WPS.\n\
\n\
An other possible cause is that the extended attributes of the Desktop have\n\
been damaged in the past.\n";

BYTE szMessageExit[]=
"\nCheckIni will terminate now. Please run RESETWPS now, and retry CHECKINI.\n";




/*******************************************************************
* Repair the desktop
*******************************************************************/
BOOL RepairDesktop(HOBJECT hObject)
{
BYTE chChar;

   pprintf(TRUE, szMessageRepair);
   pprintf(TRUE, "Should CheckIni try to repair the Desktop [YN]?:");
   fflush(stdout);

   do
      {
      chChar = getch();
      if (chChar == 'y')
         chChar = 'Y';
      else if (chChar == 'n')
         chChar = 'N';
      } while (chChar != 'N' && chChar != 'Y');
   pprintf(TRUE, "\n");
   if (chChar == 'N')
      return FALSE;

   if (chChar == 'Y')
      {
      static BYTE  szPath[CCHMAXPATH];

      if (!WinQueryObjectPath(hObject, szPath, sizeof szPath))
         {
         pprintf(TRUE, "ERROR: Cannot retrieve the path of the desktop!");
         return FALSE;
         }
//      if (!fRepairFolder(szPath, "WPDesktop"))
//         return FALSE;

      pprintf(TRUE, "Assigning OBJECTID <WP_DESKTOP> to the Desktop\n");
      MySetObjectData(hObject, "OBJECTID=<WP_DESKTOP>");
      }

   pprintf(TRUE, szMessageExit);
   return TRUE;
}


/*******************************************************************
* Recreate a directory
*******************************************************************/
BOOL RecreateDir(PSZ pszDirToCreate)
{
static BYTE  szPath[CCHMAXPATH];
APIRET rc;
PSZ    p;
BOOL   fRetco = TRUE;

   if (pszDirToCreate[1] != ':')
      return FALSE;

   if (strlen(pszDirToCreate) < 4) /* C:\ plus another character */
      return FALSE;

   if (!IsDriveLocalAttach(pszDirToCreate[0]))
      return FALSE;

   if (!MyMsgJN(szMsgRecreate, MROW, TRUE))
      return FALSE;

   p = strchr(pszDirToCreate + 3, '\\');
   while (p)
      {
      memset(szPath, 0, sizeof szPath);
      memcpy(szPath, pszDirToCreate, p - pszDirToCreate);
      if (MyAccess(szPath, 0))
         {
         rc = DosCreateDir(szPath, NULL);
         if (rc)
            {
            fRetco = FALSE;
            break;
            }
         }
      p = strchr(p+1, '\\');
      }

   if (fRetco)
      {
      rc = DosCreateDir(pszDirToCreate, NULL);
      if (rc)
         fRetco = FALSE;
      }

   if (!fRetco)
      msg_nrm_var("CHECKINI was unable to recreate the directory (SYS%4.4u)!",
         MROW, rc);
   else
      msg_nrm_var("Directory %s recreated !", MROW, pszDirToCreate);
   return fRetco;
}

/*********************************************************************
* Is a drive local ?
*********************************************************************/
BOOL IsDriveLocalAttach(BYTE bDrive)
{
BYTE  szDrive[3];
ULONG ulOrdinal = 1L;
ULONG ulBufferSize;
BYTE  Buffer[200];
PFSQBUFFER2 fsqBuf = (PFSQBUFFER2)Buffer;
APIRET rc;
  
   szDrive[0] = bDrive;
   szDrive[1] = ':';
   szDrive[2] = 0;

   ulBufferSize = sizeof Buffer;

   rc = DosQueryFSAttach(szDrive,
      ulOrdinal,
      FSAIL_QUERYNAME,
      fsqBuf,
      &ulBufferSize);
   if (rc)
      return FALSE;

   if (fsqBuf->iType == FSAT_LOCALDRV)
      return TRUE;
   return FALSE;
}

/*********************************************************************
* 
*********************************************************************/
BOOL IsDriveLocalType(BYTE bDrive)
{
APIRET rc;
BYTE szDrive[3];
HFILE hFile;
ULONG ulAction;
ULONG ulType;
ULONG ulAttr;

   szDrive[0] = bDrive;
   szDrive[1] = ':';
   szDrive[2] = 0;
   

   rc = DosOpen(szDrive,
      &hFile,
      &ulAction,                          /* action taken */
      0L,                                 /* new size     */
      0,                                 /* attributes   */
      OPEN_ACTION_OPEN_IF_EXISTS,         /* open flags   */
      OPEN_ACCESS_READONLY | OPEN_SHARE_DENYNONE | OPEN_FLAGS_DASD,
      0L);
   if (rc)
      return FALSE;

   rc = DosQueryHType(hFile, &ulType, &ulAttr);
   DosClose(hFile);

   if (rc || ulType && 0x7FFF)
      return FALSE;

   return TRUE;
}


INT MyAccess(PSZ pszFile, INT iMode)
{
   if (bAltIni)
      return 0;

   if (!bNoRemote)
      {
      USHORT rc;
      if (!memcmp(pszFile, "\\\\", 2))
         {
         BYTE szMessage[256];
         if (!fNoScroll)
            box_scroll_attr(0, 0, iMaxScreenRow, MAX_COL - 1, 1, A_TXT_NRM);
         fNoScroll = TRUE;
         sprintf(szMessage, "Checking for %s", pszFile);
         display(iMaxScreenRow - 1, 1, szMessage, 78);
         rc = access(pszFile, iMode);
         memset(szMessage, 0, sizeof szMessage);
         display(iMaxScreenRow - 1, 1, szMessage, 78);
         }
      else
         rc = access(pszFile, iMode);
      return rc;
      }

   if (!memcmp(pszFile, "\\\\", 2))
      {
      BYTE szPath[256];
      INT i, j;
      strncpy(szPath, pszFile, sizeof szPath);
      for (i=j=0; szPath[i]; i++)
         {
         if (j == 4)
            {
            szPath[i] = 0;
            break;
            }
         if (szPath[i] =='\\')
            j++;
         }

      if (access(szPath, 0))
         return 0;
      }
   else
      {
      if (!IsDriveLocalType(pszFile[0]))
         return 0;

      if (IsDriveRemovable(pszFile[0]))
         return 0;
      }
   return access(pszFile, iMode);
}

BOOL DoesDriveExist(BYTE bDrive)
{
static ULONG ulDriveMap = 0;
ULONG ulCurDisk;
ULONG ulMask;
WORD  wDrive;

   if (!ulDriveMap)
      DosQCurDisk(&ulCurDisk, &ulDriveMap);

   wDrive = (WORD)(toupper(bDrive) - 'A');
   ulMask = 0x0001 << wDrive;

   if (!(ulDriveMap & ulMask))
      return FALSE;
   return TRUE;

}

BOOL IsDriveRemovable(BYTE bDrive)
{
APIRET rc;
BYTE rgParm[2];
BYTE rgData[1];
ULONG ulParmSize;
ULONG ulDataSize;
WORD  wDrive = (WORD)(toupper(bDrive) - 'A');


   rgParm[0] = 0;
   rgParm[1] = wDrive;

   rc = DosDevIOCtl( -1,
      IOCTL_DISK,
      DSK_BLOCKREMOVABLE,
      rgParm,
      sizeof rgParm,
      &ulParmSize,
      rgData,
      sizeof rgData,
      &ulDataSize);
   if (rc)
      return FALSE;
   return (BOOL)!rgData[0];
}

/*****************************************************************
* Check PM_Objects records
*****************************************************************/
VOID CheckWPSClasses(VOID)
{
ULONG  ulSize;
BOOL   bError;
BYTE   szClassName[128];
static BYTE  szDLLName[CCHMAXPATH];
static BYTE  szFailName[CCHMAXPATH];
APIRET rc;
POBJCLASS pObj;
BOOL   fCheck = TRUE;

   if (bAltIni)
      return;

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking %s:%s\n", PM_OBJECTS, CLASSTABLE);
   pprintf(TRUE, "=================================================\n");

   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningPMObjects, MROW) == KEY_ESC)
      {
      fCheck = FALSE;
      pprintf(TRUE, "Test skipped !\n");
      }

   if (pObjClass)
      {
      free(pObjClass);
      pObjClass = NULL;
      }
   rc = WinEnumObjectClasses(NULL, &ulSize);
   if (!rc)
      errmsg("WinEnumObjectClasses failed: %s", GetPMError());
   pObjClass = (POBJCLASS)malloc(ulSize);
   if (!pObjClass)
      errmsg("Not enough memory for object classes!");
   rc = WinEnumObjectClasses(pObjClass, &ulSize);
   if (!fCheck)
      return;

   pObj = pObjClass;
   while (pObj)
      {
      bError = FALSE;
      strcpy(szClassName, pObj->pszClassName);
      strcpy(szDLLName, pObj->pszModName);
      WinUpper(0, 0, 0,szDLLName);

      if (!strstr(szClassName, "WPTouch") &&
          !strstr(szDLLName,   "WPPRINT") &&
          !strstr(szDLLName,   "LSGWPS") &&
          !strstr(szDLLName,   "ACSAGENT") &&
          !strstr(szDLLName,   "EZPLAY2"))
         {
         HMODULE hMod;
         BOOL fLoaded = FALSE;

         memset(szFailName, 0, sizeof szFailName);
         rc = DosQueryModuleHandle(szDLLName, &hMod);
         if (rc)
            {
            rc = DosLoadModule(szFailName, sizeof szFailName, szDLLName, &hMod);
            fLoaded = TRUE;
            }

         if (rc && strlen(szDLLName))
            {
            bError = TRUE;
            pprintf(bError, "Class %s in Module %s (%s CANNOT be loaded: SYS%4.4u)\n",
               szClassName,
               szDLLName,
               szFailName,
               rc);
            if (!bCorrect && !bSilent)
               msg_nrm_var("Module %s of class %s cannot be loaded!", MROW,
                  szDLLName, szClassName);
            if (bCorrect && (rc == 2 || rc == 3))
               {
               if (MyMsgJN("Deregister class %s because %s cannot be loaded?",
                     MROW, TRUE, szClassName, szDLLName))
                     {
                     rc = WinDeregisterObjectClass(szClassName);
                     if (!rc)
                        msg_nrm_var("Unable to deregister class %s", MROW, szClassName);
                     else
                        pprintf(TRUE, "Class %s succesfully deregistered!\n",
                           szClassName);
                     }
               }
            }
         else
            {
            if (fLoaded)
               DosFreeModule(hMod);
            pprintf(bError, "Class %s in Module %s\n",
               szClassName,
               szDLLName);
            }
         }
      else
         pprintf(bError, "Class %s in Module %s\n",
            szClassName,
            szDLLName);


      pObj = pObj->pNext;
      }
}

BOOL fCheckClass(PSZ pszClassName)
{
POBJCLASS pObj;

   if (!strcmp(pszClassName, "WPProgramGroup"))
      return TRUE;

   if (!pObjClass)
      return TRUE;

   pObj = pObjClass;
   while (pObj)
      {
      if (!strcmp(pszClassName, pObj->pszClassName))
         return TRUE;
      pObj = pObj->pNext;
      }
   return FALSE;
}

BOOL fGetWarpFunctions(VOID)
{
HMODULE hmod;
APIRET  rc;
BYTE szFailMod[CCHMAXPATHCOMP];

   if (!IsWarp())
      return TRUE;

   rc = DosLoadModule(szFailMod, sizeof szFailMod, "PMWP", &hmod);
   if (rc)
      errmsg("PMWP.DLL has not been loaded SYS%4.4u on %s - Exiting...", rc, szFailMod);

   // get adress of WinMoveObject
   rc = DosQueryProcAddr(hmod, 287, NULL, (PFN *)&MyMoveObject);
   if (rc)
      {
      MyMoveObject = NULL;
      return FALSE;
      }

   return TRUE;
}

BOOL MyMsgJN(PSZ pszMess, short sRow, BOOL fDefault, ...)
{
va_list va;

    if (bAlwaysDefault)
      return fDefault;

   va_start(va, fDefault);
   vsprintf(szMess, pszMess, va);
   return msg_jn(szMess, sRow, fDefault);
}

BOOL fCheckFolders(VOID)
{
ULONG ulCurDrive;
ULONG ulDrives;
USHORT usIndex;
BYTE   bDrive;

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Scanning local disks \n");
   pprintf(TRUE, "=================================================\n");
   if (!bSilent && bAlwaysDefault != 2 && msg_nrm_var(szWarningScanDisks, MROW) == KEY_ESC)
      {
      pprintf(TRUE, "Test skipped !\n");
      return TRUE;
      }


   DosQCurDisk(&ulCurDrive, &ulDrives);
   for (usIndex = 0; usIndex < 26; usIndex++)
      {
      ULONG Mask = 0x0001 << usIndex;

      if (!(ulDrives & Mask))
         continue;
      bDrive = usIndex + 'A';
      if (IsDriveLocalType(bDrive))
         {
         BYTE szDrive[4];
         szDrive[0] = bDrive;
         szDrive[1] = ':';
         szDrive[2] = '\\';
         szDrive[3] = 0;
         pprintf(TRUE, "Scanning drive %s...\n", szDrive);
         CheckFoldersOnPath(szDrive);
         }
      }
   return TRUE;
}

/*********************************************************************
* 
*********************************************************************/
VOID CheckFoldersOnPath(PSZ pszPath)
{
static USHORT usLevel = 0;
PSZ pszFull          = malloc(CCHMAXPATH);
FILEFINDBUF3 * pfind = malloc(sizeof (FILEFINDBUF3));
APIRET rc;
HDIR hFind;
ULONG ulFindCount;
ULONG ulAttr;


   usLevel++;

   strcpy(pszFull, pszPath);
   if (pszFull[strlen(pszFull)-1] != '\\')
      strcat(pszFull, "\\");
   strcat(pszFull, "*.*");

   ulAttr = FILE_NORMAL|FILE_DIRECTORY|FILE_SYSTEM|FILE_HIDDEN|FILE_READONLY|MUST_HAVE_DIRECTORY;

   hFind = HDIR_CREATE;
   ulFindCount = 1;
   rc =  DosFindFirst(pszFull,
      &hFind,
      ulAttr, 
      pfind,
      sizeof (FILEFINDBUF3), &ulFindCount,
      FIL_STANDARD);
   while (!rc)
      {
      PBYTE pObjectData;
      USHORT usEASize;
      BYTE   szClassName[50] = "WPFolder";

      strcpy(pszFull, pszPath);
      if (pszFull[strlen(pszFull)-1] != '\\')
         strcat(pszFull, "\\");
      strcat(pszFull, pfind->achName);
      if (strcmp(pfind->achName, ".") && strcmp(pfind->achName,".."))
         {
         if (GetEAValue(pszFull, ".CLASSINFO", &pObjectData, &usEASize))
            {
            GetObjectClass(pObjectData+4, szClassName, sizeof szClassName);
            free(pObjectData);
            }

         if (!fCheckClass(szClassName))
            {
            pprintf(TRUE, "FOLDER %s: CLASS %s IS NOT A REGISTERED CLASS!\n",
               pszFull, szClassName);
            if (bCorrect)
               {
               if (MyMsgJN("Reset this folder to WPFolder?", MROW, FALSE))
                  {
                  SetEAValue(pszFull, ".CLASSINFO", EA_LPBINARY, "", 0);
                  bRestartNeeded = TRUE;
                  pprintf(TRUE, "FOLDER %s reset to WPFolder\n", pszFull);
                  }
               }
            }
         else if (IsStartupClass(szClassName) && !strstr(pszFull, szArchivePath))
            {
            BYTE szKeyName[10];
            PBYTE pTest;
            ULONG ulTestSize;
            HOBJECT hFolder;

            hFolder = WinQueryObject(pszFull);

            sprintf(szKeyName, "%lX", LOUSHORT(hFolder));
            pTest = GetProfileData(STARTUP, szKeyName, hIniUser, &ulTestSize);
            if (!pTest)
               {
               pprintf(TRUE, "STARTUP FOLDER %s NOT FOUND AT %s\n",
                  pszFull, STARTUP);
               if (bCorrect &&
                  MyMsgJN("Recreate entry for this startup folder in %s?", MROW, FALSE, STARTUP))
                  {
                  WriteProfileData(STARTUP, szKeyName, hIniUser,
                     "Y", 2);
                  }
               }
            else
               free(pTest);
            }
         pprintf(FALSE, "Directory %s is of class %s\n", pszFull, szClassName);
         CheckFoldersOnPath(pszFull);
         }
      ulFindCount = 1;
      rc = DosFindNext(hFind,
         pfind,
         sizeof (FILEFINDBUF3),
         &ulFindCount);
      }
   DosFindClose(hFind);
   free(pfind);
   free(pszFull);
   usLevel--;
}


/*********************************************************************
* 
*********************************************************************/
BOOL IsStartupClass(PSZ pszClass)
{
USHORT usIndex;

   for (usIndex = 0; usIndex < usStartupCount; usIndex++)
      {
      if (!stricmp(pszClass, rgStartup[usIndex]))
         return TRUE;
      }
   return FALSE;
}

/*********************************************************************
* 
*********************************************************************/
BOOL AddToStartupClasses(PSZ pszClassName, BOOL fWrite)
{
FILE * fp;
BOOL fNew = FALSE;

   if (!stricmp(pszClassName, "WPFolder") ||
       !stricmp(pszClassName, "WPDrives") ||
       !stricmp(pszClassName, "WPDesktop") ||
       !stricmp(pszClassName, "WPTemplates"))
      {
      msg_nrm("ERROR: Class %s cannot be a startup class!", pszClassName);
      return TRUE;
      }
   if (usStartupCount >= MAX_STARTUPCLASSES)
      {
      msg_nrm("ERROR: Too many startup classes!");
      return FALSE;
      }

   pprintf(TRUE, "Folders of class %s will be regarded as startup folders.\n", pszClassName);
   rgStartup[usStartupCount++]=strdup(pszClassName);
   if (fWrite)
      {
      if (access(szStartupFile, 0))
         fNew = TRUE;
      fp = fopen(szStartupFile, "a");
      if (!fp)
         {
         msg_nrm("Error: Cannot open %s", szStartupFile);
         return FALSE;
         }
      if (fNew)
         {
         fprintf(fp, "; In this file CHECKINI stores class names for startup classes\n");
         fprintf(fp, "; other then WPStartup.\n");
         fprintf(fp, "; You might need this file if you have added an add-on to the workplace\n");
         fprintf(fp, "; shell that uses startup folders of its own class.\n");
         fprintf(fp, "; Object-Desktop is an example of such a add-on.\n");
         fprintf(fp, ";\n");
         fprintf(fp, "; Please note that the class names are case-sensitive.\n");
         }
      fprintf(fp, "%s\n", pszClassName);
      fclose(fp);
      }

   return FALSE;
}

/*********************************************************************
* 
*********************************************************************/
BOOL ReadStartupClasses(VOID)
{
FILE *fp;
BYTE szLine[128];
PSZ  pBegin, pEnd;

   fp = fopen(szStartupFile, "r");
   if (!fp)
      {
      pprintf(TRUE, "%s not found!", szStartupFile);
      return FALSE;
      }
   while (fgets(szLine, sizeof szLine, fp))
      {
      pBegin = szLine;
      while (*pBegin == ' ' || *pBegin == '\t')
         pBegin++;
      if (*pBegin == ';')
         continue;
      if (*pBegin == '\n')
         continue;
      pEnd = pBegin;
      while (!isspace(*pEnd))
         pEnd++;
      *pEnd = 0;
      AddToStartupClasses(pBegin, FALSE);
      }
   fclose(fp);
   return TRUE;
}

/*********************************************************************
* 
*********************************************************************/
BOOL fRepairFolder(PSZ pszFolder, PSZ pszClass)
{
BYTE szPath[CCHMAXPATH];
PSZ  p;
HOBJECT hObjNew;
PBYTE pObjectData;
USHORT usEASize;

   p = strrchr(pszFolder, '\\');
   if (!p)
      {
      msg_nrm("ERROR: No \\ found in %s!", pszFolder);
      return FALSE;
      }
   memset(szPath, 0, sizeof szPath);
   memcpy(szPath, pszFolder, p - pszFolder);
   pprintf(TRUE, "Creating temporary object of class %s at %s\n", pszClass, szPath);

   hObjNew = WinCreateObject(pszClass,
      "@@@@@",
      ";",
      szPath,
      CO_FAILIFEXISTS);
   if (!hObjNew)
      {
      msg_nrm("ERROR: Creating a temporary object failed!");
      return FALSE;
      }
   WinSaveObject(hObjNew, FALSE);
   strcat(szPath, "\\@@@@@");

   pprintf(TRUE, "Waiting for objectdata to be saved\n");

   while (!GetEAValue(szPath, ".CLASSINFO", &pObjectData, &usEASize))
      DosSleep(10L);

   pprintf(TRUE, "Applying object data to %s\n", pszFolder);

   SetEAValue(pszFolder, ".CLASSINFO", EA_LPBINARY, pObjectData, usEASize);
   free(pObjectData);
   DosDeleteDir(szPath);
   bRestartNeeded = TRUE;
   pprintf(TRUE, "Done\n");
   return TRUE;
}

/*********************************************************************
* 
*********************************************************************/
BOOL fGetArchivePath(PSZ pszPath, USHORT usMax)
{
APIRET rc;
ULONG  ulBootDrive;
BYTE   szArchFile[]="E:\\OS2\\BOOT\\ARCHBASE.$$$";
INT    iHandle;

   memset(pszPath, 0, usMax);

   rc = DosQuerySysInfo(QSV_BOOT_DRIVE, QSV_BOOT_DRIVE,
      &ulBootDrive, sizeof ulBootDrive);
   if (rc)
      return FALSE;
   szArchFile[0] = (BYTE)(ulBootDrive + '@');

   iHandle = open(szArchFile, O_RDONLY|O_BINARY);
   if (iHandle < 0)
      return FALSE;
   lseek(iHandle, 6L, SEEK_SET);
   if (read(iHandle, pszPath, usMax - 1) != usMax - 1)
      {
      memset(pszPath, 0, usMax);
      close(iHandle);
      return FALSE;
      }
   close(iHandle);
   WinUpper(0, 0, 0, pszPath);
   return TRUE;
}

/*********************************************************************
* 
*********************************************************************/
BOOL _System GetSCenterObjects(VOID)
{
static BYTE szConfigDir[CCHMAXPATH] = "E:\\OS2\\DLL\\";
static BYTE szFile[CCHMAXPATH];
APIRET rc;
ULONG  ulBootDrive;
INT    iHandle;
INT    iTray;

   rc = DosQuerySysInfo(QSV_BOOT_DRIVE, QSV_BOOT_DRIVE,
      &ulBootDrive, sizeof ulBootDrive);
   if (rc)
      return TRUE;

   szConfigDir[0] = (BYTE)(ulBootDrive + '@');

   usSCCount = 0;
   for (iTray = 0; iTray < 16; iTray++)
      {
      sprintf(szFile, "%sDOCK%u.CFG", szConfigDir, iTray);
      iHandle = sopen(szFile, O_RDONLY|O_BINARY, SH_DENYNO);
      if (iHandle > 0)
         {
         ULONG ulLength = filelength(iHandle);
         PTRAYDEF pTray = malloc(ulLength);
         ULONG ulIcon;

         if (read(iHandle, pTray, ulLength) != ulLength)
            {
            close(iHandle);
            free(pTray);
            continue;
            }
         close(iHandle);
         pprintf(TRUE, "Found tray: %s (%lu)\n",
            pTray->szTitle,
            pTray->ulIconCount);
         if (!pTray->bInUse)
            {
            pprintf(TRUE, "Tray appears not in use, skipping..\n");
            free(pTray);
            continue;
            }
         pprintf(TRUE, "Tray contains %lu objects\n", pTray->ulIconCount);
         for (ulIcon = 0; ulIcon < pTray->ulIconCount; ulIcon++)
            {
            if (usSCCount < MAX_SC_OBJECTS)
               rgSCObjects[usSCCount++] = pTray->rgulObjects[ulIcon];
            else
               pprintf(TRUE, "ERROR: too many tray objects found!\n");
            }
         free(pTray);
         }
      }

   return TRUE;
}


/*********************************************************************
* 
*********************************************************************/
BOOL CheckHandlesIntegrity(VOID)
{
BYTE szHandles[100];
USHORT usIndex;
BOOL fSomethingDone = FALSE;
BOOL fFirstRun = TRUE;
BOOL bSaveCorrect = bCorrect;

//   bCorrect = TRUE;

   if (rgCheckDrive)
      {
      free(rgCheckDrive);
      rgCheckDrive = NULL;
      fFirstRun = FALSE;
      }

   GetActiveHandles(hIniSystem, szHandles, sizeof szHandles);

   pprintf(TRUE, "=================================================\n");
   pprintf(TRUE, " Checking integrity of %s\n", szHandles);
   pprintf(TRUE, "=================================================\n");

   if (pAllBlocks)
      {
      free(pAllBlocks);
      pAllBlocks = NULL;
      }
   if (!fReadAllBlocks(hIniSystem, szHandles, &pAllBlocks, &ulAllBlockSize))
      {
      errmsg("FATAL ERROR: %s not found in profile!\n", szHandles);
      bCorrect = bSaveCorrect;
      return FALSE;
      }

RetryCheck:
   usDriveCount = 0;
   rgCheckDrive = BuildDrivList(pAllBlocks, ulAllBlockSize, &usDriveCount);

   for (usIndex = 0; usIndex < usDriveCount; usIndex++)
      {
      BOOL bError = FALSE;
      rgCheckDrive[usIndex].usError = 0;

      /*
         Checking for dups
      */
      if (usIndex > 0 && !stricmp(rgCheckDrive[usIndex - 1].szDrivName, rgCheckDrive[usIndex].szDrivName))
         {
         bError = TRUE;
         rgCheckDrive[usIndex].usError |= ERROR_DUPLICATE_DRIV;
         fDupDrivsFound = TRUE;

         if (fFirstRun)
            pprintf(TRUE, "DRIV %s (%4u handles) => ERROR: duplicate DRIV",
               rgCheckDrive[usIndex].szDrivName,
               rgCheckDrive[usIndex].ulNodeCount);
         else
            pprintf(TRUE, "DRIV %s (%4u handles %4u inuse) => ERROR: duplicate DRIV",
               rgCheckDrive[usIndex].szDrivName,
               rgCheckDrive[usIndex].ulNodeCount,
               rgCheckDrive[usIndex].ulNodesInUse);
         }


      /*
         Checking first node
      */

      if (stricmp(rgCheckDrive[usIndex].szDrivName, rgCheckDrive[usIndex].szNodeName))
         {
         rgCheckDrive[usIndex].usError |= ERROR_INVALID_NODE;

         if (!bError)
            {
            if (fFirstRun)
               pprintf(TRUE, "DRIV %s (%4u handles)",
                  rgCheckDrive[usIndex].szDrivName,
                  rgCheckDrive[usIndex].ulNodeCount);
            else
               pprintf(TRUE, "DRIV %s (%4u handles %4u inuse)",
                  rgCheckDrive[usIndex].szDrivName,
                  rgCheckDrive[usIndex].ulNodeCount,
                  rgCheckDrive[usIndex].ulNodesInUse);
            }
         bError = TRUE;

         pprintf(TRUE, " => ERROR: Incorrect first NODE '%s'",
            rgCheckDrive[usIndex].szNodeName);
         }
      if (bError)
         pprintf(TRUE, "\n");

      if (!bError)
         {
         if (fFirstRun)
            pprintf(FALSE, "DRIV %s (%4u handles)\n",
               rgCheckDrive[usIndex].szDrivName,
               rgCheckDrive[usIndex].ulNodeCount);
         else
            pprintf(FALSE, "DRIV %s (%4u handles %4u inuse)\n",
               rgCheckDrive[usIndex].szDrivName,
               rgCheckDrive[usIndex].ulNodeCount,
               rgCheckDrive[usIndex].ulNodesInUse);
         }
      else if (!bSilent && !bCorrect)
         msg_nrm("Problem found with DRIV block for drive %s", rgCheckDrive[usIndex].szDrivName);

      }

   if (bCorrect && fFirstRun)
      {
      for (usIndex = 0; usIndex < usDriveCount; usIndex++)
         {
         if (rgCheckDrive[usIndex].usError & ERROR_INVALID_NODE)
            {
            BOOL fRetco = MyMsgJN("Fix the invalid first node for drive %s?", MROW, TRUE, rgCheckDrive[usIndex].szDrivName);
            if (fRetco)
               {
               SHORT sExtra = strlen(rgCheckDrive[usIndex].szDrivName) - strlen(rgCheckDrive[usIndex].szNodeName);
               ULONG ulNewSize = ulAllBlockSize + sExtra;
               PBYTE pNewBuf = malloc(ulNewSize);
               PDRIV pDriv;
               PNODE pNodeSrc, pNodeTar;

               /*
                  first copy all data until current DRIV
               */
               memcpy(pNewBuf, pAllBlocks, (PBYTE)rgCheckDrive[usIndex].pDriv - pAllBlocks);

               /*
                  Then copy the DRIV block itself
               */
               pDriv = (PDRIV)(pNewBuf + ((PBYTE)rgCheckDrive[usIndex].pDriv - pAllBlocks));
               memcpy(pDriv, rgCheckDrive[usIndex].pDriv,
                  sizeof (DRIV) + strlen(rgCheckDrive[usIndex].pDriv->szName));

               /*
                  The offending NODE
               */

               pNodeTar = (PNODE)((PBYTE)pDriv + sizeof (DRIV) + strlen(pDriv->szName));
               pNodeSrc = (PNODE)((PBYTE)rgCheckDrive[usIndex].pDriv + sizeof (DRIV) + strlen(pDriv->szName));
               memcpy(pNodeTar, pNodeSrc, sizeof (NODE));
               strcpy(pNodeTar->szName, pDriv->szName);
               pNodeTar->usNameSize = strlen(pNodeTar->szName);

               /*
                  The rest of the data
               */
               pNodeTar = (PNODE)((PBYTE)pNodeTar + sizeof (NODE) + strlen(pNodeTar->szName));
               pNodeSrc = (PNODE)((PBYTE)pNodeSrc + sizeof (NODE) + strlen(pNodeSrc->szName));
               memcpy(pNodeTar, pNodeSrc, ulAllBlockSize - ((PBYTE)pNodeSrc - pAllBlocks));

               pprintf(TRUE, "First node of %s corrected.\n", rgCheckDrive[usIndex].szDrivName);
               pprintf(TRUE, "");
               pprintf(TRUE, "Going to RECHECK all ..."); 

               free(pAllBlocks);
               pAllBlocks = pNewBuf;
               ulAllBlockSize = ulNewSize;
               free(rgCheckDrive);
               fSomethingDone = TRUE;
               goto RetryCheck;
               }
            }
         }
      }

   if (bCorrect && !fFirstRun)
      {
      for (usIndex = 0; usIndex < usDriveCount; usIndex++)
         {
         if (usIndex > 0 && !stricmp(rgCheckDrive[usIndex - 1].szDrivName, rgCheckDrive[usIndex].szDrivName))
            {
            BOOL fRetco = MyMsgJN("Remove the least referenced DRIV block for drive %s ?//"
                                  "CHECKINI can only determine this IF you have run ALL tests!/"
                                  "If you have skipped any then please anwer NO and rerun CHECKINI",
                                 MROW, TRUE, rgCheckDrive[usIndex].szDrivName);
            if (fRetco)
               {
               if (rgCheckDrive[usIndex - 1].fContainsDesktop)
                  rgCheckDrive[usIndex - 1].ulNodesInUse = 0x7FFFFFFF;
               if (rgCheckDrive[usIndex].fContainsDesktop)
                  rgCheckDrive[usIndex].ulNodesInUse = 0x7FFFFFFF;

               if (rgCheckDrive[usIndex - 1].ulNodesInUse < rgCheckDrive[usIndex].ulNodesInUse)
                  ulAllBlockSize = DeleteDrive(pAllBlocks, rgCheckDrive[usIndex-1].pDriv, ulAllBlockSize, TRUE);
               else
                  ulAllBlockSize = DeleteDrive(pAllBlocks, rgCheckDrive[usIndex].pDriv, ulAllBlockSize, TRUE);
               pprintf(TRUE, "Duplicate block for drive %s removed.\n", rgCheckDrive[usIndex].szDrivName);
               pprintf(TRUE, "");
               pprintf(TRUE, "Going to RECHECK all ..."); 
               free(rgCheckDrive);
               fSomethingDone = TRUE;
               goto RetryCheck;
               }
            }
         }
      }


   if (bCorrect && fSomethingDone && msg_jn(szConfirmWrite, MROW, TRUE))
      {
      bRestartNeeded = TRUE;
      fWriteAllBlocks(hIniSystem, "PM_Workplace:Handles0", pAllBlocks, ulAllBlockSize);
      fWriteAllBlocks(hIniSystem, "PM_Workplace:Handles1", pAllBlocks, ulAllBlockSize);
      free(pAllBlocks);
      pAllBlocks = NULL;
      bCorrect = bSaveCorrect;
      return FALSE;
      }

   bCorrect = bSaveCorrect;

   if (bCorrect && fSomethingDone && msg_jn("Continue with CHECKINI?", MROW, FALSE))
      return FALSE;

   return TRUE;
}

/*********************************************************************
* 
*********************************************************************/
PDRIVE BuildDrivList(PBYTE pBuffer, ULONG ulProfileSize, PUSHORT pusCount)
{
PDRIV pDriv;
PNODE pNode;
PBYTE p;
PDRIVE rgDrive = NULL;
USHORT usMaxDrive;
USHORT usDriveCount;
BOOL fFirstNode;

   usDriveCount = 0;
   usMaxDrive = 0;
   p = pBuffer + 4;

   if (memicmp(p, "DRIV", 4))
      errmsg("BuildDrivList: Incorrect block start!");


   while (p < pBuffer + ulProfileSize)
      {
      if (!memicmp(p, "DRIV", 4))
         {
         pDriv = (PDRIV)p;
         if (usDriveCount >= usMaxDrive)
            {
            usMaxDrive += 20;
            if (!rgDrive)
               rgDrive = calloc(usMaxDrive, sizeof (DRIVE));
            else
               rgDrive = realloc(rgDrive, usMaxDrive * sizeof (DRIVE));
            if (!rgDrive)
               errmsg("Not enough memory for DRIV table!");
            }
         memset(&rgDrive[usDriveCount], 0, sizeof (DRIVE));
         rgDrive[usDriveCount].pDriv = pDriv;
         strncpy(rgDrive[usDriveCount].szDrivName,
            pDriv->szName, sizeof rgDrive[usDriveCount].szDrivName);
         strncpy(rgDrive[usDriveCount].szNodeName,
            pDriv->szName, sizeof rgDrive[usDriveCount].szNodeName);
         usDriveCount++;
         p += sizeof(DRIV) + strlen(pDriv->szName);
         fFirstNode = TRUE;
         rgDrive[usDriveCount - 1].ulNodeCount=0;

         }
      else if (!memicmp(p, "NODE", 4))
         {
         pNode = (PNODE)p;
         p += sizeof (NODE) + pNode->usNameSize;
         if (fFirstNode)
            {
            strncpy(rgDrive[usDriveCount - 1].szNodeName,
               pNode->szName, sizeof rgDrive[usDriveCount].szNodeName);
            fFirstNode = FALSE;
            }
         rgDrive[usDriveCount - 1].ulNodeCount++;
         if (fHandleUsed[pNode->usID])
            rgDrive[usDriveCount - 1].ulNodesInUse++;
         if (pNode->usID == LOUSHORT(hobjDesktop))
            rgDrive[usDriveCount - 1].fContainsDesktop = TRUE;
         }
      else
         {
         errmsg("FATAL CORRUPTION DETECTED in PM_Workplace:Handles!");
         return NULL;
         }
      }

   usMaxDrive = usDriveCount;
   pprintf(FALSE, "%u DRIV blocks found.\n", usMaxDrive);

   rgDrive = realloc(rgDrive, usMaxDrive * sizeof (DRIVE));
   qsort(rgDrive, usMaxDrive, sizeof (DRIVE), CompareDrive);

   *pusCount = usMaxDrive;
   return rgDrive;
}

/*********************************************************************
* 
*********************************************************************/
INT CompareDrive(const void *p1, const void *p2)
{
INT iRetco = 
   iRetco = stricmp(((PDRIVE)p1)->szDrivName, ((PDRIVE)p2)->szDrivName);
   if (iRetco)
      return iRetco;

   return ((PDRIVE)p1)->pDriv - ((PDRIVE)p2)->pDriv;
}

BOOL CopyInis(VOID)
{
PRFPROFILE Profile;
static BYTE szOS2INI[CCHMAXPATH];
static BYTE szOS2SYSINI[CCHMAXPATH];
static BYTE szUserIni[CCHMAXPATH];
static BYTE szSystemIni[CCHMAXPATH];
APIRET rc;
FILESTATUS3 fStat;

   Profile.pszUserName = szUserIni;
   Profile.cchUserName = sizeof szUserIni;
   Profile.pszSysName = szSystemIni;
   Profile.cchSysName = sizeof szSystemIni;
   if (!PrfQueryProfile(0, &Profile))
      errmsg("Error while retrieving current profile info!\n");

   strcpy(szOS2INI, szCurDir);
   strcat(szOS2INI, "OS2SV.INI");
   chmod(szOS2INI, S_IWRITE | S_IREAD);
   unlink(szOS2INI);
   pprintf(TRUE, "Making a backup of OS2.INI to %s\n", szOS2INI);

   rc = DosCopy(szUserIni, szOS2INI, DCPY_EXISTING);
   if (rc)
      {
      pprintf(TRUE, "COPY of OS2.INI failed! SYS%4.4u\n", rc);
      return FALSE;
      }
   rc = DosQueryPathInfo(szUserIni, FIL_STANDARD, &fStat, sizeof fStat);
   if (!rc)
      {
      fStat.attrFile = FILE_NORMAL;
      DosSetPathInfo(szOS2INI, FIL_STANDARD, &fStat, sizeof fStat, 0);
      }


   strcpy(szOS2SYSINI, szCurDir);
   strcat(szOS2SYSINI, "OS2SYSSV.INI");
   chmod(szOS2SYSINI, S_IWRITE | S_IREAD);
   unlink(szOS2SYSINI);
   pprintf(TRUE, "Making a backup of OS2SYS.INI to %s\n", szOS2SYSINI); 

   rc = DosCopy(szSystemIni, szOS2SYSINI, DCPY_EXISTING);
   if (rc)
      {
      pprintf(TRUE, "COPY of OS2SYS.INI failed! SYS%4.4u\n", rc);
      return FALSE;
      }
   rc = DosQueryPathInfo(szSystemIni, FIL_STANDARD, &fStat, sizeof fStat);
   if (!rc)
      {
      fStat.attrFile = FILE_NORMAL;
      DosSetPathInfo(szOS2SYSINI, FIL_STANDARD, &fStat, sizeof fStat, 0);
      }

//   msg_nrm_var(szBackupped, MROW, szOS2INI, szOS2SYSINI);

   return TRUE;
}

BOOL RestoreInis(VOID)
{
PRFPROFILE prfCur;
PRFPROFILE prfNew;
static BYTE szOS2INI[CCHMAXPATH];
static BYTE szOS2SYSINI[CCHMAXPATH];
static BYTE szUserIni[CCHMAXPATH];
static BYTE szSystemIni[CCHMAXPATH];
APIRET rc;
FILESTATUS3 fStat;

   prfCur.pszUserName = szUserIni;
   prfCur.cchUserName = sizeof szUserIni;
   prfCur.pszSysName = szSystemIni;
   prfCur.cchSysName = sizeof szSystemIni;
   if (!PrfQueryProfile(0, &prfCur))
      errmsg("Error while retrieving current profile info!\n");

   prfNew.pszUserName = szOS2INI;
   prfNew.cchUserName = sizeof szOS2INI;
   prfNew.pszSysName  = szOS2SYSINI;
   prfNew.cchSysName  = sizeof szOS2SYSINI;

   strcpy(szOS2INI, szCurDir);
   strcat(szOS2INI, "OS2SV.INI");

   strcpy(szOS2SYSINI, szCurDir);
   strcat(szOS2SYSINI, "OS2SYSSV.INI");

#if 0
   pprintf(TRUE, "Switching to backup of INI files...\n");
   if (!PrfReset(0, &prfNew))
      {
      pprintf(TRUE, "Error while resetting profile to backup ini's\n");
      return FALSE;
      }
#endif
   pprintf(TRUE, "Restoring OS2.INI...\n");
   rc = DosQueryPathInfo(szUserIni, FIL_STANDARD, &fStat, sizeof fStat);
   if (!rc)
      {
      fStat.attrFile = FILE_NORMAL;
      DosSetPathInfo(szUserIni, FIL_STANDARD, &fStat, sizeof fStat, 0);
      }

   rc = DosCopy(szOS2INI, szUserIni, DCPY_EXISTING);
   if (rc)
      {
      pprintf(TRUE, "RESTORE of OS2.INI failed! SYS%4.4u\n", rc);
      return FALSE;
      }

   pprintf(TRUE, "Restoring OS2SYS.INI...\n"); 
   rc = DosQueryPathInfo(szSystemIni, FIL_STANDARD, &fStat, sizeof fStat);
   if (!rc)
      {
      fStat.attrFile = FILE_NORMAL;
      DosSetPathInfo(szSystemIni, FIL_STANDARD, &fStat, sizeof fStat, 0);
      }

   rc = DosCopy(szOS2SYSINI, szSystemIni, DCPY_EXISTING);
   if (rc)
      {
      pprintf(TRUE, "RESTORE of OS2SYS.INI failed! SYS%4.4u\n", rc);
      return FALSE;
      }

   pprintf(TRUE, "Switching to original INI files...\n");
   if (!PrfReset(0, &prfCur))
      {
      pprintf(TRUE, "Error while resetting profile to INI's in \\OS2\n");
      return FALSE;
      }
   return TRUE;
}



BOOL IsDrivInUse(PBYTE pStart, PBYTE pEnd)
{
PDRIV pDriv;
PNODE pNode;
PBYTE p;

   p = pStart;
   if (memicmp(p, "DRIV", 4))
      errmsg("Internal Error in IsDrivInUse!");

   pDriv = (PDRIV)p;
   p += sizeof(DRIV) + strlen(pDriv->szName);

   while (p < pEnd)
      {
      if (!memicmp(p, "DRIV", 4))
         break;
      else if (!memicmp(p, "NODE", 4))
         {
         pNode = (PNODE)p;
         if (fHandleUsed[pNode->usID])
            return TRUE;
         p += sizeof (NODE) + pNode->usNameSize;
         }
      else
         {
         errmsg("FATAL CORRUPTION DETECTED in PM_Workplace:Handles!");
         return TRUE;
         }
      }
   return FALSE;
}

BOOL ResetWPS(VOID)
{
PRFPROFILE Profile;
static BYTE  szUserIni[CCHMAXPATH];
static BYTE  szSystemIni[CCHMAXPATH];


   Profile.pszUserName = szUserIni;
   Profile.cchUserName = sizeof szUserIni;
   Profile.pszSysName = szSystemIni;
   Profile.cchSysName = sizeof szSystemIni;
   if (!PrfQueryProfile(0, &Profile))
      {
      pprintf(TRUE, "ResetWPS: PrfQueryProfile failed\n");
      return FALSE;
      }

   if (!PrfReset(0, &Profile))
      {
      pprintf(TRUE, "ResetWPS: PrfReset failed\n");
      return FALSE;
      }

   pprintf(TRUE, "The Workplace shell has been reset!\n");
   return TRUE;
}

/*********************************************************************
*  GetPM Error
*********************************************************************/
PSZ GetPMError(VOID)
{
static BYTE szMessage[1024];
PERRINFO pErrInfo;
BYTE   szHErrNo[6];
USHORT usError;

   usError = ERRORIDERROR(WinGetLastError(0));
   memset(szMessage, 0, sizeof szMessage);
   sprintf(szMessage, "Error 0x%4.4X : ", usError);

   switch (usError)
      {
      case WPERR_PROTECTED_CLASS        :
         strcat(szMessage, "Objects class is protected");
         break;
      case WPERR_NOT_WORKPLACE_CLASS    :
      case WPERR_INVALID_CLASS          :
         strcat(szMessage, "Objects class is invalid");
         break;
      case WPERR_INVALID_SUPERCLASS     :
         strcat(szMessage, "Objects superclass is invalid");
         break;
      case WPERR_INVALID_OBJECT         :
         strcat(szMessage, "The object appears to be invalid");
         break;
      case WPERR_INI_FILE_WRITE         :
         strcat(szMessage, "An error occured when writing to the ini-files");
         break;
      case WPERR_INVALID_FOLDER         :
         strcat(szMessage, "The specified folder (location) is not valid");
         break;
      case WPERR_OBJECT_NOT_FOUND       :
         strcat(szMessage, "The object was not found");
         break;
      case WPERR_ALREADY_EXISTS         :
         strcat(szMessage,
            "The workplace shell claims that the object already exist.\nUse CHECKINI to check for possible causes!");
         break;
      case WPERR_INVALID_FLAGS          :
         strcat(szMessage, "Invalid flags were specified");
         break;
      case WPERR_INVALID_OBJECTID       :
         strcat(szMessage, "The OBJECTID is invalid");
         break;
      case PMERR_INVALID_PARAMETER:
         strcat(szMessage, "A parameter is invalid (invalid class name?)");
         break;
      case PMERR_READ_ONLY_FILE:
         strcat(szMessage, "Error: file is read-only");
         break;
      default :
         memset(szMessage, 0, sizeof szMessage);
         pErrInfo = WinGetErrorInfo(0);
         if (pErrInfo)
            {
            PUSHORT pus = (PUSHORT)((PBYTE)pErrInfo + pErrInfo->offaoffszMsg);
            PSZ     psz = (PBYTE)pErrInfo + *pus;
            PSZ     p;

            strcat(szMessage, psz);
            p = psz + strlen(psz) + 1;
            strcat(szMessage, ", ");
            strcat(szMessage, p);

            itoa(ERRORIDERROR(pErrInfo->idError), szHErrNo, 16);
            strcat(szMessage, "/");
            strcat(szMessage, "Error No = ");
            strcat(szMessage, "0x");
            strcat(szMessage, szHErrNo);
            WinFreeErrorInfo(pErrInfo);
            }
         else
            sprintf(szMessage, "PM returned error %d (0x%4.4X)",
               usError, usError);
            break;
         }
   return szMessage;
}


