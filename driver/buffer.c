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

#include "buffer.h"

/* Buffers for the data we need to keep track of */
KEXEC_BUFFER KexecKernel;
KEXEC_BUFFER KexecInitrd;
KEXEC_BUFFER KexecKernelCommandLine;

/* Set a buffer to be empty. */
void KexecInitBuffer(PKEXEC_BUFFER KexecBuffer)
{
  ExAcquireFastMutex(&KexecBuffer->Mutex);
  KexecBuffer->Size = 0;
  KexecBuffer->Data = NULL;
  ExReleaseFastMutex(&KexecBuffer->Mutex);
}

/* Free the contents (if any) of a buffer, and reinitialize it. */
void KexecFreeBuffer(PKEXEC_BUFFER KexecBuffer)
{
  ExAcquireFastMutex(&KexecBuffer->Mutex);
  if (KexecBuffer->Data)
    ExFreePool(KexecBuffer->Data);
  ExReleaseFastMutex(&KexecBuffer->Mutex);
  KexecInitBuffer(KexecBuffer);
}

/* Load data into a buffer. */
NTSTATUS KexecLoadBuffer(PKEXEC_BUFFER KexecBuffer, ULONG size, PVOID data)
{
  KexecFreeBuffer(KexecBuffer);
  ExAcquireFastMutex(&KexecBuffer->Mutex);
  KexecBuffer->Data = ExAllocatePoolWithTag(NonPagedPool,
    size, TAG('K', 'x', 'e', 'c'));
  if (!KexecBuffer->Data)
    return STATUS_INSUFFICIENT_RESOURCES;
  KexecBuffer->Size = size;
  RtlCopyMemory(KexecBuffer->Data, data, size);
  ExReleaseFastMutex(&KexecBuffer->Mutex);
  return STATUS_SUCCESS;
}

/* Retrieve data from a buffer. */
NTSTATUS KexecGetBuffer(PKEXEC_BUFFER KexecBuffer, ULONG size, PVOID buf)
{
  ExAcquireFastMutex(&KexecBuffer->Mutex);
  if (size < KexecBuffer->Size)
    return STATUS_INSUFFICIENT_RESOURCES;
  RtlCopyMemory(buf, KexecBuffer->Data, KexecBuffer->Size);
  ExReleaseFastMutex(&KexecBuffer->Mutex);
  return STATUS_SUCCESS;
}

/* Get the size of a buffer. */
ULONG KexecGetBufferSize(PKEXEC_BUFFER KexecBuffer)
{
  return KexecBuffer->Size;
}
