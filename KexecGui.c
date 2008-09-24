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

#include "KexecGuiResources.h"

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

void KexecThisProgramIsAStump(void)
{
  MessageBox(NULL, "This program is a stump.  You can help by expanding it.",
    "WinKexec GUI", MB_ICONERROR | MB_OK);
}

/* The processing routine for the main dialog. */
BOOL CALLBACK KexecGuiMainDlgProc(HWND hDlg, UINT msg,
  WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
    case WM_INITDIALOG:
      KexecThisProgramIsAStump();
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      break;
    case WM_CLOSE:
      DestroyWindow(hDlg);
      break;
    default:
      return FALSE;
  }
  return TRUE;
}

/* The entry point. */
int WINAPI WinMain(HINSTANCE in_hInst, HINSTANCE prev,
  LPSTR cmdline, int winstyle)
{
  INITCOMMONCONTROLSEX initComCtlEx;
  HWND hDlg;
  MSG msg;
  DWORD status;

  /* Go XP style. */
  initComCtlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
  initComCtlEx.dwICC = ICC_COOL_CLASSES;
  if (!InitCommonControlsEx(&initComCtlEx)) {
    KexecPerror("InitCommonControlsEx");
    exit(EXIT_FAILURE);
  }

  /* Set our hInstance aside for a rainy day. */
  hInst = in_hInst;

  /* Load the main window. */
  hDlg = CreateDialog(hInst, MAKEINTRESOURCE(KEXEC_GUI_MAIN_DLG),
    0, KexecGuiMainDlgProc);
  if (!hDlg) {
    KexecPerror("CreateDialog");
    exit(EXIT_FAILURE);
  }

  /* Now for the main loop. */
  while ((status = GetMessage(&msg, 0, 0, 0)) > 0) {
    if (!IsDialogMessage(hDlg, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return msg.wParam;
}
