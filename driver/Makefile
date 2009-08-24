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

CFLAGS += -I../include -DDRIVER

KEXEC_SYS_OBJECTS = buffer.o entry.o io.o libpe.o linuxboot.o reboot.o resource.o sha1.o util.o
KEXEC_SYS_LIBS = -lntoskrnl -lhal

# boot/reassemble.o *must* be first!
# The entry point is the first function in the first file given.
BOOT_REASSEMBLE_EXE_OBJECTS = boot/reassemble.o boot/console.o boot/pagesort.o boot/stdlib.o boot/string.o boot/verify.o sha1.o util.o

__main_target : kexec.sys kexec.inf

# Using -shared exports every symbol, but otherwise it would be impossible to debug it efficiently with WinDbg...
kexec.sys : $(KEXEC_SYS_OBJECTS)
	$(CC) $(CFLAGS) -shared -nostdlib -Wl,--entry,_DriverEntry@8 -o kexec.sys $(KEXEC_SYS_OBJECTS) $(KEXEC_SYS_LIBS)

kexec.inf : kexec.inf.in
	$(PYTHON) ../revtag/revtag.py --tag-file .. kexec.inf.in >kexec.inf

clean :
	-rm -f kexec.sys kexec.inf *.o boot/*.bin boot/*.exe boot/*.o boot/bootcode.h
.PHONY : clean

linuxboot.o : boot/bootcode.h

boot/bootcode.bin : boot/reassemble.bin

boot/reassemble.bin : boot/reassemble.exe
	$(OBJCOPY) -O binary -j .text boot/reassemble.exe boot/reassemble.bin

boot/reassemble.exe : $(BOOT_REASSEMBLE_EXE_OBJECTS)
	$(LD) -s -T boot/reassemble.ld -o boot/reassemble.exe $(BOOT_REASSEMBLE_EXE_OBJECTS)

.bin.h :
	$(PYTHON) bin2h.py $< $@ "$(shell basename $< | sed -e 's/[^a-z]/_/g')"

.SUFFIXES : .bin .h
