#include <ntddk.h>

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
    status = IoCreateSymbolicLink(&SymlinkName, &DeviceName);
    if (!NT_SUCCESS(status))
      IoDeleteDevice(DeviceObject);
  }

  return status;
}
