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

; This gets compiled into the data segment of kexec.sys as a flat binary.
; kexec.sys will load this to physical address 0x00008000, identity-map
; that piece of RAM, and finally jump to it so we can turn off paging,
; drop back to real mode, build a minimal protected mode environment,
; use that to reassemble the kernel and initrd, and finally boot the kernel.

; Where is stuff at the moment?
; 0x00008000               = this code
; in the struct at the end = pointer to page directory showing us
;                            where the kernel and initrd are
; somewhere further up     = Windows
; somewhere else up there  = the kernel and initrd we're going to load

  org 0x00008000

_start:
  bits 32

  ; Initialize the real mode stack; it will begin at 0x0000:0x5ffe.
  ; This leaves a gap of two pages (8 KB) below this code so we can
  ; put the identity-mapping page directory and page table there.
  mov word [real_stack_segment], 0x0000
  mov word [real_stack_pointer], 0x5ffe

  ; Leave protected mode.  When we return, the stack we set up above
  ; will be in effect.
  call protToReal
  bits 16

  ; Initialize the protected mode stack; it will begin at 0x000057fc.
  ; This leaves 2 KB for the real mode stack, just in case the BIOS is
  ; overly greedy with regard to stack space.  This stack will go into
  ; effect when realToProt is called.
  mov word [prot_stack_segment], 0x0010
  mov dword [prot_stack_pointer], 0x000057fc

  ; Populate the page directory pointer table embedded within us.
  ; (Really, that just means copying kx_page_directory to its second
  ; entry, effectively mapping the kernel and initrd to 0x40000000.)
  ; The first page directory will go at 0x00006000 and is already filled
  ; in for us; the second is the one we are passed from the final bit of
  ; Windows kernel API-using code; the rest are not present.
  mov eax, dword [kx_page_directory]
  mov dword [pdpt_entry1], eax
  mov eax, dword [kx_page_directory+4]
  mov dword [pdpt_entry1+4], eax

  ; Build the first page directory at 0x00006000.
  ; We only need one page table: it will identity-map the entire first
  ; megabyte of physical RAM (where we are), and let us map the areas
  ; of higher RAM that we need into the second megabyte in order to
  ; move stuff around.
  mov ecx, 1024
  mov edi, 0x00006000
  rep stosd
  mov dword [0x00006000], 0x00007023

  ; Build the page table for the first two megabytes of address space
  ; at 0x00007000.  The first megabyte is entirely an identity mapping;
  ; the second megabyte is left unmapped.
  mov eax, 0x00000023
  mov edi, 0x00007000
  cld
.writeAnotherEntry:
  stosd
  add edi, 4
  add eax, 0x00001000
  cmp eax, 0x00100000
  jb .writeAnotherEntry

  ; So it's come to this.  We're in real mode now, and our task now is to
  ; reassemble and boot the kernel.  Hopefully the BIOS services (at least
  ; int 0x10 and especially 0x15) will work.

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

  ; Go into protected mode.
  call realToProt
  bits 32

  ; Call the C code to put the kernel and initrd back together.
  ; Pass it a pointer to the boot information structure
  ; and a pointer to the character output routine.
  push dword bios_putchar
  push dword bootinfo
  call c_code
  add esp, 8

  ; Go back into real mode.
  call protToReal
  bits 16

  ; Say that we got to the end of implemented functionality.
  mov si, stumpmsg
  call display
  jmp short cliHlt

noPAE:
  mov si, noPAEmsg
  call display
  ; fall through to cliHlt

cliHlt:
  cli
.hltloop:
  hlt
  jmp short .hltloop

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


  ; Switch from real mode to protected mode.
  ; Preconditions:
  ;  - Interrupts are disabled.
  ;  - The page directory pointer table in our data section is valid and
  ;    identity-maps this routine and the routine we are returning to.
  ;  - The stored protected mode stack is valid.
  ;  - The caller's code and data segments are zero.
  ;  - The processor supports PAE.
  ; Postconditions:
  ;  - Real mode stack is stored.
  ;  - Returns in 32-bit protected mode with paging and PAE enabled.
  ;  - The active page directory pointer table is the one in our data section.
realToProt:
  bits 16

  ; Save our return address.
  pop dx

  ; Save the real mode stack.
  mov ax, ss
  mov word [real_stack_segment], ax
  mov word [real_stack_pointer], sp

  ; Install the GDT just in case.
  lgdt [gdttag]

  ; Protected mode ON!
  mov eax, cr0
  or al, 0x01
  mov cr0, eax

  ; Fix up CS for 32-bit protected mode.
  jmp 0x0008:.inProtectedMode
.inProtectedMode:
  bits 32

  ; Fix up the segments we're going to need in 32-bit protected mode.
  ; Also switch to the protected mode stack.
  mov ax, 0x0010
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, word [prot_stack_segment]
  mov esp, dword [prot_stack_pointer]

  ; Turn on PAE.
  mov eax, cr4
  or al, 0x20
  mov cr4, eax

  ; Turn on paging.
  mov eax, page_directory_pointer_table
  mov cr3, eax
  mov eax, cr0
  or eax, 0x80000000
  mov cr0, eax

  ; Return to the saved return address.
  movzx eax, dx
  push eax
  ret


  ; Switch from protected mode to real mode.
  ; Preconditions:
  ;  - If paging is on, this routine is in identity-mapped memory.
  ;  - The return address is strictly less than 0x00010000 (64 KB).
  ;  - Interrupts are disabled.
  ;  - The stored real mode stack is valid.
  ; Postconditions:
  ;  - Protected mode stack is stored.
  ;  - Returns in 16-bit real mode.
protToReal:
  bits 32

  ; Save our return address.
  pop edx

  ; Save the protected mode stack.
  mov ax, ss
  mov word [prot_stack_segment], ax
  mov dword [prot_stack_pointer], esp

  ; Paging, be gone!
  mov eax, cr0
  and eax, 0x7fffffff
  mov cr0, eax
  xor eax, eax
  mov cr3, eax  ; Paging, be very gone.  (Nuke the TLB.)

  ; Turn off PAE, if it's on.
  mov eax, cr4
  and al, 0xdf
  mov cr4, eax

  ; Install the new GDT, just in case this is the first time we're switching.
  lgdt [gdttag]

  ; Enter 16-bit protected mode through the new GDT.
  jmp 0x0018:.in16bitpmode
.in16bitpmode:
  bits 16

  ; Load real-mode-appropriate segments so the descriptor
  ; cache registers don't get royally messed up.
  mov ax, 0x0020
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  ; Apply the real mode interrupt vector table.
  lidt [real_idttag]

  ; Drop back to real mode.
  mov eax, cr0
  and al, 0xfe
  mov cr0, eax

  ; Make an absolute jump below to put us in full-blown 16-bit real mode.
  jmp 0x0000:.in16bitrmode
.in16bitrmode:

  ; Finally load the segment registers with genuine real-mode values.
  ; Also switch to the real mode stack.
  xor ax, ax
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ax, word [real_stack_segment]
  mov ss, ax
  mov sp, word [real_stack_pointer]

  ; Return to the saved return address.
  push dx
  ret


  ; Runs int 0x10 AH=0x0e with the passed value as AL.
  ; C prototype: void bios_putchar(unsigned char);
  align 16
bios_putchar:
  bits 32

  push ebp
  mov ebp, esp
  push edi
  push esi
  push ebx

  ; Grab the character from the argument list.
  mov ecx, dword [ebp + 8]

  ; Do it.
  call protToReal
  bits 16
  mov ah, 0x0e
  mov al, cl
  mov bx, 0x0007
  int 0x10
  call realToProt
  bits 32

  pop ebx
  pop esi
  pop edi
  pop ebp
  ret


; Variables
align 4
real_stack_pointer dw 0
real_stack_segment dw 0
prot_stack_pointer dd 0
prot_stack_segment dw 0

; Strings
banner db 'WinKexec: Linux bootloader implemented as a Windows device driver',\
  0x0d,0x0a,'Copyright (C) 2008-2009 John Stumpo',0x0d,0x0a,0x0d,0x0a,0x00
noPAEmsg db 'This processor does not support PAE.',0x0d,0x0a,\
  'PAE support is required in order to use WinKexec.',0x0d,0x0a,0x00
stumpmsg db 'This program is a stump.  You can help by expanding it.',\
  0x0d,0x0a,0x00  ; in honor of Kyle

; The global descriptor table used for the deactivation of paging,
; the initial switch back to real mode, the kernel reshuffling,
; and the switch back to real mode afterwards.
  align 8
gdtstart:
  nullseg dq 0x0000000000000000
  codeseg dq 0x00cf9b000000ffff
  dataseg dq 0x00cf93000000ffff
  real_codeseg dq 0x00009b000000ffff
  real_dataseg dq 0x000093000000ffff
gdtend:

; The pointer to the global descriptor table, used when loading it.
gdttag:
  gdtsize dw (gdtend - gdtstart - 1)
  gdtptr dd gdtstart

; The pointer to the real mode interrupt vector table,
; used when loading it.
real_idttag:
  real_idtsize dw 0x03ff
  real_idtptr dd 0x00000000

; The page directory pointer table used when re-entering protected mode.
  align 32
page_directory_pointer_table:
  pdpt_entry0 dq 0x0000000000006001
  pdpt_entry1 dq 0x0000000000000000
  pdpt_entry2 dq 0x0000000000000000
  pdpt_entry3 dq 0x0000000000000000

  times (4016 - ($ - $$)) db 0x00  ; pad to 4KB minus 80 bytes

  ; An 80-byte structure giving the information that we need...
  ; (Corresponds to struct bootinfo in bootinfo.h)
bootinfo:
  kernel_size dd 0                ; length of kernel         (0x8fb0)
  kernel_hash times 20 db 0       ; SHA1 of kernel           (0x8fb4)
  initrd_size dd 0                ; length of initrd         (0x8fc8)
  initrd_hash times 20 db 0       ; SHA1 of initrd           (0x8fcc)
  cmdline_size dd 0               ; length of cmdline        (0x8fe0)
  cmdline_hash times 20 db 0      ; SHA1 of cmdline          (0x8fe4)
  kx_page_directory times 8 db 0  ; PDPT entry for kernel PD (0x8ff8)

  ; Incorporate the C code.
  c_code incbin 'boot/reassemble.bin'
