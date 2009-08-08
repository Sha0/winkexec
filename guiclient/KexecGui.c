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

#define _WIN32_IE 0x0400
#include <windows.h>
#include <commctrl.h>

#include <KexecCommon.h>

#include "resource.h"

static HINSTANCE hInst;

static void KxgThisProgramIsAStump(void)
{
  /* Kyle is awesome. */
  MessageBox(NULL, "This program is a stump.  You can help by expanding it.",
    "WinKexec GUI", MB_ICONERROR | MB_OK);
}


/* The processing routine for the main dialog. */
static BOOL CALLBACK KxgMainDlgProc(HWND hDlg, UINT msg,
  WPARAM wParam, LPARAM lParam)
{
  HANDLE bigIcon, smallIcon;

  switch (msg) {
    case WM_INITDIALOG:
      KxgThisProgramIsAStump();

      bigIcon = LoadImage(hInst, MAKEINTRESOURCE(KEXEC_GUI_ICON),
        IMAGE_ICON, GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
      SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)bigIcon);

      smallIcon = LoadImage(hInst, MAKEINTRESOURCE(KEXEC_GUI_ICON),
        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
      SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);

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
int WINAPI WinMain(HINSTANCE in_hInst, HINSTANCE prev KEXEC_UNUSED,
  LPSTR cmdline KEXEC_UNUSED, int winstyle KEXEC_UNUSED)
{
  INITCOMMONCONTROLSEX initComCtlEx;
  HWND hDlg;
  MSG msg;
  DWORD status;

  KxcInit();

  /* Use styled widgets if on XP or later. */
  initComCtlEx.dwSize = sizeof(INITCOMMONCONTROLSEX);
  initComCtlEx.dwICC = ICC_COOL_CLASSES;
  if (!InitCommonControlsEx(&initComCtlEx)) {
    MessageBox(NULL, "InitCommonControlsEx failed!", "KexecGui", MB_ICONERROR | MB_OK);
    exit(EXIT_FAILURE);
  }

  /* Set our hInstance aside for a rainy day. */
  hInst = in_hInst;

  /* Load the main window. */
  hDlg = CreateDialog(hInst, MAKEINTRESOURCE(KEXEC_GUI_MAIN_DLG),
    0, KxgMainDlgProc);
  if (!hDlg) {
    MessageBox(NULL, "CreateDialog failed!", "KexecGui", MB_ICONERROR | MB_OK);
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
