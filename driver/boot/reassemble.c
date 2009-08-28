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

/* Some code to help reassemble the kernel and initrd in memory.
   Execution of this chunk of C code will begin with the only function
   in this file, which is _reassemble_start, as we link this first into the
   flat binary.  */

#include "bootinfo.h"
#include "console.h"
#include "pagesort.h"

/* The entry point.  Takes a pointer to the boot info structure,
   a pointer to the character output function, a pointer to the
   done-paging callback, and a pointer to the int 0x15 eax=0xe820
   table.  */
void _reassemble_start(struct bootinfo* info, bios_putchar_t putch,
  doneWithPaging_t dwp, struct e820_table* mem_table)
{
  console_init(putch);

  pagesort_init(info, dwp, mem_table);

  /* Sort the kernel pages and the page tables. */
  pagesort_sort();

  /* Collapse the pages into place. */
  pagesort_collapse();

  /* Verify the hashes to make sure everything is right so far. */
  pagesort_verify();

}
