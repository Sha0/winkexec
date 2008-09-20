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

#include <windows.h>

HINSTANCE hInst;

int WINAPI WinMain(HINSTANCE in_hInst, HINSTANCE prev,
  LPSTR cmdline, int winstyle)
{
  hInst = in_hInst;

  MessageBox(NULL, "This program is a stump.  You can help by expanding it.", "WinKexec GUI", MB_ICONINFORMATION | MB_OK);
  return 0;
}
