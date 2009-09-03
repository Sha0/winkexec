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

#ifndef PAGESORT_H
#define PAGESORT_H

#include "bootinfo.h"

typedef void(*doneWithPaging_t)(void);

void pagesort_init(struct bootinfo* info, doneWithPaging_t dwp, struct e820_table* mem_table);
void pagesort_verify(void);
void pagesort_sort(void);
void pagesort_collapse(void);
void pagesort_prepare_for_boot(void);

#endif
