CC = gcc
CFLAGS = -g -O2 -W -Wall -I/usr/include/w32api/ddk
CYGPATH = /bin/cygpath
MAKENSIS = "$(shell $(CYGPATH) "$(PROGRAMFILES)")/NSIS/makensis.exe"

all : kexec.exe
.PHONY : all

clean :
	-rm -f *.sys *.o *.exe
.PHONY : clean

kexec.exe : kexec.sys kexec.nsi
	$(MAKENSIS) kexec.nsi

kexec.sys : entry.o
	$(CC) $(CFLAGS) -s -mno-cygwin -shared -nostdlib -Wl,--entry,_DriverEntry@8 -o kexec.sys entry.o -lntoskrnl

entry.o : entry.c
	$(CC) $(CFLAGS) -c -o entry.o entry.c