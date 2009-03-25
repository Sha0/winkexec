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

include ../common.mk

CFLAGS += -I../include

KEXEC_SYS_OBJECTS = buffer.o entry.o io.o libpe.o reboot.o resource.o linuxboot.o
KEXEC_SYS_LIBS = -lntoskrnl -lhal

__main_target : kexec.sys kexec.inf

# Using -shared exports every symbol, but otherwise it would be impossible to debug it efficiently with WinDbg...
kexec.sys : $(KEXEC_SYS_OBJECTS)
	$(CC) $(CFLAGS) -shared -nostdlib -Wl,--entry,_DriverEntry@8 -o kexec.sys $(KEXEC_SYS_OBJECTS) $(KEXEC_SYS_LIBS)

kexec.inf : kexec.inf.in
	$(PYTHON) ../revtag/revtag.py --tag-file .. kexec.inf.in >kexec.inf

clean :
	-rm -f kexec.sys kexec.inf *.o linuxboot_blobs/*.bin
.PHONY : clean

linuxboot.o : linuxboot_blobs/flatpmode.bin linuxboot_blobs/realmode.bin
