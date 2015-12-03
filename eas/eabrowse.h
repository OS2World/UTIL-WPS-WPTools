#define MID_FILE           1
#define MID_OPEN           2
#define MID_QUIT           3
#define MID_ABOUT          4

#define MID_OPTIONS       10
#define MID_BINARY        11
#define MID_EAFILESONLY   12
#define MID_ALLOWEDIT     13
#define MID_FONT          14

#define MID_ATTRIBUTES    20
#define MID_ADD           21
#define MID_DELETE        22
#define MID_SAVE          23

#define MID_HELPMENU      30
#define MID_HELPINDEX     31
#define MID_GENERALHELP   32
#define MID_USINGHELP     33
#define MID_KEYSHELP      34

#define ID_WINDOW        100
/* the static controls */
#define ID_FILETITLE     101
#define ID_EATITLE       102
#define ID_EATYPE        103
#define ID_STATUS        104

/* selectable controls */
#define ID_FIRST         110
#define ID_FILELIST      110
#define ID_EALIST        111
#define ID_MVMTLIST      112
#define ID_MVMTLIST2     113
#define ID_ASCII         114
#define ID_BINARY        115
#define ID_ICON          116
#define ID_LAST          116

#include "eadlg.h"

#define FAMILYLEN    128

typedef struct _Options
{
BOOL   fShowAllBinary  ;
BOOL   fShowEAFilesOnly;
BOOL   fAllowEdit;
BYTE   szCurDir [300];
BYTE   szFontFamily[FAMILYLEN];
BYTE   szFaceName[FACESIZE];
ULONG  ulPointSize;
} OPTIONS, *POPTIONS;



BOOL ChangeDirDialog(HWND hwnd);

