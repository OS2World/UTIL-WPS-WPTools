:userdoc.
:title.Help for ICONTOOL

.*--------------------------------------------------------------*\
.*  Help panel for the main wildow                              *
.*--------------------------------------------------------------*/
:h1 res=4110 name=PANEL_MAIN.About ICONTOOL
:i1 id=AboutMain.About ICONTOOL
:p.:hp2.ICONTOOL by Henk Kelder:ehp2.

:p.ICONTOOL allows you to do the following things&colon.
:ul compact.
:li.Change an objects icon by dropping an icon on it.
:li.Change the name shown on the workplace shell without changing the name on disk.
:li.Copy and move non-abstract files and directories.
:li.Delete objects.
:li.Extract icons from objects.
:li.Open objects and change objects settings.
:li.Allows to modify some extended attributes for files
:li.Allows you to query the setupstring for objects
:eul.
:p.
:p.See the following for more information&colon.
.br
.br
:link reftype=hd res=5000.Disclaimer:elink.
.br
:link reftype=hd res=5001.General information:elink.
.br
:link reftype=hd res=5003.Changing directories and drives:elink.
.br
:link reftype=hd res=5004.Manipulating files:elink.
.br
:link reftype=hd res=5005.Changing object titles:elink.
.br
:link reftype=hd res=5006.Extracting icons:elink.
.br
:link reftype=hd res=5007.Setting the default icon:elink.
.br
:link reftype=hd res=5008.Abstract objects:elink.
.br
:link reftype=hd res=5009.Known problems in ICONTOOL:elink.
.br
:link reftype=hd res=5011.ICONTOOL.TXT:elink.

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
:h2 res=4119 name=PANEL_KEYSHELP.Keys Help
:p.ICONTOOLs conforms itself to the general usage of keys for folders. 
If you need information about these keys, select an icon on your desktop
and select :hp1.Keys Help:ehp1. from the menu.


.*--------------------------------------------------------------*/
:h1 res=5000 name=PANEL_DISCLAIMER.Disclaimer
:i1 id=Disclaimer.Disclaimer
:p.I allow you to use and distribute ICON.EXE freely under the condition 
that I am in no way responsible for any damage or loss you may suffer.
:p.Henk Kelder


.*--------------------------------------------------------------*/
:h1 res=5001 name=PANEL_GENERAL.General information
:i1 id=GeneralInfo.General Information
:p.ICON.EXE was written in 'plain old' C using the C/2 Set compiler and 
OS/2 2.0 toolkit. No 'SOM' programming was used. All information about
abstract objects is gotten out (hacked) off the ini-files. Clearly the
latter is fully UNDOCUMENTED ! (but it seems to work reasonally well.)

:p.ICON.EXE is a PM program for OS/2 2.0 that I initially wrote because I did
not like the complex way to set an icon for a workplace shell object and 
other objects (read: files and directories) on my harddisk and of attached
network drives. 

:p.Furthermore, since the workplace shell claims to support drag-drop 
operations I wanted to experiment with this. Why shouldn't I be able to drag
an iconfile and drop it on something else so that the icon would be assigned
to this something else.

:p.Initialy I tried to write it so it would show icons only and one could then
drag the icon on an object on the desktop. Unfortunately the desktop itself
doesn't seem to follow the specifications for drag-drop operations
completely. The solutions for this was that ICONTOOL allows you to open two
windows and you can do draging and dropping between these two windows.


.*--------------------------------------------------------------*/
:h1 res=5003 name=PANEL_DIRECTORY.Changing directories and drives
:i1 id=ChangingDirs.Changing directories and drives
:p.When ICONTOOL is configured to show drives and/or directories you can simply
change to these drives and/or directories by selecting 'open' or by double 
clicking on them. ICONTOOL will no open a new folder but will replace the
contents of the open window.


.*--------------------------------------------------------------*/
:h1 res=5004 name=PANEL_FILEOPTS.Manipulating files or directories
:i1 id=Manipulating.Manipulating Files or directories
:p.Although not it main purpose, you could also use this program to do the
following operations on most objects&colon.
:p.:hp2.Copy files:ehp2.
:p.You can copy a file or directory the by dragging it to another location while you keep
the control-button pressed. This can be the second window ICONTOOL 
allows you to open, but it can also be a workplace shell folder.
.br
You cannot copy abstract objects with ICONTOOL!
:p.:hp2.Move files:ehp2.
:p.You can move a file or directory by dragging it to another location without 
pressing any keys. This can be the second window ICONTOOL 
allows you to open, but it can also be a workplace shell folder.
.br
You cannot move abstract objects with ICONTOOL!
:p.:hp2.Make shadows of files or directories:ehp2.
:p.You can make a shadow of a file by dragging it to another location while
pressing the control and shift key. 
This can :hp2.not:ehp2. be the second window ICONTOOL allows you to open, 
but it can be a workplace shell folder.
.br
You cannot make shadows of abstract objects with ICONTOOL!
:note.The here mentioned key combinations can be different on your desktop if 
you have changed the defaults.
:p.Related information&colon.
.br
.br
:link reftype=hd res=5008.Abstract objects:elink.

.*--------------------------------------------------------------*/
:h1 res=5005 name=PANEL_CHANGENAME.Changing object titles
:i1 id=ChangeName.Changing object titles
:p.When the program allowed me to do this I found that I would also like to
change the titles of objects WITHOUT changing the underlying fysical
filename of an object. I found the the workplace shell uses the .LONGNAME
extended attribute, if present, as a title. It was no big deal to show 
these .LONGNAME ea's. (It is an option you can set under 'Open->Settings'.)

:p.If ICON.EXE shows longnames, one can change the names as one would change
the name of objects on the desktop. Please remember that only the .LONGNAME
extented attribute is changed and NOT the fysical filename.

:p.You can change names by using the standard way to rename workplace shell
objects (alt and leftmouse button or shift-f9).

.*--------------------------------------------------------------*/
:h1 res=5006 name=PANEL_EXTRACT.Extracting icons
:i1 id=ExtractIcon.Extracting icons
:p.Another nice feature would be the possibility to 'extract' an icon to an
iconfile. When this option is chosen the program first looks if there is 
an .ICON ea and if so writes this ea as a file to disk. If there is no .ICON ea
the iconfile is constucted from the icon bitmap as it is visible on the
screen. In the latter case the iconfile only contains a bitmap for the
device it is extracted from.
:p.You can extract an icon from an object by selecting the extract option
from the objects popup menu.


.*--------------------------------------------------------------*/
:h1 res=5007 name=PANEL_DEFAULT.Setting the default icon
:i1 id=DefaultIcon.Setting the default icon
:p.Setting the default icon means that the .ICON ea is removed.
:p.You can set the default icon for an object by selecting the default option
from the objects popup menu.


.*--------------------------------------------------------------*/
:h1 res=5008 name=PANEL_ABSTRACT.Abstract objects
:i1 id=Abstract.Abstract objects
:p.:hp2.Wat is an abstract object?:ehp2.
:p.An abstract object is an object that is defined in your ini-files. 
Non-abstract are always present on a disk as a file or a directory.
.br
An example of an abstract object is the color palette.


:p.:hp2.Icons shown for abstract objects:ehp2.

:p.ICONTOOL also displays abstract objects. In some cases the icon for an
abstract object is somewere deep in an OS/2 dll. In such a case I'm not able
to determine the icon and a questionmark icon is shown.

:p.:hp2.Opening abstract objects:ehp2.
:p.With ICONTOOL you can open all abstract objects with the exception of the
color palette. This is that the api-call I use to open abstract objects
(WinSetObjectData(hObject, "OPEN=DEFAULT") resets a color palette object to
a 5 by 4 palette all of them being a 'New scheme'.

:p.:hp2.Deleting abstract objects:ehp2.
:p.ICONTOOL allows you to delete abstract objects, including 
:hp2.non-deleteable objects:ehp2.

.*--------------------------------------------------------------*/
:h1 res=5009 name=PANEL_PROBLEMS.Known problems
:i1 id=Problems.Known problems
:p.When draging-dropping to another application (e.g. the workplace shell
itself) the graphical display gets distorded. This is due to a bug in OS/2
itself, don't call me, call IBM ! This problem only appears in the original 
OS/2 2.0 GA version.

:p.ICON.EXE terminates or causes your desktop to hang when starting in WINOS2 
Program in early OS/2 2.00.1 BETA version. It works properly in GA, GA+SP, 
BETA 2.1, OS/2 2.1, 2.11 en OS/2 3.0 versions.

.*--------------------------------------------------------------*/
:h1 res=5011 name=PANEL_ICON_TXT.ICONTOOL.TXT
:i1 id=IconToolTxt.ICONTOOL.TXT
:fig.
.im icontool.txt
:efig.

:euserdoc.

