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

#define _WIN32_IE 0x0400
#include <windows.h>
#include <commctrl.h>

HINSTANCE hInst;

/* Convenient wrapper around FormatMessage() and GetLastError() */
LPSTR KexecTranslateError(void)
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

/* MessageBox with printf. */
void MessageBoxPrintf(HWND parent, LPCSTR fmtstr,
  LPCSTR title, DWORD flags, ...)
{
  char buf[256];
  va_list args;
  void WINAPI (*my_vsnprintf)(LPSTR, size_t, LPCSTR, va_list);
  HMODULE msvcrt;

  va_start(args, flags);

  /* We can't KexecPerror() these errors or we could infinitely recurse... */
  if (!(msvcrt = GetModuleHandle("msvcrt"))) {
    MessageBox(NULL, KexecTranslateError(), "Opening msvcrt.dll",
      MB_ICONERROR | MB_OK);
    exit(EXIT_FAILURE);
  }
  if (!(my_vsnprintf = GetProcAddress(msvcrt, "_vsnprintf"))) {
    MessageBox(NULL, KexecTranslateError(), "Locating _vsnprintf",
      MB_ICONERROR | MB_OK);
    exit(EXIT_FAILURE);
  }
  my_vsnprintf(buf, 255, fmtstr, args);
  buf[255] = '\0';
  MessageBox(parent, buf, title, flags);
  va_end(args);
}

/* Even more convenient wrapper around KexecTranslateError().
   Use just like perror().
   This uses an error MessageBox instead of printing to stderr.
   XXX: Does the Windows API have something like this already? */
void KexecPerror(char * errmsg)
{
  MessageBoxPrintf(NULL, "%s: %s", "WinKexec GUI", MB_ICONERROR | MB_OK,
    errmsg, KexecTranslateError());
}

int WINAPI WinMain(HINSTANCE in_hInst, HINSTANCE prev,
  LPSTR cmdline, int winstyle)
{
  INITCOMMONCONTROLSEX initComCtlEx;

  initComCtlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
  initComCtlEx.dwICC = ICC_COOL_CLASSES;
  if (!InitCommonControlsEx(&initComCtlEx)) {
    KexecPerror("InitCommonControlsEx");
    exit(EXIT_FAILURE);
  }

  hInst = in_hInst;

  MessageBox(NULL, "This program is a stump.  You can help by expanding it.",
    "WinKexec GUI", MB_ICONERROR | MB_OK);
  exit(EXIT_SUCCESS);
}
