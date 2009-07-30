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
/* A structure that the binary blob uses. */
#include "linuxboot_blobs/bootinfo.h"

/* Bail out of the boot process - all we can do now is BSoD... */
static void BootPanic(PCHAR msg, DWORD code1, DWORD code2,
  DWORD code3, DWORD code4)
{
  DbgPrint("kexec: *** PANIC: %s\n", msg);
  KeBugCheckEx(0x00031337, code1, code2, code3, code4);
}

/* Stash away a pointer to a block of memory for use during boot. */
static void WriteKernelPointer(DWORD** pd KEXEC_UNUSED,
  DWORD** pdpos, DWORD** pt, DWORD** ptpos, PVOID virt_addr)
{
  PHYSICAL_ADDRESS p;

  /* Allocate a new page table, if necessary. */
  if ((!*ptpos) || (*ptpos - *pt >= 1024)) {
    *pt = *ptpos = ExAllocatePoolWithTag(NonPagedPool,
      4096, TAG('K', 'x', 'e', 'c'));
    if (!*pt)
      BootPanic("Could not allocate page table!", 0x00000002,
        (DWORD)virt_addr, 0, 0);
    p = MmGetPhysicalAddress(*pt);
    *((*pdpos)++) = p.LowPart | 0x00000023;
    *((*pdpos)++) = p.HighPart;
  }

  /* Write a 64-bit pointer into the page table.
     (Note: said table will be used with PAE enabled.)  */
  if (virt_addr) {
    p = MmGetPhysicalAddress(virt_addr);
    *((*ptpos)++) = p.LowPart | 0x00000023;  /* necessary page flags */
    *((*ptpos)++) = p.HighPart;
  } else {
    *((*ptpos)++) = 0;
    *((*ptpos)++) = 0;
  }
}

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
  PHYSICAL_ADDRESS addr;
  PVOID code_dest;
  ULONG i;
  DWORD* kx_page_directory;
  DWORD* kx_pd_position;
  DWORD* kx_page_table;
  DWORD* kx_pt_position;
  struct bootinfo* info_block;

  /* Allocate the page directory for the kernel map. */
  kx_page_directory = ExAllocatePoolWithTag(NonPagedPool,
    4096, TAG('K', 'x', 'e', 'c'));
  if (!kx_page_directory)
    BootPanic("Could not allocate page directory!", 0x00000001, 0, 0, 0);
  kx_pd_position = kx_page_directory;

  kx_pt_position = 0;

#define PAGE(a) WriteKernelPointer(&kx_page_directory, &kx_pd_position, \
  &kx_page_table, &kx_pt_position, (a))

  /* Write a series of pointers to pages of the kernel,
     followed by a sentinel zero.  */
  for (i = 0; i < KexecKernel.Size; i += 4096)
    PAGE(KexecKernel.Data + i);
  PAGE(0);

  /* Same for the initrd. */
  for (i = 0; i < KexecInitrd.Size; i += 4096)
    PAGE(KexecInitrd.Data + i);
  PAGE(0);

  /* And finally the kernel command line.
     (This *will* only be a single page [much less, actually!] if our new
     kernel [not to mention the boot code!] is to not complain loudly and
     fail, but treating it like the other two data chunks we already have
     will make things simpler as far as the boot code goes.)  */
  for (i = 0; i < KexecKernelCommandLine.Size; i += 4096)
    PAGE(KexecKernelCommandLine.Data + i);
  PAGE(0);

#undef PAGE

  /* Now that the paging structures are built, we must get the
     boot code into the right place and fill in the information
     table at the end of the first page of said code.  */
  addr.QuadPart = 0x0000000000008000ULL;
  code_dest = MmMapIoSpace(addr, REALMODE_BIN_SIZE, MmNonCached);
  RtlCopyMemory(code_dest, realmode_bin, REALMODE_BIN_SIZE);
  info_block = (struct bootinfo*)(code_dest + 0x0fb0);
  info_block->kernel_size = KexecKernel.Size;
  RtlCopyMemory(info_block->kernel_hash, KexecKernel.Sha1Hash, 20);
  info_block->initrd_size = KexecInitrd.Size;
  RtlCopyMemory(info_block->initrd_hash, KexecInitrd.Sha1Hash, 20);
  info_block->cmdline_size = KexecKernelCommandLine.Size;
  RtlCopyMemory(info_block->cmdline_hash, KexecKernelCommandLine.Sha1Hash, 20);
  addr = MmGetPhysicalAddress(kx_page_directory);
  info_block->page_directory_ptr = addr.QuadPart | 0x0000000000000021ULL;
  MmUnmapIoSpace(code_dest, REALMODE_BIN_SIZE);

  /* Now we must prepare to execute the boot code.
     The most important preparation step is to identity-map the memory
     containing it - to make its physical and virtual addresses the same.
     We do this by direct manipulation of the page table.  This has to be
     done for it to be able to safely turn off paging, return to
     real mode, and do its thing.

     Needless to say, here be dragons!
   */

  /* Abandon all interrupts, ye who execute here! */
  util_cli();

  /* PAE versus non-PAE means different paging structures.
     Naturally, we will have to take that into account.
   */
  if (util_pae_enabled()) {
    /* We have PAE.
       0x00008000 = directory 0, table 0, page 8, offset 0x000
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
       0x00008000 = table 0, page 8, offset 0x000
     */
    DWORD* page_directory;
    DWORD* page_table;

    /* Where is the page directory? */
    addr.HighPart = 0x00000000;
    addr.LowPart = util_get_cr3() & 0xfffff000;
    page_directory = MmMapIoSpace(addr, 4096, MmNonCached);

    /* If the page table isn't present, use
       the next page below the boot code.  */
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
  KIRQL irql;

  /* Fork-bomb all but the first processor.
     To do that, we create a thread that calls this function again.  */
  PsCreateSystemThread(&hThread, GENERIC_ALL, 0, NULL,
    NULL, (PKSTART_ROUTINE)KexecThreadProc, NULL);
  ZwClose(hThread);

  /* Prevent thread switching on this processor. */
  KeRaiseIrql(DISPATCH_LEVEL, &irql);

  currentProcessor = util_current_processor();
  DbgPrint("KexecThreadProc() entered on processor #%d.\n", currentProcessor);

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
