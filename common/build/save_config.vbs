Dim sAppDir         : sAppDir = session.property("APPDIR")
Dim sConfigDir	    : sConfigDir = sAppDir & "config"
Dim sConfigNewDir   : sConfigNewDir = sAppDir & "config-new"
Dim fso

Set fso = CreateObject("Scripting.FileSystemObject")

On Error resume next
If Not fso.FolderExists(sConfigDir) Then    
	fso.CopyFolder sConfigNewDir, sConfigDir
End If
