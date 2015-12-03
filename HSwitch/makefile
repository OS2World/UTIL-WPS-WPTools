PFXLIB=C:\MSC\PFX\LIB\PFXMT32.LIB
OS2LIB=E:\toolkit\lib\os2386

all : dswitch.exe hswitch.exe 

dswitch.exe: dswitch.obj
   link dswitch,,dswitch,c:\msc\pfx\lib\spfxlib /CP:1 /STACK:300;

dswitch.OBJ: dswitch.c hswitch.h
   c6
   cl /FPa /AS /W4 /G2s /Os /c dswitch.c >dswitch.err
   type dswitch.err

hswitch.exe: HSWITCH.obj dswitch.exe $(PFXLIB)
!ifdef DEBUG
   link386 /PM:VIO /STACK:10000 HSWITCH.obj, hswitch.exe,HSWITCH.map,$(OS2LIB) $(PFXLIB), HSWITCH.def /CO ;
!else
   link386 /PM:VIO /STACK:10000 HSWITCH.obj, hswitch.exe,HSWITCH.map,$(OS2LIB) $(PFXLIB), HSWITCH.def;
!endif

HSWITCH.obj: HSWITCH.c hswitch.h
   ibmc
!ifdef DEBUG
   ICC.EXE  /Sp1 /Sm /Ss /Gm+ /Ti /Gf /Kb+ /G3 /C /IC:\MSC\PFX\INCLUDE;E:\PFX .\$*.c >$*.err
!else
   ICC.EXE  /Sp1 /Sm /Ss /Gm+ /O+ /Gs /Gf /Kb+ /G3 /C /IC:\MSC\PFX\INCLUDE .\$*.c >$*.err
!endif
   type $*.err

