#define INCL_DOSERRORS
#define INCL_SPL
#define INCL_SPLDOSPRINT
#define INCL_SPLERRORS

#include <os2.h>
#include <stdio.h>

INT main (INT argc, CHAR * argv[])
{
ULONG  splerr  ;
ULONG  cbBuf;
ULONG  cbNeeded ;
ULONG  ulLevel ;
ULONG  i ;
USHORT uJobCount ;
PSZ    pszComputerName ;
PSZ    pszQueueName ;
PVOID  pBuf;
PPRJINFO2 prj2 ;
PPRQINFO3 prq3 ;

    if (argc != 2)
    {
       printf("Syntax:  setqryq  QueueName \n");
       DosExit( EXIT_PROCESS , 0 ) ;
    }

    pszComputerName = (PSZ)NULL ;
    pszQueueName = argv[1];
    ulLevel = 4L;
    splerr = SplQueryQueue(pszComputerName, pszQueueName, ulLevel,
                                (PVOID)NULL, 0L, &cbNeeded );
    if (splerr != NERR_BufTooSmall && splerr != ERROR_MORE_DATA )
    {
       printf("SplQueryQueue Error=%ld, cbNeeded=%ld\n",splerr, cbNeeded) ;
       DosExit( EXIT_PROCESS , 0 ) ;
    }
    if (!DosAllocMem( &pBuf, cbNeeded,
                      PAG_READ|PAG_WRITE|PAG_COMMIT) )
    {
       cbBuf = cbNeeded ;
       splerr = SplQueryQueue(pszComputerName, pszQueueName, ulLevel,
                              pBuf, cbBuf, &cbNeeded) ;
       prq3=(PPRQINFO3)pBuf;
       printf("Queue info: name- %s\n", prq3->pszName) ;
       if ( prq3->fsType & PRQ3_TYPE_APPDEFAULT )
          printf("  This is the application default print queue\n");
       printf("  priority - %d  starttime - %d  endtime - %d fsType - %X\n",
              prq3->uPriority ,  prq3->uStartTime , prq3->uUntilTime ,
              prq3->fsType ) ;
       printf("  pszSepFile    - %s\n", prq3->pszSepFile) ;
       printf("  pszPrProc     - %s\n", prq3->pszPrProc) ;
       printf("  pszParms      - %s\n", prq3->pszParms) ;
       printf("  pszComment    - %s\n", prq3->pszComment) ;
       printf("  pszPrinters   - %s\n", prq3->pszPrinters) ;
       printf("  pszDriverName - %s\n", prq3->pszDriverName) ;
       if (prq3->pDriverData)
       {
         printf("  pDriverData->cb           - %ld\n",
                                (ULONG)prq3->pDriverData->cb);
         printf("  pDriverData->lVersion     - %ld\n",
                                (ULONG)prq3->pDriverData->lVersion) ;
         printf("  pDriverData->szDeviceName - %s\n",
                                prq3->pDriverData->szDeviceName) ;
       }
       /* Store the job count for use later in the for loop.                  */
       uJobCount = prq3->cJobs;
       printf("Job count in this queue is %d\n\n",uJobCount);

       /* Increment the pointer to the PRQINFO3 structure so that it points to*/
       /* the first structure after itself.                                   */
       prq3++;

       /* Cast the prq3 pointer to point to a PRJINFO2 structure, and set a   */
       /* pointer to point to that place.                                     */
       prj2=(PPRJINFO2)prq3;
       for (i=0 ; i<uJobCount ;i++ ) {
          printf("Job ID    = %d\n",  prj2->uJobId);
          printf("Priority  = %d\n",  prj2->uPriority);
          printf("User Name = %s\n",  prj2->pszUserName);
          printf("Position  = %d\n",  prj2->uPosition);
          printf("Status    = %d\n",  prj2->fsStatus);
          printf("Submitted = %ld\n", prj2->ulSubmitted);
          printf("Size      = %ld\n", prj2->ulSize);
          printf("Comment   = %s\n",  prj2->pszComment);
          printf("Document  = %s\n\n",prj2->pszDocument);

          /* Increment the pointer to point to the next structure.            */
          prj2++;
       } /* endfor */
       DosFreeMem(pBuf) ;
    }
    DosExit( EXIT_PROCESS , 0 ) ;
    return (splerr);
}
 

