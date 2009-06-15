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

/* Some code to help reassemble the kernel and initrd in memory. */

/* From swappages.asm */
void swap_page(unsigned long p1_lower, unsigned long p1_higher,
               unsigned long p2_lower, unsigned long p2_higher);

/* Execution will begin from the first function defined in this file. */
void _start(unsigned long* kernel_base, unsigned long* initrd_base,
            unsigned long* cmdline_base, unsigned long* cmdline_size)
{
  swap_page(0x00100000, 0, 0x00101000, 0);  /* for testing */
}
