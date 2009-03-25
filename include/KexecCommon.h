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

KEXECCOMMON_SPEC LPSTR KexecTranslateError(void);
KEXECCOMMON_SPEC void KexecPerror(char * errmsg);
KEXECCOMMON_SPEC void MessageBoxPrintf(HWND parent, LPCSTR fmtstr,
  LPCSTR title, DWORD flags, ...);
KEXECCOMMON_SPEC BOOL KexecDriverIsLoaded(void);
KEXECCOMMON_SPEC void LoadKexecDriver(void);
KEXECCOMMON_SPEC void UnloadKexecDriver(void);
KEXECCOMMON_SPEC void KexecCommonInit(BOOL in_isGui);

#endif
