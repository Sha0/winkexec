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

/* Functions to deal with the swapping of memory areas pointed to
   by arrays of 64-bit physical pointers.  */

#include "swapptr.h"

/* From swappages.asm */
void swap_page(unsigned long p1_lower, unsigned long p1_higher,
               unsigned long p2_lower, unsigned long p2_higher);

/* Swap the memory blocks pointed to by p1 and p2 and any pointers
   that equal one of them.  */
void swap_ptr64s(PTR64 p1, PTR64 p2,
  PPTR64 kernel_start, PPTR64 kernel_end,
  PPTR64 initrd_start, PPTR64 initrd_end, PPTR64 cmdline_start)
{
  while (kernel_start < kernel_end) {
    if (kernel_start->quad_part == p1.quad_part)
      kernel_start->quad_part = p2.quad_part;
    else if (kernel_start->quad_part == p2.quad_part)
      kernel_start->quad_part = p1.quad_part;
    kernel_start++;
  }
  while (initrd_start < initrd_end) {
    if (initrd_start->quad_part == p1.quad_part)
      initrd_start->quad_part = p2.quad_part;
    else if (initrd_start->quad_part == p2.quad_part)
      initrd_start->quad_part = p1.quad_part;
    initrd_start++;
  }
  if (cmdline_start->quad_part == p1.quad_part)
    cmdline_start->quad_part = p2.quad_part;
  else if (cmdline_start->quad_part == p2.quad_part)
    cmdline_start->quad_part = p1.quad_part;
  swap_page(p1.low_part, p1.high_part, p2.low_part, p2.high_part);
}
