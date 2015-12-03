DEBUG=YES
# IBM Developer's Workframe/2 Make File Creation run at 22:59:10 on 05/10/92

.SUFFIXES: .c

!ifndef WF
ERR=>$*.err
!endif

!ifdef DEBUG
CV=/CO 
!endif
HEKLIB= ..\WPTOOLS\WPTOOLS.LIB
HEKINC= ..\WPTOOLS
PFXLIB= $(MSCDIR)\pfx\lib\pfxmt32.lib


.c.obj:
# /Sm - Migration level
# /Ss - Allow // comment
# /Gm - Single thread
# /Gf - Fast floating point
# /Ti - Debugging information
# /Kb+ - Warning on general diagnostics
   ICC.EXE  /Sm /Ss /Gm /Gf /Ti /Kb+ /G3 /C /I$(HEKINC) .\$*.c >$*.err
#   ICC.EXE  /Sm /Ss /Gm+ /Gf /Ti /Kb+ /G3 /C .\$*.c >$*.err
!ifndef WR
   type $*.err
!endif


LINKOPT=/NOI $(CV) /NOE /PM:PM /STACK:20000 /ALIGN:4 /BASE:0x10000 /MAP

ALL: ASSOEDIT.EXE
# ASSOEDIT.HLP

ASSOEDIT.EXE::  ASSOEDIT.OBJ $(HEKLIB)
   ilink /NOFREE $(LINKOPT) ASSOEDIT.OBJ,ASSOEDIT,ASSOEDIT,$(HEKLIB) $(PFXLIB),;
   rc ASSOEDIT.res ASSOEDIT.exe

ASSOEDIT.EXE:: ASSOEDIT.RES
   rc ASSOEDIT.res ASSOEDIT.exe

ASSOEDIT.res        : ASSOEDIT.rc ASSOEDIT.h ASSOEDIT.dlg
   rc -r $(*B).rc >>ASSOEDIT.err
   type ASSOEDIT.err

ASSOEDIT.OBJ    :  ASSOEDIT.C       ASSOEDIT.H 

#ASSOEDIT.HLP    :  ASSOEDIT.IPF
#        ipfc assoedit.ipf
