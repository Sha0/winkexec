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
# Assume default install path for NSIS.
MAKENSIS = "$(shell $(CYGPATH) "$(PROGRAMFILES)")/NSIS/makensis.exe"
PYTHON = python
WINDRES = windres

KEXEC_SYS_OBJECTS = KexecDriver.o KexecDriverPe.o KexecDriverReboot.o KexecDriverResources.o
KEXEC_SYS_LIBS    = -lntoskrnl -lhal
KEXEC_EXE_OBJECTS = KexecClient.o KexecClientResources.o
KEXEC_EXE_LIBS    = -lkernel32 -lmsvcrt -ladvapi32

all : KexecSetup.exe
.PHONY : all

clean :
	-rm -f *.sys *.o *.exe *.inf Revision.h Revision.nsh
.PHONY : clean

KexecSetup.exe : KexecDriver.exe kexec.exe KexecSetup.nsi EnvVarUpdate.nsh Revision.nsh LICENSE.txt
	$(MAKENSIS) KexecSetup.nsi

KexecDriver.exe : kexec.sys KexecDriver.nsi kexec.inf LICENSE.txt Revision.nsh
	$(MAKENSIS) KexecDriver.nsi

kexec.sys : $(KEXEC_SYS_OBJECTS)
	$(CC) $(CFLAGS) -shared -nostdlib -Wl,--entry,_DriverEntry@8 -o kexec.sys $(KEXEC_SYS_OBJECTS) $(KEXEC_SYS_LIBS)

KexecDriver.o : KexecDriver.c kexec.h KexecDriverReboot.h
	$(CC) $(CFLAGS) -c -o KexecDriver.o KexecDriver.c

KexecDriverPe.o : KexecDriverPe.c KexecDriverPe.h
	$(CC) $(CFLAGS) -c -o KexecDriverPe.o KexecDriverPe.c

KexecDriverReboot.o : KexecDriverReboot.c KexecDriverReboot.h KexecDriverPe.h
	$(CC) $(CFLAGS) -c -o KexecDriverReboot.o KexecDriverReboot.c

KexecDriverResources.o : KexecDriver.rc Revision.h
	$(WINDRES) -o KexecDriverResources.o KexecDriver.rc

kexec.exe : $(KEXEC_EXE_OBJECTS)
	$(CC) $(CFLAGS) -o kexec.exe $(KEXEC_EXE_OBJECTS) $(KEXEC_EXE_LIBS)

KexecClient.o : KexecClient.c kexec.h Revision.h
	$(CC) $(CFLAGS) -c -o KexecClient.o KexecClient.c

KexecClientResources.o : KexecClient.rc Revision.h
	$(WINDRES) -o KexecClientResources.o KexecClient.rc

kexec.inf : kexec.inf.in
	$(MAKE) Revision.h

Revision.h Revision.nsh : FORCE
	$(PYTHON) SvnRevision.py \
	  DRIVER=KexecDriver.c,KexecDriver.rc,KexecDriverPe.c,KexecDriverPe.h,KexecDriverReboot.c,KexecDriverReboot.h,kexec.h,kexec.inf.in,Makefile,SvnRevision.py \
	  DRIVER_NSI=DRIVER,LICENSE.txt,KexecDriver.nsi \
	  CLIENT=DRIVER,KexecClient.c,KexecClient.rc \
	  CLIENT_NSI=CLIENT,DRIVER_NSI,EnvVarUpdate.nsh,KexecSetup.nsi

FORCE :