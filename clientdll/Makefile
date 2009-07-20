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

KEXECCOMMON_DLL_OBJECTS = KexecCommon.o resource.o
KEXECCOMMON_DLL_LIBS = -lkernel32 -lmsvcrt -ladvapi32 -luser32

__main_target : KexecCommon.dll KexecCommon.lib

# We use -mdll so then only DLLEXPORTed stuff is exported.
KexecCommon.dll : $(KEXECCOMMON_DLL_OBJECTS)
	$(CC) $(CFLAGS) -mdll -o KexecCommon.dll $(KEXECCOMMON_DLL_OBJECTS) $(KEXECCOMMON_DLL_LIBS)

KexecCommon.lib : KexecCommon.dll KexecCommon.def
	$(DLLTOOL) -l KexecCommon.lib -D KexecCommon.dll -d KexecCommon.def

clean :
	-rm -f KexecCommon.dll KexecCommon.lib *.o
.PHONY : clean
