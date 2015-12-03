/* Rexx */
'start'
call RxFuncAdd 'WPToolsLoadFuncs', 'WPTOOLS', 'WPToolsLoadFuncs' 
rc = WPToolsLoadFuncs() 

rc = WPToolsUnloadByTrap()

