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

#include <kexec.h>

#include "buffer.h"
#include "io.h"
#include "reboot.h"

/* Called when \\.\kexec is opened. */
NTSTATUS DDKAPI KexecOpen(PDEVICE_OBJECT DeviceObject KEXEC_UNUSED, PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return STATUS_SUCCESS;
}

/* Called when \\.\kexec is closed. */
NTSTATUS DDKAPI KexecClose(PDEVICE_OBJECT DeviceObject KEXEC_UNUSED, PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return STATUS_SUCCESS;
}

/* Handle an ioctl^H^H^H^H^HDeviceIoControl on /dev/^H^H^H^H^H\\.\kexec */
NTSTATUS DDKAPI KexecIoctl(PDEVICE_OBJECT DeviceObject KEXEC_UNUSED, PIRP Irp)
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

/* Called before shutdown - use this chance to hook HalReturnToFirmware() */
NTSTATUS DDKAPI KexecShutdown(PDEVICE_OBJECT DeviceObject KEXEC_UNUSED, PIRP Irp)
{
  NTSTATUS status;

  status = KexecHookReboot();

  Irp->IoStatus.Status = status;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return status;
}
