'http://superuser.com/questions/110991/can-you-zip-a-file-from-the-command-prompt-using-only-windows-built-in-capabili
'Get command-line arguments.
Set objArgs = WScript.Arguments
InputFolder = objArgs(0)
ZipFile = objArgs(1)

'Create empty ZIP file.
CreateObject("Scripting.FileSystemObject").CreateTextFile(ZipFile, True).Write "PK" & Chr(5) & Chr(6) & String(18, vbNullChar)

Set objShell = CreateObject("Shell.Application")

Set source = objShell.NameSpace(InputFolder).Items

objShell.NameSpace(ZipFile).CopyHere(source)

' Wait for compression window to open
set scriptShell = CreateObject("Wscript.Shell")
Do While scriptShell.AppActivate("Compressing...") = FALSE   
   WScript.Sleep 500 ' Arbitrary polling delay
Loop  

' Wait for compression to complete before exiting script
Do While scriptShell.AppActivate("Compressing...") = TRUE   
   WScript.Sleep 500 ' Arbitrary polling delay
Loop
