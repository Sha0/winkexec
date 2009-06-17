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

#define LOCK_BUFFER(buf) ExAcquireFastMutex(&((buf)->Mutex))
#define UNLOCK_BUFFER(buf) ExReleaseFastMutex(&((buf)->Mutex))

/* Buffers for the data we need to keep track of */
KEXEC_BUFFER KexecKernel;
KEXEC_BUFFER KexecInitrd;
KEXEC_BUFFER KexecKernelCommandLine;

/* Set a buffer to be empty. */
static void KexecClearBuffer(PKEXEC_BUFFER KexecBuffer)
{
  ASSERT(KeGetCurrentIrql() == APC_LEVEL);
  KexecBuffer->Size = 0;
  KexecBuffer->Data = NULL;
}

/* Free the contents (if any) of a buffer, and reinitialize it. */
static void KexecFreeBuffer(PKEXEC_BUFFER KexecBuffer)
{
  ASSERT(KeGetCurrentIrql() == APC_LEVEL);
  if (KexecBuffer->Data)
    ExFreePool(KexecBuffer->Data);
  KexecClearBuffer(KexecBuffer);
}

/* Initialize a buffer. */
void KexecInitBuffer(PKEXEC_BUFFER KexecBuffer)
{
  ExInitializeFastMutex(&KexecBuffer->Mutex);
  LOCK_BUFFER(KexecBuffer);
  KexecClearBuffer(KexecBuffer);
  UNLOCK_BUFFER(KexecBuffer);
}

/* Destroy a buffer. */
void KexecDestroyBuffer(PKEXEC_BUFFER KexecBuffer)
{
  LOCK_BUFFER(KexecBuffer);
  KexecFreeBuffer(KexecBuffer);
  UNLOCK_BUFFER(KexecBuffer);
}

/* Load data into a buffer. */
NTSTATUS KexecLoadBuffer(PKEXEC_BUFFER KexecBuffer, ULONG size, PVOID data)
{
  ULONG alloc_size;
  /* Round the size up to the nearest multiple of 4096 to ensure that
     the buffer ends up on a page boundary.  */
  alloc_size = (size + 4095) & 0xffff000;

  LOCK_BUFFER(KexecBuffer);
  KexecFreeBuffer(KexecBuffer);
  KexecBuffer->Data = ExAllocatePoolWithTag(NonPagedPool,
    alloc_size, TAG('K', 'x', 'e', 'c'));
  if (!KexecBuffer->Data) {
    UNLOCK_BUFFER(KexecBuffer);
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  KexecBuffer->Size = size;
  RtlCopyMemory(KexecBuffer->Data, data, size);
  UNLOCK_BUFFER(KexecBuffer);
  return STATUS_SUCCESS;
}

/* Retrieve data from a buffer. */
NTSTATUS KexecGetBuffer(PKEXEC_BUFFER KexecBuffer, ULONG size, PVOID buf)
{
  LOCK_BUFFER(KexecBuffer);
  if (size < KexecBuffer->Size) {
    UNLOCK_BUFFER(KexecBuffer);
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  RtlCopyMemory(buf, KexecBuffer->Data, KexecBuffer->Size);
  UNLOCK_BUFFER(KexecBuffer);
  return STATUS_SUCCESS;
}

/* Get the size of a buffer. */
ULONG KexecGetBufferSize(PKEXEC_BUFFER KexecBuffer)
{
  /* Just one value grab - no lock needed. */
  return KexecBuffer->Size;
}
