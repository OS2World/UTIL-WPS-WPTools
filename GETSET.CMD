/* This is a REXX program 

A sample REXX file to query settings for objects using the functions
in WPTOOLS.DLL.
*/


Parse arg tCmdLine

call RxFuncAdd 'SysLoadFuncs', 'RexxUtil', 'SysLoadFuncs'
rc = SysLoadFuncs()

call RxFuncAdd 'WPToolsLoadFuncs', 'WPTOOLS', 'WPToolsLoadFuncs' 
rc = WPToolsLoadFuncs()

szObjectName = ""
szParms = ""
do i = 1 to words(tCmdLine)
  tWord = word(tCmdLine, i)
  select
    when TRANSLATE(tWord) = '/S' | TRANSLATE(tWord) = '-S' then do
      szParms = '/S'
      iterate
    end
    otherwise do
      if szObjectName = "" then 
        szObjectName = tWord
      else
        szObjectName = szObjectName || ' ' || tWord
      iterate
      end
   end
end

if szObjectName = "" then do
   say 'USAGE: GETSET OBJECTID|PATHNAME [/S]'
   say ''
   say 'Use OBJECTID without < and >'
   say '/S - Also look in subdirs'
   exit
end

call SysFileTree szObjectName, 'rgFiles', 'O'
if rgFiles.0 > 0 then do
  do i = 1 to rgFiles.0
      call GetSettings rgFiles.i, szParms
  end
  if szParms = '/S' Then Do
    szObjectName = szObjectName || '\*.*'
    call SysFileTree szObjectName, 'rgFiles', 'OS'
    if rgFiles.0 > 0 then do
      do i = 1 to rgFiles.0
        call GetSettings rgFiles.i, szParms
      end
    end
  end
  exit
end


szObjectName = '<' || szObjectName || '>'
call GetSettings szObjectName, szParms

Exit

/***********************************************************************
* GetSettings
***********************************************************************/
GetSettings:
iRetco = WPToolsQueryObject(ARG(1), "szClass", "szTitle", "szSetupString", "szLocation") 
if iRetco Then do
     Call WriteObject szClass, szTitle, szSetupString, szLocation
  end 
else
  say 'Unable to return object settings for ' ARG(1)

if ARG(2) = '/S' Then Do

iRetco = WPToolsFolderContent(ARG(1), "list.")
if iRetco = 0 Then Do
   Say 'WPToolsFolderContent for 'ARG(1)' returned 0'
   return
End

do iObject = 1 to list.0
  iRetco=WPToolsQueryObject(list.iObject, "szClass", "szTitle", "szSetupString", "szLocation") 
  if iRetco Then do
     Call WriteObject szClass, szTitle, szSetupString, szLocation
  end 
end
End
return

/***********************************************************************
* WriteObject
***********************************************************************/
WriteObject:
Parse Arg szClass, szTitle, szSetupString, szLocation

/* To find Windows progs with SET's.
   Something you should not want, since then the defaults
   from the WIN-OS/2 setup are NOT used.

   if POS('PROG_31_', szSetupString) = 0 Then Return
   if POS('SET ', szSetupString) = 0 Then Return
*/

   Say 'OBJECT "'szClass'" "'szTitle'" "'szLocation'"'

   bQuote = '"'
   iStart = 1
   Do While Length(szSetupString) > 0
        iEnd = POS(';', szSetupString, iStart) 
        if iEnd = 0 Then Leave
        iQuote = 0 /* POS('"', szSetupString, iStart) */
        if iQuote = 0 | iQuote > iEnd Then Do
           szValue = SubStr(szSetupString, 1, iEnd)
           szSetupString = SubStr(szSetupString, iEnd + 1)
           If Length(szSetupString) = 0 Then
              Say '       'bQuote||szValue'"'
           Else
              Say '       'bQuote||szValue
           iStart = 1
           bQuote = ' '
           End 
        else Do
           iStart = iEnd + 1
           End
   end

   if Length(szSetupString) > 0 Then Do
     Say '     ' szSetupString'"'
   End
   Say ' '
        

Return
