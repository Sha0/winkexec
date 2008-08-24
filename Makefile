CC = gcc
CFLAGS = -g -O2 -W -Wall -I/usr/include/w32api/ddk
CYGPATH = cygpath
MAKENSIS = "$(shell $(CYGPATH) "$(PROGRAMFILES)")/NSIS/makensis.exe"
WINDRES = windres

all : KexecDriver.exe
.PHONY : all

clean :
	-rm -f *.sys *.o *.exe
.PHONY : clean

KexecDriver.exe : kexec.sys KexecDriver.nsi kexec.inf
	$(MAKENSIS) KexecDriver.nsi

kexec.sys : KexecDriver.o KexecDriverResources.o
	$(CC) $(CFLAGS) -s -mno-cygwin -shared -nostdlib -Wl,--entry,_DriverEntry@8 -o kexec.sys KexecDriver.o KexecDriverResources.o -lntoskrnl -lhal

KexecDriver.o : KexecDriver.c kexec.h
	$(CC) $(CFLAGS) -c -o KexecDriver.o KexecDriver.c

KexecDriverResources.o : KexecDriver.rc
	$(WINDRES) -o KexecDriverResources.o KexecDriver.rc