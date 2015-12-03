#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define INCL_PM
#include <os2.h>
/******************************************************************
* Is A seamlesscommon session already active ?
******************************************************************/
int main(void)
{
ULONG ulEntries;
ULONG ulBlockSize;
PSWBLOCK pSwBlock;
ULONG ulIndex;

   ulEntries = WinQuerySwitchList(0, NULL, 0);
   ulBlockSize = sizeof (SWBLOCK) +
                 sizeof (HSWITCH) +
                 (ulEntries + 4) * (ULONG) sizeof(SWENTRY);
   pSwBlock = malloc(ulBlockSize);
   if (!pSwBlock)
      return FALSE;
   ulEntries = WinQuerySwitchList(0, pSwBlock, ulBlockSize);
   for (ulIndex = 0; ulIndex < ulEntries; ulIndex++)
      {
      PSWCNTRL ps = &pSwBlock->aswentry[ulIndex].swctl;

      printf("%8.8X %8.8X %4d %s (%8.8X)\n",
         ps->idProcess,
         ps->idSession,
         ps->bProgType,
         ps->szSwtitle,
         ps->hwnd);
      }
   free(pSwBlock);
   return FALSE;
}


