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

#ifndef KEXECCOMMON_H
#define KEXECCOMMON_H

#include <windows.h>
#include <stdio.h>

#ifdef KEXEC_DLLIMPORT
# undef KEXEC_DLLIMPORT
#endif
#ifdef __GNUC__
# define KEXEC_DLLIMPORT __attribute__((__dllimport__))
#else
# define KEXEC_DLLIMPORT __declspec(dllimport)
#endif
#ifdef KEXEC_DLLEXPORT
# undef KEXEC_DLLEXPORT
#endif
#ifdef __GNUC__
# define KEXEC_DLLEXPORT __attribute__((__dllexport__))
#else
# define KEXEC_DLLEXPORT __declspec(dllexport)
#endif

KEXEC_DLLIMPORT LPSTR KexecTranslateError(void);
KEXEC_DLLIMPORT void KexecPerror(char * errmsg);
KEXEC_DLLIMPORT void MessageBoxPrintf(HWND parent, LPCSTR fmtstr,
  LPCSTR title, DWORD flags, ...);
KEXEC_DLLIMPORT BOOL KexecDriverIsLoaded(void);
KEXEC_DLLIMPORT void LoadKexecDriver(void);
KEXEC_DLLIMPORT void UnloadKexecDriver(void);
KEXEC_DLLIMPORT void KexecCommonInit(BOOL in_isGui);

#endif
