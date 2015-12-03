/* REXX */
call RxFuncAdd 'WPToolsLoadFuncs', 'WPTOOLS', 'WPToolsLoadFuncs' 
rc = WPToolsLoadFuncs() 

Parse arg tCmdLine
if tCmdLine = "" Then Exit

iRetco = WPToolsFolderContent(tCmdLine, "list.", "F")
if iRetco = 0 Then Do
   Say 'WPToolsFolderContent for 'ARG(1)' returned 0'
   exit
End
if list.0 > 0 then do
   do i = 1 to list.0

     iRetco = WPToolsQueryObject(list.i, "szClass", "szTitle") 
     if iRetco = 1 Then    
        say list.i szClass szTitle
     else
        say list.i
   end
end

