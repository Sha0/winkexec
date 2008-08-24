#include <ntddk.h>
#include "kexec.h"

typedef struct {
  ULONG Size;
  PVOID Data;
  FAST_MUTEX Mutex;
} KEXEC_BUFFER, *PKEXEC_BUFFER;

KEXEC_BUFFER KexecKernel;
KEXEC_BUFFER KexecInitrd;
KEXEC_BUFFER KexecKernelCommandLine;

void KexecInitBuffer(PKEXEC_BUFFER KexecBuffer)
{
  ExAcquireFastMutex(&KexecBuffer->Mutex);
  KexecBuffer->Size = 0;
  KexecBuffer->Data = NULL;
  ExReleaseFastMutex(&KexecBuffer->Mutex);
}

void KexecFreeBuffer(PKEXEC_BUFFER KexecBuffer)
{
  ExAcquireFastMutex(&KexecBuffer->Mutex);
  if (KexecBuffer->Data)
    ExFreePool(KexecBuffer->Data);
  ExReleaseFastMutex(&KexecBuffer->Mutex);
  KexecInitBuffer(KexecBuffer);
}

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

NTSTATUS KexecGetBuffer(PKEXEC_BUFFER KexecBuffer, ULONG size, PVOID buf)
{
  ExAcquireFastMutex(&KexecBuffer->Mutex);
  if (size < KexecBuffer->Size)
    return STATUS_INSUFFICIENT_RESOURCES;
  RtlCopyMemory(buf, KexecBuffer->Data, KexecBuffer->Size);
  ExReleaseFastMutex(&KexecBuffer->Mutex);
  return STATUS_SUCCESS;
}

ULONG KexecGetBufferSize(PKEXEC_BUFFER KexecBuffer)
{
  return KexecBuffer->Size;
}

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
  ULONG IoctlCode;
  PKEXEC_BUFFER buf;

  status = STATUS_SUCCESS;

  IoctlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;

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

end:
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

  KexecFreeBuffer(&KexecKernel);
  KexecFreeBuffer(&KexecInitrd);
  KexecFreeBuffer(&KexecKernelCommandLine);

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

  ExInitializeFastMutex(&KexecKernel.Mutex);
  ExInitializeFastMutex(&KexecInitrd.Mutex);
  ExInitializeFastMutex(&KexecKernelCommandLine.Mutex);

  KexecInitBuffer(&KexecKernel);
  KexecInitBuffer(&KexecInitrd);
  KexecInitBuffer(&KexecKernelCommandLine);

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
