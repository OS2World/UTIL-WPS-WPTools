#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <conio.h>
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>


VOID BreakHandler(INT iSignal);

INT main(VOID)
{
APIRET rc;
USHORT usForEver = 1;
SHORT sCount;
HFILE hFile;
ULONG ulAction;

   rc = DosOpen("DOS$",
      &hFile,
      &ulAction,
      0L,
      FILE_NORMAL,
      FILE_OPEN,
      OPEN_ACCESS_READWRITE | OPEN_SHARE_DENYNONE,
      0L);

   if (rc)
      {
      printf("DosOpen returned %d", rc);
      exit(1);
      }


   signal(SIGINT, BreakHandler);
   signal(SIGTERM, BreakHandler);

   printf("SHUTDOWN version 1.0 - Henk Kelder\n");
   printf("Use CTRL-BREAK to abort.\n");


   printf("COUNTDOWN:  ");
   fflush(stdout);
   for (sCount = 5; sCount >= 0; sCount--)
      {
      printf("\b\b%2d", sCount);
      DosBeep(1000, 10);
      fflush(stdout);
      DosSleep(1000);
      }
   printf("\nShutting down filesystem....");
   fflush(stdout);
   rc = DosShutdown(0L);
   if (rc)
      {
      printf("\nDosShutdown: SYS4.4u\n", rc);
      exit(1);
      }

   printf("done!\n");
   return 1;
}


VOID BreakHandler(INT iSignal)
{
   printf("\nShutdown cancelled!");
   exit(1);

}
