#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define INCL_PM
#include <os2.h>

INT main(INT iArgc, PSZ pszArgv[])
{
HOBJECT hObject;

   if (iArgc == 1)
      {
      printf("USAGE: QUERYOBJ path or OBJECTID\n");
      exit(1);
      }


   hObject = WinQueryObject(pszArgv[1]);
   if (!hObject)
      {
      printf("Object not found !\n");
      exit(1);
      }
   printf("Object found, handle %lX\n", hObject);

   if (iArgc > 2)
      if (!stricmp(pszArgv[2], "/D"))
      {
      if (WinDestroyObject(hObject))
         printf("Object destroyed!\n");
      else
         printf("Unable to destroy object!\n");
      }

   return 1;
}
