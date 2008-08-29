/* WinKexec: kexec for Windows
 * Copyright (C) 2008 John Stumpo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <windows.h>
#include <stdio.h>
#include "kexec.h"
#include "Revision.h"

LPWSTR KexecTranslateError(void)
{
  static LPWSTR msgbuf = NULL;

  if (msgbuf) {
    LocalFree(msgbuf);
    msgbuf = NULL;
  }

  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
    NULL, GetLastError(), LANG_USER_DEFAULT, &msgbuf, 0, NULL);

  return msgbuf;
}

void KexecPerror(char * errmsg)
{
  fprintf(stderr, "%s: %s", errmsg, KexecTranslateError());
}

BOOL KexecDriverIsLoaded(void)
{
  SC_HANDLE Scm;
  SC_HANDLE KexecService;
  SERVICE_STATUS_PROCESS ServiceStatus;
  BOOL retval;
  DWORD ExtraBytes;

  Scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (!Scm) {
    KexecPerror("Could not open SCM");
    exit(EXIT_FAILURE);
  }

  KexecService = OpenService(Scm, "kexec", SERVICE_ALL_ACCESS);
  if (!KexecService) {
    KexecPerror("Could not open the kexec service");
    fprintf(stderr, "(Is the kexec driver installed, and are you an admin?)\n");
    CloseServiceHandle(Scm);
    exit(EXIT_FAILURE);
  }

  if (!QueryServiceStatusEx(KexecService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ServiceStatus,
    sizeof(ServiceStatus), &ExtraBytes))
  {
    KexecPerror("Could not query the kexec service");
    fprintf(stderr, "(Are you an admin?)\n");
    CloseServiceHandle(KexecService);
    CloseServiceHandle(Scm);
    exit(EXIT_FAILURE);
  }

  retval = (ServiceStatus.dwCurrentState == SERVICE_RUNNING);
  CloseServiceHandle(KexecService);
  CloseServiceHandle(Scm);
  return retval;
}

void LoadKexecDriver(void)
{
  SC_HANDLE Scm;
  SC_HANDLE KexecService;

  if (KexecDriverIsLoaded())
    return;

  printf("Loading the kexec driver... ");

  Scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (!Scm) {
    KexecPerror("Could not open SCM");
    exit(EXIT_FAILURE);
  }

  KexecService = OpenService(Scm, "kexec", SERVICE_ALL_ACCESS);
  if (!KexecService) {
    KexecPerror("Could not open the kexec service");
    fprintf(stderr, "(Is the kexec driver installed, and are you an admin?)\n");
    CloseServiceHandle(Scm);
    exit(EXIT_FAILURE);
  }

  if (!StartService(KexecService, 0, NULL)) {
    KexecPerror("Could not start the kexec service");
    fprintf(stderr, "(Are you an admin?)\n");
    CloseServiceHandle(KexecService);
    CloseServiceHandle(Scm);
    exit(EXIT_FAILURE);
  }

  CloseServiceHandle(KexecService);
  CloseServiceHandle(Scm);
  printf("ok\n");
  return;
}

void UnloadKexecDriver(void)
{
  SC_HANDLE Scm;
  SC_HANDLE KexecService;
  SERVICE_STATUS ServiceStatus;

  if (!KexecDriverIsLoaded())
    return;

  printf("Unloading the kexec driver... ");

  Scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (!Scm) {
    KexecPerror("Could not open SCM");
    exit(EXIT_FAILURE);
  }

  KexecService = OpenService(Scm, "kexec", SERVICE_ALL_ACCESS);
  if (!KexecService) {
    KexecPerror("Could not open the kexec service");
    fprintf(stderr, "(Is the kexec driver installed, and are you an admin?)\n");
    CloseServiceHandle(Scm);
    exit(EXIT_FAILURE);
  }

  if (!ControlService(KexecService, SERVICE_CONTROL_STOP, &ServiceStatus)) {
    KexecPerror("Could not stop the kexec service");
    fprintf(stderr, "(Are you an admin?)\n");
    CloseServiceHandle(KexecService);
    CloseServiceHandle(Scm);
    exit(EXIT_FAILURE);
  }

  CloseServiceHandle(KexecService);
  CloseServiceHandle(Scm);
  printf("ok\n");
  return;
}

int DoLoad(int argc, char** argv)
{
  DWORD klen, read_len;
  unsigned char* kbuf;
  HANDLE kernel;
  HANDLE device;

  if (argc < 1) {
    if (!KexecDriverIsLoaded())
      LoadKexecDriver();
    else
      printf("The kexec driver was already loaded; nothing to do.\n");
    exit(EXIT_SUCCESS);
  }

  if ((kernel = CreateFile(argv[0], GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE) {
    KexecPerror("Failed to load kernel");
    exit(EXIT_FAILURE);
  }

  if ((klen = GetFileSize(kernel, NULL)) == INVALID_FILE_SIZE) {
    KexecPerror("Failed to get kernel size");
    exit(EXIT_FAILURE);
  }

  if ((kbuf = malloc(klen)) == NULL) {
    perror("Could not allocate buffer for kernel");
    exit(EXIT_FAILURE);
  }
  if (!ReadFile(kernel, kbuf, klen, &read_len, NULL)) {
    KexecPerror("Could not read kernel");
    exit(EXIT_FAILURE);
  }
  CloseHandle(kernel);

  if (*(unsigned short*)(kbuf+510) != 0xaa55 ||
    strncmp(kbuf+514, "HdrS", 4) != 0)
  {
      fprintf(stderr, "warning: This does not look like a Linux kernel.\n");
      fprintf(stderr, "warning: Loading it anyway.\n");
  }

  LoadKexecDriver();
  if ((device = CreateFile("\\\\.\\kexec", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE) {
    KexecPerror("Failed to open \\\\.\\kexec");
    fprintf(stderr, "(Are you an admin?)\n");
    exit(EXIT_FAILURE);
  }

  CloseHandle(device);
}

int DoUnload(int argc, char** argv)
{
  if (KexecDriverIsLoaded())
    UnloadKexecDriver();
  else
    printf("The kexec driver was not loaded; nothing to do.\n");
  exit(EXIT_SUCCESS);
}

int DoShow(int argc, char** argv)
{
  if (!KexecDriverIsLoaded()) {
    printf("The kexec driver is not loaded.  (Use `kexec /l' to load it.)\n");
    exit(EXIT_FAILURE);
  }

  printf("The kexec driver is active.  (Use `kexec /u' to unload it.)\n");
  exit(EXIT_SUCCESS);
}

void usage()
{
  fprintf(stderr, "%s",
"\n\
WinKexec: kexec for Windows (v1.0, svn revision " CLIENT_REVISION_STR ")\n\
Copyright (C) 2008 John Stumpo\n\
\n\
This program is free software; you may redistribute or modify it under the\n\
terms of the GNU General Public License, version 3 or later.  There is\n\
ABSOLUTELY NO WARRANTY, not even for MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.  See the GPL version 3 for full details.\n\
\n\
Usage: kexec [action] [options]...\n\
Actions:\n\
  /l /load     Load a Linux kernel.\n\
    The next option is the kernel filename.  All subsequent options are\n\
    passed as the kernel command line.  If an initrd= option is given,\n\
    the named file will be loaded as an initrd.  The kexec driver will\n\
    be loaded automatically if it is not loaded.  With no options, just\n\
    load the kexec driver without loading a kernel.\n\
  /u /unload   Unload the kexec driver.  (Naturally, this causes it to\n\
    forget the currently loaded kernel, if any, as well.)\n\
  /c /clear    Clear the currently loaded Linux kernel, but leave the\n\
    kexec driver loaded.\n\
  /s /show     Show current state of kexec.\n\
  /h /? /help  Show this help.\n\
\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
  int (*action)(int, char**) = NULL;

  if (argc < 2)
    usage();

  if (argv[1][0] == '-')
    argv[1][0] = '/';

  if (!strcasecmp(argv[1], "/l") || !strcasecmp(argv[1], "/load"))
    action = DoLoad;

  if (!strcasecmp(argv[1], "/u") || !strcasecmp(argv[1], "/unload"))
    action = DoUnload;

  if (!strcasecmp(argv[1], "/s") || !strcasecmp(argv[1], "/show"))
    action = DoShow;

  if (!action)
    usage();

  exit(action(argc - 2, argv + 2));
}
