#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <direct.h>

#define INCL_VIO
#define INCL_WIN
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_GPI
#include <os2.h>

#include "wptools.h"


VOID vGetDefaults(PSZ pszOptions);


BOOL _System GetWPUrlOptions(HOBJECT hObject, PSZ pszOptions, PBYTE pObjectData)
{
BYTE szOptionText[256];
ULONG ulValue, ulSetOptions;

   vGetFSysTitle(hObject, pszOptions);

   if (GetObjectValue(pObjectData, 1, szOptionText, sizeof szOptionText))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "URL=%s;", szOptionText);
      }
   if (GetObjectValue(pObjectData, 22, szOptionText, sizeof szOptionText))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "BROWSER=%s;", szOptionText);
      }

   if (GetObjectValue(pObjectData, 54, szOptionText, sizeof szOptionText))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "PARAMETERS=%s;", szOptionText);
      }

   if (GetObjectValue(pObjectData, 53, szOptionText, sizeof szOptionText))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "WORKINGDIR=%s;", szOptionText);
      }


   if (GetObjectValue(pObjectData, 14, szOptionText, sizeof szOptionText))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "EMAILADDRESS=%s;", szOptionText);
      }

   if (GetObjectValue(pObjectData, 15, szOptionText, sizeof szOptionText))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "NEWSSERVER=%s;", szOptionText);
      }


   ulValue = ulSetOptions = 0;
   GetObjectValue(pObjectData, 25, &ulValue, sizeof ulValue);
   GetObjectValue(pObjectData, 26, &ulSetOptions, sizeof ulSetOptions);

   if (ulSetOptions & 0x00000020)
      if (ulValue & 0x00000020)
         strcat(pszOptions, "INTEGRATEDBROWSER=YES;");
      else
         strcat(pszOptions, "INTEGRATEDBROWSER=NO;");

   if (ulSetOptions & 0x00000004)
      if (ulValue & 0x00000004)
         strcat(pszOptions, "PALETTEAWARE=YES;");
      else
         strcat(pszOptions, "PALETTEAWARE=NO;");

   if (ulSetOptions & 0x00000010)
      if (ulValue & 0x00000010)
         strcat(pszOptions, "LOADGRAPHICS=YES;");
      else
         strcat(pszOptions, "LOADGRAPHICS=NO;");

   if (ulSetOptions & 0x00000008)
      if (ulValue & 0x00000008)
         strcat(pszOptions, "DISPLAYIMAGES=YES;");
      else
         strcat(pszOptions, "DISPLAYIMAGES=NO;");

   if (ulSetOptions & 0x00000040)
      if (ulValue & 0x00000040)
         strcat(pszOptions, "PRESENTATIONMODE=YES;");
      else
         strcat(pszOptions, "PRESENTATIONMODE=NO;");

   if (ulSetOptions & 0x00000002)
      if (ulValue & 0x00000002)
         strcat(pszOptions, "ENABLEPROXY=YES;");
      else
         strcat(pszOptions, "ENABLEPROXY=NO;");

   if (GetObjectValue(pObjectData, 16, szOptionText, sizeof szOptionText))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "PROXYGATEWAY=%s;", szOptionText);
      }

   if (ulSetOptions & 0x00000001)
      if (ulValue & 0x00000001)
         strcat(pszOptions, "ENABLESOCKS=YES;");
      else
         strcat(pszOptions, "ENABLESOCKS=NO;");

   if (GetObjectValue(pObjectData, 17, szOptionText, sizeof szOptionText))
      {
      sprintf(pszOptions + strlen(pszOptions),
         "SOCKSSERVER=%s;", szOptionText);
      }









   vGetDefaults(pszOptions);
   return TRUE;
}

/********************************************************************
*
********************************************************************/
VOID vGetDefaults(PSZ pszOptions)
{
ULONG ulProfileSize;
PSZ  pszString;

   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "DefaultBrowserExe", hIniUser, &ulProfileSize);
   if (pszString)
      {
      if (strlen(pszString))
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTBROWSER=%s;", pszString);
      free(pszString);
      }

   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "DefaultImagesWhileLoading", hIniUser, &ulProfileSize);
   if (pszString)
      {
      if (*pszString == '1')
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTDISPLAYIMAGES=%s;", "YES");
      else
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTDISPLAYIMAGES=%s;", "NO");
      free(pszString);
      }

   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "DefaultEmailAddress", hIniUser, &ulProfileSize);
   if (pszString)
      {
      if (strlen(pszString))
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTEMAILADDRESS=%s;", pszString);
      free(pszString);
      }

   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "ProxyEnabled", hIniUser, &ulProfileSize);
   if (pszString)
      {
      if (*pszString == '1')
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTENABLEPROXY=%s;", "YES");
      else
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTENABLEPROXY=%s;", "NO");
      free(pszString);
      }

   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "SocksEnabled", hIniUser, &ulProfileSize);
   if (pszString)
      {
      if (*pszString == '1')
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTENABLESOCKS=%s;", "YES");
      else
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTENABLESOCKS=%s;", "NO");
      free(pszString);
      }


   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "UnderstandingBrowser", hIniUser, &ulProfileSize);
   if (pszString)
      {
      if (*pszString == '1')
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTINTEGRATEDBROWSER=%s;", "YES");
      else
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTINTEGRATEDBROWSER=%s;", "NO");
      free(pszString);
      }

   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "EnableGraphics", hIniUser, &ulProfileSize);
   if (pszString)
      {
      if (*pszString == '1')
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTLOADGRAPHICS=%s;", "YES");
      else
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTLOADGRAPHICS=%s;", "NO");
      free(pszString);
      }

   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "DefaultNewsServer", hIniUser, &ulProfileSize);
   if (pszString)
      {
      sprintf(pszOptions + strlen(pszOptions),
         "DEFAULTNEWSSERVER=%s;", pszString);
      free(pszString);
      }

   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "PaletteAware", hIniUser, &ulProfileSize);
   if (pszString)
      {
      if (*pszString == '1')
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTPALETTEAWARE=%s;", "YES");
      else
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTPALETTEAWARE=%s;", "NO");
      free(pszString);
      }

   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "DefaultParameters", hIniUser, &ulProfileSize);
   if (pszString)
      {
      if (strlen(pszString))
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTPARAMETERS=%s;", pszString);
      free(pszString);
      }

   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "PresentationMode", hIniUser, &ulProfileSize);
   if (pszString)
      {
      if (*pszString == '1')
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTPRESENTATIONMODE=%s;", "YES");
      else
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTPRESENTATIONMODE=%s;", "NO");
      free(pszString);
      }

   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "DefaultProxyGateway", hIniUser, &ulProfileSize);
   if (pszString)
      {
      if (strlen(pszString))
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTPROXYGATEWAY=%s;", pszString);
      free(pszString);
      }

   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "DefaultSocksServer", hIniUser, &ulProfileSize);
   if (pszString)
      {
      if (strlen(pszString))
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTSOCKSSERVER=%s;", pszString);
      free(pszString);
      }

   pszString = GetProfileData("WPURLDEFAULTSETTINGS", "DefaultWorkingDir", hIniUser, &ulProfileSize);
   if (pszString)
      {
      if (strlen(pszString))
         sprintf(pszOptions + strlen(pszOptions),
            "DEFAULTWORKINGDIR=%s;", pszString);
      free(pszString);
      }
   return;
}
