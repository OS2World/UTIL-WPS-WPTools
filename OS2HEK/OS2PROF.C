#include <stdlib.h>
#include <malloc.h>
#define INCL_WINSHELLDATA
#include <os2.h>

#include "os2hek.h"
/*************************************************************
* Get all names
*************************************************************/
PVOID _System GetAllProfileNames(PSZ pszApp, HINI hini, PULONG pulProfileSize)
{
PBYTE pBuffer;

   if (!PrfQueryProfileSize(hini,
      pszApp,
      NULL,
      pulProfileSize))
      return NULL;

   if (*pulProfileSize == NULL)
      return NULL;

   pBuffer = malloc(*pulProfileSize);
   if (!pBuffer)
      {
      MessageBox("GetAllProfileName", "Not enough memory for profile data!");
      return NULL;
      }

   if (!PrfQueryProfileData(hini,
      pszApp,
      NULL,
      pBuffer,
      pulProfileSize))
      {
      MessageBox("GetAllProfileNames", "Error while retrieving profile data!");
      free(pBuffer);
      return NULL;
      }

   return pBuffer;
}

/*************************************************************
* Get the data from the profile
*************************************************************/
PVOID _System GetProfileData(PSZ pszApp, PSZ pszKey, HINI hini, PULONG pulProfileSize)
{
PBYTE pBuffer;

   *pulProfileSize = 0;

   if (!PrfQueryProfileSize(hini,
      pszApp,
      pszKey,
      pulProfileSize))
      {
      return NULL;
      }
   if (*pulProfileSize == 0)
      return "";

   pBuffer = malloc(*pulProfileSize);
   if (!pBuffer)
      {
      MessageBox("GetProfileData", "Not enough memory for profile data!");
      return NULL;
      }

   if (!PrfQueryProfileData(hini,
      pszApp,
      pszKey,
      pBuffer,
      pulProfileSize))
      {
      MessageBox("GetProfileData", "Error while retrieving profile data!");
      free(pBuffer);
      return NULL;
      }
   return pBuffer;
}

