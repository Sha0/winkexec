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

#ifndef KEXEC_DRIVER_PE_H
#define KEXEC_DRIVER_PE_H

#include <ddk/ntddk.h>

PVOID PeReadSystemFile(PCHAR Filename);
PIMAGE_NT_HEADERS PeGetNtHeaders(PVOID PeFile);
PIMAGE_SECTION_HEADER PeGetFirstSectionHeader(PVOID PeFile);
WORD PeGetSectionCount(PVOID PeFile);
PIMAGE_SECTION_HEADER PeFindSectionHeaderForAddress(PVOID PeFile, DWORD Address);
PVOID PeConvertRva(PVOID PeFile, DWORD Rva);

#define PeForEachSectionHeader(PeFile, SectionVariable) \
  for ((SectionVariable) = PeGetFirstSectionHeader(PeFile); \
    (SectionVariable) - PeGetFirstSectionHeader(PeFile) < \
      PeGetSectionCount(PeFile); \
    (SectionVariable)++)

#endif
