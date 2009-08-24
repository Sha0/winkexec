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

#include "verify.h"
#include "string.h"
#include "console.h"
#include "stdlib.h"
#include "../sha1.h"

const char* hexdigits = "0123456789abcdef";

void verify_hash(const void* data, size_t len, const unsigned char* hash)
{
  unsigned char sha1hash[20];
  unsigned char c;
  int i;

  sha1(sha1hash, data, len);

  if (memcmp(sha1hash, hash, 20) != 0) {
    putstr("Hash is incorrect.\nIt should be: ");
    for (i = 0; i < 20; i++) {
      c = hash[i];
      putchar(hexdigits[c >> 4]);
      putchar(hexdigits[c & 0x0f]);
    }
    putstr("\nIt really is: ");
    for (i = 0; i < 20; i++) {
      c = sha1hash[i];
      putchar(hexdigits[c >> 4]);
      putchar(hexdigits[c & 0x0f]);
    }
    putstr("\nAborting.\n");
    abort();
  }

}
