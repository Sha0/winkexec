# WinKexec: kexec for Windows
# Copyright (C) 2008-2009 John Stumpo
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

CC = $(CROSS)gcc
ifdef DEBUG
CFLAGS = -g3 -O2 -W -Wall -mno-cygwin
else
CFLAGS = -s -O2 -W -Wall -mno-cygwin
endif
CYGPATH = cygpath
DLLTOOL = $(CROSS)dlltool
ifeq ($(CROSS),)
# Use the Registry to locate the NSIS install path.
MAKENSIS = "$(shell $(CYGPATH) "$(shell cat /proc/registry/HKEY_LOCAL_MACHINE/SOFTWARE/NSIS/@)")/makensis.exe"
else
MAKENSIS = makensis
endif
NASM = nasm
OBJCOPY = $(CROSS)objcopy
PYTHON = python
WINDRES = $(CROSS)windres

all : __main_target
.PHONY : all
.PHONY : __main_target

.rc.o :
	$(WINDRES) $(RCFLAGS) -o $@ $<

.asm.o :
	$(NASM) $(NASMFLAGS) -f coff -o $@ $<

.asm.bin :
	$(NASM) $(NASMFLAGS) -f bin -o $@ $<

.SUFFIXES :
.SUFFIXES : .c .o .rc .asm .bin
