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

#include <ddk/ntddk.h>
#include <kexec.h>

#include "buffer.h"
#include "io.h"

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
  KexecDestroyBuffer(&KexecKernel);
  KexecDestroyBuffer(&KexecInitrd);
  KexecDestroyBuffer(&KexecKernelCommandLine);
}

/* The entry point - this is called when kexec.sys is loaded, and it
   runs to completion before the associated userspace call to
   StartService() returns, no matter what. */
NTSTATUS DDKAPI DriverEntry(PDRIVER_OBJECT DriverObject,
  PUNICODE_STRING RegistryPath KEXEC_UNUSED)
{
  NTSTATUS status;
  UNICODE_STRING DeviceName;
  UNICODE_STRING SymlinkName;
  PDEVICE_OBJECT DeviceObject;

  DbgPrint("Loading kexec driver\n");

  /* Allow kexec.sys to be unloaded. */
  DriverObject->DriverUnload = DriverUnload;

  /* Init the buffers. */
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
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN] = KexecShutdown;
    status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
    if (NT_SUCCESS(status)) {
      status = IoRegisterShutdownNotification(DeviceObject);
      if (!NT_SUCCESS(status)) {
        IoDeleteSymbolicLink(&SymlinkName);
        IoDeleteDevice(DeviceObject);
      }
    } else
      IoDeleteDevice(DeviceObject);
  }

  return status;
}
