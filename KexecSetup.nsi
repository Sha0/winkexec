# WinKexec: kexec for Windows
# Copyright (C) 2008 John Stumpo
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

!include ".\Revision.nsh"
!include ".\EnvVarUpdate.nsh"

!include "MUI2.nsh"

Name "WinKexec r${CLIENT_REVISION}"
OutFile KexecSetup.exe

InstallDir "$PROGRAMFILES\WinKexec"
InstallDirRegKey HKLM "Software\WinKexec" InstallRoot

SetCompressor zlib

RequestExecutionLevel admin

!define MUI_ABORTWARNING
!define MUI_UNABORTWARNING
!define MUI_FINISHPAGE_NOAUTOCLOSE
!define MUI_UNFINISHPAGE_NOAUTOCLOSE
!define MUI_LICENSEPAGE_RADIOBUTTONS

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

VIProductVersion "1.0.0.${CLIENT_NSI_REVISION}"
VIAddVersionKey /LANG=1033 "CompanyName" "John Stumpo"
VIAddVersionKey /LANG=1033 "FileDescription" "WinKexec Setup"
VIAddVersionKey /LANG=1033 "FileVersion" "1.0 (r${CLIENT_NSI_REVISION})"
VIAddVersionKey /LANG=1033 "InternalName" "KexecSetup.exe"
VIAddVersionKey /LANG=1033 "LegalCopyright" "© 2008 John Stumpo.  GNU GPL v3 or later."
VIAddVersionKey /LANG=1033 "OriginalFilename" "KexecSetup.exe"
VIAddVersionKey /LANG=1033 "ProductName" "WinKexec"
VIAddVersionKey /LANG=1033 "ProductVersion" "1.0 (r${CLIENT_NSI_REVISION})"

ShowInstDetails show
ShowUninstDetails show

Function .onInit
  System::Call 'kernel32::CreateMutexA(i 0, i 0, t "KexecInstallerMutex") i .r1 ?e'
  Pop $R0
  StrCmp $R0 0 +3
    MessageBox MB_OK|MB_ICONSTOP "WinKexec r${CLIENT_REVISION} Setup is already running." /SD IDOK
    Abort
FunctionEnd

Function un.onInit
  System::Call 'kernel32::CreateMutexA(i 0, i 0, t "KexecUninstallerMutex") i .r1 ?e'
  Pop $R0
  StrCmp $R0 0 +3
    MessageBox MB_OK|MB_ICONSTOP "WinKexec r${CLIENT_REVISION} Uninstall is already running." /SD IDOK
    Abort
FunctionEnd

Section "Kexec"
  SetOutPath $INSTDIR
  File kexec.exe
  File KexecDriver.exe
  ExecWait "$\"$INSTDIR\KexecDriver.exe$\" /S"
  WriteUninstaller "$INSTDIR\KexecUninstall.exe"
  WriteRegStr HKLM "Software\WinKexec" InstallRoot $INSTDIR
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinKexec" "DisplayName" "WinKexec (r${CLIENT_REVISION})"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinKexec" "UninstallString" "$\"$INSTDIR\KexecUninstall.exe$\""
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinKexec" "InstallLocation" "$INSTDIR"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinKexec" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinKexec" "NoRepair" 1
  ${EnvVarUpdate} $0 PATH A HKLM $INSTDIR
SectionEnd

Section "Uninstall"
  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Kexec" "UninstallString"
  IfErrors NoDriverUninstall
  ExpandEnvStrings $1 $0
  ExecWait $1
  Goto DoneDriverUninstall
NoDriverUninstall:
  DetailPrint "Not uninstalling driver because no kexec driver is installed."
  ClearErrors
DoneDriverUninstall:
  ${un.EnvVarUpdate} $0 PATH R HKLM $INSTDIR
  Delete $INSTDIR\kexec.exe
  Delete $INSTDIR\KexecDriver.exe
  DeleteRegValue HKLM "Software\WinKexec" InstallRoot
  DeleteRegKey /ifempty HKLM "Software\WinKexec"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\WinKexec"
  Delete "$INSTDIR\KexecUninstall.exe"
  RmDir $INSTDIR
SectionEnd