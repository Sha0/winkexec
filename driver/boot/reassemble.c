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
#include "verify.h"

/* The entry point.  Takes a pointer to the boot info structure and
   a pointer to the character output function.  */
void _reassemble_start(struct bootinfo* info, bios_putchar_t putch)
{
  void* kernel_vbase;
  void* initrd_vbase;
  void* cmdline_vbase;

  kernel_vbase = (void*)0x40000000;
  initrd_vbase = (void*)((uint32_t)(kernel_vbase + info->kernel_size + 4095) & 0xfffff000) + 4096;
  cmdline_vbase = (void*)((uint32_t)(initrd_vbase + info->initrd_size + 4095) & 0xfffff000) + 4096;

  console_init(putch);

  /* Verify the hashes to make sure everything is right so far. */
  putstr("Verifying kernel integrity...\n");
  verify_hash(kernel_vbase, info->kernel_size, info->kernel_hash);
  putstr("Verifying initrd integrity...\n");
  verify_hash(initrd_vbase, info->initrd_size, info->initrd_hash);
  putstr("Verifying kernel command line integrity...\n");
  verify_hash(cmdline_vbase, info->cmdline_size, info->cmdline_hash);

  putstr("\nDereferencing null pointer to test exception handlers.\n");
  *(uint32_t*)(0x00000000) = 42;
}
