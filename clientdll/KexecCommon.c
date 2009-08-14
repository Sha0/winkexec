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
#undef NDEBUG
#include <KexecCommon.h>
#include <assert.h>

static char kxciLastErrorMsg[256];

HINSTANCE hInst;


/* Convenient wrapper around FormatMessage() and GetLastError() */
static LPSTR KxciTranslateError(void)
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


/* Set the most recent KexecCommon-related error message. */
static void KxciBuildErrorMessage(LPSTR errmsg)
{
  snprintf(kxciLastErrorMsg, 255, "%s: %s", errmsg, KxciTranslateError());
  kxciLastErrorMsg[255] = '\0';
}


/* Reset the error message. */
static void KxciResetErrorMessage(void)
{
  memset(kxciLastErrorMsg, 0, 256);
}


/* Did an error occur? */
KEXEC_DLLEXPORT BOOL KxcErrorOccurred(void)
{
  return (strlen(kxciLastErrorMsg) > 0);
}

/* Get the KexecCommon error message. */
KEXEC_DLLEXPORT const char* KxcGetErrorMessage(void)
{
  if (KxcErrorOccurred())
    return kxciLastErrorMsg;
  else
    return NULL;
}


/* Report an error to stderr. */
KEXEC_DLLEXPORT void KxcReportErrorStderr(void)
{
  if (KxcErrorOccurred())
    fprintf(stderr, "%s\n", kxciLastErrorMsg);
}


/* Report an error to stderr. */
KEXEC_DLLEXPORT void KxcReportErrorMsgbox(HWND parent)
{
  if (KxcErrorOccurred())
    MessageBox(parent, kxciLastErrorMsg, "KexecCommon", MB_ICONERROR | MB_OK);
}


/* Read a file into memory. */
KEXEC_DLLEXPORT PVOID KxcLoadFile(const char* filename, DWORD* length)
{
  HANDLE hFile;
  PVOID buf;
  DWORD filelen, readlen;

  /* Open it... */
  if ((hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE) {
    KxciBuildErrorMessage("Failed to load file");
    return NULL;
  }

  /* ...get the size... */
  if ((filelen = GetFileSize(hFile, NULL)) == INVALID_FILE_SIZE) {
    KxciBuildErrorMessage("Failed to get file size");
    CloseHandle(hFile);
    return NULL;
  }

  /* ...grab a buffer... */
  if ((buf = malloc(filelen)) == NULL) {
    snprintf(kxciLastErrorMsg, 255, "%s: %s", "Could not allocate buffer for file", strerror(errno));
    kxciLastErrorMsg[255] = '\0';
    CloseHandle(hFile);
    return NULL;
  }
  /* ...read it in... */
  if (!ReadFile(hFile, buf, filelen, &readlen, NULL)) {
    KxciBuildErrorMessage("Could not read file");
    CloseHandle(hFile);
    return NULL;
  }
  assert(filelen == readlen);
  /* ...and close it. */
  CloseHandle(hFile);

  *length = readlen;
  return buf;
}


/* Perform a driver operation by ioctl'ing the kexec virtual device. */
KEXEC_DLLEXPORT BOOL KxcDriverOperation(DWORD opcode, LPVOID ibuf, DWORD ibuflen, LPVOID obuf, DWORD obuflen)
{
  HANDLE dev;
  DWORD foo;
  DWORD filemode;

  KxciResetErrorMessage();

  if ((opcode & KEXEC_OPERATION_MASK) == KEXEC_SET)
    filemode = GENERIC_WRITE;
  else
    filemode = GENERIC_READ;

  /* \\.\kexec is the interface to kexec.sys. */
  if ((dev = CreateFile("\\\\.\\kexec", filemode, 0, NULL, OPEN_EXISTING, 0, NULL)) == INVALID_HANDLE_VALUE) {
    KxciBuildErrorMessage("Failed to open \\\\.\\kexec");
    return FALSE;
  }

  if (!DeviceIoControl(dev, opcode, ibuf, ibuflen, obuf, obuflen, &foo, NULL)) {
    KxciBuildErrorMessage("Driver operation failed");
    CloseHandle(dev);
    return FALSE;
  }

  CloseHandle(dev);
  return TRUE;
}


/* Is the kexec driver loaded? */
KEXEC_DLLEXPORT BOOL KxcIsDriverLoaded(void)
{
  SC_HANDLE Scm;
  SC_HANDLE KexecService;
  SERVICE_STATUS_PROCESS ServiceStatus;
  BOOL retval;
  DWORD ExtraBytes;

  KxciResetErrorMessage();

  Scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (!Scm) {
    KxciBuildErrorMessage("Could not open SCM");
    KxcReportErrorMsgbox(NULL);
    exit(EXIT_FAILURE);
  }

  KexecService = OpenService(Scm, "kexec", SERVICE_ALL_ACCESS);
  if (!KexecService) {
    KxciBuildErrorMessage("Could not open the kexec service");
    KxcReportErrorMsgbox(NULL);
    CloseServiceHandle(Scm);
    exit(EXIT_FAILURE);
  }

  if (!QueryServiceStatusEx(KexecService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ServiceStatus,
    sizeof(ServiceStatus), &ExtraBytes))
  {
    KxciBuildErrorMessage("Could not query the kexec service");
    KxcReportErrorMsgbox(NULL);
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
KEXEC_DLLEXPORT BOOL KxcLoadDriver(void)
{
  SC_HANDLE Scm;
  SC_HANDLE KexecService;

  KxciResetErrorMessage();

  if (KxcIsDriverLoaded())
    return TRUE;

  Scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (!Scm) {
    KxciBuildErrorMessage("Could not open SCM");
    return FALSE;
  }

  KexecService = OpenService(Scm, "kexec", SERVICE_ALL_ACCESS);
  if (!KexecService) {
    KxciBuildErrorMessage("Could not open the kexec service");
    CloseServiceHandle(Scm);
    return FALSE;
  }

  /* This does not return until DriverEntry() has completed in kexec.sys. */
  if (!StartService(KexecService, 0, NULL)) {
    KxciBuildErrorMessage("Could not start the kexec service");
    CloseServiceHandle(KexecService);
    CloseServiceHandle(Scm);
    return FALSE;
  }

  CloseServiceHandle(KexecService);
  CloseServiceHandle(Scm);
  return TRUE;
}


/* If kexec.sys is loaded into the kernel, unload it. */
KEXEC_DLLEXPORT BOOL KxcUnloadDriver(void)
{
  SC_HANDLE Scm;
  SC_HANDLE KexecService;
  SERVICE_STATUS ServiceStatus;

  KxciResetErrorMessage();

  if (!KxcIsDriverLoaded())
    return TRUE;

  Scm = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
  if (!Scm) {
    KxciBuildErrorMessage("Could not open SCM");
    return FALSE;
  }

  KexecService = OpenService(Scm, "kexec", SERVICE_ALL_ACCESS);
  if (!KexecService) {
    KxciBuildErrorMessage("Could not open the kexec service");
    CloseServiceHandle(Scm);
    return FALSE;
  }

  /* This does not return until DriverUnload() has completed in kexec.sys. */
  if (!ControlService(KexecService, SERVICE_CONTROL_STOP, &ServiceStatus)) {
    KxciBuildErrorMessage("Could not stop the kexec service");
    CloseServiceHandle(KexecService);
    CloseServiceHandle(Scm);
    return FALSE;
  }

  CloseServiceHandle(KexecService);
  CloseServiceHandle(Scm);
  return TRUE;
}


KEXEC_DLLEXPORT void KxcInit(void)
{
  KxciResetErrorMessage();
}

BOOL WINAPI DllMain(HINSTANCE in_hInst, DWORD reason KEXEC_UNUSED,
  LPVOID lpvReserved KEXEC_UNUSED)
{
  hInst = in_hInst;
  return TRUE;
}
