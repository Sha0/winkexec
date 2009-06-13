; WinKexec: kexec for Windows
; Copyright (C) 2008-2009 John Stumpo
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
; This gets compiled into the data segment of kexec.sys as a flat binary.
; It is loaded at absolute address 0x00008000, then jumped to once
; real mode is re-entered.  The address used is 0x0800:0x0000.

; Where is stuff at the moment?
; 0x00008000 = this code
; 0x00010000 = kernel map
; 0x00090000 = formerly the 32-bit stub code, but now scratch
; somewhere further up    = Windows
; somewhere else up there = the kernel and initrd we're going to load

  org 0

  ; Leetness! We've entered 1980s-land!  Our task now is to reassemble
  ; and boot the kernel.  Hopefully the BIOS services (at least int 0x10
  ; and especially 0x15) will work.

  ; We really should leave interrupts disabled just in case Windows did
  ; something really strange.  We don't really need interrupts to be
  ; delivered anyway as all we're doing is shuffling tiny pieces of the
  ; kernel and initrd around in RAM before we boot the kernel.

  ; Drop to text mode with a clean screen.
  mov ax,0x0003
  xor bx,bx
  int 0x10

  ; Say hello.
  mov si,banner
  call display

  ; This code is a stump.  You can help by expanding it.
  cli
  hlt

  ; Display a message on the screen.
  ; Message is at ds:si.
  ; Trashes all registers.
display:
  cld
.readMore:
  lodsb
  test al,al
  jz .done
  mov ah,0x0e
  mov bx,0x0007
  int 0x10
  jmp short .readMore
.done:
  ret

; Strings
banner db 'WinKexec: Linux bootloader implemented as a Windows device driver',\
  0x0d,0x0a,'Copyright (C) 2008-2009 John Stumpo',0x0d,0x0a,0x0d,0x0a,0x00
