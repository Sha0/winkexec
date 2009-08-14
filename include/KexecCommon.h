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

#ifndef KEXECCOMMON_H
#define KEXECCOMMON_H

#include <windows.h>
#include <stdio.h>

#include <kexec.h>

#ifdef IN_KEXEC_COMMON
# define KEXECCOMMON_SPEC KEXEC_DLLEXPORT
#else
# define KEXECCOMMON_SPEC KEXEC_DLLIMPORT
#endif

KEXECCOMMON_SPEC BOOL KxcErrorOccurred(void);
KEXECCOMMON_SPEC const char* KxcGetErrorMessage(void);
KEXECCOMMON_SPEC void KxcReportErrorStderr(void);
KEXECCOMMON_SPEC void KxcReportErrorMsgbox(HWND parent);
KEXECCOMMON_SPEC void KxcAboutWinKexec(HWND parent);
KEXECCOMMON_SPEC PVOID KxcLoadFile(const char* filename, DWORD* length);
KEXECCOMMON_SPEC BOOL KxcDriverOperation(DWORD opcode, LPVOID ibuf, DWORD ibuflen, LPVOID obuf, DWORD obuflen);
KEXECCOMMON_SPEC BOOL KxcIsDriverLoaded(void);
KEXECCOMMON_SPEC BOOL KxcLoadDriver(void);
KEXECCOMMON_SPEC BOOL KxcUnloadDriver(void);
KEXECCOMMON_SPEC void KxcInit(void);

#endif
