#include "..\wptools\wptools.h"

#define WM_REFILL              WM_USER + 100
#define WM_DELDLG              WM_USER + 101
#define MAX_PATH               512
#define MAX_DIR                256
#define FILTER_SIZE            10
#define EANAME_SIZE            50
#define APPS_NAME              "ICONTOOL"
#define MAX_ASSOC              5
#define DELTA_VALUE           0

#define SORT_NONE              1
#define SORT_NAME              2
#define SORT_REALNAME          3
#define SORT_EXT               4

#define READEA_NONE            0
#define READEA_SUBJECT         1
#define READEA_COMMENTS        2
#define READEA_KEYPHRASES      3
#define READEA_HISTORY         4
#define READEA_USER            5


typedef struct _FileInfo
{
HPOINTER  hptrIcon;
HPOINTER  hptrMiniIcon;
BOOL      bIconCreated;
BOOL      bMiniIconCreated;
BYTE      szFileName[MAX_PATH];
BYTE      szTitle[MAX_PATH];
BYTE      chType;
HAPP      happ;
HOBJECT   hObject;
HOBJECT   hObjAssoc[MAX_ASSOC];
} FILEINFO, *PFILEINFO;

#define TYPE_DRIVE     0
#define TYPE_DIRECTORY 1
#define TYPE_FILE      2
#define TYPE_ABSTRACT  3


#define WPPROGRAM_PROGTYPE  1
#define WPPROGRAM_EXEHANDLE 2
#define WPPROGRAM_PARAMS    3
#define WPPROGRAM_DIRHANDLE 4
#define WPPROGRAM_DOSSET    6
#define WPPROGRAM_EXENAME   9
#define WPPROGRAM_STRINGS   10
#define WPPROGRAM_DATA      11

#define WPPROGFILE_PROGTYPE  1
#define WPPROGFILE_DOSSET    5
#define WPPROGFILE_DATA     10

#define WPPGM_STR_EXENAME   0
#define WPPGM_STR_ARGS      1

#define WPABSTRACT_TITLE    1
#define WPABSTRACT_TYPE     2

#define WPOBJECT_SZID       6
#define WPSHADOW_LINK     104


#define ASSOC_FILTER   "PMWP_ASSOC_FILTER"
#define ASSOC_TYPE     "PMWP_ASSOC_TYPE"
#define HANDLES        "PM_Workplace:Handles"
#define OBJECTS        "PM_Abstract:Objects"
#define ICONS          "PM_Abstract:Icons"
#define FOLDERCONTENT  "PM_Abstract:FldrContent"
#define ACTIVEHANDLES  "PM_Workplace:ActiveHandles"
#define HANDLESAPP     "HandlesAppName"

#define KEY "BLOCK1"


typedef struct _RecordInfo
{
MINIRECORDCORE Record;
FILEINFO   finfo;
} RECINFO, *PRECINFO;

typedef struct _Instance
{
/* Options that are saved in the structure */

BOOL     bFlowed;
BOOL     bDirectories;
BOOL     bDrives;
BOOL     bRealNames;
BOOL     bSaveOnExit;
BOOL     bSmallIcons;
BOOL     bAnimIcons;
USHORT   usSort;
USHORT   usReadEA;
BYTE     szEAName[EANAME_SIZE];
BYTE     szFilter[FILTER_SIZE];
BYTE     szExtractPath[128];

BYTE     szCurDir [MAX_PATH];
BYTE     szCurDir2[MAX_PATH];
BYTE     szSaveDir[MAX_PATH];
BYTE     szLastDir[MAX_PATH];
BOOL     bSecondWindowOpen;
BOOL     bMsgWait;
BOOL     bEdit;
USHORT   usDragCount;
ULONG    ulDragItemCount;
PDRAGINFO pdinfo;
PRECINFO pcrec;
USHORT   usWindowNo;
BOOL     volatile bScanActive;
HMTX     hmtx;
INT      iFillThread;

BYTE     szCurEAFile[MAX_PATH];
BYTE     szCurEAName[EANAME_SIZE];

HWND     hwndDelete;
HWND     hwndMenu;
HWND     hwndContainer;
HWND     hwndMLE;
HWND     hwndStatic;
} INSTANCE, *PINSTANCE;


/************************************************************
* Globals
************************************************************/
extern HAB hab;

/************************************************************
* Function prototypes
************************************************************/
#pragma linkage(FilterContainerFunc, system)
#pragma linkage(SortContainerFunc, system)



VOID     CreateContainer   (HWND hwnd);
BOOL     SetContainerAttr  (PINSTANCE wd);
VOID     InitDrag          (HWND hwnd, MPARAM mp1, MPARAM mp2);
MRESULT  DragOver          (HWND hwnd, MPARAM mp1, MPARAM mp2);
VOID     DragDrop          (HWND hwnd, MPARAM mp1, MPARAM mp2);
VOID     EndConversation   (HWND hwnd, MPARAM mp1, MPARAM mp2);
VOID     DiscardObject     (HWND hwnd, MPARAM mp1, MPARAM mp2);
VOID     DeleteObject      (HWND hwnd);
VOID     DeleteDialog      (HWND hwnd);
VOID     AboutBoxDlg       (HWND hwnd);


BOOL     RemoveRecord      (HWND hwndContainer, PRECINFO pcrec);
PRECINFO AddRecord         (PINSTANCE wd, PRECINFO pAfter, PFILEINFO pFinfo, BOOL bInvalidate);
void     FillContainer     (HWND hwnd, PINSTANCE wd);
BOOL     DebugBox          (PSZ pszTitle, PSZ pszMes, BOOL bInfo);
BOOL     OptionsDialog     (HWND hwnd);
BOOL     ChangeDirDialog   (HWND hwnd);
BOOL     FilterContainerFunc(PRECINFO p1, PINSTANCE wd);
SHORT    SortContainerFunc (PRECINFO p1, PRECINFO p2, PINSTANCE wd);
VOID     GetIcons          (PSZ pszFileName, PFILEINFO pInfo, PINSTANCE wd);
VOID     DeleteIcons       (PFILEINFO pInfo);
HPOINTER ReadIcon          (HWND hwnd, PSZ pszIcon, USHORT usIconSize);
HPOINTER Buffer2Icon       (HWND hwnd, PBYTE pchIcon, USHORT usIconSize);
BOOL     GetLongName       (PSZ pszFile, PSZ pszTarget, USHORT usMax);
BOOL     DeletePath        (PSZ pszPath);
BOOL     GetIconFromPointer(HAB hab, HPOINTER hptr, PSZ pszFileName);
VOID     SaveOptions       (HWND hwnd);
BOOL     GetEAValue        (PSZ pszFile, PSZ pszKeyName, PBYTE *pTar, PUSHORT pusLength);
BOOL     SetEAValue        (PSZ pszFile, PSZ pszKey, USHORT usType, PBYTE pchValue, USHORT cbValue);
VOID     ReadEAValue       (HWND hwnd, PRECINFO pcrec, BOOL fRead);



#define TAG_LONG        2
#define TAG_TEXT        3
#define TAG_BLOCK       4

BOOL   GetObjectName       (HOBJECT hObject, PSZ pszFname, USHORT usMax);
BOOL   GetAssocFilter      (PSZ pszFileName, PULONG pulAssoc, USHORT usMax);
HPOINTER  GetObjectIcon    (HOBJECT hObject, PBYTE pObjectData);
ULONG GetObjectProgType    (HOBJECT hObject);
BOOL GetObjectTitle        (HOBJECT hObject, PSZ pszBuffer, USHORT usMax);
BOOL GetAbstractObject     (HOBJECT hObject, PFILEINFO pFinfo);
BOOL GetFolderContent      (PSZ pszFolder, PBYTE *pBuffer, PULONG pulProfileSize);
BOOL GetObjectCurDir       (HOBJECT hObject, PSZ pszCurDir, USHORT usMax);
BOOL GetObjectParameters   (HOBJECT hObject, PSZ pszParms, USHORT usMax);
BOOL GetDosSettings(HOBJECT hObject, PSZ pszDosSettings, USHORT usMax);
USHORT GetObjectData       (PBYTE pObjData, PSZ pszObjName, USHORT usTag, PVOID pOutBuf, USHORT usMaxBuf);
HOBJECT MakeAbstractHandle (USHORT usObject);
BOOL MySetObjectData       (HOBJECT hObject, PSZ pszSettings);
BOOL MyDestroyObject       (HOBJECT hObject);

