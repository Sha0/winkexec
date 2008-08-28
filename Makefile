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
CFLAGS = -g -O2 -W -Wall -mno-cygwin
CYGPATH = cygpath
# Assume default install path for NSIS.
MAKENSIS = "$(shell $(CYGPATH) "$(PROGRAMFILES)")/NSIS/makensis.exe"
PYTHON = python
WINDRES = windres

all : KexecSetup.exe
.PHONY : all

clean :
	-rm -f *.sys *.o *.exe *.inf Revision.h Revision.nsh
.PHONY : clean

KexecSetup.exe : KexecDriver.exe kexec.exe KexecSetup.nsi EnvVarUpdate.nsh Revision.nsh LICENSE.txt
	$(MAKENSIS) KexecSetup.nsi

KexecDriver.exe : kexec.sys KexecDriver.nsi kexec.inf LICENSE.txt Revision.nsh
	$(MAKENSIS) KexecDriver.nsi

kexec.sys : KexecDriver.o KexecDriverResources.o
	$(CC) $(CFLAGS) -s -shared -nostdlib -Wl,--entry,_DriverEntry@8 -o kexec.sys KexecDriver.o KexecDriverResources.o -lntoskrnl -lhal

KexecDriver.o : KexecDriver.c kexec.h
	$(CC) $(CFLAGS) -c -o KexecDriver.o KexecDriver.c

KexecDriverResources.o : KexecDriver.rc Revision.h
	$(WINDRES) -o KexecDriverResources.o KexecDriver.rc

kexec.exe : KexecClient.o KexecClientResources.o
	$(CC) $(CFLAGS) -s -o kexec.exe KexecClient.o KexecClientResources.o

KexecClient.o : KexecClient.c kexec.h Revision.h
	$(CC) $(CFLAGS) -c -o KexecClient.o KexecClient.c

KexecClientResources.o : KexecClient.rc Revision.h
	$(WINDRES) -o KexecClientResources.o KexecClient.rc

kexec.inf : kexec.inf.in
	$(MAKE) Revision.h

Revision.h Revision.nsh : FORCE
	$(PYTHON) SvnRevision.py DRIVER=KexecDriver.c,KexecDriver.rc,kexec.h,kexec.inf.in,LICENSE.txt,Makefile,SvnRevision.py CLIENT=KexecDriver.c,KexecDriver.rc,kexec.h,kexec.inf.in,LICENSE.txt,Makefile,SvnRevision.py,KexecClient.c,KexecClient.rc

FORCE :