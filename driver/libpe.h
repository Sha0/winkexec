/* libpe: Small library to do interesting things with PE
 *   executables from kernel mode under Windows.
 * Originally developed as part of WinKexec: kexec for Windows
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
 * This file is available under a proprietary license as well.
 * Contact John Stumpo <stump@jstump.com> for details.
 */

#ifndef __LIBPE_H
#define __LIBPE_H

#include <ddk/ntddk.h>

PVOID PeReadSystemFile(PWCHAR Filename);
PIMAGE_NT_HEADERS PeGetNtHeaders(PVOID PeFile);
PIMAGE_SECTION_HEADER PeGetFirstSectionHeader(PVOID PeFile);
WORD PeGetSectionCount(PVOID PeFile);
PIMAGE_SECTION_HEADER PeFindSectionHeaderForAddress(PVOID PeFile, DWORD Address);
PVOID PeConvertRva(PVOID PeFile, DWORD Rva);
PIMAGE_EXPORT_DIRECTORY PeGetExportDirectory(PVOID PeFile);
PIMAGE_IMPORT_DESCRIPTOR PeGetFirstImportDescriptor(PVOID PeFile);
DWORD PeGetExportFunction(PVOID PeFile, PCHAR FunctionName);
DWORD PeGetImportPointer(PVOID PeFile, PCHAR DllName, PCHAR FunctionName);

#define PeForEachSectionHeader(PeFile, SectionVariable) \
  for ((SectionVariable) = PeGetFirstSectionHeader(PeFile); \
    (SectionVariable) - PeGetFirstSectionHeader(PeFile) < \
      PeGetSectionCount(PeFile); \
    (SectionVariable)++)

#endif
