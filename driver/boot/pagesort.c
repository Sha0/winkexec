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

#include "pagesort.h"
#include "verify.h"
#include "console.h"
#include "string.h"
#include "stdlib.h"
#include "../util.h"

static void* kernel_vbase;
static void* initrd_vbase;
static void* cmdline_vbase;
static struct bootinfo* boot_info;
static uint64_t* kmap_pagedir;
static uint64_t* kmap_pagedir_end;
static uint64_t* kmap_pagetables;
static uint64_t* kmap_pagetables_end;
static void* scratch_page;

#ifdef DEBUG
#define DEBUG_OUTPUT(a) putstr(#a " = "); puthex((uint32_t)a); putchar('\n');
#else
#define DEBUG_OUTPUT(a) /*nothing*/
#endif

void pagesort_init(struct bootinfo* info)
{
  kernel_vbase = (void*)0x40000000;
  initrd_vbase = (void*)((uint32_t)(kernel_vbase + info->kernel_size + 4095) & 0xfffff000) + 4096;
  cmdline_vbase = (void*)((uint32_t)(initrd_vbase + info->initrd_size + 4095) & 0xfffff000) + 4096;
  kmap_pagetables = (uint64_t*)0x3fe00000;
  kmap_pagetables_end = kmap_pagetables + ((cmdline_vbase - kernel_vbase) / 4096) + 1;
  kmap_pagedir = (uint64_t*)0x001ff000;
  kmap_pagedir_end = kmap_pagedir + ((kmap_pagetables_end - kmap_pagetables + 4095) / 4096);
  scratch_page = (void*)0x00005000;

  DEBUG_OUTPUT(kernel_vbase);
  DEBUG_OUTPUT(initrd_vbase);
  DEBUG_OUTPUT(cmdline_vbase);
  DEBUG_OUTPUT(kmap_pagetables);
  DEBUG_OUTPUT(kmap_pagetables_end);
  DEBUG_OUTPUT(kmap_pagedir);
  DEBUG_OUTPUT(kmap_pagedir_end);
  DEBUG_OUTPUT(scratch_page);

  boot_info = info;
}


/* Verify the SHA1 hashes as an internal consistency check. */
void pagesort_verify(void)
{
  putstr("Verifying kernel integrity...\n");
  verify_hash(kernel_vbase, boot_info->kernel_size, boot_info->kernel_hash);
  putstr("Verifying initrd integrity...\n");
  verify_hash(initrd_vbase, boot_info->initrd_size, boot_info->initrd_hash);
  putstr("Verifying kernel command line integrity...\n");
  verify_hash(cmdline_vbase, boot_info->cmdline_size, boot_info->cmdline_hash);
}


/* Convert a pointer into the page tables to the
   corresponding virtual address.  */
static void* pagesort_convert_ptr(const uint64_t* p)
{
  if (p >= kmap_pagetables)
    return kernel_vbase + (4096 * (p - kmap_pagetables));
  else if (p >= kmap_pagedir)
    return kmap_pagetables + (4096 * (p - kmap_pagedir));
  else
    return NULL;
}


/* Sort the kernel pages and page tables so that all of the pages are in
   order starting at 0x00100000, followed by the page tables.  This will
   make it safe to just copy the kernel pages into their final places in
   increasing order without having to worry about grinding our feet over
   not-yet-done pages.  The order of the page tables doesn't matter.

   Note that our idea of "in order" starts at 0x00100000 and goes up to
   the top of physical memory, then wraps around to the beginning of
   memory and continues up to strictly less than 0x00100000.

   Since swapping pages is far more expensive an operation than checking
   the pointers, we will use selection sort.  */
void pagesort_sort(void)
{
  uint64_t* pos;
  uint64_t* lowest;
  uint64_t* x;
  void* virt_a;
  void* virt_b;

  putstr("Sorting pages...\n");

  for (pos = kmap_pagetables; pos < kmap_pagetables_end; pos++) {
    if (!*pos)
      continue;  /* leave the null separators as-is */

    lowest = pos;
    for (x = pos; x < kmap_pagetables_end; x++)
      if (*x && !((*x < *lowest) ^ !((*x < 0x00100000) ^ (*lowest < 0x00100000))))
        lowest = x;
    for (x = kmap_pagedir; x < kmap_pagedir_end; x++)
      if (*x && !((*x < *lowest) ^ !((*x < 0x00100000) ^ (*lowest < 0x00100000))))
        lowest = x;

    if (pos != lowest) {
      /* Page swapping time! */
      virt_a = pagesort_convert_ptr(pos);
      virt_b = pagesort_convert_ptr(lowest);

#if 0
      putstr("Swapping ");
      puthex((uint32_t)virt_a);
      putstr(" (phys=");
      puthex(*(uint32_t*)pos);
      putstr(") with ");
      puthex((uint32_t)virt_b);
      putstr(" (phys=");
      puthex(*(uint32_t*)lowest);
      putstr(")\n");
#endif

      /* Copy and remap A to the scratch page while
         mapping 0x00100000 to the original location of A.  */
      memcpy(scratch_page, virt_a, 4096);
      *(uint64_t*)0x00007800 = *pos;
      *pos = 0x0000000000005023ULL;
      util_invlpg((uint32_t)virt_a);
      util_invlpg(0x00100000);

      /* Copy and remap B to the original location of A while
         mapping 0x00101000 to the original location of B.  */
      memcpy((void*)0x00100000, virt_b, 4096);
      *(uint64_t*)0x00007808 = *lowest;
      *lowest = *(uint64_t*)0x00007800;
      util_invlpg((uint32_t)virt_b);
      util_invlpg(0x00101000);

      /* Copy the scratch page to the original location of B and
         remap A to the original location of B.  */
      memcpy((void*)0x00101000, scratch_page, 4096);
      *pos = *(uint64_t*)0x00007808;
      util_invlpg((uint32_t)virt_a);

      /* Unmap the temporary pages. */
      *(uint64_t*)0x00007800 = *(uint64_t*)0x00007808 = 0;
      util_invlpg(0x00100000);
      util_invlpg(0x00101000);

    }

  }

  /* Verify that the pages are in the correct order. */
  x = 0;
  for (pos = kmap_pagetables; pos < kmap_pagetables_end; pos++) {
#if 0
    putstr("*(uint32_t*)pos = ");
    puthex(*(uint32_t*)pos);
    putchar('\n');
#endif
    if (!*pos)
      continue;
    if (x == 0) {
      x = pos;
      continue;
    } else if (*pos >= 0x00100000) {
      if (*pos <= *x || *x < 0x00100000) {
        putstr("Inconsistency detected after sort.\n");
        abort();
      }
    } else {
      if (*pos <= *x && *x < 0x00100000) {
        putstr("Inconsistency detected after sort.\n");
        abort();
      }
    }
    x = pos;
  }

  /* Verify that all page tables are after the last kernel page. */
  for (pos = kmap_pagedir; pos < kmap_pagedir_end; pos++) {
#if 0
    putstr("*(uint32_t*)pos = ");
    puthex(*(uint32_t*)pos);
    putchar('\n');
#endif
    if (*pos >= 0x00100000) {
      if (*pos <= *x || *x < 0x00100000) {
        putstr("Inconsistency detected after sort.\n");
        abort();
      }
    } else {
      if (*pos <= *x && *x < 0x00100000) {
        putstr("Inconsistency detected after sort.\n");
        abort();
      }
    }
    /* Don't set x to pos; we don't care about the order,
       as long as it's all after the last kernel page.  */
  }

}


/* Collapse the sorted pages into place in memory at 0x00100000. */
void pagesort_collapse(void)
{
  uint32_t pdpt_addr;
  uint64_t* pos;
  uint32_t dest;

  /* First, move the kernel page directory to the scratch page
     so we don't accidentally hit it.  */
  memcpy(scratch_page, kmap_pagedir, 4096);

  /* AT&T syntax makes me vomit. */
  pdpt_addr = util_get_cr3();
  *(uint64_t*)(pdpt_addr + 8) = (uint64_t)((uint32_t)scratch_page | 0x00000001);
  /* Force the PDPT to be reloaded by reloading cr3. */
  util_reload_cr3();

  kmap_pagedir = scratch_page;

  putstr("Moving pages to contiguous memory...\n");

  dest = 0x00100000;
  for (pos = kmap_pagetables; pos < kmap_pagetables_end; pos++) {
    if (!*pos) {
      dest += 4096;
      continue;  /* leave the null separators as-is */
    }

#if 0
    putstr("Moving ");
    puthex(*(uint32_t*)pos);
    putstr(" to ");
    puthex(dest);
    putstr(".\n");
#endif

    /* Map 0x00100000 to the destination. */
    *(uint64_t*)0x00007800 = dest | 0x0000000000000023ULL;
    util_invlpg(0x00100000);

    /* Copy the page to the destination. */
    memcpy((void*)0x00100000, pagesort_convert_ptr(pos), 4096);

    /* Remap the page to the destination. */
    *pos = *(uint64_t*)0x00007800;
    util_invlpg((uint32_t)pagesort_convert_ptr(pos));

    /* Unmap 0x00100000. */
    *(uint64_t*)0x00007800 = 0;
    util_invlpg(0x00100000);

    dest += 4096;
  }

}
