//#define DEBUG
/***************************************************************************
* WPSRestore
*
***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <ctype.h>

#define INCL_PM
#define INCL_DOS
#define INCL_WPERRORS
#include <os2.h>

#include "portable.h"
#include "wptools\wptools.h"

/***************************************************************************
* Defines & Typedefs
***************************************************************************/

#define COMMENT_CHAR '*'
#define OBJECT       "OBJECT"
#define SPACE        0x20

/***************************************************************************
* Global variables
***************************************************************************/
static BYTE pBuffer[10000];
static PSZ pszExtraOpts[]=
   {
   ";SET ",
   ";ASSOCFILTER=",
   ";ASSOCTYPE=",
   ";MAXIMIZED=",
   ";MINIMIZED=",
   NULL,
   };
static ULONG ulTotalLines = 1;
static ULONG ulCurrentLine;
static BOOL  fRecreate = FALSE;
static BOOL  fYes = FALSE;

/***************************************************************************
* Function parameters
***************************************************************************/
PRIVATE BOOL CreateObject (PSZ pszLine);
PRIVATE VOID GetExtraOpts (PSZ pszExtra, PSZ pszSettings);
PRIVATE PSZ  MyGets       (PBYTE pBuffer, USHORT usBufSize, FILE *fh);
PRIVATE PSZ  GetAWord     (PSZ pszLine, PSZ pszTarget);
PRIVATE BOOL ParseLine(PSZ pszLine, PSZ pszClass, PSZ pszTitle, PSZ pszLocation, PSZ pszSettings);
PRIVATE PSZ  pszWPError(VOID);

/***************************************************************
* The main function
***************************************************************/
int main(INT iArgc, PSZ pszArg[])
{
FILE *fInput;
BYTE  chChar;
PSZ   pszFname="WPSBKP.DAT";
INT iArg;
BYTE szOnlyObject[100];

   printf("WPSREST Version 1.40 (%s) - By Henk Kelder\n", __DATE__);
   printf("USAGE: WPSREST [filename] [options]\n");
   printf("\n");
   if (!Is21()) /* GA version */
      {
      printf(" PLEASE MAKE SURE YOU HAVE NO OPEN FOLDERS OR PROGRAMS ON YOUR DESKTOP!\n");
      printf("\n");
      printf("IF YOU DO HAVE OPEN FOLDERS, YOU RUN THE RISK THAT THIS PROGRAM WILL\n");
      printf("                   CAUSE YOU DESKTOP TO HANG !\n");
      printf("\n");
      }


   memset(szOnlyObject, 0, sizeof szOnlyObject);
   for (iArg = 1; iArg < iArgc; iArg++)
      {
      strupr(pszArg[iArg]);
      if (pszArg[iArg][0] == '/' || pszArg[iArg][0] == '-')
         {
         switch (pszArg[iArg][1])
            {
            case 'R':
               fRecreate = TRUE;
               break;
            case 'O':
               sprintf(szOnlyObject, "OBJECTID=%s", pszArg[iArg]+3);
               break;
            case 'Y':
               fYes = TRUE;
               break;
            default :
               printf("Unknown option %s specified\n\n", pszArg[iArg]);
               printf("Valid options are:\n");
               printf("/R      - Recreate existing objects instead of updating.\n");
               printf("/Oxxxxx - Only recreate object with OBJECTID=xxxxx\n");
               printf("/Y      - Auto anwer questions about recreating objects with Y\n");
               exit(1);

            }
         }
      else
         pszFname = pszArg[1];
      }

   printf("Continue ?  [Y/N]:");
   fflush(stdout);
   if (fYes)
      {
      chChar = 'Y';
      printf("Y");
      }
   else
      chChar = getche();
   if (chChar != 'y' && chChar != 'Y')
      exit(1);
   printf("\n");

   fInput = fopen(pszFname, "r");
   if (!fInput)
      {
      printf("Unable to open %s\n", pszFname);
      exit(1);
      }
   while (MyGets(pBuffer, sizeof pBuffer, fInput))
      {
      if (!strlen(szOnlyObject) || strstr(pBuffer, szOnlyObject))
         CreateObject(pBuffer);
      }

   return 0;
}

/***************************************************************
* Create an object
***************************************************************/
BOOL CreateObject(PSZ pszLine)
{
static BYTE szClass[50];
static BYTE szTitle[100];
static BYTE szLocation[256];
static BYTE szSettings[4096];
static BYTE szExtra[4096];
PBYTE  p;
BYTE   szObjectID[50];
HOBJECT hObject = 0L;

   memset(szClass, 0, sizeof szClass);
   memset(szTitle, 0, sizeof szTitle);
   memset(szLocation, 0, sizeof szLocation);
   memset(szSettings, 0, sizeof szSettings);
   memset(szExtra, 0, sizeof szExtra);

   if (!ParseLine(pszLine, szClass, szTitle, szLocation, szSettings))
      return FALSE;

   printf("Processing \'%s\', ", szTitle);
#ifdef DEBUG
   printf("\n%s : ", szSettings);
#endif
   if (!Is21())
      GetExtraOpts(szExtra, szSettings);

   /*
      Get OBJECTID
   */
   memset(szObjectID, 0, sizeof szObjectID);
   p = strstr(szSettings, "OBJECTID=");
   if (p)
      {
      strncpy(szObjectID, p + strlen("OBJECTID="), sizeof szObjectID - 1);
      p = strchr(szObjectID, ';');
      if (p)
         *p = 0;
      hObject = WinQueryObject(szObjectID);
      if (!hObject)
         {
         DosSleep(300);
         hObject = WinQueryObject(szObjectID);
         }
      if (hObject)
         printf("Object %s already exist !", szObjectID);
      else
         printf("Object %s does not exist!", szObjectID);
      }
   printf("\n");

   /*
      Create object if it does not exist, or it is a launchpad
      or the recreate switch is specified.
   */
   if (!hObject ||
      !stricmp(szClass, "WPLaunchPad") ||
      fRecreate)
      {
      BYTE chChar;
      printf("Do you want to (re)create this object? [YN]:\n");
      do
         {
         if (fYes)
            {
            chChar = 'Y';
            printf("Y");
            }
         else
            chChar = getch();
         if (chChar == 'y')
            chChar = 'Y';
         else if (chChar == 'n')
            chChar = 'N';
         }
      while (chChar != 'Y' && chChar != 'N');
      printf("\n");
      if (chChar == 'N')
         return TRUE;


      if (!WinQueryObject(szLocation))
         {
         printf("Location %s not found, creating object on the Desktop\n");
         strcpy(szLocation, "<WP_DESKTOP>");
         }

      if (!fRecreate &&
           strcmp(szClass, "WPLaunchPad"))
         hObject = WinCreateObject(szClass,
            szTitle,
            szSettings,
            szLocation,
            CO_FAILIFEXISTS);
      else if (strcmp(szClass, "SmartCenter"))
         hObject = WinCreateObject(szClass,
            szTitle,
            szSettings,
            szLocation,
            CO_REPLACEIFEXISTS);
      else
         hObject = WinCreateObject(szClass,
            szTitle,
            szSettings,
            szLocation,
            CO_UPDATEIFEXISTS);


      if (!hObject)
         {
         printf("Error while creating object %s!\n", szTitle);
         printf("%s\n", pszWPError());
         printf("Hit any key...\n");
         getch();
         return FALSE;
         }
      }
   else
      {
      if (!stricmp(szClass, "WPShadow"))
         return TRUE;
#ifndef DEBUG
      if (!WinSetObjectData(hObject, szSettings))
         {
         printf("Error while setting options.\n");
         printf("%s\nHit any key..\n", pszWPError());
         getch();
         return FALSE;
         }
#endif
      }

   /*
      Special for DOS settings, a | means a linefeed
   */
   while ((p = strchr(szExtra, '|')) != NULL)
      *p = '\n';

   if (*szExtra && !WinSetObjectData(hObject, szExtra))
      {
      printf("Error while setting extra options\n");
      printf("%s\nHit any key..\n", pszWPError());
      getch();
      }
   return TRUE;
}

/***************************************************************
* Get Extra options 
***************************************************************/
VOID GetExtraOpts(PSZ pszExtra, PSZ pszSettings)
{
PBYTE pStart, pEnd, pTarget;
PSZ *pszOpts;
USHORT usSize;

   pTarget = pszExtra;
   for (pszOpts = pszExtraOpts; *pszOpts; pszOpts++)
      {
      while ((pStart = strstr(pszSettings, *pszOpts)) != NULL)
         {
         pStart++;
         pEnd = strchr(pStart, ';');
         if (!pEnd)
            pEnd = pStart + strlen(pStart);
         else
            pEnd++;
         usSize = pEnd - pStart;
         memcpy(pTarget, pStart, usSize);
         pTarget += usSize;
         strcpy(pStart, pEnd);
         }
      }
}

/**********************************************************************
*  Get A line from the input file
**********************************************************************/
PSZ MyGets(PBYTE pBuffer, USHORT usBufSize, FILE *fh)
{
PBYTE pCur, p;
PBYTE pBufEnd = pBuffer + usBufSize;
BOOL  fFirstLine;
ULONG ulOffset;

   memset(pBuffer, 0, usBufSize);

   ulCurrentLine = ulTotalLines;

   pCur = pBuffer;
   fFirstLine = TRUE;
   ulOffset = ftell(fh);
   while (fgets(pCur, pBufEnd - pCur, fh))
      {
      ulTotalLines++;
      if (*pCur != COMMENT_CHAR && *pCur != '\n')
         {
         /*
            If it is the first line and it does not start with 'OBJECT'
            that it must be of the old format, so return
         */
         if (fFirstLine && memicmp(pCur, OBJECT, strlen(OBJECT)))
            return pBuffer;

         if (!memicmp(pCur, OBJECT, strlen(OBJECT)))
            {
            if (fFirstLine)
               memmove(pCur, pCur+strlen(OBJECT), strlen(pCur+strlen(OBJECT))+1);
            else
               break;
            }

         /*
            Remove leading spaces
         */
         while (isspace(*pCur))
            memmove(pCur, pCur+1, strlen(pCur+1) + 1);

         /*
            Remove trailing line feed and white space
         */
         p = strchr(pCur, '\n');
         if (!p)
            p = pCur + strlen(pCur);
         else
            *p = 0;
         while (p > pCur && isspace(*(p - 1)))
            *p-- = 0;

         pCur += strlen(pCur);
         fFirstLine = FALSE;
         }
      ulOffset = ftell(fh);
      }

   fseek(fh, ulOffset, SEEK_SET);
   if (*pBuffer)
      return pBuffer;
   else
      return NULL;
}

/***************************************************************************************
*  Parse A line
***************************************************************************************/
BOOL ParseLine(PSZ pszLine, PSZ pszClass, PSZ pszTitle, PSZ pszLocation, PSZ pszSettings)
{
PSZ pStart, pEnd;

   pszLine = GetAWord(pszLine, pszClass);
   if (!pszLine)
      return FALSE;

   pszLine = GetAWord(pszLine, pszTitle);
   if (!pszLine)
      return FALSE;

   pszLine = GetAWord(pszLine, pszLocation);
   if (!pszLine)
      return FALSE;

   pStart = strchr(pszLine, '\"');
   if (!pStart)
      {
      printf("\aERROR: Expected token \" not found in OBJECT starting \nat line %ld\n", ulCurrentLine);
      printf("Hit any key...\n");
      getch();
      return FALSE;
      }
   pStart++;
   pEnd = strrchr(pStart, '\"');
   if (!pEnd)
      {
      printf("\aERROR: Expected token \" not found in OBJECT starting \nat line %ld\n", ulCurrentLine);
      printf("Hit any key...\n");
      getch();
      return FALSE;
      }

   memcpy(pszSettings, pStart, pEnd - pStart);

//   pszLine = GetAWord(pszLine, pszSettings);
//   if (!pszLine)
//      return FALSE;
   return TRUE;
}


/***************************************************************************************
*  Get A word
***************************************************************************************/
PSZ GetAWord(PSZ pszLine, PSZ pszTarget)
{
PBYTE pStart;
PBYTE p;

   pStart = strchr(pszLine, '\"');
   if (!pStart)
      {
      printf("\aERROR: Expected token \" not found in OBJECT starting \nat line %ld\n", ulCurrentLine);
      printf("Hit any key...\n");
      getch();
      return NULL;
      }
   pStart++;
   pszLine = pStart;

   for (;;)
      {
      p = strchr(pszLine, '\"');
      if (!p)
         {
         printf("\aERROR: Expected token \" not found in OBJECT starting \nat line %ld\n", ulCurrentLine);
         printf("Hit any key...\n");
         getch();
         return NULL;
         }
      if (*(p - 1) == '^')
         {
         pszLine = p+1;
         continue;
         }

      break;
      }
   pszLine = p+1;
   memcpy(pszTarget, pStart, p - pStart);
   return pszLine;
}

/*********************************************************************
*  Debug Box
*********************************************************************/
PSZ pszWPError(VOID)
{
static BYTE szMessage[1024];
PERRINFO pErrInfo;
BYTE   szHErrNo[6];
USHORT usError;

   usError = ERRORIDERROR(WinGetLastError(0));
   memset(szMessage, 0, sizeof szMessage);
   sprintf(szMessage, "Error 0x%4.4X : ", usError);

   switch (usError)
      {
      case WPERR_PROTECTED_CLASS        :
         strcat(szMessage, "Objects class is protected");
         break;
      case WPERR_NOT_WORKPLACE_CLASS    :
      case WPERR_INVALID_CLASS          :
         strcat(szMessage, "Objects class is invalid");
         break;
      case WPERR_INVALID_SUPERCLASS     :
         strcat(szMessage, "Objects superclass is invalid");
         break;
      case WPERR_INVALID_OBJECT         :
         strcat(szMessage, "The object appears to be invalid");
         break;
      case WPERR_INI_FILE_WRITE         :
         strcat(szMessage, "An error occured when writing to the ini-files");
         break;
      case WPERR_INVALID_FOLDER         :
         strcat(szMessage, "The specified folder (location) is not valid");
         break;
      case WPERR_OBJECT_NOT_FOUND       :
         strcat(szMessage, "The object was not found");
         break;

      case WPERR_ALREADY_EXISTS         :
         strcat(szMessage,
            "The workplace shell claims that the object already exist.\nUse CHECKINI to check for possible causes!");
         break;
      case WPERR_INVALID_FLAGS          :
         strcat(szMessage, "Invalid flags were specified");
         break;
      case WPERR_INVALID_OBJECTID       :
         strcat(szMessage, "The OBJECTID is invalid");
         break;
      case PMERR_INVALID_PARAMETER:
         strcat(szMessage, "A parameter is invalid (invalid class name?)");
         break;
      default :
         memset(szMessage, 0, sizeof szMessage);
         pErrInfo = WinGetErrorInfo(0);
         if (pErrInfo)
            {
            PUSHORT pus = (PUSHORT)((PBYTE)pErrInfo + pErrInfo->offaoffszMsg);
            PSZ     psz = (PBYTE)pErrInfo + *pus;
            PSZ     p;

            strcat(szMessage, psz);
            p = psz + strlen(psz) + 1;
            strcat(szMessage, ", ");
            strcat(szMessage, p);

            itoa(ERRORIDERROR(pErrInfo->idError), szHErrNo, 16);
            strcat(szMessage, "\n");
            strcat(szMessage, "Error No = ");
            strcat(szMessage, "0x");
            strcat(szMessage, szHErrNo);
            WinFreeErrorInfo(pErrInfo);
            }
         else
            sprintf(szMessage, "PM returned error %d (0x%4.4X)\n",
               usError, usError);
            break;
         }



   return szMessage;
}


