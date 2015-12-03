:userdoc.
:title.Help for the association editor
.*--------------------------------------------------------------*\
.*  Help panel for the main wildow                              *
.*--------------------------------------------------------------*/
:h1 res=4110 name=PANEL_MAIN.About Association Editor
:i1 global id=AboutMain.Associations Editor
:p.:hp2.Association editor by Henk Kelder:ehp2.
:p.The association editor (:hp1.ASSOEDIT.EXE:ehp1.) is a tool that
allows you to inspect and, optionally change associations set for
file types and file filters.
:p.The Workplace shell uses these associations to determine the action it should
take when a user clicks on a file object that is show in a folder.
:p.Associations can be made based on file :hp1.types:ehp1. and on file 
:hp1.filters:ehp1..
:p.:hp2.File Types:ehp2.
:p.A file type is a short descriptive text. Some existing types are 
'Plain text', 'C code' or 'Executable'.
:p.:hp2.File Filters:ehp2.
:p.A file filter is a mask that can be used for selecting files. In this mask
the so-called wildcards(* and ?) can be used. 
An example of a filter is '*.TXT' meaning all files with extention .txt. You
could also specify a filename (e.g. 'MAKEFILE').
:p.For more help select the Help button in the various dialogs, or Press F1 
for specific help for buttons, lists, entryfields or other dialog elements.

:p.Related information&colon.
.br
.br
:link reftype=hd res=4120.o The associations dialog:elink.
.br
:link reftype=hd res=4140.o Add new type or filter:elink.
.br
:link reftype=hd res=4130.o Add association dialog:elink.
.br
:link reftype=hd res=4150.o Note about unknown objects:elink.



.*--------------------------------------------------------------*\
.*  Help menu item Help for help... help panel                  *
.*--------------------------------------------------------------*/
:h1 res=4111 name=PANEL_GENERALHELP.General Help
:i2 refid=Help.Help for Help
:p.Use this choice to get general help for the program. General help
provides general information about the Association editor.

.*--------------------------------------------------------------*\
.*  Help menu item Extended help... help panel                  *
.*--------------------------------------------------------------*/
:h1 res=4112 name=PANEL_USINGHELP.Extended
:i2 refid=Help.Using Help
:p.Use this choice to obtain information on how to
use the Help facility.

.*--------------------------------------------------------------*\
.*  Help menu item Help index... help panel                     *
.*--------------------------------------------------------------*/
:h1 res=4113 name=PANEL_HELPINDEX.Index
:i2 refid=Help.Help Index
:p.Use this choice to display the help index.
:p.The help index lists the titles of the help information that is available.

.*--------------------------------------------------------------*\
.*  Help menu item Keys help... help panel                      *
.*--------------------------------------------------------------*/
:h1 res=4114 name=PANEL_TUTORIAL.Tutorial of Keys
:i2 refid=Help.Keys Help
:p.Use this choice to see a list of keys and a description of the
function of the keys.

.*--------------------------------------------------------------*\
.*  Help menu item Product... help panel                        *
.*--------------------------------------------------------------*/
:h1 res=4115 name=PANEL_HELPPRODUCTINFO.Information
:i2 refid=Help.PRODUCT Information
:p.This dialog displays information about the application, such as version
number and the author.

.*--------------------------------------------------------------*\
.*  Keys help panel                                             *
.*      res = PANEL_KEYSHELP                                    *
.*--------------------------------------------------------------*/
:h2 res=4119 name=PANEL_KEYSHELP.Key Assigments
:p.The list of keys is arranged in groups.
:p.:hp2.ACCELERATOR KEYS:ehp2.
:dl tsize=2 break=all.
:p.:hp2.HELP KEYS:ehp2.
:dl tsize=5 break=all.
:dt.F1
:dd.Get help
:dt.Esc
:dd.Previous Help Panel, or End help if only one panel
:dt.Alt
:dd.Switch IN/OUT action bar
:dt.Alt+F4
:dd.End help or End program
:edl.
:p.:hp2.SYSTEM KEYS:ehp2.
:dl tsize=5 break=all.
:dt.Alt+Esc
:dd.Switch to the next program, including full-screen programs
:dt.Ctrl+Esc
:dd.Switch to the Task List
:dt.Alt+k
:dd.Call key help
:dt.F3
:dd.Exit immediately
:edl.
:p.:hp2.WINDOW KEYS:ehp2.
:dl tsize=5 break=all.
:dt.Underlined letter
:dd.Move among the choices on the action bar and pull-downs submenu or
execut the function chosen
:edl.
:edl.

.*--------------------------------------------------------------*\
.*  Help panel for the ASSOCIATIONS panel                       *
.*--------------------------------------------------------------*/
:h1 res=4120 name=PANEL_ASSOC.Associations dialog
:i1 id=assocDlg.Associations Dialog
:p.:hp2.Associations Dialog:ehp2.
:p.The associations dialog shows all types or filters in the lefthand listbox.
When a type or filter is selected the righthand listbox shows all associated
objects or programs. The first item in the righthand listbox (if any) is the
:hp2.Default:ehp2. associated item. You can change this default using
the 'Default' button.
:p.You can add an association by :hp2.dragging:ehp2. a single program object 
from the workplace shell and drop it on the righthand listbox
:p.You can add or delete file types or filters. You can also add or delete 
associations with programs or objects. Also you can change the default
associated program or object for a type or filter.
:p.The :hp2.Delete:ehp2. button below the lefthand listbox will become active once an item 
is selected in the lefthand listbox.
:p.The :hp2.Delete:ehp2. button below the righthand listbox will become active once an item 
is selected in the righthand listbox.
:p.The :hp2.Make Default:ehp2. button below the righthand listbox will become active once an item
other than the first in this list is selected.
:p.For more help press F1 for help about the current selected dialog item.

:p.Related information&colon.
.br
.br
:link reftype=hd res=4110.o About association editor:elink.
.br
:link reftype=hd res=4140.o Add new type or filter:elink.
.br
:link reftype=hd res=4130.o Add association dialog:elink.
.br
:link reftype=hd res=4150.o Note about unknown objects:elink.


.*--------------------------------------------------------------*\
.*  Help panel for the listbox of types/filters
.*--------------------------------------------------------------*/
:h1 res=4121 name=PANEL_ASSOCLIST.Types or filters listbox
:p.This listbox shows all types or filters depending of the choice you made
from the menu. You can select a type or filter. A selected type or filter
can be deleted. 

:p.Related information&colon.
.br
.br
:link reftype=hd res=4120.o The associations dialog:elink.

.*--------------------------------------------------------------*\
.*  Help panel for Add button                                   *
.*--------------------------------------------------------------*/
:h1 res=4122.Add type or filter
:p.Use the :hp2.Add:ehp2. button below the lefthand listbox in the associations dialog
to add a new file type or File filter.
:p.Related information&colon.
.br
.br
:link reftype=hd res=4120.o The associations dialog:elink.

.*--------------------------------------------------------------*\
.*  Help for the Delete button
.*--------------------------------------------------------------*/
:h1 res=4123.Delete type or filter
:p.Use the :hp2.Delete:ehp2. button below the lefthand listbox in the association dialog
to remove the type or filter that is selected in this listbox.
:p.Related information&colon.
.br
.br
:link reftype=hd res=4120.o The associations dialog:elink.

.*--------------------------------------------------------------*\
.*  Help panel for the add button
.*--------------------------------------------------------------*/
:h1 res=4124.Add association
:p.Use the :hp2.Add:ehp2. button below the righhand listbox in the association dialog
to add an association with a program or object for the current selected 
file type or filter.
:p.Related information&colon.
.br
.br
:link reftype=hd res=4120.o The associations dialog:elink.

.*--------------------------------------------------------------*\
.*  Help panel for the Delete                                   *
.*--------------------------------------------------------------*/
:h1 res=4125.Delete association
:p.Use the :hp2.Delete:ehp2. button below the righthand listbox in the association dialog
to delete the association that is currently selected in this listbox.
:p.Related information&colon.
.br
.br
:link reftype=hd res=4120.o The associations dialog:elink.

.*--------------------------------------------------------------*\
.*  Help panel for Make Default                                 *
.*--------------------------------------------------------------*/
:h1 res=4126.Set the default association
:p.Use the :hp2.Make Default:ehp2. button below the righthand listbox in the association dialog
to make the current selected program or object in this listbox the default
association for the selected file type or filter in the righthand listbox. 
:p.Related information&colon.
.br
.br
:link reftype=hd res=4120.o The associations dialog:elink.

.*--------------------------------------------------------------*\
.*  Help panel for associations listbox                         *
.*--------------------------------------------------------------*/
:h1 res=4127.Association listbox
:p.This listbox shows all programs or objects currently associated with the
selected file type or filter.
:p.The first item in the list (if any) is the default associated program or 
object for the file type or filter. You can change this default by using
the Default button.
:p.To show items in this list make sure you have selected an element 
from the lefthand list.
:p.You can add a new association by pressing the 'Add' button. Alternatively
you can drop a program object from the desktop into this listbox
:p.Related information&colon.
.br
.br
:link reftype=hd res=4120.o The associations dialog:elink.
.br
:link reftype=hd res=4150.o note about unknown objects:elink.


.*--------------------------------------------------------------*\
.*  Help panel for the ASSOC panel                              *
.*--------------------------------------------------------------*/
:h1 res=4130 name=PANEL_ADDHANDLEDLG.Add association dialog
:i1 id=addHandleDlg.Add associations dialog
:p.:hp2.Add Association Dialog:ehp2.
:p.In the Add Associations Dialog you can add a new association for the file type or filter you
have selected in the Associations dialog.
:p.There are two possibilities for a new association. You could add an 
association with an Workplace shell program :hp1.object:ehp1., but you 
can also add an association with an executable (DOS, WIN-OS2 or OS/2 
:hp1.program:ehp1.).
:p.The listbox shows all program objects present on your desktop, 
but is inactive if the radiobutton 'Object' is not selected.
:p.In the entryfield you can type a programs filename if the radiobutton 'Program' is 
selected.

:note.You can add 20 new associations per type or filter in a session. 
If you encounter the message that the maximum has been reached, close the 
association dialog and retry.
:p.For more help press F1 for help about the current selected dialog item.
:p.Related information&colon.
.br
.br
:link reftype=hd res=4110.o About association editor:elink.
.br
:link reftype=hd res=4140.o Add new type or filter:elink.
.br
:link reftype=hd res=4120.o The associations dialog:elink.

.*--------------------------------------------------------------*\
.*  Help panel for type 'object'                                *
.*--------------------------------------------------------------*/
:h1 res=4131 name=PANEL_TYPEOBJ.Type Object
:p.Select this button if you want to add an association with an existing workplace
shell program object. You can then select an program object from the list 
and press the Ok button.
:p.Related information&colon.
.br
.br
:link reftype=hd res=4130.o Add association dialog:elink.

.*--------------------------------------------------------------*\
.*  Help panel for type 'Program'                               *
.*--------------------------------------------------------------*/
:h1 res=4132 name=PANEL_TYPEPROG.Type Program
:p.Select this button if you want to add an association with an existing program.
You can then specify the programs name in this dialog and press the Ok button.
:p.Related information&colon.
.br
.br
:link reftype=hd res=4130.o Add association dialog:elink.

.*--------------------------------------------------------------*\
.*  Help panel for the listbox of objects                       *
.*--------------------------------------------------------------*/
:h1 res=4133 name=PANEL_OBJLIST.Objects listbox
:p.This listbox shows all objects installed on you desktop. This listbox is
only active when the 'Object' radiobutton is selected.
:p.Related information&colon.
.br
.br
:link reftype=hd res=4130.o Add association dialog:elink.

.*--------------------------------------------------------------*\
.*  Help panel for the new programs name                        *
.*--------------------------------------------------------------*/
:h1 res=4134 name=PANEL_NEWPROG.Associated programs filename
:p.In this field you can specify a program that will be associated to the
file type or file filter that is selected in the underlying dialog. This 
field will only be active if the 'Program' radiobutton is selected.
:p.Related information&colon.
.br
.br
:link reftype=hd res=4130.o Add association dialog:elink.

.*--------------------------------------------------------------*\
.*  Help panel for the ADDASSOCDLG panel                        *
.*--------------------------------------------------------------*/
:h1 res=4140 name=PANEL_ADDASSOCDLG.Add new type or filter dialog
:i1 id=addAssocDlg.Add new type or filter dialog
:p.:hp2.Add New type or Filter:ehp2.
:p.In this dialog you can fill in a new file type or file filter depening
of the selection you have made in the menu.
:p.Fill in a new value and press OK or press cancel.
:p.:hp2.File Types:ehp2.

:p.A file type is a short descriptive text which types a file. Some examples of
existing file types are&colon.
:ul compact.
:li.Plain text
:li.OS/2 Command File
:li.DOS Command File
:li.My own file type
:eul.

:p.:hp2.File Filters:ehp2.
:p.A file filter is a file specification which applies to one or more files.
The filter can contain the wildcard characters :hp2.*:ehp2. and 
:hp2.?:ehp2.. Some examples of filters are&colon.

:ul compact.
:li.*.INF
:li.*.W51
:li.*.ICO
:li.*.XL?
:eul.
:note.You can add 20 new types or filters in a session. 
If you encounter the message that the maximum has been reached, close the 
association dialog and retry.

:p.Related information&colon.
.br
.br
:link reftype=hd res=4110.o About association editor:elink.
.br
:link reftype=hd res=4120.o The associations dialog:elink.
.br
:link reftype=hd res=4130.o Add association dialog:elink.
.br
:link reftype=hd res=4150.o Note about unknown objects:elink.


:h1 res=4150 name=PANEL_SPECIALNOTE.Special note about unknown objects 
:i1 id=specialNote.Special note 'unknown object xxxxx'

:p.:hp2.Special note:ehp2.
:p.
When you add a associations to a program (executable) it is possible 
that the listbox shows :hp1.Unknown object xxxx:ehp1. instead of the 
programs filename. This is due to the fact that the workplace shell caches some
informations and writtes it when it has time. Simple close the associations
dialog and reopen it again and the proper description will appear. If it doesn't
appear it will probably appear later.

:p.
If you have not added an association but still see some unknown objects you
should remove them! 
Unknown objects can be created when one deletes a program file 
that is referenced in an association.

:euserdoc.

