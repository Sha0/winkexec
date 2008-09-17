; WinKexec: kexec for Windows
; Copyright (C) 2008 John Stumpo
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <http://www.gnu.org/licenses/>.

bits 16

; The real-mode part of the Linux bootloader.

  ; This is called with CS set so this is at the base of its segment.
  ; All other segment registers are set to this segment as well.
  ; This called in 16-bit real-mode.  EDI still points to the code
  ; entry point, and the stack is at the end of this segment.  The
  ; IDT has been reset to its real-mode state, so theoretically
  ; it should be safe enough to enable interrupts.
  org 0

  ; Leave four bytes for the kernel map address.
  ; The code that actually calls us will populate then skip it.
  KernelMap dd 0

  ; Leetness! We've entered 1980s-land!  Our task now is to reassemble
  ; and boot the kernel.  Hopefully the BIOS services (at least int 0x10
  ; and especially 0x15) will work.

  ; The IDT should be in a sane enough state...
  sti

  ; Drop to text mode.
  mov ax,0x0003
  int 0x10

  ; This code is a stump.  You can help by expanding it.
  cli
  hlt
