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
; real mode is re-entered.  The address used is 0x0000:0x8000.

; Where is stuff at the moment?
; 0x00008000 = this code
; 0x00010000 = kernel map
; 0x00090000 = formerly the 32-bit stub code, but now scratch
; somewhere further up    = Windows
; somewhere else up there = the kernel and initrd we're going to load

  org 0x00008000

  ; Leetness! We've entered 1980s-land!  Our task now is to reassemble
  ; and boot the kernel.  Hopefully the BIOS services (at least int 0x10
  ; and especially 0x15) will work.

  ; We really should leave interrupts disabled just in case Windows did
  ; something really strange.  We don't really need interrupts to be
  ; delivered anyway as all we're doing is shuffling tiny pieces of the
  ; kernel and initrd around in RAM before we boot the kernel.

  ; Drop to text mode with a clean screen.
  mov ax, 0x0003
  xor bx, bx
  int 0x10

  ; Say hello.
  mov si, banner
  call display

  ; Make sure PAE is supported.
  mov eax, 0x00000001
  cpuid
  test edx, 0x00000040
  jz noPAE

  ; Put the kernel and initrd back together.
  call theGreatReshuffling

  ; This code is a stump.  You can help by expanding it.
  cli
  hlt

noPAE:
  mov si, noPAEmsg
  call display
  cli
  hlt

  ; Display a message on the screen.
  ; Message is at ds:si.
  ; Trashes all registers.
display:
  cld
.readMore:
  lodsb
  test al, al
  jz .done
  mov ah, 0x0e
  mov bx, 0x0007
  int 0x10
  jmp short .readMore
.done:
  ret

  ; Reassemble the kernel and initrd and populate our base variables.
  ; Temporarily goes into 32-bit protected mode with PAE in order to do that.
  ; Trashes all registers.
theGreatReshuffling:
  ; Protected mode, here we come!
  ; Start by stashing the current stack pointer.
  mov word [old_stack_pointer], sp
  mov ax, ss
  mov word [old_stack_segment], ss

  ; Our new GDT.
  lgdt [gdttag]

  ; Protected mode ON!
  mov eax, cr0
  or al, 0x01
  mov cr0, eax

  ; Fix up CS for 32-bit protected mode.
  jmp 0x0008:.inProtectedMode
.inProtectedMode:
  bits 32

  ; Fix up the rest of the segments for 32-bit protected mode.
  mov eax, 0x00000010
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
  mov esp, 0x00006ffc

  ; Turn on PAE.
  mov eax, cr4
  or al, 0x20
  mov cr4, eax

  ; Build the page directory pointer table at 0x00007000.
  ; The first page directory will go at 0x00090000; the rest are not present.
  cld
  xor eax, eax
  mov ecx, 8
  mov edi, 0x00007000
  rep stosd
  mov dword [0x00007000], 0x00090021

  ; Build the page directory at 0x00090000.
  ; We only need two page tables: one to identity-map the entire first
  ; megabyte of physical RAM (where we are), and one to let us map the areas
  ; of higher RAM that we need in order to move stuff around.
  mov ecx, 1024
  mov edi, 0x00090000
  rep stosd
  mov dword [0x00090000], 0x00091023
  mov dword [0x00090008], 0x00092023

  ; Build the page table for the first megabyte of address space
  ; at 0x00091000.  This is entirely an identity mapping.
  mov eax, 0x00000023
  mov edi, 0x00091000
.writeAnotherEntry:
  stosd
  add edi, 4
  add eax, 0x00001000
  cmp eax, 0x00100000
  jb .writeAnotherEntry

  ; Build the second page table.  This is all zeros for now.
  xor eax, eax
  mov edi, 0x00092000
  mov ecx, 1024
  rep stosd

  ; Turn on paging.
  mov eax, 0x00007000
  mov cr3, eax
  mov eax, cr0
  or eax, 0x80000000
  mov cr0, eax

  ; This code is a stump.  You can help by expanding it.

  ; Turn off paging.
  mov eax, cr0
  and eax, 0x7fffffff
  mov cr0, eax
  xor eax, eax
  mov cr3, eax

  ; Turn off PAE.
  mov eax, cr4
  and al, 0xdf
  mov cr4, eax

  ; Load 16-bit segments for the switch back to real mode.
  mov eax, 0x00000020
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
  mov esp, 0x000006ffe

  ; Temporarily drop to 16-bit protected mode.
  jmp 0x0018:.halfwayOutOfProtectedMode
.halfwayOutOfProtectedMode:
  bits 16

  ; Protected mode OFF!
  mov eax, cr0
  and al, 0xfe
  mov cr0, eax

  ; Fix up CS.
  jmp 0x0000:.fullyOutOfProtectedMode
.fullyOutOfProtectedMode:

  ; Load the real mode values back into the segments.
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax

  ; Unstash the old stack pointer and return.
  mov ax, word [old_stack_segment]
  mov ss, ax
  mov sp, word [old_stack_pointer]
  ret

; Variables
align 4
kernel_base dd 0
initrd_base dd 0
cmdline_base dd 0
cmdline_size dd 0
old_stack_pointer dw 0
old_stack_segment dw 0

; Strings
banner db 'WinKexec: Linux bootloader implemented as a Windows device driver',\
  0x0d,0x0a,'Copyright (C) 2008-2009 John Stumpo',0x0d,0x0a,0x0d,0x0a,0x00
noPAEmsg db 'This processor does not support PAE.',0x0d,0x0a,\
  'PAE support is required in order to use WinKexec.',0x0d,0x0a,0x00

; The global descriptor table used for the kernel reshuffling
; and the switch back to real mode afterwards.
  align 8
gdtstart:
  nullseg dq 0x0000000000000000
  codeseg dq 0x00cf9b000000ffff
  dataseg dq 0x00cf93000000ffff
  real_codeseg dq 0x00009b000000ffff
  real_dataseg dq 0x000093000000ffff
gdtend:

gdttag:
  gdtsize dw (gdtend - gdtstart - 1)
  gdtptr dd gdtstart
