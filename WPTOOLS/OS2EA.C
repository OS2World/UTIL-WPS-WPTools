#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#define INCL_DOSERRORS
#define INCL_DOS
#include <os2.h>
#include "wptools.h"

/*************************************************************************
*  Get Long filename
*************************************************************************/
BOOL _System GetEAValue(PSZ pszFile, PSZ pszKeyName, PBYTE *pTarget, PUSHORT pusLength)
{
APIRET    rc;
EAOP2     eaop2;
PGEA2LIST pGeaList;
PFEA2LIST pFeaList;
ULONG     ulGeaSize, ulFeaSize,
          ulFindCount;
PBYTE     pbData;
ULONG     ulDataSize;
FILEFINDBUF4 find;
USHORT    usEAType;
USHORT    usMultiCnt;
USHORT    usIndex;
PBYTE     p;
HDIR      hDir;


   /*
      Query size of FEA2LIST
   */
   hDir        = HDIR_SYSTEM;
   ulFindCount = 1;

   rc = DosFindFirst(pszFile, &hDir,
      FILE_SYSTEM|FILE_HIDDEN|FILE_DIRECTORY|FILE_READONLY|FILE_ARCHIVED,
      &find, sizeof find, &ulFindCount, FIL_QUERYEASIZE);
   if (rc)
      {
      if (rc != ERROR_PATH_NOT_FOUND &&
          rc != ERROR_FILE_NOT_FOUND &&
          rc != ERROR_NO_MORE_FILES &&
          rc != ERROR_SHARING_VIOLATION &&
          rc != ERROR_ACCESS_DENIED &&
          rc != ERROR_FILENAME_EXCED_RANGE)
          MessageBox("GetEAValue", "Get %s size for %s, Error : %d",
            pszKeyName, pszFile, rc);
      return FALSE;
      }
   if (find.cbList <= 4)
      return FALSE;


   /*
      Create GEA2LIST
   */
   ulGeaSize = sizeof (GEA2LIST) + strlen(pszKeyName) + 1;
   pGeaList = (PGEA2LIST)malloc(ulGeaSize);
   if (!pGeaList)
      {
      MessageBox("GetEAValue", "Not enough memory for GeaList!");
      return FALSE;
      }

   memset(pGeaList, 0, ulGeaSize);
   pGeaList->cbList = ulGeaSize - sizeof (ULONG);
   pGeaList->list->oNextEntryOffset = 0L;
   strcpy(pGeaList->list->szName, pszKeyName);
   pGeaList->list->cbName = strlen(pGeaList->list->szName);



   /*
      And create FEA2LIST
   */
   ulFeaSize = find.cbList * 2 +  sizeof (ULONG);
   pFeaList = (PFEA2LIST)malloc(ulFeaSize);
   if (!pFeaList)
      {
      MessageBox("GetEAValue", "Not enough memory for FeaList!");
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

#if 0
   if (rc == ERROR_BUFFER_OVERFLOW || rc == ERROR_EAS_DIDNT_FIT)
      {
      ulFeaSize = pFeaList->cbList * 2 + sizeof(ULONG);
      free(pFeaList);
      pFeaList = (PFEA2LIST)malloc(ulFeaSize);
      if (!pFeaList)
         {
         MessageBox("GetEAValue", "Not enough memory for FeaList(2)!");
         free(pGeaList);
         return FALSE;
         }

      memset(pFeaList, 0, ulFeaSize);
      pFeaList->cbList = ulFeaSize - sizeof (ULONG);
      eaop2.fpFEA2List = pFeaList;
      rc = DosQueryPathInfo(pszFile,
         FIL_QUERYEASFROMLIST,
         (PBYTE) &eaop2,
         sizeof eaop2);
      }
#endif

   if (rc)
      {
      free(pGeaList);
      free(pFeaList);
      if (rc != ERROR_SHARING_VIOLATION && rc != ERROR_ACCESS_DENIED &&
          rc != 35289)
         MessageBox("GetEAValue", "Get %s for %s, Error : %d",
         pszKeyName, pszFile, rc);
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
      free(pGeaList);
      free(pFeaList);
      MessageBox("GetEAValue", "Not enough memory for resultbuffer!");
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
BOOL _System SetEAValue(PSZ pszFile, PSZ pszKey, USHORT usType, PBYTE pchValue, USHORT cbValue)
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
      MessageBox("SetEAValue", "Not enough memory for FeaList!");
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
            pFeaList->list->cbValue = cbValue + sizeof (USHORT) + 1;
         strncpy(p, pchValue, cbValue);
         p += cbValue;
         break;

      /* Not supported types */
      case EA_MVMT      :
      case EA_MVST      :
      case EA_ASN1      :
         MessageBox("SetEAValue", "EA types not supported!");
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
         MessageBox("SetEAValue", "Sharing violation while writing %s for %s!",
           pszKey, pszFile );
         return FALSE;
      default        :
         MessageBox("SetEAValue", "Error %d while writing %s for %s!",
           rc, pszKey, pszFile );
         return FALSE;
      }
   return TRUE;
}

