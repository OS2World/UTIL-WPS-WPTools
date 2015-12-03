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


#define TAG_LONG        2
#define TAG_TEXT        3
#define TAG_BLOCK       4
#define STEP           16

/*
   Several defines to read EA's
*/
#define EA_LPBINARY      0xfffe       /* Length preceeded binary            */
#define EA_LPASCII       0xfffd       /* Length preceeded ascii             */
#define EA_ASCIIZ        0xfffc       /* Asciiz                             */
#define EA_LPBITMAP      0xfffb       /* Length preceeded bitmap            */
#define EA_LPMETAFILE    0xfffa       /* metafile                           */
#define EA_LPICON        0xfff9       /* Length preceeded icon              */
#define EA_ASCIIZFN      0xffef       /* Asciiz file name of associated dat */
#define EA_ASCIIZEA      0xffee       /* Asciiz EA of associated data       */
#define EA_MVMT          0xffdf       /* Multi-value multi-typed field      */
#define EA_MVST          0xffde       /* Multy value single type field      */
#define EA_ASN1          0xffdd       /* ASN.1 field                        */


#pragma pack(1)
typedef struct _ObjectPos
{
ULONG  ulX;
ULONG  ulY;
BYTE   szName[1];
} OPOS, *POPOS;
#pragma pack()

VOID DumpObjectData        (PSZ pszKey, PBYTE pBuffer, ULONG ulBufSize);
BOOL   GetEAValue          (PSZ pszFile, PSZ pszKeyName, PBYTE *pTarget, PUSHORT pusLength);
BOOL SetEAValue(PSZ pszFile, PSZ pszKey, USHORT usType, PBYTE pchValue, USHORT cbValue);

PBYTE pszName = NULL;
ULONG ulX, ulY;

int main(INT iArgc, PSZ pszArgv[])
{
PBYTE pBuffer;
USHORT usEASize;

   if (iArgc < 2)
      {
      printf("USAGE: GETIPOS pathname [name x y]");
      exit(1);
      }

   if (iArgc == 5)
      {
      pszName = pszArgv[2];
      ulX     = atol(pszArgv[3]);
      ulY     = atol(pszArgv[4]);
      }

   if (GetEAValue(pszArgv[1], ".ICONPOS", &pBuffer, &usEASize))
      {
      DumpObjectData(pszArgv[1], pBuffer, usEASize);
      if (iArgc > 2)
         SetEAValue(pszArgv[1], ".ICONPOS", EA_LPBINARY, pBuffer, usEASize);
      free(pBuffer);
      }
   else
      printf("No .ICONPOS found!\n");
   return 0;
}
/****************************************************
*
*****************************************************/
VOID DumpObjectData(PSZ pszKey, PBYTE pBuffer, ULONG ulProfileSize)
{
PBYTE pb, pBufEnd;
POPOS pPos;

   printf("OBJECT %s\n", pszKey);

   pBufEnd = pBuffer + ulProfileSize;
   pb = pBuffer + 21;
   while (pb < pBufEnd)
      {
      pPos = (POPOS)pb;
      printf("%4d:%4d - %s\n",
         pPos->ulX,
         pPos->ulY,
         pPos->szName);
      pb += sizeof (OPOS) + strlen(pPos->szName);
      if (pszName && !stricmp(pPos->szName, pszName))
         {
         pPos->ulX = ulX;
         pPos->ulY = ulY;
         }
      }
}


/*************************************************************************
*  Get Long filename
*************************************************************************/
BOOL GetEAValue(PSZ pszFile, PSZ pszKeyName, PBYTE *pTarget, PUSHORT pusLength)
{
APIRET    rc;
EAOP2     eaop2;
PGEA2LIST pGeaList;
PFEA2LIST pFeaList;
ULONG     ulGeaSize, ulFeaSize;
PBYTE     pbData;
ULONG     ulDataSize;
FILESTATUS4 fStat;
USHORT    usEAType;
USHORT    usMultiCnt;
USHORT    usIndex;
PBYTE     p;


   /*
      Create GEA2LIST
   */
   ulGeaSize = sizeof (GEA2LIST) + strlen(pszKeyName) + 1;
   pGeaList = (PGEA2LIST)malloc(ulGeaSize);
   if (!pGeaList)
      {
      printf("Not enough memory for GeaList!");
      exit(1);
      }

   memset(pGeaList, 0, ulGeaSize);
   pGeaList->cbList = ulGeaSize - sizeof (ULONG);
   pGeaList->list->oNextEntryOffset = 0L;
   strcpy(pGeaList->list->szName, pszKeyName);
   pGeaList->list->cbName = strlen(pGeaList->list->szName);


   /*
      Query size of FEA2LIST
   */
   rc = DosQueryPathInfo(pszFile,
      FIL_QUERYEASIZE,
      (PBYTE) &fStat,
      sizeof fStat);
   if (rc)
      {
      BYTE szMessage[80];
      sprintf(szMessage, "IconEA, Get %s size for %s, Error : %d",
         pszKeyName, pszFile, rc);
      if (rc != ERROR_PATH_NOT_FOUND &&
          rc != ERROR_FILE_NOT_FOUND &&
          rc != ERROR_SHARING_VIOLATION &&
          rc != ERROR_ACCESS_DENIED)
         printf(szMessage);
      free(pGeaList);
      return FALSE;
      }

   /*
      And create FEA2LIST
   */
   ulFeaSize = fStat.cbList * 2 + sizeof (ULONG);
   pFeaList = (PFEA2LIST)malloc(ulFeaSize);
   if (!pFeaList)
      {
      printf("Not enough memory for FeaList!");
      free(pGeaList);
      return FALSE;
      }
   memset(pFeaList, 0, ulFeaSize);
   pFeaList->cbList = ulFeaSize - sizeof (ULONG);

   memset(&eaop2, 0, sizeof eaop2);
   eaop2.fpGEA2List = pGeaList;
   eaop2.fpFEA2List = pFeaList;

   rc = DosQueryPathInfo(pszFile,
      FIL_QUERYEASFROMLIST,
      (PBYTE) &eaop2,
      sizeof eaop2);

   if (rc)
      {
      BYTE szMessage[150];
      sprintf(szMessage, "IconEA, Get %s for %s, Error : %d",
         pszKeyName, pszFile, rc);

      if (rc != ERROR_SHARING_VIOLATION && rc != ERROR_ACCESS_DENIED &&
          rc != 35289)
         printf(szMessage);
      free(pGeaList);
      free(pFeaList);
      return FALSE;
      }
   if (!pFeaList->list->cbValue)
      {
      free(pGeaList);
      free(pFeaList);
      return FALSE;
      }

   pbData = pFeaList->list->szName + pFeaList->list->cbName + 1;
   usEAType = *(PUSHORT)pbData;
   switch(usEAType)
      {
      /* All length preceding items */

      case EA_LPBINARY  :
      case EA_LPASCII   :
      case EA_LPBITMAP  :
      case EA_LPMETAFILE:
      case EA_LPICON    :
         pbData += sizeof (USHORT);
         ulDataSize = *(PUSHORT)pbData;
         pbData += sizeof (USHORT);
         if (!ulDataSize)
            {
            free(pGeaList);
            free(pFeaList);
            return FALSE;
            }
         break;
      /* All ASCIIZ items */
      case EA_ASCIIZ    :
      case EA_ASCIIZFN  :
      case EA_ASCIIZEA  :
         pbData += 2;
         ulDataSize = strlen(pbData);
         if (!ulDataSize)
            {
            free(pGeaList);
            free(pFeaList);
            return FALSE;
            }
         ulDataSize++;
         break;

      /* Not supported types */
      case EA_MVMT      :
         {
         pbData += 4;
         usMultiCnt = *(PUSHORT)pbData;
         pbData += 2;

         p = pbData;
         ulDataSize = 0;
         for (usIndex = 0; usIndex < usMultiCnt; usIndex++)
            {
            USHORT usEASize;

            switch (*(PUSHORT)pbData)
               {
               /* All length preceding items */

               case EA_LPBINARY  :
               case EA_LPASCII   :
               case EA_LPBITMAP  :
               case EA_LPMETAFILE:
               case EA_LPICON    :
                  p          += sizeof (USHORT);
                  usEASize    = *(PUSHORT)p;
                  ulDataSize += usEASize + 1;
                  p          += sizeof (USHORT);
                  p          += usEASize; 
                  break;

               /* All ASCIIZ items */

               case EA_ASCIIZ    :
               case EA_ASCIIZFN  :
               case EA_ASCIIZEA  :
                  p          += 2;
                  usEASize    = strlen(p);
                  ulDataSize += usEASize + 1;
                  p          += sizeof (USHORT);
                  p          += usEASize; 
                  break;

               /* Unsupported types */

               case EA_MVMT      :
               case EA_MVST      :
               case EA_ASN1      :
                  free(pGeaList);
                  free(pFeaList);
                  return FALSE;
               }
            }
         break;
         }
      case EA_MVST      :
      case EA_ASN1      :
         free(pGeaList);
         free(pFeaList);
         return FALSE;
      }

   if (ulDataSize > 20000)
      {
      free(pGeaList);
      free(pFeaList);
      return FALSE;
      }

   *pTarget = malloc(ulDataSize);
   if (!(*pTarget))
      {
      printf("Not enough memory for EA resultbuffer !");
      free(pGeaList);
      free(pFeaList);
      return FALSE;
      }
   memset(*pTarget, 0, ulDataSize);
   *pusLength = (USHORT)ulDataSize;

   if (usEAType != EA_MVMT)
      memcpy(*pTarget, pbData, ulDataSize);
   else
      {
      p = *pTarget;
      for (usIndex = 0; usIndex < usMultiCnt; usIndex++)
         {
         USHORT usEASize;

         switch (*(PUSHORT)pbData)
            {
            /* All length preceding items */

            case EA_LPBINARY  :
            case EA_LPASCII   :
            case EA_LPBITMAP  :
            case EA_LPMETAFILE:
            case EA_LPICON    :
               pbData     += sizeof (USHORT);
               usEASize    = *(PUSHORT)pbData;
               pbData     += sizeof (USHORT);
               memcpy(p, pbData, usEASize);
               pbData     += usEASize;
               p          += usEASize + 1;
               break;

            /* All ASCIIZ items */

            case EA_ASCIIZ    :
            case EA_ASCIIZFN  :
            case EA_ASCIIZEA  :
               pbData     += 2;
               usEASize    = strlen(pbData);
               pbData     += sizeof (USHORT);
               memcpy(p, pbData, usEASize);
               pbData     += usEASize;
               p          += usEASize + 1;
               break;
            }
         }
      }
   free(pGeaList);
   free(pFeaList);
   return TRUE;
}
/***********************************************************************
* SetLongName
***********************************************************************/
BOOL SetEAValue(PSZ pszFile, PSZ pszKey, USHORT usType, PBYTE pchValue, USHORT cbValue)
{
APIRET    rc;
EAOP2     eaop2;
PBYTE     p;
PFEA2LIST pFeaList;
ULONG     ulFeaSize;


   ulFeaSize = sizeof (FEA2LIST) + strlen(pszKey) + 5L + cbValue + 10;

   pFeaList = (PFEA2LIST)malloc(ulFeaSize);
   if (!pFeaList)
      {
      printf("Not enough memory !\n");
      return FALSE;
      }

   memset(pFeaList, 0, ulFeaSize);
   pFeaList->list->cbName = strlen(pszKey);
   p = pFeaList->list->szName;
   strcpy(p, pszKey);
   p += strlen(p) + 1;

   *(USHORT *)p = usType;
   p+= sizeof (USHORT);
   switch(usType)
      {
      /* All length preceding items */

      case EA_LPBINARY  :
      case EA_LPASCII   :
      case EA_LPBITMAP  :
      case EA_LPMETAFILE:
      case EA_LPICON    :
         if (!cbValue)
            pFeaList->list->cbValue = 0;
         else
            pFeaList->list->cbValue = cbValue + 2 * sizeof (USHORT);

         *(USHORT *)p = cbValue;
         p+= sizeof (USHORT);
         memcpy(p, pchValue, cbValue);
         p += cbValue;
         break;
      /* All ASCIIZ items */
      case EA_ASCIIZ    :
      case EA_ASCIIZFN  :
      case EA_ASCIIZEA  :
         if (!cbValue)
            pFeaList->list->cbValue = 0;
         else
            pFeaList->list->cbValue = cbValue + sizeof (USHORT);
         strncpy(p, pchValue, cbValue);
         p += cbValue;
         break;

      /* Not supported types */
      case EA_MVMT      :
      case EA_MVST      :
      case EA_ASN1      :
         free(pFeaList);
         return FALSE;
      }
   pFeaList->cbList = (p - (PBYTE)pFeaList) - sizeof (ULONG);

   // align four-word boundary

   if (pFeaList->cbList % sizeof (ULONG))
      pFeaList->cbList += sizeof (ULONG) - (pFeaList->cbList % sizeof (ULONG));

   memset(&eaop2, 0, sizeof eaop2);
   eaop2.fpFEA2List = pFeaList;

   rc = DosSetPathInfo(pszFile,
      FIL_QUERYEASIZE,
      (PBYTE) &eaop2,
      sizeof eaop2,
      DSPI_WRTTHRU);

   free(pFeaList);
   switch (rc)
      {
      case NO_ERROR :
         return TRUE;
      case ERROR_SHARING_VIOLATION:
         printf("Sharing violation while writing EA\n");
         return FALSE;
      default        :
         printf("Error while writing EA!\n");
         return FALSE;
      }
   return TRUE;
}

