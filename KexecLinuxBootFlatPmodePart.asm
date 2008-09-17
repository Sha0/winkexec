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

bits 32

; This goes into .ftext in kexec.sys.  kexec.sys will identity-map the
; RAM containing this then jump to it so we can turn off paging and do
; other stuff to set up for dropping back to real mode.  This code must
; be position-independent to an extent; in reality we can take advantage
; of certain parts of our calling convention...

  org 0  ; so we can just add ESI to the symbols

  ; This is called with interrupts off, our base address in ESI, and the
  ; address of the real-mode code in EDI.

  ; Paging, be gone!
  mov eax,cr0
  and eax,0x7fffffff
  mov cr0,eax
  xor eax,eax
  mov cr3,eax  ; Paging, be very gone.  (Nuke the TLB.)

  ; Reload the segment registers (and stack) so they become flat.
  mov eax,0x00000010
  mov ds,ax
  mov es,ax
  mov fs,ax
  mov gs,ax
  mov ss,ax
  mov esp,0x00097ffe  ; Stack accessible in real mode.

  ; Now all the segment registers are straight, flat 4GB segments.
  ; Let's map another segment to just us, with real mode appropriate
  ; attributes.  Then we can use direct addressing.

  ; Let's patch in the base address.
  mov word [esi+real_codeseg+2],si
  mov eax,esi
  shr eax,16
  mov byte [esi+real_codeseg+4],al
  mov byte [esi+real_codeseg+7],ah

  ; Prepare the GDT tag.
  mov eax,gdtend
  sub eax,gdtstart
  mov word [esi+gdtsize],ax
  lea eax,[esi+gdtstart]
  mov dword [esi+gdtptr],eax

  ; Install the new GDT.
  lea ebx,[esi+gdttag]
  lgdt [ebx]

  ; Enter 16-bit protected mode through the new GDT.
  jmp 0x0018:in16bitpmode
in16bitpmode:
  bits 16

  ; Patch the real-mode jump to go to the code pointed to by EDI.
  mov ecx,edi
  mov edx,edi
  and edx,0x000ffff0
  shr edx,4
  and ecx,0x0000000f
  ; CX = offset; DX = segment
  mov word [real_newip],cx
  mov word [real_newcs],dx

  ; Finally, drop back to real mode.
  mov eax,cr0
  and eax,0xfffffffe
  mov cr0,eax

  ; Apply the real mode interrupt vector table.
  lidt [real_idttag]

  ; Apply the real mode segments.
  mov ax,dx
  mov ds,ax
  mov es,ax
  mov fs,ax
  mov gs,ax
  mov ss,ax
  mov sp,0xfffe

  ; Jump to the real-mode code.
  db 0xea
  real_newip dw 0
  real_newcs dw 0

gdtstart:
  nullseg dq 0x0000000000000000
  codeseg dq 0x00cf9a000000ffff
  dataseg dq 0x00cf92000000ffff
  real_codeseg dq 0x00009a000000ffff
gdtend:

gdttag:
  gdtsize dw 0
  gdtptr dd 0

real_idttag:
  real_idtsize dw 0xffff
  real_idtptr dw 0x0000