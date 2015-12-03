#ifndef EABACK_H
#define EABACK_H

#define VERSION 1

typedef struct _BackupHeader
{
BYTE  bVersion;
BYTE  szFileName[255];
ULONG ulEASize;
} BHDR, *PBHDR;


#endif

