' this script is used by Advanced Installer to update the service install/uninstall batch 
' files with the actual path to the pion.exe binary
Const ForReading = 1, ForWriting = 2, ForAppending = 8

Dim fso, f
Dim appdir
Dim file
Dim txt

'on error resume next   
appdir = Session.Property("CustomActionData")
file=appdir+"pion_install_service.bat"

'read the file first   
Set fso = CreateObject("Scripting.FileSystemObject")

Set f = fso.OpenTextFile(file, ForReading, True)
txt = f.ReadAll()
f.Close

' replace the placeholder with the actual install directory
Set f = fso.OpenTextFile(file, ForWriting, True)
f.Write(Replace(txt,"{APPDIR}",appdir))
f.Close
