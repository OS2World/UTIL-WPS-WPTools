#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <io.h>
#define INCL_WIN
#define INCL_DOS
#include <os2.h>
#include "wptools\wptools.h"
#include "files.h"

int main(int argc, PSZ pszArg[])
{
HOBJECT hObject;
BYTE    szClass[50];
BYTE    szPath[CCHMAXPATH];
PBYTE   pData;

   if (argc < 2)
      {
      printf("GETOBJ path|OBJECT_ID");
      exit(1);
      }
   ParseFilename(szPath, pszArg[1]);
   if (!access(szPath, 0))
      hObject = WinQueryObject(szPath);
   else
      hObject = WinQueryObject(pszArg[1]);
   if (!hObject)
      {
      printf("Object not found!\n");
      exit(1);
      }


   if (WinQueryObjectPath(hObject, szPath, sizeof szPath))
      printf("Physical file name: %s\n", szPath);

   pData = pszGetObjectSettings(hObject, szClass, sizeof szClass, FALSE);
   if (pData)
      {
      printf("Class             : %s\n", szClass);
      printf("Setupstring       : %s\n", pData);
      }
   else
      {
      printf("No CLASSINFO found.\n");
      }
   return 0;
}

