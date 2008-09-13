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
#include "KexecDriver.h"

void KexecDoReboot(void)
{
  KexecLinuxBoot();
  /* Linux boot failed... the only sensible thing is a BSoD (if we can...)
     If KexecLinuxBoot() has messed things up thoroughly enough, we will likely
     get a triple fault here, which reboots the machine (for real) anyway... */
  KeBugCheckEx(0x42424242, 0x42424242, 0x42424242, 0x42424242, 0x42424242);
}

NTSTATUS KexecHookReboot(void)
{
  PVOID Ntoskrnl = NULL;
  PVOID KernelBase;
  DWORD ImportOffset;
  PVOID Target;
  PMDL Mdl;
  void DDKAPI (*MyMmBuildMdlForNonPagedPool)(PMDL);
  int i;
  PWCHAR KernelFilenames[] = {L"ntoskrnl.exe", L"ntkrnlpa.exe",
                              L"ntkrnlmp.exe", L"ntkrpamp.exe", NULL};

  for (i = 0; KernelFilenames[i]; i++) {
    if (!(Ntoskrnl = PeReadSystemFile(KernelFilenames[i])))
      return STATUS_INSUFFICIENT_RESOURCES;

    /* Compute base load address of ntoskrnl.exe by doing this to
       an arbitrary function from ntoskrnl.exe. */
    KernelBase = KeBugCheckEx - PeGetExportFunction(Ntoskrnl, "KeBugCheckEx");
    if (KernelBase == KeBugCheckEx) {
      ExFreePool(Ntoskrnl);
      Ntoskrnl = NULL;
      continue;
    }

    /* Make sure it's right. */
    if (!MmIsAddressValid(KernelBase) || !PeGetNtHeaders(KernelBase)) {
      ExFreePool(Ntoskrnl);
      Ntoskrnl = NULL;
      continue;
    }

    break;
  }

  if (!Ntoskrnl)
    return STATUS_UNSUCCESSFUL;

  if (!(ImportOffset = PeGetImportPointer(Ntoskrnl, "hal.dll", "HalReturnToFirmware"))) {
    ExFreePool(Ntoskrnl);
    return STATUS_UNSUCCESSFUL;
  }

  /* Cygwin doesn't have this function... */
  MyMmBuildMdlForNonPagedPool =
    KernelBase + PeGetExportFunction(Ntoskrnl, "MmBuildMdlForNonPagedPool");
  if (MyMmBuildMdlForNonPagedPool == KernelBase) {
    ExFreePool(Ntoskrnl);
    return STATUS_UNSUCCESSFUL;
  }

  ExFreePool(Ntoskrnl);

  Target = KernelBase + ImportOffset;

  /* Here we go!
     We need to unprotect the chunk of RAM the import table is in. */
  if (!(Mdl = IoAllocateMdl(Target, sizeof(void(**)(void)), FALSE, FALSE, NULL)))
    return STATUS_UNSUCCESSFUL;
  MyMmBuildMdlForNonPagedPool(Mdl);
  Mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
  if (!(Target = MmMapLockedPagesSpecifyCache(Mdl, KernelMode, MmNonCached,
    NULL, FALSE, HighPagePriority)))
  {
    IoFreeMdl(Mdl);
    return STATUS_UNSUCCESSFUL;
  }
  *(void(**)(void))Target = KexecDoReboot;  /* This is it! */
  MmUnmapLockedPages(Target, Mdl);
  IoFreeMdl(Mdl);

  return STATUS_SUCCESS;
}
