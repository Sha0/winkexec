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

!include "MUI2.nsh"

Name "Kexec Driver r${DRIVER_REVISION}"
OutFile KexecDriver.exe

SetCompressor zlib

ShowInstDetails show
RequestExecutionLevel admin

!define MUI_LICENSEPAGE_RADIOBUTTONS
!define MUI_FINISHPAGE_NOAUTOCLOSE

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_LANGUAGE "English"

VIProductVersion "1.0.0.${DRIVER_REVISION}"
VIAddVersionKey /LANG=1033 "CompanyName" "John Stumpo"
VIAddVersionKey /LANG=1033 "FileDescription" "Kexec Driver Setup"
VIAddVersionKey /LANG=1033 "FileVersion" "1.0 (r${DRIVER_REVISION})"
VIAddVersionKey /LANG=1033 "InternalName" "KexecDriver.exe"
VIAddVersionKey /LANG=1033 "LegalCopyright" "© 2008 John Stumpo.  GNU GPL v3 or later."
VIAddVersionKey /LANG=1033 "OriginalFilename" "KexecDriver.exe"
VIAddVersionKey /LANG=1033 "ProductName" "WinKexec"
VIAddVersionKey /LANG=1033 "ProductVersion" "1.0 (r${DRIVER_REVISION})"

Section "Kexec"
  SetOutPath $TEMP
  File kexec.sys
  File kexec.inf
  ExecWait "rundll32.exe setupapi.dll,InstallHinfSection DefaultInstall 132 $TEMP\kexec.inf"
  Delete kexec.sys
  Delete kexec.inf
SectionEnd