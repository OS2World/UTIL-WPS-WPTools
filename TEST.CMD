/*REXX*/
call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'  /*PM_Dialog_Enhancer_Colours*/
call SysLoadFuncs
RC=SYSCREATEOBJECT("WPSchemePalette","Dialog Enhancer Palette","<WP_DESKTOP>","SCHEMES=Test:PM_Colors,Test:PM_Colors;XCELLCOUNT=1;YCELLCOUNT=1","REPLACE")
say rc


