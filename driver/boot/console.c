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

#include "console.h"

extern const char* hexdigits;
static bios_putchar_t __bios_putchar;


/* Initialize the console functions by storing a
   function pointer wrapping around int 0x10 ah=0x0e.  */
void console_init(bios_putchar_t putch)
{
  __bios_putchar = putch;
}


/* Output a single character.
   Handle newline appropriately.  */
int putchar(int c)
{
  if (c == '\n')
    __bios_putchar('\r');
  __bios_putchar((unsigned char)c);
  return c;
}


/* Output a null-terminated string. */
void putstr(const char* str)
{
  const char* i;
  for (i = str; *i; i++)
    putchar(*i);
}


/* Output a 32-bit word in hexadecimal. */
void puthex(uint32_t w)
{
  int i;

  putstr("0x");
  for (i = 28; i >= 0; i -= 4)
    putchar(hexdigits[(w >> i) & 0x0000000f]);

}
