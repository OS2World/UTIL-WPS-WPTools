#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#define INCL_WIN
#define INCL_DOS
#include <os2.h>

#define APP      "PM_Workplace:Handles"
#define OBJECT   "PM_Abstract:Objects"
#define HANDLES  "PM_Workplace:Handles"
#define KEY "BLOCK1"
#define STEP 16

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

PSZ pszTagName[]=
{
"Tag 0       ",
"Program type",
"Prog handle ",
"Parameters  ",
"Dir handle  ",
"Tag 5       ",
"Setting     ",
"Tag 7       ",
"Tag 8       ",
"ProgString  ",
"Tag 10      ",
"Tag 12      ",
"Tag 13      ",
"Tag 14      ",
"Tag 15      ",
"Tag 16      ",
"Tag 17      ",
"Tag 18      ",
"Tag 19      ",

};

#define PROG_TYPE   0x0001
#define PROG_HANDLE 0x0002
#define PROG_PARM   0x0003
#define DIR_HANDLE  0x0004
#define DOS_SETTING 0x0006
#define PROG_NAME   0x0009
#define TAG_END     0x000B

#define TAG_LONG        2
#define TAG_TEXT        3
#define TAG_BLOCK       4

void  GetData(PSZ pszApp, PSZ pszKey, HINI hini);
VOID  DumpObjectData(PSZ pszKey, PBYTE pBuffer, ULONG ulProfileSize);

int main(int argc, PSZ pszArg[])
{
ULONG ulProfileSize;
PBYTE pBuffer, p;
HINI hini;
HOBJECT hObject;
BYTE    szObjectID[10];


   hini = PrfOpenProfile(0, "E:\\RJ\\OS2.INI");
   if (!hini)
      {
      printf("Error: Unable to open requested ini's\n");
      exit(1);
      }

   if (argc != 1)
      {
      hObject = WinQueryObject(pszArg[1]);
      if (!hObject)
         {
         printf("Object not found!\n");
         exit(1);
         }
      sprintf(szObjectID, "%lX", LOUSHORT(hObject));
      GetData(OBJECT, szObjectID, hini);
      return 1;
      }

   if (!PrfQueryProfileSize(hini,
      OBJECT,
      NULL,
      &ulProfileSize))
      {
      printf("Error while retrieving profile size for %s", OBJECT);
      return 0;
      }

   pBuffer = malloc(ulProfileSize);

   if (!PrfQueryProfileData(hini,
      OBJECT,
      NULL,
      pBuffer,
      &ulProfileSize))
      {
      printf("Error while retrieving profile data for %s", OBJECT);
      return 0;
      }

   p = pBuffer;
   while (*p)
      {
      GetData(OBJECT, p, hini);
      p += strlen(p) + 1;
      }
   free(pBuffer);
   return 0;
}

void GetData(PSZ pszApp, PSZ pszKey, HINI hini)
{
ULONG ulProfileSize;
PBYTE pBuffer;

   if (!PrfQueryProfileSize(hini,
      pszApp,
      pszKey,
      &ulProfileSize))
      {
      printf("Error while retrieving profile size for %s", pszKey);
      return;
      }

   pBuffer = malloc(ulProfileSize+10);

   if (!PrfQueryProfileData(hini,
      pszApp,
      pszKey,
      pBuffer,
      &ulProfileSize))
      {
      printf("Error while retrieving profile data for %s", pszKey);
      return;
      }

   DumpObjectData(pszKey, pBuffer, ulProfileSize);
   free(pBuffer);
}

/****************************************************
*
*****************************************************/
VOID DumpObjectData(PSZ pszKey, PBYTE pBuffer, ULONG ulProfileSize)
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


