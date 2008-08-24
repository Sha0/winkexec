CC = gcc
CFLAGS = -g -O2 -W -Wall -I/usr/include/w32api/ddk
CYGPATH = cygpath
MAKENSIS = "$(shell $(CYGPATH) "$(PROGRAMFILES)")/NSIS/makensis.exe"
WINDRES = windres

all : kexec.exe
.PHONY : all

clean :
	-rm -f *.sys *.o *.exe
.PHONY : clean

kexec.exe : kexec.sys kexec.nsi
	$(MAKENSIS) kexec.nsi

kexec.sys : entry.o version.o
	$(CC) $(CFLAGS) -s -mno-cygwin -shared -nostdlib -Wl,--entry,_DriverEntry@8 -o kexec.sys entry.o version.o -lntoskrnl -lhal

entry.o : entry.c kexec.h
	$(CC) $(CFLAGS) -c -o entry.o entry.c

version.o : kexec.rc
	$(WINDRES) -o version.o kexec.rc