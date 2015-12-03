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
/*******************************************************************
* Get DataFileoptions
*******************************************************************/
BOOL _System GetWPDataFileOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
   vGetFSysTitle(hObject, pszOptions);
   return TRUE;
}

