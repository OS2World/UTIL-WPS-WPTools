#ifndef HSWITCH_H
#define HSWITCH_H

#define DOSNAME "DOSSWITCH"
#define TITLE   "HSWITCH 1.7"

#pragma pack(1)
typedef struct _DosParms
{
BYTE bScreenHeight;
BYTE bScreenWidth;
BYTE szCurTitle[30];
BYTE szOrigTitle[30];
} DOSPARMS, *PDOSPARMS;
#pragma pack()

#endif

