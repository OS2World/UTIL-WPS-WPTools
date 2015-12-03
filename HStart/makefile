#
# MAKEFILE for HSTART.EXE
# MicroSoft C 6.0a.
#

!ifdef DEBUG
CV=/CO
!endif

LINKOPT=/NOI /NOE /PM:VIO $(CV)
MOD=L

.c.obj:
     cl /Zi /Lp /Gs /Osip /A$(MOD) /Zpi /W4 /c $*.c >%|dpfF.err
     type $*.err      

.obj.exe:
     bind $* /n DOSSTARTSESSION /n DOSCREATEQUEUE /n DOSREADQUEUE /n DOSCLOSEQUEUE
     $(MSCDIR)\bin\link $*,,,/NOD:$(MOD)LIBCE $(MOD)LIBCEP os2 $(LINKOPT);>>%|dpfF.err


all: hstart.exe hwait.exe semfuncs.lib semfuncs.dll


HSTART.OBJ   : HSTART.C
HWAIT.OBJ    : HWAIT.C

HSTART.EXE   : HSTART.OBJ
     $(MSCDIR)\bin\link $*,,,/NOD:$(MOD)LIBCE $(MOD)LIBCEP os2 $(LINKOPT) ;>>%|dpfF.err
     bind $* /n @<<
DOSSTARTSESSION
DOSCREATEQUEUE
DOSREADQUEUE
DOSCLOSEQUEUE
<<NOKEEP
     copy hstart.exe e:\os2\mdos   

HWAIT.EXE : HWAIT.OBJ
     $(MSCDIR)\bin\link $*,,,/NOD:$(MOD)LIBCE $(MOD)LIBCEP OS2+SEMFUNCS, $(LINKOPT) /INFO;>>%|dpfF.err
     bind $* /n @<<
DOS16CREATEEVENTSEM
DOS16CLOSEEVENTSEM
DOS16OPENEVENTSEM
DOS16POSTEVENTSEM
DOS16WAITEVENTSEM
<<NOKEEP



SEMFUNCS.LIB   : SEMFUNCS.DLL
     implib semfuncs.lib semfuncs.def

SEMFUNCS.DLL   : SEMFUNCS.OBJ
     link386 semfuncs,semfuncs.dll,nul,,semfuncs.def;
                                 
SEMFUNCS.OBJ : SEMFUNCS.C
     ICC.EXE /I$(INC) /Sm /Sp1 /Ss /Gm /Ge- /Gd- /Ti /Kb+ /C %s >%|dpfF.err
     type %|dpfF.err
