#ifndef WPTOOLS_H
#define WPTOOLS_H

#define INCL_DOS
#define INCL_GPI
#include <os2.h>
#include <pmbitmap.h>

#include "portable.h"

#define OPTIONS_SIZE 32767
#define VERSION "2.12"



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
#define CLASSTABLE      "ClassTable"
#define PRINTOBJECTS    "PM_PrintObject"
#define PM_OBJECTS      "PM_Objects"


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
#define WPPROGFILE_STYLE     6
#define WPPROGFILE_DATA     10
#define WPPROGFILE_STRINGS  11
#define WPPRGFIL_STR_ARGS    0

#define WPFSYS_MENUCOUNT     4
#define WPFSYS_MENUARRAY     3

#define IDKEY_PRNQUEUENAME 3
#define IDKEY_PRNCOMPUTER  5
#define IDKEY_PRNJOBDIALOG 9
#define IDKEY_PRNREMQUEUE 13

#define IDKEY_RPRNNETID    1

#define IDKEY_DRIVENUM     1

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
LONG  lDefaultView;
ULONG ulHelpPanel;
ULONG ulUnknown3;
ULONG ulObjectStyle;
ULONG ulMinWin;
ULONG ulConcurrent;
ULONG ulViewButton;
ULONG ulMenuFlag;
} WPOBJDATA;

typedef struct _WPFolderData
{
ULONG ulIconView;
ULONG ulTreeView;
ULONG ulDetailsView;
ULONG ulFolderFlag;
ULONG ulTreeStyle;
ULONG ulDetailsStyle;
BYTE  rgbIconTextBkgnd[4];
BYTE  Filler1[4];
BYTE  rgbIconTextColor[4];
BYTE  Filler2[8]; 
BYTE  rgbTreeTextColor[4];
BYTE  rgbDetailsTextColor[4];
BYTE  Filler3[4]; 
USHORT fIconTextVisible;
USHORT fIconViewFlags;
USHORT fTreeTextVisible;
USHORT fTreeViewFlags;
BYTE  Filler4[4];
ULONG ulMenuFlag;
BYTE  rgbIconShadowColor[4];
BYTE  rgbTreeShadowColor[4];
BYTE  rgbDetailsShadowColor[4];
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
BYTE   bBlue;
BYTE   bGreen;
BYTE   bRed;
BYTE   bExtra;
//RGB    rgb;
//BYTE   bFiller;
BYTE   bColorOnly; /* 0x28 Image, 0x27 Color only */
BYTE   bFiller2;
BYTE   bImageType; /* 2=Normal,3=tiled,4=scaled */
BYTE   bFiller3;
BYTE   bScaleFactor; 
BYTE   bFiller4;
} FLDBKGND, *PFLDBKGND;

#define BASECLASS_TRANSIENT 0
#define BASECLASS_ABSTRACT  1
#define BASECLASS_FILESYS   2
#define BASECLASS_OTHER    99

#pragma pack()

IMPORT PSZ _System GetWPToolsVersion(VOID);
IMPORT HINI   hIniSystem, hIniUser;

IMPORT VOID   SetInis(HINI hUser, HINI hSystem);
IMPORT PSZ    GetBaseClassString(HOBJECT hObject);
IMPORT USHORT GetBaseClassType(HOBJECT hObject);

IMPORT HOBJECT _rgLaunchPadObjects[];
IMPORT BOOL   _System fGetObjectClass(HOBJECT hObject, PSZ pszClass, USHORT usMax);
IMPORT PSZ    _System pszGetObjectSettings(HOBJECT hObject, PSZ pszCls, USHORT usMax, BOOL fAddComment);
IMPORT BOOL   _System GetSCenterOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPProgramOptions (HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPProgramFileOptions (HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT VOID   _System ConvertDosSetting(PBYTE pTarget, USHORT usTargetSize, PSZ pSrc, USHORT usSrcSize);
IMPORT BOOL   _System GetAssocFilters(HOBJECT hObject, PSZ pszAssoc, USHORT usMax);
IMPORT BOOL   _System GetAssocTypes(HOBJECT hObject, PSZ pszAssoc, USHORT usMax);
IMPORT BOOL   _System GetWPAbstractOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT VOID   _System ConvertTitle(PBYTE pTarget, USHORT usTargetSize, PSZ pSrc, USHORT usSrcSize);
IMPORT BOOL   _System GetWPObjectOptions  (HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPFsysOptions    (HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPFolderOptions  (HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPDataFileOptions  (HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPShadowOptions  (HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPPaletteOptions (HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPFontPaletteOptions (HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPColorPaletteOptions (HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetPrintDestOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPRPrinterOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPDiskOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPLaunchPadOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPDesktopOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPHostOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT BOOL   _System GetWPUrlOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData);
IMPORT VOID   _System vGetFSysTitle(HOBJECT hObject, PSZ pszOptions);
IMPORT BOOL   _System GetObjectLocation(HOBJECT hObject, PSZ pszLocation, USHORT usMax);

IMPORT PVOID  _System GetAllProfileNames(PSZ pszApp, HINI hini, PULONG pulProfileSize);
IMPORT PVOID  _System GetProfileData(PSZ pszApp, PSZ pszKey, HINI hini, PULONG pulProfileSize);

IMPORT BOOL   _System GetActiveHandles(HINI hIniSystem, PSZ pszHandlesAppName, USHORT usMax);
IMPORT PNODE  _System PathFromObject(HINI hIniSystem, HOBJECT hObject, PSZ pszFname, USHORT usMax, PBYTE pfHandleUsed);
IMPORT PNODE  _System GetPartName(PBYTE pBuffer, ULONG ulBufSize, USHORT usID, PSZ pszFname, USHORT usMax, PBYTE pfHandleUsed);
IMPORT VOID   _System ResetBlockBuffer(VOID);
IMPORT HOBJECT _System MyQueryObjectID(HINI hIniUser, HINI hIniSystem, PSZ pszName);
IMPORT BOOL   _System fReadAllBlocks(HINI hiniSystem, PSZ pszActiveHandles, PBYTE * ppBlock, PULONG pulSize);


IMPORT BOOL   _System GetEAValue(PSZ pszFile, PSZ pszKeyName, PBYTE *pTarget, PUSHORT pusLength);
IMPORT BOOL   _System SetEAValue(PSZ pszFile, PSZ pszKey, USHORT usType, PBYTE pchValue, USHORT cbValue);

IMPORT USHORT _System GetObjectValue(PBYTE pObjData, USHORT usTag, PVOID pOutBuf, USHORT usMax);
IMPORT USHORT _System GetObjectValueSize(PBYTE pObjData, USHORT usTag);
IMPORT USHORT _System GetGenObjectValue(PBYTE pObjData, PSZ pszObjName, USHORT usTag, PVOID pOutBuf, USHORT usMax);

IMPORT BOOL   _System GetObjectID(PBYTE pObjectData, PSZ pszObjectID, USHORT usMax);
IMPORT PBYTE  _System GetClassInfo(HINI hini, HOBJECT hObject, PULONG pulProfileSize);
IMPORT BOOL   _System ObjectIDFromData(PBYTE pObjectData, PSZ pszObjectID, USHORT usMax);
IMPORT BOOL   _System GetObjectValueSubValue(PBYTE pObjData, USHORT usTag,
                         USHORT usStringTag, PVOID pOutBuf, USHORT usMax);
IMPORT BOOL _System GetGenObjectValueSubValue(PBYTE pObjData, PSZ pszObjName, USHORT usTag,
                         USHORT usStringTag, PVOID pOutBuf, USHORT usMax);

IMPORT BOOL   _System IsObjectDisk         (HOBJECT hObject);
IMPORT BOOL   _System IsObjectAbstract     (HOBJECT);

IMPORT HOBJECT _System MakeAbstractHandle  (USHORT usObject);
IMPORT HOBJECT _System MakeDiskHandle      (USHORT usObject);
IMPORT BOOL    _System MySetObjectData     (HOBJECT hObject, PSZ pszSettings);
IMPORT BOOL    _System MyDestroyObject     (HOBJECT hObject);

IMPORT BOOL   _System MessageBox(PSZ pszTitle, PSZ pszMes, ...);
IMPORT BOOL   _System Is21(VOID);
IMPORT BOOL   _System IsWarp4(VOID);
IMPORT BOOL   _System IsWarp(VOID);
IMPORT PSZ    _System pszObjectIDFromHandle(HOBJECT hObject);

IMPORT HPOINTER _System ReadIcon(HWND hwnd, PSZ pszIcon, USHORT usIconSize);
IMPORT HPOINTER _System Buffer2Icon(HWND hwnd, PBYTE pchIcon, USHORT usIconSize);
IMPORT PSZ _System DumpObjectData(HOBJECT hObject);

#endif


