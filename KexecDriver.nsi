Name Kexec
OutFile KexecDriver.exe

SetCompressor zlib

XPStyle on
SilentInstall silent
RequestExecutionLevel admin

VIProductVersion "1.0.0.0"
VIAddVersionKey /LANG=1033 "CompanyName" "John Stumpo"
VIAddVersionKey /LANG=1033 "FileDescription" "Kexec Driver Setup"
VIAddVersionKey /LANG=1033 "FileVersion" "1.0"
VIAddVersionKey /LANG=1033 "InternalName" "KexecDriver.exe"
VIAddVersionKey /LANG=1033 "LegalCopyright" "© 2008 John Stumpo.  GNU GPL v3 or later."
VIAddVersionKey /LANG=1033 "OriginalFilename" "KexecDriver.exe"
VIAddVersionKey /LANG=1033 "ProductName" "WinKexec"
VIAddVersionKey /LANG=1033 "ProductVersion" "1.0"

Section "Kexec"
  SetOutPath $TEMP
  File kexec.sys
  File kexec.inf
  ExecWait "rundll32.exe setupapi.dll,InstallHinfSection DefaultInstall 132 $TEMP\kexec.inf"
  Delete kexec.sys
  Delete kexec.inf
SectionEnd