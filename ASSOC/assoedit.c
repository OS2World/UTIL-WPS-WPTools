#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <stdarg.h>
#include <io.h>
#define INCL_PM
#define INCL_GPI
#include <os2.h>


#include "assoedit.h"
#include "wptools.h"

#define OBJECT_FROM_PREC( prec )   ( (PVOID)( * (((PULONG)prec)-2) ) )

#define HANDLE_TEXT_SIZE 10
#define DELETED         (ULONG)0xFFFFFFFF

#define MAX_TYPE_SIZE    50
#define EXTRA_HANDLES    20
#define EXTRA_TYPES      20
#define APPS_NAME        "ASSOEDIT"
#define MAIN_WINDOW      "MAINPOS"
#define ASSOC_WINDOW     "ASSOCPOS"
#define ADDASSOC_WINDOW  "ADDASSOCPOS"
#define ADDHANDLE_WINDOW "ADDHANDLEPOS"
#define HELP_WINDOW      "HELPPOS"

typedef struct _AssocData
{
BYTE      szAssoc[MAX_TYPE_SIZE];
HOBJECT * pHandleArray;
ULONG     ulHandleCount;
ULONG     ulMaxHandleCount;
BOOL      fChanged;
BOOL      fNew;
} ASSOCDATA, * PASSOCDATA;

typedef struct _DialogData
{
PSZ        pszApp;
ULONG      ulAssocCount;
ULONG      ulMaxAssocCount;
PASSOCDATA pAssocDataArray;
PASSOCDATA pCurAssocData;
BOOL       fChanged;
BOOL       fTypes;
} DDATA,   *PDDATA;
/*************************************************************
* Global variables
*************************************************************/
static HAB hab;
static HMQ hmq;

static BOOL fHelpEnabled;
static HWND hwndHelp;
static PFNWP OldListProc;
/*************************************************************
* Function prototypes
*************************************************************/
BOOL    DebugBox(PSZ pszTitle, PSZ pszMes, BOOL bShowInfo, ...);
BOOL    CreateMainWindow(HWND hwndParent);
MRESULT EXPENTRY WindowProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
MRESULT EXPENTRY AssocDialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
MRESULT EXPENTRY MyListProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2);
BOOL    CreateAssocList     (HWND hwndOwner, PSZ pszApp);
BOOL    GetObjectTitle(HOBJECT hObject, PSZ pszTitle, USHORT usMax);
BOOL    BuildHandleList(HWND hwnd, PDDATA pData, BOOL fSelect);
BOOL    WriteChanges(PDDATA pData);
BOOL    AddType(HWND hwnd, PDDATA pData);
MRESULT EXPENTRY AddTypeProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL    AddHandle(HWND hwnd, PDDATA pData);
MRESULT EXPENTRY AddHandleProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 );
BOOL    FillObjectList(HWND hwnd);
BOOL    ProcessAddHandleInput(HWND hwnd, PDDATA pData);
HWND    InitHelpInstance(HWND hwndMain);
MRESULT DragOver(HWND hwndDlg, MPARAM mp1, MPARAM mp2);
MRESULT DragDrop(HWND hwndDlg, MPARAM mp1, MPARAM mp2);
VOID    DrawTargetEmphasis(HWND hwndList, BOOL fShow);


/*************************************************************
* The main thing
*************************************************************/
INT main(VOID)
{
QMSG  qmsg;
HWND  hwndFrame;

   hab = WinInitialize(0);
   if (!hab)
      exit(1);
   hmq = WinCreateMsgQueue(hab, 0);
   if (!hmq)
      exit(1);

   hwndFrame = CreateMainWindow(HWND_DESKTOP);
   if (hwndFrame)
      {
      hwndHelp = InitHelpInstance(hwndFrame);
      while( WinGetMsg( hab, &qmsg, 0L, 0, 0 ) )
         WinDispatchMsg( hab, &qmsg );
      WinStoreWindowPos(APPS_NAME, MAIN_WINDOW, hwndFrame);
      }

   WinDestroyHelpInstance(hwndHelp);
   WinDestroyWindow(hwndFrame);
   WinDestroyMsgQueue(hmq);
   WinTerminate(hab);
   return 0;
}


/*********************************************************************
*  Create the main window
*********************************************************************/
BOOL CreateMainWindow(HWND hwndParent)
{
ULONG ulStyle;
HWND  hwndWindow,
      hwndClient;

   if (!WinRegisterClass(hab,
      "MyClass", 
      WindowProc, 
      CS_SIZEREDRAW,                    
      sizeof (PVOID)))
      {
      DebugBox("ICONTOOL", "Fout bij WinRegisterClass !", TRUE);
      exit(1);
      }

   ulStyle = FCF_SYSMENU | FCF_TITLEBAR | FCF_MINBUTTON | FCF_ICON |
      FCF_NOBYTEALIGN | FCF_TASKLIST | FCF_MENU | FCF_BORDER;

   hwndWindow = WinCreateStdWindow(hwndParent,      
      0L,                
      &ulStyle,          
      "MyClass",    
      "Association Editor",
      0L,                
      (HMODULE)0,        
      ID_WINDOW,            
      &hwndClient);
   if (!hwndWindow)
      {
      DebugBox("ERROR", "Error in WinCreateStdWindow", TRUE);
      return 0;
      }

   if (!WinRestoreWindowPos(APPS_NAME, MAIN_WINDOW, hwndWindow))
      WinSetWindowPos(hwndWindow,
                  HWND_TOP, 100, 90, 300, 100,
                  SWP_SIZE | SWP_ACTIVATE | SWP_SHOW | SWP_MOVE);
   else
      WinSetWindowPos(hwndWindow,
                  HWND_TOP, 100, 90, 300, 100,
                  SWP_ACTIVATE | SWP_SHOW);
   return hwndWindow;

}

/*************************************************************************
* Winproc procedure for the panel
*************************************************************************/
MRESULT EXPENTRY WindowProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
MRESULT      mResult;

   switch( msg )
      {
      case WM_CREATE:
         return (MRESULT)FALSE;

      case WM_ERASEBACKGROUND:
         return (MRESULT)TRUE;

      case WM_CLOSE     :
         WinPostMsg( hwnd, WM_QUIT, 0L, 0L );  /* Cause termination        */
         return 0L;

      case WM_COMMAND:
            switch (SHORT1FROMMP(mp1))          /* test the command value from mp1 */
               {
               case ID_QUIT:
                  WinPostMsg( hwnd, WM_QUIT, 0L, 0L );  /* Cause termination        */
                  return 0L;

               case ID_HELPPRODUCTINFO :
               case ID_ABOUT :
                  if (WinDlgBox(HWND_DESKTOP,
                     hwnd,
                     WinDefDlgProc,
                     NULL,
                     ID_ABOUTDLG,
                     NULL) == DID_ERROR)
                     {
                     DebugBox("ERROR","Error on WinDlgBox", TRUE);
                     break;
                     }
                  break;
               case ID_ASSOCFILTERS:
                  CreateAssocList(hwnd, ASSOC_FILTER);
                  break;
               case ID_ASSOCTYPES:
                  CreateAssocList(hwnd, ASSOC_TYPE);
                  break;
               case ID_USINGHELP       :
                  if (fHelpEnabled)
                     WinSendMsg(hwndHelp, HM_DISPLAY_HELP, 0, 0);
                  return (MRESULT)TRUE;

               case ID_HELPINDEX       :
                  if (fHelpEnabled)
                     WinSendMsg(hwndHelp, HM_HELP_INDEX, 0, 0);
                  return (MRESULT)TRUE;

               case ID_KEYSHELP        :
                  if (fHelpEnabled)
                     WinSendMsg(hwndHelp, HM_KEYS_HELP, 0, 0);
                  return (MRESULT)TRUE;

               case ID_GENERALHELP     :
                  if (fHelpEnabled)
                     WinSendMsg(hwndHelp, HM_EXT_HELP, 0, 0);
                  return (MRESULT)TRUE;





               default :
                  break;
               }
            break;

      case HM_QUERY_KEYS_HELP:
          return ((MRESULT)4119);

      case WM_CONTROL:
         switch (SHORT2FROMMP(mp1))
            {
            default:
               break;
            }
            break;

      default:
            mResult = WinDefWindowProc( hwnd, msg, mp1, mp2 );
            return mResult;
      }
   mResult = WinDefWindowProc( hwnd, msg, mp1, mp2 );
   return mResult;
}

/*********************************************************************
*  Create the assocselectionlist window
*********************************************************************/
BOOL CreateAssocList(HWND hwndOwner, PSZ pszApp)
{
PBYTE pAssoc,
      pHandleArray,
      pHandle;
ULONG ulProfileSize;
HWND  hwndDialog;
PDDATA pData;
ULONG ulCount,
      ulResult;
PBYTE pAssocArray;


   pData = malloc(sizeof (DDATA));
   if (!pData)
      {
      DebugBox("ERROR", "Not enough memory!", FALSE);
      return FALSE;
      }
   memset(pData, 0, sizeof (DDATA));
   pData->pszApp = pszApp;

   pAssocArray = GetAllProfileNames(pszApp, HINI_USER, &ulProfileSize);
   if (!pAssocArray)
      return FALSE;

   pData->ulAssocCount=0;
   for (pAssoc = pAssocArray; *pAssoc; pAssoc += strlen(pAssoc) + 1)
      pData->ulAssocCount++;

   pData->ulMaxAssocCount = pData->ulAssocCount + EXTRA_TYPES;
   pData->pAssocDataArray = calloc(pData->ulMaxAssocCount, sizeof (ASSOCDATA));
   if (!pData->pAssocDataArray)
      {
      DebugBox("ERROR", "Not enough memory!", FALSE);
      return FALSE;
      }
   memset(pData->pAssocDataArray, 0, pData->ulMaxAssocCount * sizeof (ASSOCDATA));


   hwndDialog = WinLoadDlg(HWND_DESKTOP,
      hwndOwner,
      AssocDialogProc,
      0L,
      ID_ASSOCDLG,
      pData);
   if (!strcmp(pszApp, ASSOC_FILTER))
      WinSetWindowText(hwndDialog, "Association filters");
   else
      {
      WinSetWindowText(hwndDialog, "Association types");
      pData->fTypes = TRUE;
      }


   WinSetPointer(HWND_DESKTOP, WinQuerySysPointer(HWND_DESKTOP, SPTR_WAIT, FALSE));

   pAssoc = pAssocArray;
   ulCount = 0;
   while (*pAssoc)
      {
      ULONG ulCount2;
      SHORT sItem;

      strncpy(pData->pAssocDataArray[ulCount].szAssoc, pAssoc,
         sizeof pData->pAssocDataArray[ulCount].szAssoc);

      sItem = (SHORT)WinSendDlgItemMsg(hwndDialog, ID_ASSOCLIST,
         LM_INSERTITEM, MPFROMSHORT(LIT_SORTASCENDING),
         (MPARAM)pAssoc);

      WinSendDlgItemMsg(hwndDialog, ID_ASSOCLIST,
         LM_SETITEMHANDLE, (MPARAM)sItem, (MPARAM)&pData->pAssocDataArray[ulCount]);


      pData->pAssocDataArray[ulCount].ulHandleCount = 0;
      pHandleArray = GetProfileData(pszApp, pAssoc, HINI_USER, &ulProfileSize);
      if (pHandleArray)
         {
         pHandle = pHandleArray;
         while (pHandle < pHandleArray + ulProfileSize)
            {
            if (!atol(pHandle))
               break;
            pData->pAssocDataArray[ulCount].ulHandleCount++;
            pHandle += strlen(pHandle) + 1;
            }


         pData->pAssocDataArray[ulCount].ulMaxHandleCount =
            pData->pAssocDataArray[ulCount].ulHandleCount + EXTRA_HANDLES;

         pData->pAssocDataArray[ulCount].pHandleArray = (HOBJECT *) 
            calloc(pData->pAssocDataArray[ulCount].ulMaxHandleCount, sizeof(HOBJECT));
         memset(pData->pAssocDataArray[ulCount].pHandleArray, 0,
            pData->pAssocDataArray[ulCount].ulMaxHandleCount * sizeof(HOBJECT));

         pHandle = pHandleArray;
         for (ulCount2 = 0; ulCount2 < pData->pAssocDataArray[ulCount].ulHandleCount; ulCount2++)
            {
            pData->pAssocDataArray[ulCount].pHandleArray[ulCount2] = atol(pHandle);
            pHandle += strlen(pHandle) + 1;
            }
         free(pHandleArray);
         }
      else
         {
         pData->pAssocDataArray[ulCount].ulHandleCount = EXTRA_HANDLES;
         pData->pAssocDataArray[ulCount].pHandleArray = (HOBJECT *) 
            calloc(pData->pAssocDataArray[ulCount].ulMaxHandleCount, sizeof(HOBJECT));
         memset(pData->pAssocDataArray[ulCount].pHandleArray, 0,
            pData->pAssocDataArray[ulCount].ulMaxHandleCount * sizeof(HOBJECT));
         }

      ulCount++;
      pAssoc += strlen(pAssoc) + 1;
      }
   free(pAssocArray);

   /*
      Select the first item
   */
   WinSendDlgItemMsg(hwndDialog, ID_ASSOCLIST,
      LM_SELECTITEM, MPFROMSHORT(0), MPFROMSHORT(TRUE));

   WinRestoreWindowPos(APPS_NAME, ASSOC_WINDOW, hwndDialog);
   WinSetWindowPos(hwndDialog, HWND_TOP, 0, 0, 0, 0, SWP_ACTIVATE | SWP_SHOW);
   WinSetPointer(HWND_DESKTOP, WinQuerySysPointer(HWND_DESKTOP, SPTR_ARROW, FALSE));


   ulResult = WinProcessDlg(hwndDialog);
   if (ulResult == DID_OK)
      WriteChanges(pData);

   for (ulCount= 0; ulCount < pData->ulAssocCount; ulCount++)
      {
      if (pData->pAssocDataArray[ulCount].pHandleArray)
         free(pData->pAssocDataArray[ulCount].pHandleArray);
      }
   free(pData->pAssocDataArray);
   free(pData);

   WinStoreWindowPos(APPS_NAME, ASSOC_WINDOW, hwndDialog);
   WinDestroyWindow(hwndDialog);
   return TRUE;
}

/*****************************************************************
* Write changes
****************************************************************/
BOOL WriteChanges(PDDATA pData)
{
ULONG ulIndex;
PBYTE pOutput;

   for (ulIndex = 0 ; ulIndex < pData->ulAssocCount; ulIndex++)
      {
      PASSOCDATA pAssocData = &pData->pAssocDataArray[ulIndex];
      if (pAssocData->fChanged )
         {
         ULONG ulIndex1;
         PBYTE p;

         if (pAssocData->ulHandleCount != DELETED)
            {
            pOutput = calloc(pAssocData->ulHandleCount + 1, HANDLE_TEXT_SIZE);
            if (!pOutput)
               {
               DebugBox("ERROR","Not enough memory!", FALSE);
               return FALSE;
               }
            memset(pOutput, 0, (pAssocData->ulHandleCount + 1) * HANDLE_TEXT_SIZE);
            for (p = pOutput, ulIndex1 = 0;
               ulIndex1 < pAssocData->ulHandleCount; ulIndex1++)
               {
               sprintf(p, "%ld", pAssocData->pHandleArray[ulIndex1]);
               p += strlen(p) + 1;
               }
            if (!pAssocData->ulHandleCount)
               *p++ = 0;

            if (!PrfWriteProfileData(HINI_USER,
               pData->pszApp,
               pAssocData->szAssoc,
               pOutput,
               (ULONG)(p - pOutput)))
               {
               DebugBox("ERROR", "Error on PrfWriteProfileData", TRUE);
               free(pOutput);
               return FALSE;
               }
            free(pOutput);
            }
         else if (!pAssocData->fNew)
            {
            if (!PrfWriteProfileData(HINI_USER,
               pData->pszApp,
               pAssocData->szAssoc,
               NULL, 
               0))
               {
               DebugBox("ERROR", "Error on PrfWriteProfileData (0)", TRUE);
               free(pOutput);
               return FALSE;
               }
            }
         }
      }
      return TRUE;
}
/*************************************************************************
* Winproc procedure for the process dialog
*************************************************************************/
MRESULT EXPENTRY AssocDialogProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
MRESULT mResult;
PDDATA  pData;
ULONG   ulReply;
BYTE    szMessage[100];
SHORT   sSelect;
HOBJECT hObjectSave;

   pData = (PDDATA)WinQueryWindowPtr(hwnd, 0);    // retrieve instance data pointer


   switch( msg )
      {
      case WM_INITDLG :
         pData = (PDDATA)mp2;
         WinSetWindowPtr(hwnd, 0, (PVOID)pData);
         OldListProc =
            WinSubclassWindow(WinWindowFromID(hwnd, ID_ASSOC), MyListProc);
         break;

      case WM_CLOSE   :
         WinPostMsg(hwnd, WM_COMMAND, MPFROMSHORT(DID_CANCEL), 0);
         return (MRESULT)TRUE;

      case WM_COMMAND :
         switch(SHORT1FROMMP(mp1))
            {
            case DID_OK :
               if (pData->fChanged)
                  WinDismissDlg(hwnd, DID_OK);
               else
                  WinDismissDlg(hwnd, DID_CANCEL);
               return (MRESULT)FALSE;

            case DID_CANCEL :
               if (pData->fChanged)
                  {
                  ulReply = WinMessageBox(HWND_DESKTOP, hwnd,
                     "Data has changed, Save changes ?","",
                     0, MB_ICONQUESTION|MB_YESNOCANCEL|MB_DEFBUTTON3);
                  if (ulReply == MBID_CANCEL)
                     return (MRESULT)FALSE;

                  if (ulReply == MBID_NO)
                     WinDismissDlg(hwnd, DID_CANCEL);
                  else
                     WinDismissDlg(hwnd, DID_OK);
                  }
               else
                  WinDismissDlg(hwnd, DID_CANCEL);
               return (MRESULT)FALSE;

            case ID_DELASSOC :

               if (pData->fTypes)
                  sprintf(szMessage, "Delete type %s?", pData->pCurAssocData->szAssoc);
               else
                  sprintf(szMessage, "Delete filter %s?", pData->pCurAssocData->szAssoc);
               ulReply = WinMessageBox(HWND_DESKTOP, hwnd,
                  szMessage,"",
                  0, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2);
               if (ulReply == MBID_NO)
                  return (MRESULT)FALSE;

               sSelect = (SHORT)WinSendDlgItemMsg(hwnd, ID_ASSOCLIST,
                     LM_QUERYSELECTION, (MPARAM)LIT_FIRST, 0L);
               WinSendDlgItemMsg(hwnd, ID_ASSOCLIST, LM_DELETEITEM,
                  MPFROMSHORT(sSelect), 0);

               pData->pCurAssocData->ulHandleCount = DELETED;
               pData->pCurAssocData->fChanged = TRUE;
               pData->fChanged = TRUE;

               BuildHandleList(hwnd, pData, FALSE);
               return (MRESULT)TRUE;

            case ID_ADDASSOC:
               AddType(hwnd, pData);
               return (MRESULT)TRUE;

            case ID_DELHANDLE:
               sSelect = (SHORT)WinSendDlgItemMsg(hwnd, ID_ASSOC,
                     LM_QUERYSELECTION, (MPARAM)LIT_FIRST, 0L);
               if (sSelect == LIT_NONE)
                  return (MRESULT)FALSE;

               strcpy(szMessage, "Delete association with ");
               WinSendDlgItemMsg(hwnd, ID_ASSOC, LM_QUERYITEMTEXT,
                  MPFROM2SHORT(sSelect, 50),
                  (MPARAM)(szMessage + strlen(szMessage)));
               strcat(szMessage, "?");
               ulReply = WinMessageBox(HWND_DESKTOP, hwnd,
                  szMessage,"",
                  0, MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2);
               if (ulReply == MBID_NO)
                  return (MRESULT)FALSE;

               memmove(&pData->pCurAssocData->pHandleArray[sSelect],
                  &pData->pCurAssocData->pHandleArray[sSelect + 1],
                  (pData->pCurAssocData->ulHandleCount - sSelect - 1) * sizeof (HOBJECT));
               pData->pCurAssocData->ulHandleCount--;

               WinSendDlgItemMsg(hwnd, ID_ASSOC, LM_DELETEITEM,
                  MPFROMSHORT(sSelect), 0);
               WinEnableWindow(WinWindowFromID(hwnd, ID_DELHANDLE), FALSE);
               WinEnableWindow(WinWindowFromID(hwnd, ID_DEFAULT)  , FALSE);

               pData->pCurAssocData->fChanged = TRUE;
               pData->fChanged = TRUE;
               return (MRESULT)TRUE;

            case ID_ADDHANDLE:
               if (AddHandle(hwnd, pData))
                  BuildHandleList(hwnd, pData, TRUE);
               return (MRESULT)TRUE;

            case ID_DEFAULT  :
               /*
                  Make the selected program the default
               */
               sSelect = (SHORT)WinSendDlgItemMsg(hwnd, ID_ASSOC,
                     LM_QUERYSELECTION, (MPARAM)LIT_FIRST, 0L);
               if (sSelect == LIT_NONE || !sSelect)
                  return (MRESULT)FALSE;

               hObjectSave = pData->pCurAssocData->pHandleArray[0];
               pData->pCurAssocData->pHandleArray[0] = pData->pCurAssocData->pHandleArray[sSelect];
               pData->pCurAssocData->pHandleArray[sSelect] = hObjectSave;

               BuildHandleList(hwnd, pData, FALSE);
               pData->pCurAssocData->fChanged = TRUE;
               pData->fChanged = TRUE;
               return (MRESULT)TRUE;
            }
         break;
      case WM_CONTROL:
         if (SHORT1FROMMP(mp1) == ID_ASSOCLIST)
            {
            switch(SHORT2FROMMP(mp1))
               {
               case LN_ENTER :
               case LN_SELECT:
                  BuildHandleList(hwnd, pData, FALSE);
                  break;
               }
            }
         if (SHORT1FROMMP(mp1) == ID_ASSOC)
            {
            switch(SHORT2FROMMP(mp1))
               {
               case LN_ENTER :
               case LN_SELECT:
                  WinEnableWindow(WinWindowFromID(hwnd, ID_DELHANDLE), TRUE);
                  sSelect = (SHORT)WinSendDlgItemMsg(hwnd, ID_ASSOC,
                        LM_QUERYSELECTION, (MPARAM)LIT_FIRST, 0L);
                  if (sSelect > 0)
                     WinEnableWindow(WinWindowFromID(hwnd, ID_DEFAULT), TRUE);
                  else
                     WinEnableWindow(WinWindowFromID(hwnd, ID_DEFAULT), FALSE);
                  break;
               }
            }


         break;

      default:
         break;

      }
   mResult = WinDefDlgProc( hwnd, msg, mp1, mp2 );
   return mResult;
}

/*********************************************************************
*  My Listbox proc, a specially for the DRAG/DROP messages
*********************************************************************/
MRESULT MyListProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   switch (msg)
      {
      case DM_DRAGOVER:
         DrawTargetEmphasis(hwnd, TRUE);
         return DragOver(hwnd, mp1, mp2);
      case DM_DROP:
         DrawTargetEmphasis(hwnd, FALSE);
         return DragDrop(hwnd, mp1, mp2);
      case DM_DRAGLEAVE:
         DrawTargetEmphasis(hwnd, FALSE);
         break;
      }
   return (*OldListProc)(hwnd, msg, mp1, mp2);
}

/*********************************************************************
*  Drag over
*********************************************************************/
MRESULT DragOver(HWND hwndList, MPARAM mp1, MPARAM mp2)
{
PDRAGINFO pDragInfo   = (PDRAGINFO)mp1;
PDRAGITEM pDragItem;
ULONG     ulItemCount = 0;
BOOL      fDrop;
static BYTE  szContainerName[256];
static BYTE  szObjectTitle[256];


   if (!DrgAccessDraginfo(pDragInfo))
      return MPFROM2SHORT(DOR_NEVERDROP, 0);

   fDrop = FALSE;

   ulItemCount = DrgQueryDragitemCount(pDragInfo);
   if (ulItemCount == 1)
      {
      pDragItem = DrgQueryDragitemPtr(pDragInfo, 0);

      if (DrgVerifyRMF(pDragItem, "DRM_OS2FILE", "DRF_TEXT"))
         {
         PBYTE pszExt;
         DrgQueryStrName(pDragItem->hstrSourceName, sizeof szObjectTitle, szObjectTitle);
         strupr(szObjectTitle);
         pszExt = strrchr(szObjectTitle, '.');
         if (!pszExt)
            fDrop = FALSE;
         else
            {
            if (!stricmp(pszExt, ".EXE") ||
                !stricmp(pszExt, ".COM") ||
                !stricmp(pszExt, ".BAT") ||
                !stricmp(pszExt, ".CMD"))
               fDrop = TRUE;
            }
         }
      else if (DrgVerifyRMF(pDragItem, "DRM_OBJECT",  "DRF_OBJECT"))
         {
         HOBJECT hFolder;

         DrgQueryStrName(pDragItem->hstrContainerName, sizeof szContainerName, szContainerName);
         DrgQueryStrName(pDragItem->hstrSourceName, sizeof szObjectTitle, szObjectTitle);

         szContainerName[strlen(szContainerName) - 1] = 0;
         hFolder = WinQueryObject(szContainerName);
         if (hFolder)
            {
            PULONG pulObjects;
            ULONG  ulProfileSize;
            BYTE   szObjID[15];
            sprintf(szObjID, "%X", LOUSHORT(hFolder));

            pulObjects = (PULONG)
               GetProfileData(FOLDERCONTENT, szObjID, HINI_USERPROFILE, &ulProfileSize);
            if (pulObjects)
               {
               ULONG ulIndex;
               ulProfileSize /= sizeof (ULONG);
               for (ulIndex = 0; ulIndex < ulProfileSize; ulIndex++)
                  {
                  static BYTE szObjectName[256];
                  HOBJECT hObject = MakeAbstractHandle(pulObjects[ulIndex]);
                  if (GetObjectTitle(hObject,
                     szObjectName, sizeof szObjectName) &&
                     !strcmp(szObjectName, szObjectTitle))
                     {
                     ULONG ulDataSize;
                     PBYTE pObjectData = GetClassInfo(HINI_USERPROFILE, hObject, &ulDataSize);
                     if (pObjectData)
                        {
                        if (!stricmp(pObjectData + 4, "WPProgram"))
                           {
                           fDrop = TRUE;
                           break;
                           }
                        free(pObjectData);
                        }
                     }
                  }
               free(pulObjects);
               }
            }
         }
      }

   DrgFreeDraginfo(pDragInfo);
   if (!fDrop)
      return MPFROM2SHORT(DOR_NEVERDROP, 0);

   return(MPFROM2SHORT(DOR_DROP, DO_MOVE));

}

/*********************************************************************
*  Show Target Emphasis or not
*********************************************************************/
VOID DrawTargetEmphasis(HWND hwndList, BOOL fShow)
{
HWND hwndDlg = WinQueryWindow(hwndList, QW_PARENT);
SWP  swp;
HPS  hps;
POINTL ptl;
BOOL fNoDrgPs = FALSE;

   WinQueryWindowPos(hwndList, &swp);

   hps = DrgGetPS(hwndDlg);
   if (!hps)
      {
      fNoDrgPs = TRUE;
      hps = WinGetPS(hwndDlg);
      }
   ptl.x = swp.x - 2;
   ptl.y = swp.y - 2;
   GpiSetCurrentPosition(hps, &ptl);
   ptl.x = swp.x + swp.cx + 1;
   ptl.y = swp.y + swp.cy + 1;
   GpiSetLineType(hps, LINETYPE_DEFAULT);
   if (fShow)
      GpiSetColor(hps, CLR_DARKGRAY);
   else
      GpiSetColor(hps, CLR_PALEGRAY);
   GpiBox(hps, DRO_OUTLINE, &ptl, 0, 0);
   if (fNoDrgPs)
      WinReleasePS(hps);
   else
      DrgReleasePS(hps);
}

/*********************************************************************
*  Drag over
*********************************************************************/
MRESULT DragDrop(HWND hwndList, MPARAM mp1, MPARAM mp2)
{
PDRAGINFO pDragInfo   = (PDRAGINFO)mp1;
PDRAGITEM pDragItem;
ULONG     ulItemCount = 0;
BOOL      fDrop;
PDDATA    pData;
PASSOCDATA pAssocData;
HOBJECT   hNewObject = NULL;   
static    BYTE szObjectName[256];
static    BYTE szObjectTitle[256];
static    BYTE szContainerName[256];

   pData = (PDDATA)WinQueryWindowPtr(WinQueryWindow(hwndList, QW_PARENT), 0);
   pAssocData = pData->pCurAssocData;
   if (pAssocData->ulHandleCount >= pAssocData->ulMaxHandleCount)
      {
      DebugBox("Warning","Maximum number of added associations for this session has been reached, save and try again!", FALSE);
      return (MRESULT)FALSE;
      }

   if (!DrgAccessDraginfo(pDragInfo))
      return MPFROM2SHORT(DOR_NEVERDROP, 0);

   fDrop = FALSE;

   ulItemCount = DrgQueryDragitemCount(pDragInfo);
   if (ulItemCount == 1)
      {
      pDragItem = DrgQueryDragitemPtr(pDragInfo, 0);

      if (DrgVerifyRMF(pDragItem, "DRM_OS2FILE", "DRF_TEXT"))
         {
         DrgQueryStrName(pDragItem->hstrContainerName, sizeof szContainerName, szContainerName);
         if (szContainerName[strlen(szContainerName) - 1] != '\\')
            strcat(szContainerName, "\\");
         DrgQueryStrName(pDragItem->hstrSourceName, sizeof szObjectTitle, szObjectTitle);
         strcat(szContainerName, szObjectTitle);
         hNewObject = WinQueryObject(szContainerName);
         strcpy(szObjectTitle, szContainerName);
         fDrop = TRUE;
         }
      else if (DrgVerifyRMF(pDragItem, "DRM_OBJECT",  "DRF_OBJECT"))
         {
         HOBJECT hFolder;

         DrgQueryStrName(pDragItem->hstrContainerName, sizeof szContainerName, szContainerName);
         DrgQueryStrName(pDragItem->hstrSourceName, sizeof szObjectTitle, szObjectTitle);

         szContainerName[strlen(szContainerName) - 1] = 0;
         hFolder = WinQueryObject(szContainerName);
         if (hFolder)
            {
            PULONG pulObjects;
            ULONG  ulProfileSize;
            BYTE   szObjID[15];
            sprintf(szObjID, "%X", LOUSHORT(hFolder));

            pulObjects = (PULONG)
               GetProfileData(FOLDERCONTENT, szObjID, HINI_USERPROFILE, &ulProfileSize);
            if (pulObjects)
               {
               ULONG ulIndex;
               ulProfileSize /= sizeof (ULONG);
               for (ulIndex = 0; ulIndex < ulProfileSize; ulIndex++)
                  {
                  HOBJECT hObject = MakeAbstractHandle(pulObjects[ulIndex]);
                  if (GetObjectTitle(hObject, szObjectName, sizeof szObjectName) &&
                     !strcmp(szObjectName, szObjectTitle))
                     {
                     ULONG ulDataSize;
                     PBYTE pObjectData = GetClassInfo(HINI_USERPROFILE, hObject, &ulDataSize);
                     if (pObjectData)
                        {
                        if (!stricmp(pObjectData + 4, "WPProgram"))
                           {
                           hNewObject = hObject;
                           fDrop = TRUE;
                           break;
                           }
                        free(pObjectData);
                        }
                     }
                  }
               free(pulObjects);
               }
            }
         }
      }

   if (hNewObject)
      {
      ULONG ulIndex;
      for (ulIndex = 0; ulIndex < pAssocData->ulHandleCount; ulIndex++)
         {
         if (pAssocData->pHandleArray[ulIndex] == hNewObject)
            {
            DebugBox("Warning","Association already present!", FALSE);
            break;
            }
         }
      if (ulIndex == pAssocData->ulHandleCount)
         {
         pAssocData->pHandleArray[pAssocData->ulHandleCount++] = hNewObject;
         pAssocData->fChanged = TRUE;
         pData->fChanged = TRUE;
         (*OldListProc)(hwndList, LM_INSERTITEM,
            MPFROMSHORT(LIT_END), (MPARAM)szObjectTitle);
         fDrop = TRUE;
         }
      }

//   if (!fDrop)
      DrgSendTransferMsg(pDragItem->hwndItem, DM_ENDCONVERSATION,
         (MPARAM)pDragItem->ulItemID, (MPARAM)DMFL_TARGETFAIL);
//   else
//      DrgSendTransferMsg(pDragItem->hwndItem, DM_ENDCONVERSATION,
//         (MPARAM)pDragItem->ulItemID, (MPARAM)DMFL_TARGETSUCCESSFUL);

   DrgFreeDraginfo(pDragInfo);
   return (MRESULT)FALSE;
}


/*********************************************************************
*  Build the handlelist
*********************************************************************/
BOOL BuildHandleList(HWND hwnd, PDDATA pData, BOOL fSelect)
{
SHORT sSelect;
HWND  hwndList = WinWindowFromID(hwnd, ID_ASSOC);
BYTE  szObjectName[200];
ULONG ulIndex;
PASSOCDATA pAssocData;

   WinEnableWindow(WinWindowFromID(hwnd, ID_DELHANDLE), FALSE);
   WinEnableWindow(WinWindowFromID(hwnd, ID_DEFAULT),   FALSE);

   WinEnableWindowUpdate(hwndList, FALSE);
   WinSendMsg(hwndList, LM_DELETEALL, 0, 0);

   sSelect = (SHORT)WinSendDlgItemMsg(hwnd, ID_ASSOCLIST,
         LM_QUERYSELECTION, (MPARAM)LIT_FIRST, 0L);

   if (sSelect == LIT_NONE)
      {
      WinEnableWindow(WinWindowFromID(hwnd, ID_ADDHANDLE), FALSE);
      WinEnableWindow(WinWindowFromID(hwnd, ID_DELASSOC), FALSE);
      WinEnableWindowUpdate(hwndList, TRUE);
      return TRUE;
      }

   WinEnableWindow(WinWindowFromID(hwnd, ID_ADDHANDLE), TRUE);
   WinEnableWindow(WinWindowFromID(hwnd, ID_DELASSOC), TRUE);

   pAssocData = (PASSOCDATA)WinSendDlgItemMsg(hwnd, ID_ASSOCLIST,
      LM_QUERYITEMHANDLE, (MPARAM)sSelect, 0L);
   pData->pCurAssocData = pAssocData;

   if (!pAssocData->ulHandleCount || pAssocData->ulHandleCount == DELETED)
      {
      WinEnableWindowUpdate(hwndList, TRUE);
      return TRUE;
      }

   /*
      Force that PM_Workplace:Handles is reread !
   */
   ResetBlockBuffer();

   for (ulIndex = 0; ulIndex < pAssocData->ulHandleCount; ulIndex++)
      {
      PBYTE p;
      if (!GetObjectTitle(pAssocData->pHandleArray[ulIndex],
         szObjectName, sizeof szObjectName))
         {
         sprintf(szObjectName, "Unknown object %lX (see help)",
            pAssocData->pHandleArray[ulIndex]);
         }
      else
         {
         while ((p = strchr(szObjectName, '\n')) != NULL)
            *p = ' ';
         }
      WinSendMsg(hwndList, LM_INSERTITEM,
         MPFROMSHORT(LIT_END), (MPARAM)szObjectName);
      }
   if (fSelect)
      {
      WinSendMsg(hwndList, LM_SELECTITEM,
         (MPARAM)(ulIndex - 1), (MPARAM)TRUE);
      }

   WinEnableWindowUpdate(hwndList, TRUE);
   return TRUE;
}

/*********************************************************************
*  Add type
*********************************************************************/
BOOL AddType(HWND hwnd, PDDATA pData)
{
HWND hwndDialog;

   if (pData->ulAssocCount >= pData->ulMaxAssocCount)
      {
      DebugBox("Message","Maximum number of added %s for this session has been reached, save and try again!", FALSE,
         (pData->fTypes ? "types" : "filters"));
      return FALSE;
      }

   hwndDialog = WinLoadDlg(HWND_DESKTOP,
      hwnd,
      AddTypeProc,
      0L,
      ID_ADDASSOCDLG,
      pData);

   WinRestoreWindowPos(APPS_NAME, ADDASSOC_WINDOW, hwndDialog);
   WinSetWindowPos(hwndDialog, HWND_TOP, 0, 0, 0, 0, SWP_ACTIVATE | SWP_SHOW);


   if (!pData->fTypes)
      WinSetWindowText(hwndDialog, "Add new filter");
   else
      WinSetWindowText(hwndDialog, "Add new type");
   
   WinSendDlgItemMsg(hwndDialog, ID_NEWTYPE, EM_SETTEXTLIMIT,
      MRFROMSHORT((USHORT)(MAX_TYPE_SIZE - 1)), 0L);

   if (WinProcessDlg(hwndDialog) == DID_OK)
      {
      SHORT sNew = (SHORT)WinSendDlgItemMsg(hwnd, ID_ASSOCLIST,
         LM_INSERTITEM, MPFROMSHORT(LIT_SORTASCENDING),
         (MPARAM)pData->pAssocDataArray[pData->ulAssocCount - 1].szAssoc);
      WinSendDlgItemMsg(hwnd, ID_ASSOCLIST,
         LM_SETITEMHANDLE, (MPARAM)sNew,
         (MPARAM)&pData->pAssocDataArray[pData->ulAssocCount - 1]);
      WinSendDlgItemMsg(hwnd, ID_ASSOCLIST,
         LM_SELECTITEM, (MPARAM)sNew, (MPARAM)TRUE);
      }

   WinStoreWindowPos(APPS_NAME, ADDASSOC_WINDOW, hwndDialog);
   WinDestroyWindow(hwndDialog);
   return TRUE;

}

/*************************************************************************
* Winproc procedure for the process dialog
*************************************************************************/
MRESULT EXPENTRY AddTypeProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
MRESULT mResult;
PDDATA  pData;
BYTE    ulIndex;
BYTE    szNewType[MAX_TYPE_SIZE];

   pData = (PDDATA)WinQueryWindowPtr(hwnd, 0);    // retrieve instance data pointer

   switch( msg )
      {
      case WM_INITDLG :
         pData = (PDDATA)mp2;
         WinSetWindowPtr(hwnd, 0, (PVOID)pData);
         break;

      case WM_CLOSE   :
         WinPostMsg(hwnd, WM_COMMAND, MPFROMSHORT(DID_CANCEL), 0);
         return (MRESULT)TRUE;

      case WM_COMMAND :
         switch(SHORT1FROMMP(mp1))
            {
            case DID_OK :
               WinQueryDlgItemText(hwnd,
                  ID_NEWTYPE, sizeof szNewType - 1, szNewType);
               if (!strlen(szNewType))
                  {
                  DebugBox("ERROR","A %s must be specified!", FALSE,
                     (pData->fTypes ? "type" : "filter"));
                  return (MRESULT)FALSE;
                  }

               if (!pData->fTypes)
                  strupr(szNewType);

               for (ulIndex = 0; ulIndex < pData->ulAssocCount; ulIndex++)
                  {
                  if (pData->pAssocDataArray[ulIndex].ulHandleCount == DELETED)
                     continue;
                  if (!stricmp(pData->pAssocDataArray[ulIndex].szAssoc, szNewType))
                     {
                     DebugBox("ERROR","%s %s already exists!", FALSE,
                        (pData->fTypes ? "Type" : "Filter"), szNewType);
                     return (MRESULT)FALSE;
                     }
                  }
               strcpy(pData->pAssocDataArray[ulIndex].szAssoc, szNewType);
               pData->pAssocDataArray[ulIndex].ulHandleCount   = 0;
               pData->pAssocDataArray[ulIndex].ulMaxHandleCount = EXTRA_HANDLES;

               pData->pAssocDataArray[ulIndex].pHandleArray = (HOBJECT *) 
                  calloc(pData->pAssocDataArray[ulIndex].ulMaxHandleCount, sizeof(HOBJECT));
               memset(pData->pAssocDataArray[ulIndex].pHandleArray, 0,
                  pData->pAssocDataArray[ulIndex].ulMaxHandleCount * sizeof(HOBJECT));
               pData->fChanged = TRUE;
               pData->pAssocDataArray[ulIndex].fNew     = TRUE;
               pData->pAssocDataArray[ulIndex].fChanged = TRUE;

               pData->ulAssocCount++;

               WinDismissDlg(hwnd, DID_OK);
               return (MRESULT)FALSE;

            case DID_CANCEL :
               WinDismissDlg(hwnd, DID_CANCEL);
               return (MRESULT)FALSE;

            }
         break;
      default:
         break;

      }
   mResult = WinDefDlgProc( hwnd, msg, mp1, mp2 );
   return mResult;
}


/*********************************************************************
*  Add type
*********************************************************************/
BOOL AddHandle(HWND hwnd, PDDATA pData)
{
HWND hwndDialog;
ULONG ulRetco;
BYTE  szWindowTitle[100];

   if (pData->pCurAssocData->ulHandleCount >= pData->pCurAssocData->ulMaxHandleCount)
      {
      DebugBox("Message","Maximum number of added associations for this session has been reached, save and try again!", FALSE);
      return FALSE;
      }

   hwndDialog = WinLoadDlg(HWND_DESKTOP,
      hwnd,
      AddHandleProc,
      0L,
      ID_ADDHANDLEDLG,
      pData);

   sprintf(szWindowTitle, "Add association for %s", pData->pCurAssocData->szAssoc);
   WinSetWindowText(hwndDialog, szWindowTitle);

   WinSendDlgItemMsg(hwndDialog, ID_NEWPROG, EM_SETTEXTLIMIT,
      MRFROMSHORT((USHORT)255), 0L);

   WinSetPointer(HWND_DESKTOP, WinQuerySysPointer(HWND_DESKTOP, SPTR_WAIT, FALSE));

   FillObjectList(hwndDialog);

   WinRestoreWindowPos(APPS_NAME, ADDHANDLE_WINDOW, hwndDialog);
   WinSetWindowPos(hwndDialog, HWND_TOP, 0, 0, 0, 0, SWP_ACTIVATE | SWP_SHOW);

   WinSetPointer(HWND_DESKTOP, WinQuerySysPointer(HWND_DESKTOP, SPTR_ARROW, FALSE));
   ulRetco = WinProcessDlg(hwndDialog);

   WinStoreWindowPos(APPS_NAME, ADDHANDLE_WINDOW, hwndDialog);
   WinDestroyWindow(hwndDialog);
   if (ulRetco == DID_OK)
      return TRUE;
   else
      return FALSE;
}

/*************************************************************
* Fill the objectlist
*************************************************************/
BOOL FillObjectList(HWND hwnd)
{
PBYTE pBuffer;
ULONG ulProfileSize;
PBYTE pObject;


   WinEnableWindowUpdate(hwnd, FALSE);

   pBuffer = GetAllProfileNames(OBJECTS, HINI_USER, &ulProfileSize);
   if (pBuffer)
      {
      pObject = pBuffer;
      while (*pObject)
         {
         PBYTE pObjectData = GetProfileData(OBJECTS, pObject, HINI_USER, &ulProfileSize);
         if (pObjectData)
            {
            PBYTE pszClass = pObjectData + 4;

            if (!strcmp(pszClass, "WPProgram"))
               {
               BYTE    szTitle[256];
               PBYTE   pStop;
               HOBJECT hObject = MakeAbstractHandle(strtol(pObject, &pStop, 16));

               if (GetGenObjectValue(pObjectData, "WPAbstract", WPABSTRACT_TITLE, szTitle, sizeof szTitle))
                  {
                  SHORT sPosition;
                  PBYTE p;
                  while ((p = strchr(szTitle, '\n')) != NULL)
                     *p = ' ';
                     
                  sPosition = (SHORT)WinSendDlgItemMsg(hwnd, ID_OBJECTLIST,
                     LM_INSERTITEM, MPFROMSHORT(LIT_SORTASCENDING),
                     (MPARAM)szTitle);

                  WinSendDlgItemMsg(hwnd, ID_OBJECTLIST,
                     LM_SETITEMHANDLE, (MPARAM)sPosition, (MPARAM)hObject);
                  }
               }
            free(pObjectData);
            }
         pObject += strlen(pObject) + 1;
         }

      free(pBuffer);
      }

   WinEnableWindowUpdate(hwnd, TRUE);

   return TRUE;
}


/*************************************************************************
* Winproc procedure for the process dialog
*************************************************************************/
MRESULT EXPENTRY AddHandleProc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{
MRESULT mResult;
PDDATA  pData;

   pData = (PDDATA)WinQueryWindowPtr(hwnd, 0);    // retrieve instance data pointer

   switch( msg )
      {
      case WM_INITDLG :
         pData = (PDDATA)mp2;
         WinSetWindowPtr(hwnd, 0, (PVOID)pData);
         break;

      case WM_CLOSE   :
         WinPostMsg(hwnd, WM_COMMAND, MPFROMSHORT(DID_CANCEL), 0);
         return (MRESULT)TRUE;

      case WM_COMMAND :
         switch(SHORT1FROMMP(mp1))
            {
            case DID_OK :
               if (ProcessAddHandleInput(hwnd, pData))
                  WinDismissDlg(hwnd, DID_OK);
               return (MRESULT)FALSE;

            case DID_CANCEL :
               WinDismissDlg(hwnd, DID_CANCEL);
               return (MRESULT)FALSE;

            }
         break;
      case WM_CONTROL:
         switch(SHORT2FROMMP(mp1)) /* notification code */
            {
            case BN_CLICKED :
               if (SHORT1FROMMP(mp1) == ID_TYPEPROG) /* source of message */
                  {
                  WinEnableWindow(WinWindowFromID(hwnd, ID_OBJECTLIST), FALSE);
                  WinEnableWindow(WinWindowFromID(hwnd, ID_TEXTOBJ),    FALSE);

                  WinEnableWindow(WinWindowFromID(hwnd, ID_NEWPROG),    TRUE);
                  WinEnableWindow(WinWindowFromID(hwnd, ID_TEXTPROG),   TRUE);
                  }
               else if (SHORT1FROMMP(mp1) == ID_TYPEOBJ)
                  {
                  WinEnableWindow(WinWindowFromID(hwnd, ID_OBJECTLIST), TRUE);
                  WinEnableWindow(WinWindowFromID(hwnd, ID_TEXTOBJ),    TRUE);

                  WinEnableWindow(WinWindowFromID(hwnd, ID_NEWPROG),    FALSE);
                  WinEnableWindow(WinWindowFromID(hwnd, ID_TEXTPROG),   FALSE);
                  }
               return (MRESULT)TRUE;
            }
         break;

      default:
         break;

      }
   mResult = WinDefDlgProc( hwnd, msg, mp1, mp2 );
   return mResult;
}

/*********************************************************************
*  Check the handle input
*********************************************************************/
BOOL ProcessAddHandleInput(HWND hwnd, PDDATA pData)
{
BOOL    fNewIsObject;
HOBJECT hObject;
PASSOCDATA pAssocData = pData->pCurAssocData;
ULONG ulIndex;
PBYTE pszExt;

   fNewIsObject = (BOOL)WinSendDlgItemMsg(hwnd, ID_TYPEOBJ, BM_QUERYCHECK, 0, 0);
   if (fNewIsObject)
      {
      SHORT sSelect;

      sSelect = (SHORT)WinSendDlgItemMsg(hwnd, ID_OBJECTLIST,
         LM_QUERYSELECTION, (MPARAM)LIT_FIRST, 0L);
      if (sSelect == LIT_NONE)
         {
         DebugBox("Warning","No object is selected!", FALSE);
         return FALSE;
         }
      hObject = (HOBJECT)WinSendDlgItemMsg(hwnd, ID_OBJECTLIST,
         LM_QUERYITEMHANDLE, (MPARAM)sSelect, 0L);
      }
   else
      {
      BYTE szProgName[256];

      WinQueryDlgItemText(hwnd,
         ID_NEWPROG, sizeof szProgName - 1, szProgName);
      if (!strlen(szProgName))
         {
         DebugBox("Warning","No program name has been entered!", FALSE);
         return FALSE;
         }
      if (access(szProgName, 0))
         {
         DebugBox("Warning","%s not found!", FALSE, szProgName);
         return FALSE;
         }
      pszExt = strchr(szProgName, '.');
      if (!pszExt || !(!stricmp(pszExt, ".BAT") ||
                       !stricmp(pszExt, ".CMD") ||
                       !stricmp(pszExt, ".EXE") ||
                       !stricmp(pszExt, ".COM")))
         {
         DebugBox("Warning","This is not a program!", FALSE);
         return FALSE;
         }
      hObject = WinQueryObject(szProgName);
      if (!hObject)
         {
         DebugBox("Warning","WinQueryObject: %s not found!", TRUE, szProgName);
         return FALSE;
         }
      ResetBlockBuffer();
      }
   for (ulIndex = 0; ulIndex < pAssocData->ulHandleCount; ulIndex++)
      {
      if (pAssocData->pHandleArray[ulIndex] == hObject)
         {
         DebugBox("Warning","Association already present!", FALSE);
         return FALSE;
         }
      }

   pAssocData->pHandleArray[pAssocData->ulHandleCount++] = hObject;
   pAssocData->fChanged = TRUE;
   pData->fChanged = TRUE;

   return TRUE;
}
/*********************************************************************
*  Debug Box
*********************************************************************/
BOOL DebugBox(PSZ pszTitle, PSZ pszMes, BOOL bShowInfo, ...)
{
BYTE szMessage[1024];
PERRINFO pErrInfo = WinGetErrorInfo(hab);
USHORT usSeverity;
CHAR   szSev[14],
       szErrNo[6],
       szHErrNo[6];
va_list va;


   va_start(va, bShowInfo);
   vsprintf(szMessage, pszMes, va);

   if (bShowInfo && pErrInfo)
      {
      PUSHORT pus = (PUSHORT)((PBYTE)pErrInfo + pErrInfo->offaoffszMsg);
      PSZ     psz = (PBYTE)pErrInfo + *pus;
      PSZ     p;

      strcat(szMessage, "\n");
      strcat(szMessage, psz);
      p = psz + strlen(psz) + 1;
      strcat(szMessage, "\n");
      strcat(szMessage, p);

      usSeverity = ERRORIDSEV(pErrInfo->idError);          
      itoa(ERRORIDERROR(pErrInfo->idError), szErrNo, 10);
      itoa(ERRORIDERROR(pErrInfo->idError), szHErrNo, 16);
      switch (usSeverity)
         {
         case 4:
            strcpy(szSev, "Warning");
            break;
  
         case 8:
            strcpy(szSev, "Error");
            break;
  
         case 12:
            strcpy(szSev, "Severe");
            break;
  
         case 16:
            strcpy(szSev, "Unrecoverable");
            break;
  
         default:
            strcpy(szSev, "None");
            break;
         }
      strcat(szMessage, "\n");
      strcat(szMessage, "Error No = ");
      strcat(szMessage, szErrNo);
      strcat(szMessage, "  (0x");
      strcat(szMessage, szHErrNo);
      strcat(szMessage, ")");
      strcat(szMessage, "\n");
      strcat(szMessage, "Severity = ");
      strcat(szMessage, szSev);

      }
   if (pErrInfo)
      WinFreeErrorInfo(pErrInfo);


   WinMessageBox(HWND_DESKTOP,HWND_DESKTOP, 
       (PSZ) szMessage , (PSZ) pszTitle, 0, 
       MB_OK | MB_INFORMATION );



   return TRUE;
}



/*******************************************************************
* Get the objects title
*******************************************************************/
BOOL GetObjectTitle(HOBJECT hObject, PSZ pszTitle, USHORT usMax)
{
USHORT usObjID;
PBYTE  pBuffer;
ULONG  ulProfileSize;
BYTE  szObjID[10];

   usObjID = LOUSHORT(hObject);
   if (IsObjectDisk(hObject))
      {
      if (PathFromObject(HINI_SYSTEM, hObject, pszTitle, usMax, NULL))
//      if (WinQueryObjectPath(hObject, pszTitle, usMax))
         return TRUE;
      else
         return FALSE;
      }

   sprintf(szObjID, "%X", usObjID);
   pBuffer = GetProfileData(OBJECTS, szObjID, HINI_USER, &ulProfileSize);
   if (!pBuffer)
      return FALSE;
   if (!GetGenObjectValue(pBuffer, "WPAbstract", WPABSTRACT_TITLE, pszTitle, usMax))
      {
      free(pBuffer);
      return FALSE;
      }
   free(pBuffer);
   return TRUE;
}

/****************************************************************
* Init the helpinstance
****************************************************************/
HWND InitHelpInstance(HWND hwndMain)
{
HELPINIT hini;
HWND     hwndHelpInstance;

   fHelpEnabled = FALSE;

   memset(&hini, 0, sizeof hini);
   hini.cb = sizeof(HELPINIT);
   hini.pszHelpWindowTitle = "Association Editor Help";
   hini.pszHelpLibraryName = "assoedit.hlp";
 
   hini.phtHelpTable = (PHELPTABLE)MAKELONG(EAS_HELP_TABLE, 0xFFFF);
                                
   hwndHelpInstance = WinCreateHelpInstance(hab, &hini);

   if (!hwndHelpInstance || hini.ulReturnCode)
      {
      DebugBox("WARNING", "Cannot find helpfile, no help available!", FALSE);
      return NULL;
      }
      
   if (!WinAssociateHelpInstance(hwndHelpInstance, hwndMain))
      {
      DebugBox("Error","Error on WinAssoctiateHelpInstance", TRUE);
      return NULL;
      }

   fHelpEnabled = TRUE;
   return hwndHelpInstance;
}
