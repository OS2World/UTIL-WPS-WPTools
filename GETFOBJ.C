#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include "wptools\wptools.h"

#define TAG_LONG        2
#define TAG_TEXT        3
#define TAG_BLOCK       4
#define STEP           16


VOID ShowObjectData        (PSZ pszKey, PBYTE pBuffer, ULONG ulBufSize);

int main(INT iArgc, PSZ pszArgv[])
{
PBYTE pObjectData;
ULONG ulProfileSize;
HOBJECT hObject;

   if (iArgc != 2)
      {
      printf("USAGE: GETFOBJ objecid|pathname");
      exit(1);
      }

   hObject = WinQueryObject(pszArgv[1]);
   if (!hObject)
      {
      printf("Location %s does not exist!\n", pszArgv[1]);
      exit(1);
      }
   pObjectData = GetClassInfo(NULL, hObject, &ulProfileSize);
   if (pObjectData)
      {
      printf("HOBJECT = %lX\n", hObject);
      ShowObjectData(pszArgv[1], pObjectData, ulProfileSize);
      free(pObjectData);
      }
   else
      printf("No classinfo found!\n");
   return 0;
}
/****************************************************
*
*****************************************************/
VOID ShowObjectData(PSZ pszKey, PBYTE pBuffer, ULONG ulProfileSize)
{
PBYTE pb, pBufEnd, p;
USHORT usIndex;
ULONG ulObjSize;
BYTE  szBlockText[STEP+1];

   ulObjSize = *(PULONG)pBuffer;
   printf("OBJECT %s, %4ld bytes, CLASS: %s\n",
      pszKey, *(PULONG)pBuffer, pBuffer + sizeof (ULONG));

   pb = pBuffer + sizeof (ULONG);
   pb += strlen(pb) + 1;

   for (usIndex = 0; usIndex < 16; usIndex++)
      {
      printf("%2.2X ", (USHORT)(*pb));
      pb++;
      }
   printf("\n");

   pBufEnd = pBuffer + ulObjSize;
   while (pb < pBufEnd)
      {
      POINFO pOinfo;
      PBYTE  pbGroepBufEnd;

      pOinfo = (POINFO)pb;
      pb += sizeof(OINFO) + strlen(pOinfo->szName);

      printf("CLASSDATA : %s ", pOinfo->szName);
      printf("LenNaam : %2d ", pOinfo->cbName);
      printf("LenData : %2d\n", pOinfo->cbData);

      pbGroepBufEnd = pb + pOinfo->cbData;
      while (pb < pbGroepBufEnd)
         {
         PTAG pTag   = (PTAG)pb;
         pb += sizeof (TAG);
         printf("Tag %4d (%2d bytes):", pTag->usTag, pTag->cbTag);
         switch(pTag->usTagFormat)
            {
            case TAG_TEXT:
               printf("Text : %s\n", pb);
               break;
            case TAG_LONG:
               printf("Long : %8.8X", *(PULONG)pb);
               printf("\n");
               break;
            case TAG_BLOCK:
               printf("Block:\n");
               memset(szBlockText, 0, sizeof szBlockText);
               usIndex = 0;
               for (p = pb; p < pb + pTag->cbTag; p++)
                  {
                  if (usIndex >= STEP)
                     {
                     printf("  %s\n", szBlockText);
                     memset(szBlockText, 0, sizeof szBlockText);
                     usIndex = 0;
                     }
                  printf("%2.2X ",(USHORT)*p);
                  if (*p == '\a' || *p == '\n' || *p == '\r' || *p == 0)
                     szBlockText[usIndex] = '.';
                  else
                     szBlockText[usIndex] = *p;
                  usIndex++;
                  }
               if (usIndex)
                  {
                  while (usIndex < STEP)
                     {
                     printf("   ");
                     usIndex++;
                     }
                  printf("  %s\n", szBlockText);
                  }
               break;
            default :
               printf("Unknown tagformat %d\n", pTag->usTagFormat);
               break;
            }
         pb += pTag->cbTag;
         }
      }

   printf("Additional data:\n");
   memset(szBlockText, 0, sizeof szBlockText);
   usIndex = 0;
   pBufEnd = pBuffer + ulProfileSize;
   for (p = pb; p < pBufEnd; p++)
      {
      if (usIndex >= STEP)
         {
         printf("  %s\n", szBlockText);
         memset(szBlockText, 0, sizeof szBlockText);
         usIndex = 0;
         }
      printf("%2.2X ",(USHORT)*p);
      if (*p == '\a' || *p == '\n' || *p == '\r' || *p == 0)
         szBlockText[usIndex] = '.';
      else
         szBlockText[usIndex] = *p;
      usIndex++;
      }
   if (usIndex)
      {
      while (usIndex < STEP)
         {
         printf("   ");
         usIndex++;
         }
      printf("  %s\n", szBlockText);
      }
   printf("\n");
}


