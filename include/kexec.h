/* WinKexec: kexec for Windows
 * Copyright (C) 2008 John Stumpo
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

#ifndef KEXEC_H
#define KEXEC_H

#ifdef DRIVER
#include <ddk/ntddk.h>
#else
#include <windows.h>
#include <winioctl.h>
#endif

/* Buffers to operate on */
#define KEXEC_KERNEL CTL_CODE(0, 0x000, 0, 0)
#define KEXEC_INITRD CTL_CODE(0, 0x004, 0, 0)
#define KEXEC_KERNEL_COMMAND_LINE CTL_CODE(0, 0x008, 0, 0)

/* Operations */
#define KEXEC_SET CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define KEXEC_GET_SIZE CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA)
#define KEXEC_GET CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_READ_DATA)

/* Conveniently mask off either part of the ioctl code */
#define KEXEC_BUFFER_MASK CTL_CODE(0, 0x00c, 0, 0)
#define KEXEC_OPERATION_MASK (~KEXEC_BUFFER_MASK)

/* Convenience macros */
#ifdef __GNUC__
# define KEXEC_DLLIMPORT __attribute__((__dllimport__))
# define KEXEC_DLLEXPORT __attribute__((__dllexport__))
# define KEXEC_UNUSED __attribute__((__unused__))
# define KEXEC_PACKED __attribute__((packed))
#else
# define KEXEC_DLLIMPORT __declspec(dllimport)
# define KEXEC_DLLEXPORT __declspec(dllexport)
# define KEXEC_UNUSED
# define KEXEC_PACKED
#endif

#endif
