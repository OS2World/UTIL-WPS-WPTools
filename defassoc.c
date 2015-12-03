#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INCL_PM
#include <os2.h>

#include "wptools.h"

PSZ GetObjectName(HOBJECT hObject);

BYTE szData[2048];

int main(int iArgc, PSZ pszArgv[])
{
HOBJECT hObject;
PSZ     pszApplication;
PSZ     pszAssocs;
HOBJECT rgObj[25];
USHORT  usCount;
USHORT  usIndex;
ULONG   ulProfileSize;
PSZ     p;

   printf("DEFASSOC version 1.0 for OS/2 32 bits.\n");
   printf("A program to inspect associations or set the default association.\n");
   printf("Made by Henk Kelder.\n");
   if (iArgc < 2)
      {
      printf("USAGE: DEFASSOC assoctype [filter|type [associated_program [/R]]]\n");
      printf(" assoctype must be F(ilter) or T(ype)\n");
      printf(" use /R to remove an association\n");
      printf("\n");
      printf("Example:\n");
      printf("DEFASSOC F *.HTM E:\\NETSCAPE\\NETSCAPE.EXE\n");
      printf("\n");
      printf("NOTE 1: If an argument contains blanks or is like <xxx>\n");
      printf("        embed it with double quotes.\n");
      printf("NOTE 2: filters or type are CASE SENSITIVE!\n");
      exit(1);
      }
   strupr(pszArgv[1]);
   if (pszArgv[1][0] != 'F' && pszArgv[1][0] != 'T')
      {
      printf("ERROR: assoctype should be F or T\n");
      exit(1);
      }
   if (pszArgv[1][0] == 'T')
      pszApplication = "PMWP_ASSOC_TYPE";
   else
      pszApplication = "PMWP_ASSOC_FILTER";

   if (iArgc < 3)
      {
      PSZ pszFilters;
      ULONG ulSize;

      printf("The following types/filters are defined:\n");
      pszFilters = GetAllProfileNames(pszApplication, HINI_USER, &ulSize);
      if (pszFilters)
         {
         p = pszFilters;
         while (*p && p < pszFilters + ulSize)
            {
            usCount = 0;

            printf("  '%s' is associated with:\n", p);
            pszAssocs = GetProfileData(pszApplication, p, HINI_USER, &ulProfileSize);
            if (pszAssocs)
               {
               PSZ p1 = pszAssocs;
               while (p1 < pszAssocs + ulProfileSize)
                  {
                  hObject = atol(p1);
                  if (!hObject)
                     break;
                  printf("      %s%s\n", GetObjectName(hObject), (usCount ? "" : " (default)"));
                  p1 += strlen(p1) + 1;
                  usCount++;
                  }
               free(pszAssocs);
               }
            if (!usCount)
               printf("      nothing.\n");
            p += strlen(p) + 1;
            }
         free(pszFilters);
         return 0;
         }
      }

   usCount = 0;
   pszAssocs = GetProfileData(pszApplication,
      pszArgv[2],
      HINI_USER,
      &ulProfileSize);

   if (pszAssocs)
      {
      p = pszAssocs;
      while (p < pszAssocs + ulProfileSize)
         {
         if (!atol(p))
            break;
         rgObj[usCount++] = atol(p);
         p += strlen(p) + 1;
         }
      free(pszAssocs);
      }

   if (iArgc < 4)
      {
      printf("'%s' is associated with : \n", pszArgv[2]);
      for (usIndex = 0; usIndex < usCount; usIndex++)
         printf("  %s%s\n", GetObjectName(rgObj[usIndex]), (usIndex ? "": " (default)"));
      if (!usCount)
         printf("  nothing.\n");
      return 0;
      }


   hObject = WinQueryObject(pszArgv[3]);
   if (!hObject)
      {
      printf("ERROR: %s not found\n", pszArgv[3]);
      exit(1);
      }


   for (usIndex = 0; usIndex < usCount; usIndex++)
      {
      if (rgObj[usIndex] == hObject)
         break;
      }

   if (iArgc > 4)
      {
      if (stricmp(pszArgv[4], "/R"))
         {
         printf("Invalid argument %s found!\n", pszArgv[4]);
         exit(1);
         }
      if (usIndex == usCount)
         {
         printf("%s is not associated with %s\n",
            pszArgv[3], pszArgv[2]);
         exit(1);
         }
      printf("Removing %s as an associated program for %s\n",
         pszArgv[3], pszArgv[2]);
      while (usIndex < usCount - 1)
         {
         rgObj[usIndex] = rgObj[usIndex+1];
         usIndex++;
         }
      usCount--;
      }
   else
      {
      if (usIndex == usCount)
         {
         printf("Adding default assocation for %s with %s\n",
            pszArgv[2], pszArgv[3]);
         for (usIndex = usCount; usIndex >0; usIndex--)
            rgObj[usIndex] = rgObj[usIndex-1];
         rgObj[0] = hObject;
         usCount++;
         }
      else
         {
         printf("Makeing %s the default assocation for %s\n",
            pszArgv[3], pszArgv[2]);
         while (usIndex > 0)
            {
            rgObj[usIndex] = rgObj[usIndex-1];
            usIndex--;
            }
         rgObj[0] = hObject;
         }
      }

   memset(szData, 0, sizeof szData);
   for (p = szData, usIndex = 0; usIndex < usCount; usIndex++)
      {
      sprintf(p, "%lu", rgObj[usIndex]);
      p += strlen(p) + 1;
      }
   if (!usCount)
      p++;

   if (!PrfWriteProfileData(HINI_USER,
      pszApplication,
      pszArgv[2],
      szData,
      (ULONG)(p - szData)))
      {
      printf("ERROR: PrfWriteProfileData failed!\n");
      exit(1);
      }
   return 0;
}

PSZ GetObjectName(HOBJECT hObject)
{
static BYTE szAssoc[CCHMAXPATH];
ULONG ulProfileSize;

   if (!WinQueryObjectPath(hObject, szAssoc, sizeof szAssoc))
      {
      if (IsObjectAbstract(hObject))
         {
         PBYTE pClassInfo;
         pClassInfo = GetClassInfo(HINI_USER, hObject, &ulProfileSize);
         if (pClassInfo)
            {
            if (!GetObjectID(pClassInfo, szAssoc, sizeof szAssoc))
               sprintf(szAssoc, "Object %lX", hObject);
            free(pClassInfo);
            }
         else
            sprintf(szAssoc, "Object %lX", hObject);
         }
      else if (IsObjectDisk(hObject))
         {
         BYTE szPath[CCHMAXPATH];
         if (PathFromObject(HINI_SYSTEM, hObject, szPath, sizeof szPath, NULL))
            sprintf(szAssoc,"%s (might be invalid)", szPath);
         else
            sprintf(szAssoc, "Object %lX (might be invalid)", hObject);
         }
      else
         sprintf(szAssoc, "Object %lX (might be invalid)", hObject);
      }

   return szAssoc;
}
