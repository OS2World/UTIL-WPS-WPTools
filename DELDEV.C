#define INCL_SPL
#define INCL_SPLERRORS
#define INCL_DOSERRORS
#include <os2.h>
#include <stdio.h>       /* for printf function */

#define NERR_QInvalidState        (NERR_BASE+63)  /* A spooler memory allocation failure occurred. */


INT main (INT argc, PSZ argv[])
{
SPLERR splerr ;
PSZ    pszComputerName = NULL ;
PSZ    pszDeviceName ;

   /* Get queue name from the input argument  */
   pszDeviceName = argv[1];

   /*
      Call the function to do the delete. If an error is returned, print it.
   */
   splerr=SplDeleteDevice(pszComputerName, pszDeviceName);

   if (splerr != 0L)
   {
      switch (splerr)
      {
         case  NERR_QNotFound :
            printf("Queue does not exist.\n");
            break;
         case  NERR_QInvalidState:
            printf("This operation can't be performed on the print device.\n");
            break;
         default:
            printf("Errorcode = %ld\n",splerr);
      } /* endswitch */
   }
   else
   {
      printf("Device %s was deleted.\n",pszDeviceName);
   } /* endif */
   DosExit( EXIT_PROCESS , 0 ) ;
   return (splerr);
}
   


