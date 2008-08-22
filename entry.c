#include <ntddk.h>
#include "kexec.h"

NTSTATUS DDKAPI KexecOpen(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return STATUS_SUCCESS;
}

NTSTATUS DDKAPI KexecClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return STATUS_SUCCESS;
}

NTSTATUS DDKAPI KexecIoctl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
  PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
  NTSTATUS status;

  status = STATUS_SUCCESS;

  switch (IrpStack->Parameters.DeviceIoControl.IoControlCode) {
    case KEXEC_BSOD:
      KeBugCheckEx(0x13371337, 0x13371337, 0x13371337, 0x13371337, 0x13371337);
      break;
    default:
      status = STATUS_INVALID_PARAMETER;
      break;
  }

  Irp->IoStatus.Status = status;
  Irp->IoStatus.Information = 0;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return status;
}

void DDKAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{
  UNICODE_STRING SymlinkName;

  DbgPrint("Unloading kexec driver\n");

  RtlInitUnicodeString(&SymlinkName, L"\\??\\kexec");
  IoDeleteSymbolicLink(&SymlinkName);
  IoDeleteDevice(DriverObject->DeviceObject);

  return;
}

NTSTATUS DDKAPI DriverEntry(PDRIVER_OBJECT DriverObject,
  PUNICODE_STRING RegistryPath)
{
  NTSTATUS status;
  UNICODE_STRING DeviceName;
  UNICODE_STRING SymlinkName;
  PDEVICE_OBJECT DeviceObject;

  DbgPrint("Loading kexec driver\n");

  DriverObject->DriverUnload = DriverUnload;

  RtlInitUnicodeString(&DeviceName, L"\\Device\\Kexec");
  RtlInitUnicodeString(&SymlinkName, L"\\??\\kexec");

  status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN,
    0, FALSE, &DeviceObject);
  if (NT_SUCCESS(status)) {
    DriverObject->MajorFunction[IRP_MJ_CREATE] = KexecOpen;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = KexecClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = KexecIoctl;
    status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
    if (!NT_SUCCESS(status))
      IoDeleteDevice(DeviceObject);
  }

  return status;
}
