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

/* Hook system reboot, and define the stuff needed to handle it. */

#include "libpe.h"
#include "linuxboot.h"
#include "reboot.h"
#include "buffer.h"

/* NOTE: This is undocumented! */
typedef enum _FIRMWARE_REENTRY {
  HalHaltRoutine,
  HalPowerDownRoutine,
  HalRestartRoutine,
  HalRebootRoutine,
  HalInteractiveModeRoutine,
  HalMaximumRoutine,
} FIRMWARE_REENTRY, *PFIRMWARE_REENTRY;

typedef VOID NTAPI(*halReturnToFirmware_t)(FIRMWARE_REENTRY);

static halReturnToFirmware_t real_HalReturnToFirmware;

/* Our "enhanced" version of HalReturnToFirmware.
   Drops through if we don't have a kernel to load or if an invalid
   operation type is specified.  The guts of ntoskrnl.exe will be
   tricked into calling this after everything is ready for "reboot."  */
static VOID NTAPI KexecDoReboot(FIRMWARE_REENTRY RebootType)
{
  if (RebootType == HalRebootRoutine && KexecGetBufferSize(&KexecKernel))
    KexecLinuxBoot();
  else
    real_HalReturnToFirmware(RebootType);

  /* Should never happen. */
  KeBugCheckEx(0x42424242, 0x42424242, 0x42424242, 0x42424242, 0x42424242);
}

NTSTATUS KexecHookReboot(void)
{
  PVOID Ntoskrnl = NULL;
  PVOID KernelBase;
  DWORD ImportOffset;
  halReturnToFirmware_t* Target;
  PMDL Mdl;
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

  ExFreePool(Ntoskrnl);

  Target = (halReturnToFirmware_t*)(KernelBase + ImportOffset);

  /* Here we go!
     We need to unprotect the chunk of RAM the import table is in. */
  if (!(Mdl = IoAllocateMdl(Target, sizeof(halReturnToFirmware_t), FALSE, FALSE, NULL)))
    return STATUS_UNSUCCESSFUL;
  MmBuildMdlForNonPagedPool(Mdl);
  Mdl->MdlFlags |= MDL_MAPPED_TO_SYSTEM_VA;
  if (!(Target = MmMapLockedPagesSpecifyCache(Mdl, KernelMode, MmNonCached,
    NULL, FALSE, HighPagePriority)))
  {
    IoFreeMdl(Mdl);
    return STATUS_UNSUCCESSFUL;
  }
  real_HalReturnToFirmware = *Target;
  *Target = KexecDoReboot;  /* This is it! */
  MmUnmapLockedPages(Target, Mdl);
  IoFreeMdl(Mdl);

  return STATUS_SUCCESS;
}
