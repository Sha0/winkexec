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

#ifndef SWAPPTR_H
#define SWAPPTR_H

typedef union _PTR64 {
  unsigned long long quad_part;
  struct {
    unsigned long low_part;
    unsigned long high_part;
  };
} PTR64, *PPTR64;

void swap_ptr64s(PTR64 p1, PTR64 p2,
  PPTR64 kernel_start, PPTR64 kernel_end,
  PPTR64 initrd_start, PPTR64 initrd_end, PPTR64 cmdline_start);

#endif
