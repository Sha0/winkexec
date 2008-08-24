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

#define KEXEC_KERNEL 0
#define KEXEC_INITRD 4
#define KEXEC_KERNEL_COMMAND_LINE 8

#define KEXEC_SET 1
#define KEXEC_GET_SIZE 2
#define KEXEC_GET 3

#define KEXEC_OPERATION_MASK 0x00000003

#endif
