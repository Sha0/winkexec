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

#ifndef KEXEC_DRIVER_BUFFER_H
#define KEXEC_DRIVER_BUFFER_H

#include <ddk/ntddk.h>

typedef struct {
  ULONG Size;
  PVOID Data;
  FAST_MUTEX Mutex;
  unsigned char Sha1Hash[20];
} KEXEC_BUFFER, *PKEXEC_BUFFER;

extern KEXEC_BUFFER KexecKernel;
extern KEXEC_BUFFER KexecInitrd;
extern KEXEC_BUFFER KexecKernelCommandLine;

void KexecInitBuffer(PKEXEC_BUFFER KexecBuffer);
void KexecDestroyBuffer(PKEXEC_BUFFER KexecBuffer);
NTSTATUS KexecLoadBuffer(PKEXEC_BUFFER KexecBuffer, ULONG size, PVOID data);
NTSTATUS KexecGetBuffer(PKEXEC_BUFFER KexecBuffer, ULONG size, PVOID buf);
ULONG KexecGetBufferSize(PKEXEC_BUFFER KexecBuffer);

#endif
