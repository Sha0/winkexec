#include <ntddk.h>

void DDKAPI DriverUnload(PDRIVER_OBJECT DriverObject)
{
  DbgPrint("Unloading kexec driver\n");
  return;
}

NTSTATUS DDKAPI DriverEntry(PDRIVER_OBJECT DriverObject,
  PUNICODE_STRING RegistryPath)
{
  DbgPrint("Loading kexec driver\n");

  DriverObject->DriverUnload = DriverUnload;
  DbgPrint("Hello World\n");

  return STATUS_SUCCESS;
}
