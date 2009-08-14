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
 *
 *
 * Some code for the implementation of the "About WinKexec" dialog box
 * (specifically, the code supporting the hyperlink to the home page) is
 * derived from the nsDialogs plugin for NSIS.
 *
 * Copyright (C) 1995-2009 Contributors
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 *   1. The origin of this software must not be misrepresented; you must
 *      not claim that you wrote the original software. If you use this
 *      software in a product, an acknowledgment in the product
 *      documentation would be appreciated but is not required.
 *
 *   2. Altered source versions must be plainly marked as such, and must
 *      not be misrepresented as being the original software.
 *
 *   3. This notice may not be removed or altered from any source
 *      distribution.
 */

#define IN_KEXEC_COMMON
#include <KexecCommon.h>
#include "resource.h"

#ifndef ODS_NOFOCUSRECT
#define ODS_NOFOCUSRECT 0x0200
#endif

extern HINSTANCE hInst;


/* Window procedure for links in the "About WinKexec" dialog. */
static BOOL CALLBACK KxciLinkWndProc(HWND hWnd, UINT msg,
  WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
    case WM_SETCURSOR:
      SetCursor(LoadCursor(NULL, IDC_HAND));
      return TRUE;

    default:
      return CallWindowProc((WNDPROC)GetWindowLong(hWnd, GWL_USERDATA),
        hWnd, msg, wParam, lParam);
  }
  return TRUE;
}


/* Procedure for the "About WinKexec" dialog. */
static BOOL CALLBACK KxciAboutDlgProc(HWND hDlg, UINT msg,
  WPARAM wParam, LPARAM lParam)
{
  HWND hCtl;
  DRAWITEMSTRUCT* dis;
  char linktext[1024];
  RECT rect;
  COLORREF oldcolor;

  switch (msg) {
    case WM_INITDIALOG:
      /* Set the chained window procedure on the link. */
      hCtl = GetDlgItem(hDlg, KXC_ID_HOMEPAGE);
      SetWindowLong(hCtl, GWL_USERDATA,
        SetWindowLong(hCtl, GWL_WNDPROC, (LONG)KxciLinkWndProc));
      break;

    case WM_COMMAND:
      /* A widget was triggered. */
      hCtl = (HWND)lParam;
      switch (LOWORD(wParam)) {
        case IDCANCEL:
          EndDialog(hDlg, IDCANCEL);
          break;

        case KXC_ID_HOMEPAGE:
          ShellExecute(hDlg, "open", "http://www.jstump.com/",
            NULL, NULL, SW_SHOW);
          break;

        default:
          return FALSE;
      }
      break;

    case WM_DRAWITEM:
      /* The custom drawing routine for the hyperlinks. */
      dis = (DRAWITEMSTRUCT*)lParam;
      rect = dis->rcItem;
      linktext[0] = '\0';
      if (dis->itemAction & ODA_DRAWENTIRE) {
        GetWindowText(dis->hwndItem, linktext, 1024);
        oldcolor = SetTextColor(dis->hDC, RGB(0, 0, 192));
        DrawText(dis->hDC, linktext, -1, &rect, 0);
        SetTextColor(dis->hDC, oldcolor);
      }
      if (( (dis->itemAction & ODA_FOCUS) ||
           ((dis->itemAction & ODA_DRAWENTIRE) && (dis->itemState & ODS_FOCUS))
          ) && !(dis->itemState & ODS_NOFOCUSRECT))
        DrawFocusRect(dis->hDC, &rect);
      break;

    default:
      return FALSE;
  }
  return TRUE;
}


/* Display the "About WinKexec" dialog. */
KEXEC_DLLEXPORT void KxcAboutWinKexec(HWND parent)
{
  DialogBox(hInst, MAKEINTRESOURCE(KXC_ABOUT_DLG), parent, KxciAboutDlgProc);
}
