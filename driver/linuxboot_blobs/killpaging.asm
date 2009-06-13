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

bits 32

; This gets compiled into the data segment of kexec.sys as a flat binary.
; kexec.sys will load this to physical address 0x00090000, identity-map
; that piece of RAM, and finally jump to it so we can turn off paging
; and do other stuff to set up for dropping back to real mode.

; Where is stuff at the moment?
; 0x00008000 = real-mode code
; 0x00010000 = kernel map
; 0x00090000 = this code
; somewhere further up    = Windows
; somewhere else up there = the kernel and initrd we're going to load

  org 0  ; really 0x00090000, but that only comes into
         ; play once before addressing of this segment becomes zero-based

  ; Paging, be gone!
  mov eax,cr0
  and eax,0x7fffffff
  mov cr0,eax
  xor eax,eax
  mov cr3,eax  ; Paging, be very gone.  (Nuke the TLB.)

  ; Install a new GDT.
  lgdt [gdttag+0x00090000]

  ; Reload the segment registers (and stack) with
  ; 16-bit real-mode-appropriate descriptors based
  ; at 0x00090000.
  mov eax,0x00000010
  mov ds,ax
  mov es,ax
  mov fs,ax
  mov gs,ax
  mov ss,ax
  mov esp,0x00007ffe  ; Stack accessible in real mode.

  ; Enter 16-bit protected mode through the new GDT.
  jmp 0x0008:in16bitpmode
in16bitpmode:
  bits 16

  ; Drop back to real mode.
  mov eax,cr0
  and al,0xfe
  mov cr0,eax

  ; Apply the real mode interrupt vector table.
  lidt [real_idttag]

  ; Apply the real mode segments.
  mov ax,0x0800
  mov ds,ax
  mov es,ax
  mov fs,ax
  mov gs,ax
  mov ss,ax
  mov sp,0x7ffe  ; just below the kernel map

  ; Jump to the real-mode code.
  jmp 0x0800:0x0000

  ; The global descriptor table used for the switch back to real mode
  align 8
gdtstart:
  nullseg dq 0x0000000000000000
  codeseg dq 0x00009b090000ffff
  dataseg dq 0x000093090000ffff
gdtend:

gdttag:
  gdtsize dw (gdtend - gdtstart - 1)
  gdtptr dd (gdtstart + 0x00090000)

real_idttag:
  real_idtsize dw 0x03ff
  real_idtptr dd 0x00000000
