/* WinKexec: kexec for Windows
 * Copyright (C) 2008-2009 John Stumpo
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

#define IN_KEXEC_COMMON
#include <KexecCommon.h>

/* Are we GUI? */
BOOL isGui;

/* Convenient wrapper around FormatMessage() and GetLastError() */
KEXEC_DLLEXPORT LPSTR KexecTranslateError(void)
{
  static LPSTR msgbuf = NULL;

  if (msgbuf) {
    LocalFree(msgbuf);
    msgbuf = NULL;
  }

  FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
    NULL, GetLastError(), LANG_USER_DEFAULT, (LPSTR)&msgbuf, 0, NULL);

  return msgbuf;
}

/* Even more convenient wrapper around the above function.
   Use just like perror().
   XXX: Does the Windows API have something like this already? */
KEXEC_DLLEXPORT void KexecPerror(char * errmsg)
{
  if (isGui)
    MessageBoxPrintf(NULL, "%s: %s", "WinKexec GUI", MB_ICONERROR | MB_OK,
      errmsg, KexecTranslateError());
  else
    fprintf(stderr, "%s: %s", errmsg, KexecTranslateError());
}

/* MessageBox with printf. */
KEXEC_DLLEXPORT void MessageBoxPrintf(HWND parent, LPCSTR fmtstr,
  LPCSTR title, DWORD flags, ...)
{
  char buf[256];
  va_list args;

  va_start(args, flags);

  /* We can't KexecPerror() these errors or we could infinitely recurse... */
  vsnprintf(buf, 255, fmtstr, args);
  buf[255] = '\0';
  MessageBox(parent, buf, title, flags);
  va_end(args);
}

/* Is the kexec driver loaded? */
KEXEC_DLLEXPORT BOOL KexecDriverIsLoaded(void)
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

/* Load kexec.sys into the kernel, if it isn't already. */
KEXEC_DLLEXPORT void LoadKexecDriver(void)
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

  /* This does not return until DriverEntry() has completed in kexec.sys. */
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

/* If kexec.sys is loaded into the kernel, unload it. */
KEXEC_DLLEXPORT void UnloadKexecDriver(void)
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

  /* This does not return until DriverUnload() has completed in kexec.sys. */
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

/* What we must know before continuing... */
KEXEC_DLLEXPORT void KexecCommonInit(BOOL in_isGui)
{
  isGui = in_isGui;
}
