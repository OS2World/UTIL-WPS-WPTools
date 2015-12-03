#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#define INCL_WIN
#define INCL_DOS
#include <os2.h>


#define TAG_LONG        2
#define TAG_TEXT        3
#define TAG_BLOCK       4
#define STEP           16

#pragma pack(1)
typedef struct _ObjectInfo
{
USHORT cbName;
USHORT cbData;
BYTE   szName[1];
} OINFO, *POINFO;

typedef struct _TagInfo
{
USHORT usTagFormat;
USHORT usTag;
USHORT cbTag;
} TAG, *PTAG;

#pragma pack()

PBYTE ReadRootFolder(PSZ pszFname);
VOID DumpObjectData(PSZ pszKey, PBYTE pBuffer);


int main(INT iArgc, PSZ pszArgv[])
{
PBYTE pBuffer;
PSZ   pszFname = "C:\\WP ROOT. SF";

   for (*pszFname = 'C' ; *pszFname < 'F'; (*pszFname)++)
      {
      pBuffer = ReadRootFolder(pszFname);
      if (pBuffer)
         {
         DumpObjectData(pszFname, pBuffer+4);
         free(pBuffer);
         }
      }
   return 0;
}

PBYTE ReadRootFolder(PSZ pszFname)
{
INT iHandle;
ULONG ulLength;
PBYTE pBuffer;

   iHandle = open(pszFname, O_RDONLY|O_BINARY, 0);
   if (iHandle==-1)
      {
      printf("Unable to open %s\n", pszFname);
      return NULL;
      }
   ulLength = filelength(iHandle);
   pBuffer = malloc(ulLength);
   if (!pBuffer)
      {
      printf("Not enough memory !\n");
      return NULL;
      }

   read(iHandle, pBuffer, ulLength);
   close(iHandle);
   return pBuffer;
}

/****************************************************
*
*****************************************************/
VOID DumpObjectData(PSZ pszKey, PBYTE pBuffer)
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
                  if (*p == '\a' || *p == '\n' || *p == '\r')
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
   printf("\n");
}


