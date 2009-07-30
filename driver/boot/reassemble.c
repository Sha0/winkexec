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
#include "bootinfo.h"

/* The entry point.  Takes a pointer to the boot info structure.  */
void _start(struct bootinfo* info)
{
  /* To be continued... */
}
