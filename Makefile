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

CC = gcc
ifdef DEBUG
CFLAGS = -g3 -O2 -W -Wall -mno-cygwin
else
CFLAGS = -s -O2 -W -Wall -mno-cygwin
endif
CYGPATH = cygpath
DLLTOOL = dlltool
# Use the Registry to locate the NSIS install path.
MAKENSIS = "$(shell $(CYGPATH) "$(shell cat /proc/registry/HKEY_LOCAL_MACHINE/SOFTWARE/NSIS/@)")/makensis.exe"
NASM = nasm
PYTHON = python
WINDRES = windres

KEXEC_SYS_OBJECTS = KexecDriver.o KexecDriverPe.o KexecDriverReboot.o KexecDriverResources.o KexecLinuxBoot.o
KEXEC_SYS_LIBS    = -lntoskrnl -lhal
KEXEC_EXE_OBJECTS = KexecClient.o KexecClientResources.o KexecCommon.lib
KEXEC_EXE_LIBS    = -lkernel32 -lmsvcrt
KEXEC_GUI_OBJECTS = KexecGui.o KexecGuiResources.o KexecCommon.lib
KEXEC_GUI_LIBS    = -lkernel32 -lmsvcrt -luser32 -lcomctl32
KEXECCOMMON_DLL_OBJECTS = KexecCommon.o KexecCommonResources.o
KEXECCOMMON_DLL_LIBS = -lkernel32 -lmsvcrt -ladvapi32 -luser32

all : KexecSetup.exe
.PHONY : all

clean :
	-rm -f *.sys *.o *.exe *.inf *.bin *.dll *.lib Revision.h Revision.nsh
.PHONY : clean

KexecSetup.exe : KexecDriver.exe KexecGui.exe kexec.exe KexecSetup.nsi EnvVarUpdate.nsh Revision.nsh LICENSE.txt
	$(MAKENSIS) KexecSetup.nsi

KexecDriver.exe : kexec.sys KexecDriver.nsi kexec.inf LICENSE.txt Revision.nsh
	$(MAKENSIS) KexecDriver.nsi

# Using -shared exports every symbol, but otherwise it would be impossible to debug it efficiently with WinDbg...
kexec.sys : $(KEXEC_SYS_OBJECTS)
	$(CC) $(CFLAGS) -shared -nostdlib -Wl,--entry,_DriverEntry@8 -o kexec.sys $(KEXEC_SYS_OBJECTS) $(KEXEC_SYS_LIBS)

KexecDriver.o : KexecDriver.c kexec.h KexecDriver.h
	$(CC) $(CFLAGS) -c -o KexecDriver.o KexecDriver.c

KexecDriverPe.o : KexecDriverPe.c KexecDriver.h
	$(CC) $(CFLAGS) -c -o KexecDriverPe.o KexecDriverPe.c

KexecDriverReboot.o : KexecDriverReboot.c KexecDriver.h
	$(CC) $(CFLAGS) -c -o KexecDriverReboot.o KexecDriverReboot.c

KexecDriverResources.o : KexecDriver.rc Revision.h
	$(WINDRES) -o KexecDriverResources.o KexecDriver.rc

KexecLinuxBoot.o : KexecLinuxBoot.asm KexecLinuxBootFlatPmodePart.bin KexecLinuxBootRealModePart.bin
	$(NASM) -f coff -o KexecLinuxBoot.o KexecLinuxBoot.asm

KexecLinuxBootFlatPmodePart.bin : KexecLinuxBootFlatPmodePart.asm
	$(NASM) -f bin -o KexecLinuxBootFlatPmodePart.bin KexecLinuxBootFlatPmodePart.asm

KexecLinuxBootRealModePart.bin : KexecLinuxBootRealModePart.asm
	$(NASM) -f bin -o KexecLinuxBootRealModePart.bin KexecLinuxBootRealModePart.asm

kexec.exe : $(KEXEC_EXE_OBJECTS)
	$(CC) $(CFLAGS) -o kexec.exe $(KEXEC_EXE_OBJECTS) $(KEXEC_EXE_LIBS)

KexecClient.o : KexecClient.c kexec.h Revision.h KexecCommon.h
	$(CC) $(CFLAGS) -c -o KexecClient.o KexecClient.c

KexecClientResources.o : KexecClient.rc Revision.h
	$(WINDRES) -o KexecClientResources.o KexecClient.rc

KexecGui.exe : $(KEXEC_GUI_OBJECTS)
	$(CC) -mwindows $(CFLAGS) -o KexecGui.exe $(KEXEC_GUI_OBJECTS) $(KEXEC_GUI_LIBS)

KexecGui.o : KexecGui.c kexec.h Revision.h KexecCommon.h
	$(CC) $(CFLAGS) -c -o KexecGui.o KexecGui.c

KexecGuiResources.o : KexecGui.rc Revision.h KexecGuiManifest.xml Kexec.ico
	$(WINDRES) -o KexecGuiResources.o KexecGui.rc

KexecCommon.o : KexecCommon.c KexecCommon.h
	$(CC) $(CFLAGS) -c -o KexecCommon.o KexecCommon.c

KexecCommonResources.o : KexecCommon.rc Revision.h
	$(WINDRES) -o KexecCommonResources.o KexecCommon.rc

# We use -mdll so then only DLLEXPORTed stuff is exported.
KexecCommon.dll : $(KEXECCOMMON_DLL_OBJECTS)
	$(CC) $(CFLAGS) -mdll -o KexecCommon.dll $(KEXECCOMMON_DLL_OBJECTS) $(KEXECCOMMON_DLL_LIBS)

KexecCommon.lib : KexecCommon.dll KexecCommon.def
	$(DLLTOOL) -l KexecCommon.lib -D KexecCommon.dll -d KexecCommon.def

kexec.inf : kexec.inf.in
	$(MAKE) Revision.h

Revision.h Revision.nsh : FORCE
	$(PYTHON) SvnRevision.py \
	  DRIVER=KexecDriver.c,KexecDriver.rc,KexecDriverPe.c,KexecDriver.h,KexecDriverReboot.c,KexecLinuxBoot.asm,KexecLinuxBootFlatPmodePart.asm,KexecLinuxBootRealModePart.asm,kexec.h,kexec.inf.in,Makefile,SvnRevision.py \
	  DRIVER_NSI=DRIVER,LICENSE.txt,KexecDriver.nsi \
	  COMMON=DRIVER,KexecCommon.c,KexecCommon.h,KexecCommon.rc,KexecCommon.def \
	  CLIENT=COMMON,KexecClient.c,KexecClient.rc \
	  GUI=COMMON,KexecGui.c,KexecGui.rc,KexecGuiManifest.xml,Kexec.ico \
	  CLIENT_NSI=CLIENT,GUI,DRIVER_NSI,EnvVarUpdate.nsh,KexecSetup.nsi

FORCE :