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

#include "string.h"

int memcmp(const void* a, const void* b, size_t len)
{
  const unsigned char* c = a;
  const unsigned char* d = b;
  size_t i;

  for (i = 0; i < len; i++)
    if (c[i] != d[i])
      return c[i] - d[i];

  return 0;
}


void* memcpy(void* dest, const void* src, size_t len)
{
  const unsigned char* a = src;
  unsigned char* b = dest;
  size_t i;

  for (i = 0; i < len; i++)
    b[i] = a[i];

  return dest;
}
