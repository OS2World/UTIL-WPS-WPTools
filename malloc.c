#include <malloc.h>
#include <string.h>

#define INCL_DOS
#include <os2.h>

#include "msg.h"


void _FAR_ * _FAR_ _cdecl malloc(size_t tSize)
{
SEL Sel;
PVOID pv;

   if (DosAllocSeg(tSize, &Sel, SEG_NONSHARED))
      return NULL;
   pv = MAKEP(Sel, 0);
   memset(pv, 0, tSize);
   return pv;
}


void _FAR_ _cdecl free(void _FAR_ * pv)
{
SEL Sel = SELECTOROF(pv);

   if (DosFreeSeg(Sel))
      errmsg("Invalid pointer used in free");
}

void _FAR_ * _FAR_ _cdecl calloc(size_t tSize1, size_t tSize2)
{
   return malloc(tSize1 * tSize2);
}

void _FAR_ * _FAR_ _cdecl realloc(void _FAR_ *pv , size_t tSize)
{
SEL Sel = SELECTOROF(pv);

   if (DosReallocSeg(tSize, Sel))
      return NULL;
   return pv;
}



