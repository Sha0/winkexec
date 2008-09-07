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

/* Hook system reboot, and define the stuff needed to handle it. */

#include <ddk/ntddk.h>
#include "KexecDriverPe.h"
#include "KexecDriverReboot.h"

void KexecDoReboot(void)
{
  KeBugCheckEx(0x42424242, 0x42424242, 0x42424242, 0x42424242, 0x42424242);
}

NTSTATUS KexecHookReboot(void)
{
  PVOID Ntoskrnl;
  PVOID KernelBase;
  DWORD ImportOffset;
  PVOID Target;
  PMDL Mdl;
  void (*MyMmBuildMdlForNonpagedPool)(PMDL);

  if (!(Ntoskrnl = PeReadSystemFile(L"ntoskrnl.exe")))
    return STATUS_INSUFFICIENT_RESOURCES;

  /* Compute base load address of ntoskrnl.exe by doing this to
     an arbitrary function from ntoskrnl.exe. */
  KernelBase = IoCreateDevice - PeGetExportFunction(Ntoskrnl, "IoCreateDevice");
  if (KernelBase == IoCreateDevice) {
    ExFreePool(Ntoskrnl);
    return STATUS_UNSUCCESSFUL;
  }

  if (!(ImportOffset = PeGetImportPointer(Ntoskrnl, "hal.dll", "HalReturnToFirmware"))) {
    ExFreePool(Ntoskrnl);
    return STATUS_UNSUCCESSFUL;
  }

  /* Cygwin doesn't have this function... */
  MyMmBuildMdlForNonpagedPool =
    KernelBase + PeGetExportFunction(Ntoskrnl, "MmBuildMdlForNonPagedPool");
  if (MyMmBuildMdlForNonpagedPool == KernelBase) {
    ExFreePool(Ntoskrnl);
    return STATUS_UNSUCCESSFUL;
  }

  ExFreePool(Ntoskrnl);

  Target = KernelBase + ImportOffset;

  /* Here we go!
     We need to unprotect the chunk of RAM the import table is in. */
  if (!(Mdl = IoAllocateMdl(Target, sizeof(void(**)(void)), FALSE, FALSE, NULL)))
    return STATUS_UNSUCCESSFUL;
  MyMmBuildMdlForNonpagedPool(Mdl);
  Mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
  if (!(Target = MmMapLockedPagesSpecifyCache(Mdl, KernelMode, MmNonCached,
    NULL, FALSE, HighPagePriority)))
  {
    IoFreeMdl(Mdl);
    return STATUS_UNSUCCESSFUL;
  }
  *(void(**)(void))Target = KexecDoReboot;
  MmUnmapLockedPages(Target, Mdl);
  IoFreeMdl(Mdl);

  return STATUS_SUCCESS;
}
