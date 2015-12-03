#define INCL_DOSERRORS
#define INCL_BASE
#define INCL_DOSMEMMGR
#define INCL_SPL
#define INCL_SPLDOSPRINT
#define INCL_SPLERRORS

#include <os2.h>
#include <stdio.h>

INT main (INT argc, CHAR *argv[])
{
SPLERR splerr  ;
ULONG  cbBuf;
ULONG  cbNeeded ;
ULONG  ulLevel ;
PSZ    pszComputerName ;
PSZ    pszPrintDeviceName ;
PVOID  pBuf ;
PPRDINFO3 pprd3 ;

    if (argc != 2)
    {
       printf("Syntax:  sdqry   DeviceName  \n");
       DosExit( EXIT_PROCESS , 0 ) ;
    }

    pszComputerName = (PSZ)NULL ;
    pszPrintDeviceName = argv[1];
    ulLevel = 3;
    splerr = SplQueryDevice(pszComputerName, pszPrintDeviceName,
                            ulLevel, (PVOID)NULL, 0L, &cbNeeded );
    if (splerr != NERR_BufTooSmall)
    {
       printf("SplQueryDevice Err=%ld, cbNeeded=%ld\n",splerr, cbNeeded) ;
       DosExit( EXIT_PROCESS , 0 ) ;
    }
    if (!DosAllocMem( &pBuf, cbNeeded,
                      PAG_READ|PAG_WRITE|PAG_COMMIT) ){
       cbBuf= cbNeeded ;
       splerr = SplQueryDevice(pszComputerName, pszPrintDeviceName,
                              ulLevel, pBuf, cbBuf, &cbNeeded) ;

       printf("SplQueryDevice Error=%ld, Bytes Needed=%ld\n", splerr,
                              cbNeeded) ;

       pprd3=(PPRDINFO3)pBuf;

       printf("Print Device info: name - %s\n", pprd3->pszPrinterName) ;
       printf("User Name      = %s\n", pprd3->pszUserName) ;
       printf("Logical Address= %s\n", pprd3->pszLogAddr) ;
       printf("Job ID         = %d\n", pprd3->uJobId) ;
       printf("Status         = %d\n", pprd3->fsStatus) ;
       printf("Status Comment = %s\n", pprd3->pszStatus) ;
       printf("Comment        = %s\n", pprd3->pszComment) ;
       printf("Drivers        = %s\n", pprd3->pszDrivers) ;
       printf("Time           = %d\n", pprd3->time) ;
       printf("Time Out       = %d\n", pprd3->usTimeOut) ;
       DosFreeMem(pBuf) ;
    }
    DosExit( EXIT_PROCESS , 0 ) ;
    return (splerr);
}


