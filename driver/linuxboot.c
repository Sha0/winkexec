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

#include "linuxboot.h"
#include "buffer.h"
#include "util.h"  /* assembly routines - we avoid inline assembly
                      in case we try to port to another compiler.  */

/* A binary blob that we are going to need -
   namely, the boot code that will finish the job for us.  */
#include "linuxboot_blobs/realmode.h"

/* By the time this is called, Windows thinks it just handed control over
   to the BIOS to reboot the computer.  As a result, nothing is really going
   on, and we can take great liberties (such as hard-coding physical memory
   addresses) in taking over and booting Linux.

   Does not return.
 */
static void DoLinuxBoot(void)
{
  /* Here's how things are going to go:
     At physical address 0x00010000, we will start building a list of the
       physical addresses of 4K pages that hold the kernel, initrd, and
       kernel command line, with null pointers in between.
       (We will use 64-bit pointers just in case of PAE.)
       Maximum length: 65536 pointers (using up to address 0x00090000),
         allowing up to 256 MB total kernel+initrd size.

       (Addendum: That cake is a lie, but for now it's still the way we
        do things.  Windows seems to like to put the page directory there,
        and when we encroach upon it [by way of overwriting it with garbage]
        Bad Things(tm) tend to happen, like triple faults.  The code is
        currently being prepared to generate page tables instead for the
        kernel map.)

     At physical address 0x00008000, we will copy the code that will be used
       to escape from protected mode, set things up for the boot, and
       reassemble and boot the Linux kernel.  In this file, we refer to that
       code as the "boot code."
   */
  DWORD* kernel_map;
  DWORD* current_position;
  PHYSICAL_ADDRESS addr;
  PVOID code_dest;
  ULONG i;
  int kernel_map_length;

  /* Figure out how long the kernel map has to be. */
  kernel_map_length  = 8 * ((KexecKernel.Size + 4095) / 4096);
  kernel_map_length += 8;  /* for the terminator of the kernel list */
  kernel_map_length += 8 * ((KexecInitrd.Size + 4095) / 4096);
  kernel_map_length += 8;  /* for the terminator of the initrd list */
  kernel_map_length += 8;  /* for the kernel command line length */
  kernel_map_length += 8;  /* for the kernel command line */

  /* Bomb if it's too long. */
  if (kernel_map_length > 65536) {
    DbgPrint("kexec: *** PANIC: kernel+initrd too large!\n");
    KeBugCheckEx(0x00031337, 0x0000001, kernel_map_length, 0, 0);
  }

  /* Map the area of low physical memory that will hold the kernel map. */
  addr.QuadPart = 0x0000000000010000ULL;
  kernel_map = MmMapIoSpace(addr, 0x00080000, MmNonCached);
  current_position = kernel_map;

  /* Write a series of pointers to pages of the kernel,
     followed by a sentinel zero.  */
  for (i = 0; i < KexecKernel.Size; i += 4096) {
    PHYSICAL_ADDRESS p = MmGetPhysicalAddress(KexecKernel.Data + i);
    *(current_position++) = p.LowPart;
    *(current_position++) = p.HighPart;
  }
  *(current_position++) = 0;
  *(current_position++) = 0;

  /* Same for the initrd. */
  for (i = 0; i < KexecInitrd.Size; i += 4096) {
    PHYSICAL_ADDRESS p = MmGetPhysicalAddress(KexecInitrd.Data + i);
    *(current_position++) = p.LowPart;
    *(current_position++) = p.HighPart;
  }
  *(current_position++) = 0;
  *(current_position++) = 0;

  /* Finally, write the size of the kernel command line,
     followed by a pointer to the command line itself.  */
  *(current_position++) = KexecKernelCommandLine.Size;
  *(current_position++) = 0;
  addr = MmGetPhysicalAddress(KexecKernelCommandLine.Data);
  *(current_position++) = addr.LowPart;
  *(current_position++) = addr.HighPart;

  /* Done with the kernel map. */
  MmUnmapIoSpace(kernel_map, 0x00080000);

  /* Now that the map is built, we must get the
     boot code into the right place.  */
  addr.QuadPart = 0x0000000000008000ULL;
  code_dest = MmMapIoSpace(addr, REALMODE_BIN_SIZE, MmNonCached);
  RtlCopyMemory(code_dest, realmode_bin, REALMODE_BIN_SIZE);
  MmUnmapIoSpace(code_dest, REALMODE_BIN_SIZE);

  /* Now we must prepare to execute the boot code.
     The most important preparation step is to identity-map the memory
     containing it - to make its physical and virtual addresses the same.
     We do this by direct manipulation of the page table.  This has to be
     done for it to be able to safely turn off paging, return to
     real mode, and do its thing.

     Needless to say, here be dragons!
   */

  if (util_pae_enabled()) {
    /* We have PAE.  Stuff will be a bit different.
       0x00090000 = directory 0, table 0, page 0x90, offset 0x000
     */
    DWORD* page_directory_pointer_table;
    DWORD* page_directory;
    DWORD* page_table;

    /* Where is the page directory pointer table? */
    addr.HighPart = 0x00000000;
    addr.LowPart = util_get_cr3() & 0xfffff000;
    page_directory_pointer_table = MmMapIoSpace(addr, 4096, MmNonCached);

    /* If the page directory isn't present, use
       the second page below the boot code.  */
    if (!(page_directory_pointer_table[0] & 0x00000001)) {
      page_directory_pointer_table[0] = 0x00006000;
      page_directory_pointer_table[1] = 0x00000000;
    }
    page_directory_pointer_table[0] |= 0x00000021;
    page_directory_pointer_table[1] &= 0x7fffffff;

    /* Where is the page directory? */
    addr.HighPart = page_directory_pointer_table[1];
    addr.LowPart = page_directory_pointer_table[0] & 0xfffff000;
    page_directory = MmMapIoSpace(addr, 4096, MmNonCached);

    /* If the page table isn't present, use
       the next page below the boot code.  */
    if (!(page_directory[0] & 0x00000001)) {
      page_directory[0] = 0x00007000;
      page_directory[1] = 0x00000000;
    }
    page_directory[0] |= 0x00000023;
    page_directory[1] &= 0x7fffffff;

    /* Map the page table and tweak it to our needs. */
    addr.HighPart = page_directory[1];
    addr.LowPart = page_directory[0] & 0xfffff000;
    page_table = MmMapIoSpace(addr, 4096, MmNonCached);
    page_table[0x10] = 0x00008023;
    page_table[0x11] = 0x00000000;
    MmUnmapIoSpace(page_table, 4096);

    MmUnmapIoSpace(page_directory, 4096);
    MmUnmapIoSpace(page_directory_pointer_table, 4096);
  } else {
    /* No PAE - it's the original x86 paging mechanism.
       0x00090000 = table 0, page 0x90, offset 0x000
     */
    DWORD* page_directory;
    DWORD* page_table;

    /* Where is the page directory? */
    addr.HighPart = 0x00000000;
    addr.LowPart = util_get_cr3() & 0xfffff000;
    page_directory = MmMapIoSpace(addr, 4096, MmNonCached);

    /* If the page table isn't present, use
       the next page below the real-mode code.  */
    if (!(page_directory[0] & 0x00000001))
      page_directory[0] = 0x00007000;
    page_directory[0] |= 0x00000023;

    /* Map the page table and tweak it to our needs. */
    addr.HighPart = 0x00000000;
    addr.LowPart = page_directory[0] & 0xfffff000;
    page_table = MmMapIoSpace(addr, 4096, MmNonCached);
    page_table[0x08] = 0x00008023;
    MmUnmapIoSpace(page_table, 4096);

    MmUnmapIoSpace(page_directory, 4096);
  }

  /* Flush the page from the TLB... */
  util_invlpg(0x00008000);

  /* ...and away we go! */
  ((void (*)())0x00008000)();

  /* Should never happen. */
  KeBugCheckEx(0x42424242, 0x42424242, 0x42424242, 0x42424242, 0x42424242);
}

/* A kernel thread routine.
   We use this to bring down all but the first processor.
   Does not return.  */
static VOID KexecThreadProc(PVOID Context KEXEC_UNUSED)
{
  HANDLE hThread;
  ULONG currentProcessor;

  util_cli();  /* abandon all interrupts, ye who execute here! */
  currentProcessor = util_current_processor();

  DbgPrint("KexecThreadProc() entered on processor #%d.\n", currentProcessor);

  /* Fork-bomb all but the first processor.
     To do that, we create a thread that calls this function again.  */
  PsCreateSystemThread(&hThread, GENERIC_ALL, 0, NULL,
    NULL, (PKSTART_ROUTINE)KexecThreadProc, NULL);
  ZwClose(hThread);

  /* If we're the first processor, go ahead. */
  if (currentProcessor == 0)
    DoLinuxBoot();

  /* Otherwise, come to a screeching halt. */
  DbgPrint("kexec: killing processor #%d", currentProcessor);
  util_cli();
  util_hlt();
}

/* Initiate the Linux boot process.
   Does not return.  */
void KexecLinuxBoot(void)
{
  KexecThreadProc(NULL);
}
