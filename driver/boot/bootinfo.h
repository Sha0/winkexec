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

/* The declaration for the information structure passed to the boot code. */

#ifndef BOOTINFO_H
#define BOOTINFO_H

#include <kexec.h>
#include <stdint.h>

struct bootinfo {
  uint32_t kernel_size;
  unsigned char kernel_hash[20];
  uint32_t initrd_size;
  unsigned char initrd_hash[20];
  uint32_t cmdline_size;
  unsigned char cmdline_hash[20];
  uint64_t page_directory_ptr;
} KEXEC_PACKED;

struct e820 {
  uint64_t base;
  uint64_t size;
  uint32_t type;
} KEXEC_PACKED;

struct e820_table {
  uint32_t count;
  struct e820 entries[];
} KEXEC_PACKED;

#endif
