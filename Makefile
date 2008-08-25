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
CFLAGS = -g -O2 -W -Wall -I/usr/include/w32api/ddk
CYGPATH = cygpath
# Assume default install path for NSIS.
MAKENSIS = "$(shell $(CYGPATH) "$(PROGRAMFILES)")/NSIS/makensis.exe"
PYTHON = python
WINDRES = windres

all : KexecDriver.exe
.PHONY : all

clean :
	-rm -f *.sys *.o *.exe Revision.h Revision.nsh
.PHONY : clean

KexecDriver.exe : kexec.sys KexecDriver.nsi kexec.inf LICENSE.txt Revision.nsh
	$(MAKENSIS) KexecDriver.nsi

kexec.sys : KexecDriver.o KexecDriverResources.o
	$(CC) $(CFLAGS) -s -mno-cygwin -shared -nostdlib -Wl,--entry,_DriverEntry@8 -o kexec.sys KexecDriver.o KexecDriverResources.o -lntoskrnl -lhal

KexecDriver.o : KexecDriver.c kexec.h
	$(CC) $(CFLAGS) -c -o KexecDriver.o KexecDriver.c

KexecDriverResources.o : KexecDriver.rc Revision.h
	$(WINDRES) -o KexecDriverResources.o KexecDriver.rc

Revision.h Revision.nsh : FORCE
	$(PYTHON) SvnRevision.py DRIVER=KexecDriver.c,KexecDriver.rc,kexec.h

FORCE :