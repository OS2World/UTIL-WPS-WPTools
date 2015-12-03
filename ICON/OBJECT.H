#define TAG_LONG        2
#define TAG_TEXT        3
#define TAG_BLOCK       4

PBYTE  GetAllProfileNames  (PSZ pszApp, HINI hini, PULONG pulProfileSize);
PVOID  GetProfileData      (PSZ pszApp, PSZ pszKey, HINI hini, PULONG puProfileSize);
BOOL   GetObjectName       (HOBJECT hObject, PSZ pszFname, USHORT usMax);
BOOL   GetAssocFilter      (PSZ pszFileName, PULONG pulAssoc, USHORT usMax);


