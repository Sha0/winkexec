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
   in this file, which is _start, as we link this first into the
   flat binary.  */

#include "swapptr.h"

/* The entry point.  Process the kernel map and return the base physical
   addresses of the kernel, initrd, and cmdline, as well as the size of
   the cmdline, through the pointers we are given.  */
void _start(unsigned long* kernel_base, unsigned long* initrd_base,
            unsigned long* cmdline_base, unsigned long* cmdline_size)
{
  PPTR64 kernel_start, kernel_end, initrd_start, initrd_end,
    cmdline_start, cmdline_size_ptr;
  PTR64 dest;

  /* Find the range of the kernel pointers. */
  kernel_start = kernel_end = (PPTR64)0x00010000;
  while (kernel_end->quad_part)
    kernel_end++;

  /* Find the range of the initrd pointers. */
  initrd_start = initrd_end = kernel_end + 1;
  while (initrd_end->quad_part)
    initrd_end++;

  /* Set up the cmdline pointers. */
  cmdline_start = initrd_end + 2;
  cmdline_size_ptr = initrd_end + 1;

  /* Where we're going to start to put stuff. */
  dest.quad_part = 0x0000000000100000ULL;

  /* Do the kernel. */
  *kernel_base = dest.low_part;
  while (kernel_start < kernel_end) {
    swap_ptr64s(*kernel_start, dest, kernel_start, kernel_end,
      initrd_start, initrd_end, cmdline_start);
    kernel_start++;
    dest.quad_part += 0x1000;
  }

  /* Do the initrd. */
  *initrd_base = dest.low_part;
  while (initrd_start < initrd_end) {
    swap_ptr64s(*initrd_start, dest, kernel_start, kernel_end,
      initrd_start, initrd_end, cmdline_start);
    initrd_start++;
    dest.quad_part += 0x1000;
  }

  /* Do the cmdline. */
  dest.quad_part = 0x0000000000098000ULL;
  swap_ptr64s(*cmdline_start, dest, kernel_start, kernel_end,
    initrd_start, initrd_end, cmdline_start);
  *cmdline_base = dest.low_part;
  *cmdline_size = cmdline_size_ptr->low_part;
}
