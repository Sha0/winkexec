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

#include <ddk/ntddk.h>
#include "kexec.h"

typedef struct {
  ULONG Size;
  PVOID Data;
  FAST_MUTEX Mutex;
} KEXEC_BUFFER, *PKEXEC_BUFFER;

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

/* Called when \\.\kexec is opened. */
NTSTATUS DDKAPI KexecOpen(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return STATUS_SUCCESS;
}

/* Called when \\.\kexec is closed. */
NTSTATUS DDKAPI KexecClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return STATUS_SUCCESS;
}

/* Handle an ioctl^H^H^H^H^HDeviceIoControl on /dev/^H^H^H^H^H\\.\kexec */
NTSTATUS DDKAPI KexecIoctl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
  NTSTATUS status;
  ULONG IoctlCode;
  PKEXEC_BUFFER buf;

  status = STATUS_SUCCESS;

  IoctlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;

  /* Select the buffer we are operating on. */
  switch (IoctlCode & ~KEXEC_OPERATION_MASK) {
    case KEXEC_KERNEL:
      buf = &KexecKernel;
      break;
    case KEXEC_INITRD:
      buf = &KexecInitrd;
      break;
    case KEXEC_KERNEL_COMMAND_LINE:
      buf = &KexecKernelCommandLine;
      break;
    default:
      status = STATUS_INVALID_PARAMETER;
      goto end;
  }
  /* Perform the requested operation. */
  switch (IoctlCode & KEXEC_OPERATION_MASK) {
    case KEXEC_SET:
      status = KexecLoadBuffer(buf,
        IrpStack->Parameters.DeviceIoControl.InputBufferLength,
        Irp->AssociatedIrp.SystemBuffer);
      break;
    case KEXEC_GET:
      status = KexecGetBuffer(buf,
        IrpStack->Parameters.DeviceIoControl.OutputBufferLength,
        Irp->AssociatedIrp.SystemBuffer);
      break;
    case KEXEC_GET_SIZE:
      status = KexecGetBufferSize(buf);
      break;
    default:
      status = STATUS_INVALID_PARAMETER;
      goto end;
  }

  /* Return the results. */
end:
  Irp->IoStatus.Status = status;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return status;
}

/* Called just before kexec.sys is unloaded. */
void DDKAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{
  UNICODE_STRING SymlinkName;

  DbgPrint("Unloading kexec driver\n");

  /* Unregister \\.\kexec with the Windows kernel. */
  RtlInitUnicodeString(&SymlinkName, L"\\??\\kexec");
  IoDeleteSymbolicLink(&SymlinkName);
  IoDeleteDevice(DriverObject->DeviceObject);

  /* Don't waste kernel memory! */
  KexecFreeBuffer(&KexecKernel);
  KexecFreeBuffer(&KexecInitrd);
  KexecFreeBuffer(&KexecKernelCommandLine);

  return;
}

/* The entry point - this is called when kexec.sys is loaded, and it
   runs to completion before the associated userspace call to
   StartService() returns, no matter what. */
NTSTATUS DDKAPI DriverEntry(PDRIVER_OBJECT DriverObject,
  PUNICODE_STRING RegistryPath)
{
  NTSTATUS status;
  UNICODE_STRING DeviceName;
  UNICODE_STRING SymlinkName;
  PDEVICE_OBJECT DeviceObject;

  DbgPrint("Loading kexec driver\n");

  /* Allow kexec.sys to be unloaded. */
  DriverObject->DriverUnload = DriverUnload;

  /* Init the locks on the buffers. */
  ExInitializeFastMutex(&KexecKernel.Mutex);
  ExInitializeFastMutex(&KexecInitrd.Mutex);
  ExInitializeFastMutex(&KexecKernelCommandLine.Mutex);

  /* Init the buffers themselves. */
  KexecInitBuffer(&KexecKernel);
  KexecInitBuffer(&KexecInitrd);
  KexecInitBuffer(&KexecKernelCommandLine);

  RtlInitUnicodeString(&DeviceName, L"\\Device\\Kexec");
  RtlInitUnicodeString(&SymlinkName, L"\\??\\kexec");

  /* Register \\.\kexec with the Windows kernel. */
  status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN,
    0, FALSE, &DeviceObject);
  if (NT_SUCCESS(status)) {
    /* Set our handlers for I/O operations on \\.\kexec. */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = KexecOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = KexecClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KexecIoctl;
    status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
    if (!NT_SUCCESS(status))
      IoDeleteDevice(DeviceObject);
  }

  return status;
}
