#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <string.h>

#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include "wptools.h"


int main(int iArgc, PSZ rgpszArgv[])
{
PSZ pBuffer;
USHORT usSize;

   if (iArgc != 2)
      {
      printf("USAGE: QueryPRN driverfilename\n");
      exit(1);
      }
   if (access(rgpszArgv[1], 0))
      {
      printf("%s cannot be found!\n", rgpszArgv[1]);
      exit(1);
      }
   if (!GetEAValue(rgpszArgv[1], ".EXPAND",
      &pBuffer, &usSize))
      {
      printf(".EXPAND extended attribute not found!\n");
      exit(1);
      }
   while (*pBuffer)
      {
      printf("%s\n", pBuffer);
      pBuffer = pBuffer + strlen(pBuffer) + 1;
      }

   return 0;
}
