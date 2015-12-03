#include <stdio.h>

#define INCL_PM
#include <os2.h>

int main(void)
{
BYTE szName[512];
ULONG ulIndex;
HOBJECT hObject = WinQueryObject("<FUCKLEDUCK>");
HOBJECT hObj;

   for (ulIndex = 0; ulIndex < 65535L; ulIndex++)
      {
      if (!(ulIndex % 1000))
         {
         printf("%5u\b\b\b\b\b", ulIndex);
         fflush(stdout);
         }
      sprintf(szName, "EXENAME=E:\\PIET\\%u.dat", ulIndex);
      WinSetObjectData(hObject, szName);
      hObj = WinQueryObject(szName);
      if (HIUSHORT(hObj) == 4)
         printf("\n444!\n");
      }
   return 0;
}

