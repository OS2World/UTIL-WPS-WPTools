#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <conio.h>
#include <ctype.h>
#define INCL_WIN
#define INCL_DOS
#include <os2.h>

BYTE szRegel[256];

BOOL fWriteIni(PSZ pszRegel);


int main(int iArgc, PSZ pszArg[])
{
FILE *fp;

   if (iArgc < 2)
      {
      printf("USAGE : WRINI file");
      exit(1);
      }


   fp = fopen(pszArg[1], "r");
   if (!fp)
      {
      printf("Unable to open %s", pszArg[1]);
      exit(1);
      }

   while (fgets(szRegel, sizeof szRegel, fp))
      {
      fWriteIni(szRegel);
      }
   fclose(fp);

   return 0;
}

BOOL fWriteIni(PSZ pszRegel)
{
ULONG ulProfileSize;
HINI hini = HINI_SYSTEMPROFILE;
BYTE szApp[100];
BYTE szKey[100];
BYTE szData[100];
PSZ  pTar;

   memset(szApp, 0, sizeof szApp);
   memset(szKey, 0, sizeof szKey);
   memset(szData, 0, sizeof szData);

   /*
      Get the application name
   */
   while (isspace(*pszRegel))
      pszRegel++;

   if (*pszRegel != '\"')
      return FALSE;
   pszRegel++;

   pTar = szApp;
   while (*pszRegel && *pszRegel != '\"')
      *pTar++ =*pszRegel++;
   pszRegel++;

   /*
      Get the key name
   */
   while (isspace(*pszRegel))
      pszRegel++;

   if (*pszRegel != '\"')
      return FALSE;
   pszRegel++;

   pTar = szKey;
   while (*pszRegel && *pszRegel != '\"')
      *pTar++ =*pszRegel++;
   pszRegel++;


   /*
      Get the data
   */
   while (isspace(*pszRegel))
      pszRegel++;

   if (*pszRegel != '\"')
      return FALSE;
   pszRegel++;
   pTar = szData;
   while (*pszRegel)
      {
      if (*pszRegel == '\\')
         {
         pszRegel++;
         continue;
         }
      if (*pszRegel == '\"' && *(pszRegel-1) != '\\')
         break;
      *pTar++ =*pszRegel++;
      }

   printf("Writing %s:%s:%s\n", szApp, szKey, szData);

   PrfWriteProfileData(hini, szApp, szKey,
      szData, strlen(szData) + 1);

   return TRUE;
}
