Name Kexec
OutFile kexec.exe

SetCompressor zlib

XPStyle on
SilentInstall silent
RequestExecutionLevel admin

Section "Kexec"
  SetOutPath $TEMP
  File kexec.sys
  File kexec.inf
  ExecWait "rundll32.exe setupapi.dll,InstallHinfSection DefaultInstall 132 $TEMP\kexec.inf"
  Delete kexec.sys
  Delete kexec.inf
SectionEnd