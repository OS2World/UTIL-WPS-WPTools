DEBUG=YES

.SUFFIXES:

.SUFFIXES: .c

!ifdef DEBUG
CV=/CO 
!endif

IBMC=E:\IBMCPP
LIB=$(TOOLKIT)\LIB;$(IBMC)\LIB
INCLUDE=$(TOOLKIT)\H;$(IBMC)\INCLUDE

.c.obj:
   ICC.EXE  /Sm /Ss /Gm+ /Gf /Ti /Kb+ /G3 /C .\$*.c >$*.err


LINKOPT=/NOI $(CV) /NOE /PM:PM /STACK:32000 /ALIGN:4 /BASE:0x10000 /MAP

ALL: EABROWSE.EXE

EABROWSE.EXE::  EABROWSE.OBJ EAOPEN.OBJ
   ilink /NOFREE $(LINKOPT) $**,EABROWSE,EABROWSE,..\OS2HEK\OS2HEK.LIB,;
   rc eabrowse.res eabrowse.exe

EABROWSE.EXE:: EABROWSE.RES
   rc eabrowse.res eabrowse.exe >>eabrowse.err

eabrowse.res        : eabrowse.rc eabrowse.h eabrowse.dlg eadlg.h
   rc -r $(*B).rc
   type eabrowse.err


EABROWSE.OBJ    :  EABROWSE.C  EABROWSE.H EADLG.H
EAOPEN.OBJ      :  EAOPEN.C    EABROWSE.H EADLG.H
