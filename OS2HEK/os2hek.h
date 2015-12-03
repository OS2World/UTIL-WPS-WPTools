#ifndef OS2HEK_H
#define OS2HEK_H

#define INCL_DOS
#define INCL_GPI
#include <os2.h>
#include <pmbitmap.h>

/*
   Several defines to read WPShell data
*/
#define FOLDERCONTENT  "PM_Abstract:FldrContent"
#define OBJECTS        "PM_Abstract:Objects"
#define ICONS          "PM_Abstract:Icons"

#define ACTIVEHANDLES  "PM_Workplace:ActiveHandles"
#define HANDLESAPP     "HandlesAppName"

#define FOLDERPOS      "PM_Workplace:FolderPos"
#define HANDLES        "PM_Workplace:Handles"
#define LOCATION       "PM_Workplace:Location"
#define PALETTEPOS     "PM_Workplace:PalettePos"
#define STATUSPOS      "PM_Workplace:StatusPos"
#define STARTUP        "PM_Workplace:Startup"
#define TEMPLATES      "PM_Workplace:Templates"
#define HANDLEBLOCK    "BLOCK1"
#define ASSOC_FILTER   "PMWP_ASSOC_FILTER"
#define ASSOC_TYPE     "PMWP_ASSOC_TYPE"
#define ASSOC_CHECKSUM "PMWP_ASSOC_CHECKSUM"
#define WORKAREARUNNING "FolderWorkareaRunningObjects"
#define JOBCNRPOS       "PM_PrintObject:JobCnrPos"
#define PM_OBJECTS      "PM_Objects"
#define CLASSTABLE      "ClassTable"
#define PRINTOBJECTS    "PM_PrintObject"

#ifdef FORGET_IT
#define EAT_BINARY      0xFFFE      /* length preceeded binary */
#define EAT_ASCII       0xFFFD      /* length preceeded ASCII */
#define EAT_BITMAP      0xFFFB      /* length preceeded bitmap */
#define EAT_METAFILE    0xFFFA      /* length preceeded metafile */
#define EAT_ICON        0xFFF9      /* length preceeded icon */
#define EAT_EA          0xFFEE      /* length preceeded ASCII */
                                    /* name of associated data (#include) */
#define EAT_MVMT        0xFFDF      /* multi-valued, multi-typed field */
#define EAT_MVST        0xFFDE      /* multi-valued, single-typed field */
#define EAT_ASN1        0xFFDD      /* ASN.1 field */
#endif
/*
   Several defines to read EA's
*/
#define EA_LPBINARY      EAT_BINARY      /* Length preceeded binary            */
#define EA_LPASCII       EAT_ASCII       /* Length preceeded ascii             */
#define EA_ASCIIZ        0xFFFC          /* Asciiz                             */
#define EA_LPBITMAP      EAT_BITMAP      /* Length preceeded bitmap            */
#define EA_LPMETAFILE    EAT_METAFILE    /* metafile                           */
#define EA_LPICON        EAT_ICON        /* Length preceeded icon              */
#define EA_ASCIIZFN      0xFFEF          /* Asciiz file name of associated dat */
#define EA_ASCIIZEA      EAT_EA          /* Name of associated data, LP Ascii  */
#define EA_MVMT          EAT_MVMT        /* Multi value, multi type            */
#define EA_MVST          EAT_MVST        /* Multi value, single type           */
#define EA_ASN1          EAT_ASN1        /* ASN.1 Field                        */



/*
   The high word of a HOBJECT determines it's type.
*/

#define OBJECT_ABSTRACT 0x0001
#define OBJECT_ABSTRACT2 0x0002
#define OBJECT_FSYS     0x0003

/*
   Several datatags
*/
#define WPPROGRAM_PROGTYPE  1
#define WPPROGRAM_EXEHANDLE 2
#define WPPROGRAM_PARAMS    3
#define WPPROGRAM_DIRHANDLE 4
#define WPPROGRAM_DOSSET    6
#define WPPROGRAM_STYLE     7
#define WPPROGRAM_EXENAME   9
#define WPPROGRAM_DATA      11
#define WPPROGRAM_STRINGS   10
#define WPPGM_STR_EXENAME   0
#define WPPGM_STR_ARGS      1

#define WPABSTRACT_TITLE    1
#define WPABSTRACT_STYLE     2 /* appears to contain the same as WPOBJECT 7 */

#define WPOBJECT_HELPPANEL   2
#define WPOBJECT_SZID        6
#define WPOBJECT_STYLE       7
#define WPOBJECT_MINWIN      8
#define WPOBJECT_CONCURRENT  9
#define WPOBJECT_VIEWBUTTON 10
#define WPOBJECT_DATA       11
#define WPOBJECT_STRINGS    12
#define WPOBJ_STR_OBJID      1

#define WPSHADOW_LINK     104

#define WPFOLDER_FOLDERFLAG  13
#define WPFOLDER_ICONVIEW  2900
#define WPFOLDER_DATA      2931
#define WPFOLDER_FONTS     2932

#define WPPROGFILE_PROGTYPE  1
#define WPPROGFILE_DOSSET    5
#define WPPROGFILE_DATA     10

#define WPFSYS_MENUCOUNT     4
#define WPFSYS_MENUARRAY     3


/*
   Two structure needed for finding a filename based on a object handle
*/

#pragma pack(1)

typedef struct _NodeDev
{
BYTE   chName[4];  /* = 'NODE' */
USHORT usLevel;
USHORT usID;
USHORT usParentID;
BYTE   ch[18];
USHORT fIsDir;   
USHORT usNameSize;
BYTE   szName[1];
} NODE, *PNODE;

typedef struct _DrivDev
{
BYTE  chName[4];  /* = 'DRIV' */
BYTE  ch1[8];
ULONG ulSerialNr;
BYTE  ch2[4];
BYTE  szName[1];
} DRIV, *PDRIV;
                               
/*
   Two structures needed for reading WPAbstract's object information
*/
typedef struct _ObjectInfo
{
USHORT cbName;       /* Size of szName */
USHORT cbData;       /* Size of variable length data, starting after szName */
BYTE   szName[1];    /* The name of the datagroup */
} OINFO, *POINFO;

typedef struct _TagInfo
{
USHORT usTagFormat;  /* Format of data       */
USHORT usTag;        /* The data-identifier  */
USHORT cbTag;        /* The size of the data */
} TAG, *PTAG;

typedef struct _WPProgramRefData
{
HOBJECT hExeHandle;
HOBJECT hCurDirHandle;
ULONG   ulFiller1;
ULONG   ulProgType;
BYTE    bFiller[12];
} WPPGMDATA;

typedef struct _WPProgramFileData
{
HOBJECT hCurDirHandle;
ULONG   ulProgType;
ULONG   ulFiller1;
ULONG   ulFiller2;
ULONG   ulFiller3;
} WPPGMFILEDATA;


typedef struct _WPObjectData
{
ULONG ulUnknown1;
ULONG ulHelpPanel;
ULONG ulUnknown3;
ULONG ulObjectStyle;
ULONG ulMinWin;
ULONG ulConcurrent;
ULONG ulViewButton;
ULONG ulUnknown4;
} WPOBJDATA;

typedef struct _WPFolderData
{
ULONG ulIconView;
ULONG ulTreeView;
ULONG ulDetailsView;
ULONG ulFolderFlag;
ULONG ulTreeStyle;
ULONG ulDetailsStyle;
BYTE  Filler[60];
} WPFOLDATA;


typedef struct _FsysMenu
{
   USHORT  usIDMenu   ;
   USHORT  usIDParent ;
   USHORT  usMenuType; /*  1 = Cascade, 2 = condcascade, 3 = choice */
   HOBJECT hObject;
   BYTE    szTitle[32];
} FSYSMENU, *PFSYSMENU;

typedef struct _FolderSort
{
LONG lDefaultSortIndex;
BOOL fAlwaysSort;
LONG lCurrentSort;
BYTE bFiller[12];
} FOLDERSORT, *PFOLDERSORT;

typedef struct _FldBkgnd
{
ULONG  ulUnknown;
RGB    rgb;
BYTE   bFiller;
BYTE   bColorOnly; /* 0x28 Image, 0x27 Color only */
BYTE   bFiller2;
BYTE   bImageType; /* 2=Normal,3=tiled,4=scaled */
BYTE   bFiller3;
BYTE   bScaleFactor; 
BYTE   bFiller4;
} FLDBKGND, *PFLDBKGND;


#pragma pack()

/*****************************************************************************
* Function prototypes
*****************************************************************************/
PVOID  _System GetAllProfileNames(PSZ pszApp, HINI hini, PULONG pulProfileSize);
PVOID  _System GetProfileData(PSZ pszApp, PSZ pszKey, HINI hini, PULONG pulProfileSize);

BOOL   _System GetActiveHandles(HINI hIniSystem, PSZ pszHandlesAppName, USHORT usMax);
PNODE  _System PathFromObject(HINI hIniSystem, HOBJECT hObject, PSZ pszFname, USHORT usMax);
PNODE  _System GetPartName(PBYTE pBuffer, ULONG ulBufSize, USHORT usID, PSZ pszFname, USHORT usMax);
VOID   _System ResetBlockBuffer(VOID);
HOBJECT _System MyQueryObjectID(HINI hIniUser, HINI hIniSystem, PSZ pszName);
BOOL   _System fReadAllBlocks(HINI hiniSystem, PSZ pszActiveHandles, PBYTE * ppBlock, PULONG pulSize);


BOOL   _System GetEAValue(PSZ pszFile, PSZ pszKeyName, PBYTE *pTarget, PUSHORT pusLength);
BOOL   _System SetEAValue(PSZ pszFile, PSZ pszKey, USHORT usType, PBYTE pchValue, USHORT cbValue);

USHORT _System GetObjectData(PBYTE pObjData, PSZ pszObjName, USHORT usTag, PVOID pOutBuf, USHORT usMax);
PBYTE  _System GetClassInfo(HINI hini, HOBJECT hObject, PULONG pulProfileSize);
BOOL   _System GetObjectID(PBYTE pObjectData, PSZ pszObjectID, USHORT usMax);
BOOL   _System GetObjectDataString(PBYTE pObjData, PSZ pszObjName, USHORT usTag,
                         USHORT usStringTag, PVOID pOutBuf, USHORT usMax);
BOOL   _System IsObjectDisk         (HOBJECT hObject);
BOOL   _System IsObjectAbstract     (HOBJECT);

HOBJECT _System MakeAbstractHandle  (USHORT usObject);
HOBJECT _System MakeDiskHandle      (USHORT usObject);
BOOL    _System MySetObjectData     (HOBJECT hObject, PSZ pszSettings);
BOOL    _System MyDestroyObject     (HOBJECT hObject);

BOOL   _System MessageBox(PSZ pszTitle, PSZ pszMes, ...);
BOOL   _System Is21(VOID);
BOOL   _System IsWarp(VOID);


HPOINTER _System ReadIcon(HWND hwnd, PSZ pszIcon, USHORT usIconSize);
HPOINTER _System Buffer2Icon(HWND hwnd, PBYTE pchIcon, USHORT usIconSize);


#endif
