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

#define _WIN32_WINNT 0x0500
#define _WIN32_IE 0x0500

#include <windows.h>
#include <commctrl.h>

#include <KexecCommon.h>
#include <kexec.h>

#include "resource.h"

#define OFNBUFSIZE 1024

static HINSTANCE hInst;
static BOOL ctlButtonLoadsDriver;


/* Update the state of the driver-related items in the main dialog. */
static void KxgUpdateDriverState(HWND hDlg)
{
  DWORD klen, ilen;
  unsigned char buf[256];

  if (KxcIsDriverLoaded()) {
    SetDlgItemText(hDlg, KXG_ID_CONTROL_DRIVER, "Unload driver");
    if (!KxcDriverOperation(KEXEC_GET_SIZE | KEXEC_KERNEL, NULL, 0, &klen, sizeof(DWORD))) {
      KxcReportErrorMsgbox(hDlg);
      klen = 0;
    }
    if (klen == 0)
      strcpy(buf, "The kexec driver is active, but no kernel is loaded.");
    else {
      if (!KxcDriverOperation(KEXEC_GET_SIZE | KEXEC_INITRD, NULL, 0, &ilen, sizeof(DWORD))) {
        KxcReportErrorMsgbox(hDlg);
        ilen = 0;
      }
      if (ilen == 0)
        sprintf(buf, "Driver active with %lu-byte kernel and no initrd.", klen);
      else
        sprintf(buf, "Driver active with %lu-byte kernel and %lu-byte initrd.", klen, ilen);
    }
    SetDlgItemText(hDlg, KXG_ID_STATUS_TEXT, buf);
    EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
    ctlButtonLoadsDriver = FALSE;
  } else {
    SetDlgItemText(hDlg, KXG_ID_CONTROL_DRIVER, "Load driver");
    SetDlgItemText(hDlg, KXG_ID_STATUS_TEXT, "The kexec driver is not loaded.");
    EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
    ctlButtonLoadsDriver = TRUE;
  }
}


/* Apply the settings given in the dialog. */
static void KxgApplySettings(HWND hDlg)
{
  unsigned char buf[1024];
  PVOID kbuf = NULL;
  PVOID ibuf = NULL;
  DWORD klen, ilen;

  /* Read the kernel into a buffer. */
  if (IsDlgButtonChecked(hDlg, KXG_ID_KERNEL_SWITCH)) {
    GetDlgItemText(hDlg, KXG_ID_KERNEL_FILENAME, buf, 1024);
    SetDlgItemText(hDlg, KXG_ID_STATUS_TEXT, "Reading kernel... ");
    if (!(kbuf = KxcLoadFile(buf, &klen))) {
      KxcReportErrorMsgbox(hDlg);
      goto end;
    }

#if 0
    /* Magic numbers in a Linux kernel image */
    if (*(unsigned short*)(kbuf+510) != 0xaa55 ||
      strncmp(kbuf+514, "HdrS", 4) != 0)
    {
      fprintf(stderr, "warning: This does not look like a Linux kernel.\n");
      fprintf(stderr, "warning: Loading it anyway.\n");
    }
#endif
  }

  /* Read the initrd into a buffer. */
  if (IsDlgButtonChecked(hDlg, KXG_ID_INITRD_SWITCH)) {
    GetDlgItemText(hDlg, KXG_ID_INITRD_FILENAME, buf, 1024);
    SetDlgItemText(hDlg, KXG_ID_STATUS_TEXT, "Reading initrd... ");
    if (!(ibuf = KxcLoadFile(buf, &ilen))) {
      KxcReportErrorMsgbox(hDlg);
      goto end;
    }
  }

  /* Now let kexec.sys know about it.
     Do the kernel... */
  SetDlgItemText(hDlg, KXG_ID_STATUS_TEXT, "Loading kernel into kexec driver... ");
  if (!KxcDriverOperation(KEXEC_SET | KEXEC_KERNEL, kbuf, klen, NULL, 0)) {
    KxcReportErrorMsgbox(hDlg);
    goto end;
  }

  /* ...and the initrd. */
  if (ibuf) {
    SetDlgItemText(hDlg, KXG_ID_STATUS_TEXT, "Loading initrd into kexec driver... ");
    if (!KxcDriverOperation(KEXEC_SET | KEXEC_INITRD, ibuf, ilen, NULL, 0)) {
      KxcReportErrorMsgbox(hDlg);
      goto end;
    }
  } else {
    SetDlgItemText(hDlg, KXG_ID_STATUS_TEXT, "Setting null initrd... ");
    if (!KxcDriverOperation(KEXEC_SET | KEXEC_INITRD, NULL, 0, NULL, 0)) {
      KxcReportErrorMsgbox(hDlg);
      goto end;
    }
  }

  /* Set the kernel command line. */
  GetDlgItemText(hDlg, KXG_ID_KERNEL_COMMAND_LINE, buf, 1024);
  SetDlgItemText(hDlg, KXG_ID_STATUS_TEXT, "Setting kernel command line... ");
  if (!KxcDriverOperation(KEXEC_SET | KEXEC_KERNEL_COMMAND_LINE, buf, strlen(buf), NULL, 0)) {
    KxcReportErrorMsgbox(hDlg);
    goto end;
  }

  KxgUpdateDriverState(hDlg);
  if (kbuf)
    MessageBox(hDlg, "Application successful.\n\n"
      "To boot into the newly loaded kernel, reboot Windows as you normally would.",
      "WinKexec GUI", MB_ICONINFORMATION | MB_OK);
  else
    MessageBox(hDlg, "Application successful.", "WinKexec GUI", MB_ICONINFORMATION | MB_OK);

end:
  if (ibuf)
    free(ibuf);
  if (kbuf)
    free(kbuf);
  KxgUpdateDriverState(hDlg);

}

/* The processing routine for the main dialog. */
static BOOL CALLBACK KxgMainDlgProc(HWND hDlg, UINT msg,
  WPARAM wParam, LPARAM lParam)
{
  HANDLE bigIcon, smallIcon;
  HWND hCtl;
  UINT newState;
  OPENFILENAME ofn;
  unsigned char ofnbuf[OFNBUFSIZE];
  DWORD cmdlen;
  unsigned char* cmdbuf;

  switch (msg) {
    case WM_INITDIALOG:
      /* The dialog is being created; set the proper icon. */
      bigIcon = LoadImage(hInst, MAKEINTRESOURCE(KXG_ICON),
        IMAGE_ICON, GetSystemMetrics(SM_CXICON),
        GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR | LR_SHARED);
      SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)bigIcon);

      smallIcon = LoadImage(hInst, MAKEINTRESOURCE(KXG_ICON),
        IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR | LR_SHARED);
      SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)smallIcon);

      /* Set some initial state. */
      KxgUpdateDriverState(hDlg);
      if (KxcIsDriverLoaded()) {
        /* Populate the kernel command line field with the existing kernel command line. */
        if (!KxcDriverOperation(KEXEC_GET_SIZE | KEXEC_KERNEL_COMMAND_LINE, NULL, 0, &cmdlen, sizeof(DWORD))) {
          KxcReportErrorMsgbox(NULL);
        } else {
          if (cmdlen > 0) {
            if (!(cmdbuf = malloc(cmdlen + 1))) {
              MessageBox(NULL, "malloc failure", "KexecGui", MB_ICONERROR | MB_OK);
              exit(EXIT_FAILURE);
            }
            if (!KxcDriverOperation(KEXEC_GET | KEXEC_KERNEL_COMMAND_LINE, NULL, 0, cmdbuf, cmdlen)) {
              KxcReportErrorMsgbox(NULL);
            } else {
              cmdbuf[cmdlen] = '\0';
              SetDlgItemText(hDlg, KXG_ID_KERNEL_COMMAND_LINE, cmdbuf);
            }
            free(cmdbuf);
          }
        }
      }

      break;

    case WM_COMMAND:
      hCtl = (HWND)lParam;
      switch (LOWORD(wParam)) {
        case IDOK:
          /* Apply button was pressed. */
          KxgApplySettings(hDlg);
          break;

        case IDCANCEL:
          /* Close button was pressed. */
          PostQuitMessage(0);
          break;

        case KXG_ID_KERNEL_SWITCH:
          /* Kernel checkbox was toggled. */
          newState = IsDlgButtonChecked(hDlg, KXG_ID_KERNEL_SWITCH);
          EnableWindow(GetDlgItem(hDlg, KXG_ID_KERNEL_FILENAME), newState);
          EnableWindow(GetDlgItem(hDlg, KXG_ID_KERNEL_BROWSE), newState);
          EnableWindow(GetDlgItem(hDlg, KXG_ID_KERNEL_COMMAND_LINE), newState);
          EnableWindow(GetDlgItem(hDlg, KXG_ID_INITRD_SWITCH), newState);
          break;

        case KXG_ID_INITRD_SWITCH:
          /* Initrd checkbox was toggled. */
          newState = IsDlgButtonChecked(hDlg, KXG_ID_INITRD_SWITCH);
          EnableWindow(GetDlgItem(hDlg, KXG_ID_INITRD_FILENAME), newState);
          EnableWindow(GetDlgItem(hDlg, KXG_ID_INITRD_BROWSE), newState);
          break;

        case KXG_ID_KERNEL_BROWSE:
          /* Kernel "Browse..." button was pressed. */
          ofn.lStructSize = sizeof(OPENFILENAME);
          ofn.hwndOwner = hDlg;
          ofn.lpstrFilter = "Linux kernels (*.lkrn, *bzImage*, *vmlinu?*, *.bzi)\0*.lkrn;*bzImage*;*vmlinu?*;*.bzi\0All Files\0*.*\0";
          ofn.lpstrCustomFilter = NULL;
          ofn.nFilterIndex = 1;
          ofn.lpstrFile = ofnbuf;
          GetDlgItemText(hDlg, KXG_ID_KERNEL_FILENAME, ofnbuf, OFNBUFSIZE);
          ofn.nMaxFile = OFNBUFSIZE;
          ofn.lpstrFileTitle = NULL;
          ofn.lpstrInitialDir = NULL;
          ofn.lpstrTitle = "Select Linux kernel";
          ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
          ofn.lpstrDefExt = NULL;
          if (GetOpenFileName(&ofn))
            SetDlgItemText(hDlg, KXG_ID_KERNEL_FILENAME, ofnbuf);

          break;

        case KXG_ID_INITRD_BROWSE:
          /* Initrd "Browse..." button was pressed. */
          ofn.lStructSize = sizeof(OPENFILENAME);
          ofn.hwndOwner = hDlg;
          ofn.lpstrFilter = "Initrd images (*.img, *.gz)\0*.img;*.gz\0All Files\0*.*\0";
          ofn.lpstrCustomFilter = NULL;
          ofn.nFilterIndex = 1;
          ofn.lpstrFile = ofnbuf;
          GetDlgItemText(hDlg, KXG_ID_INITRD_FILENAME, ofnbuf, OFNBUFSIZE);
          ofn.nMaxFile = OFNBUFSIZE;
          ofn.lpstrFileTitle = NULL;
          ofn.lpstrInitialDir = NULL;
          ofn.lpstrTitle = "Select initial root disk image";
          ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
          ofn.lpstrDefExt = NULL;
          if (GetOpenFileName(&ofn))
            SetDlgItemText(hDlg, KXG_ID_INITRD_FILENAME, ofnbuf);

          break;

        case KXG_ID_CONTROL_DRIVER:
          /* (Un)load driver button was pressed. */
          if (ctlButtonLoadsDriver)
            KxcLoadDriver();
          else
            KxcUnloadDriver();

          if (KxcErrorOccurred())
            KxcReportErrorMsgbox(hDlg);

          KxgUpdateDriverState(hDlg);

          break;

        case KXG_ID_HELP_ABOUT:
          /* "About WinKexec..." option in the Help menu */
          KxcAboutWinKexec(hDlg);
          break;

        default:
          return FALSE;
      }
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
  KxcIsDriverLoaded();

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
  hDlg = CreateDialog(hInst, MAKEINTRESOURCE(KXG_MAIN_DLG),
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
