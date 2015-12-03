#ifndef SEMFUNCS_H
#define SEMFUNCS_H


USHORT APIENTRY Dos16CreateEventSem (PSZ pszName,  PULONG ulHandle,
                                     ULONG flAttr, BOOL fState);

USHORT APIENTRY Dos16CloseEventSem  (ULONG ulHandle);

USHORT APIENTRY Dos16OpenEventSem (PSZ pszName, PULONG ulHandle);

USHORT APIENTRY Dos16WaitEventSem (ULONG ulHandle, ULONG ulTimeOut);
USHORT APIENTRY Dos16PostEventSem (ULONG ulHandle);
                                                   
#endif

