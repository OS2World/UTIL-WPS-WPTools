#include <stdlib.h>
#include <malloc.h>
#define INCL_WINSHELLDATA
#include <os2.h>

#include "wptools.h"
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

   if (*pulProfileSize == 0)
      return NULL;

   pBuffer = malloc(*pulProfileSize);
   if (!pBuffer)
      {
      MessageBox("GetAllProfileNames", "Not enough memory for profile data! (need %u bytes)", *pulProfileSize);
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

   if (!PrfQueryProfileSize(hini,
      pszApp,
      pszKey,
      pulProfileSize))
      {
      return NULL;
      }

   if (*pulProfileSize == 0)
      return NULL;

   pBuffer = malloc(*pulProfileSize);
   if (!pBuffer)
      {
      MessageBox("GetProfileData", "Not enough memory for profile data!, need %u bytes", *pulProfileSize);
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

