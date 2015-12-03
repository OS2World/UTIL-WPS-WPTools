#include <stdio.h>
#include <stdlib.h>
#define INCL_DOS
#include <os2.h>

BYTE   bDena[20000];

INT main(INT iArgc, PSZ pszArgv[])
{
APIRET rc;
ULONG  ulOrd;
ULONG  ulDenaCnt;
PDENA2 pDena;

   if (iArgc < 2)
      {
      printf("USAGE: QUERYEA filename\n");
      exit(1);
      }

   ulOrd = 1;
   ulDenaCnt = (ULONG)-1;
   rc = DosEnumAttribute(ENUMEA_REFTYPE_PATH,
      pszArgv[1],
      ulOrd,
      bDena,
      sizeof bDena,
      &ulDenaCnt,
      ENUMEA_LEVEL_NO_VALUE);

   if (rc)
      {
      printf("DosEnumAttribute: Error SYS%4.4u\n", rc);
      exit(1);
      }

   pDena = (PDENA2)bDena;
   while (ulDenaCnt--)
      {
      printf("%s\n", pDena->szName);
      if (!pDena->oNextEntryOffset)
         break;
      pDena = (PDENA2) ((PBYTE)pDena + pDena->oNextEntryOffset);
      } 
   return 0;
}
