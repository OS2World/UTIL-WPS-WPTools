#define INCL_DOS
#include <os2.h>

#define EXPENTRY16 _Far16 _Pascal

/**********************************************************************
*
**********************************************************************/
USHORT EXPENTRY16 Dos16CreateEventSem(BYTE * _Seg16 pszName,
   ULONG * _Seg16 pulHandle,
   ULONG ulAttr,
   BOOL16  fSet)
   
{
HEV hev;
APIRET rc;
PSZ psz = pszName;

   hev = *pulHandle;
   rc = DosCreateEventSem(psz, &hev, ulAttr, fSet);
   *pulHandle = hev;
   return (USHORT)rc;
}

/**********************************************************************
*
**********************************************************************/
USHORT EXPENTRY16 Dos16CloseEventSem(ULONG ulHandle)
{
   return (USHORT)DosCloseEventSem(ulHandle);
}

/**********************************************************************
*
**********************************************************************/
USHORT EXPENTRY16 Dos16OpenEventSem(BYTE * _Seg16 pszName, ULONG * _Seg16 pulHandle)
{
HEV hev;
APIRET rc;
PSZ psz = pszName;

   hev = *pulHandle;
   rc = DosOpenEventSem(psz, &hev);
   *pulHandle = hev;
   return (USHORT)rc;
}

/**********************************************************************
*
**********************************************************************/
USHORT EXPENTRY16 Dos16WaitEventSem(ULONG ulHandle, ULONG ulTimeOut)
{
   return (USHORT)DosWaitEventSem(ulHandle, ulTimeOut);
}

/**********************************************************************
*
**********************************************************************/
USHORT EXPENTRY16 Dos16PostEventSem(ULONG ulHandle)
{
   return (USHORT)DosPostEventSem(ulHandle);
}
