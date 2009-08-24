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

#ifndef KEXEC_DRIVER_UTIL_H
#define KEXEC_DRIVER_UTIL_H

#include <stdint.h>

void util_cli(void);
void util_hlt(void) KEXEC_NORETURN;

int util_pae_enabled(void);
uint32_t util_get_cr3(void);
void util_invlpg(uint32_t page_address);
/* Because KeGetCurrentProcessorNumber() epically fails under MinGW. */
uint32_t util_current_processor(void);
void util_int3(void);
void util_reload_cr3(void);

#endif
