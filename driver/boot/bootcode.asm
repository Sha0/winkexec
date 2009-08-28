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

  ; Initialize the real mode stack; it will begin at 0x0000:0x4ffe.
  ; This leaves a gap of three pages (12 KB) below this code so we can
  ; put the identity-mapping page directory and page table there and
  ; have a scratch page available.
  mov word [real_stack_segment], 0x0000
  mov word [real_stack_pointer], 0x4ffe

  ; Leave protected mode.  When we return, the stack we set up above
  ; will be in effect.
  call protToReal
  bits 16

  ; Reset the control registers that might affect what we are doing
  ; to a known state.  (Now we don't have to worry about potential
  ; nasties like Windows' page mappings still being cached due to
  ; cr4.pge, which Windows likes to set.)
  mov eax, 0x60000010
  mov cr0, eax
  xor eax, eax
  mov cr4, eax

  ; Initialize the protected mode stack; it will begin at 0x000047fc.
  ; This leaves 2 KB for the real mode stack, just in case the BIOS is
  ; overly greedy with regard to stack space.  This stack will go into
  ; effect when realToProt is called.
  mov word [prot_stack_segment], 0x0010
  mov dword [prot_stack_pointer], 0x000047fc

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
  ; move stuff around.  We also add the page directory that maps the kernel
  ; as a page table, giving us simple access to the kernel map page tables.
  cld
  mov ecx, 1024
  mov edi, 0x00006000
  rep stosd
  mov dword [0x00006000], 0x00007023
  mov eax, dword [pdpt_entry1]
  or al, 0x23
  mov dword [0x00006ff8], eax
  mov eax, dword [pdpt_entry1+4]
  mov dword [0x00006ffc], eax

  ; Build the page table for the first two megabytes of address space
  ; at 0x00007000.  The first megabyte is entirely an identity mapping,
  ; with the exception of the bottom-most page (so we can detect if the
  ; C code tries to follow a null pointer); the second megabyte is left
  ; unmapped.  We zero the page out first because it is likely to contain
  ; garbage from system boot, as boot sectors often like to put their
  ; stack in this area.  We also map the kernel map page directory (by
  ; copying the page directory entry) so we can access it.
  xor eax, eax
  mov ecx, 1024
  mov edi, 0x00007000
  rep stosd
  mov eax, 0x00001023
  mov edi, 0x00007008
.writeAnotherEntry:
  stosd
  add edi, 4
  add eax, 0x00001000
  cmp eax, 0x00100000
  jb .writeAnotherEntry
  mov eax, dword [0x00006ff8]
  mov dword [0x00007ff8], eax
  mov eax, dword [0x00006ffc]
  mov dword [0x00007ffc], eax

  ; So it's come to this.  We're in real mode now, and our task now is to
  ; reassemble and boot the kernel.  Hopefully the BIOS services (at least
  ; int 0x10 and especially 0x15) will work.

  ; We really should leave interrupts disabled just in case Windows did
  ; something really strange.  We don't really need interrupts to be
  ; delivered anyway as all we're doing is shuffling tiny pieces of the
  ; kernel and initrd around in RAM before we boot the kernel.

  ; Virtual memory map for when we're in protected mode:
  ;  0x00001000 - 0x00100000  = identity mapping
  ;  0x001ff000               = kernel map page directory
  ;  0x3fe00000               = kernel map page tables
  ;  (only as much is mapped as actually is used)
  ;  0x40000000               = kernel, etc.
  ;  (kernel, initrd, and kernel command line, each separated by
  ;   an unmapped page)

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

  ; Build the int 0x15 eax=0xe820 memory map.
  call build_e820

  ; Go into protected mode.
  call realToProt
  bits 32

  ; Call the C code to put the kernel and initrd back together.
  ; Pass it a pointer to the boot information structure,
  ; a pointer to the character output routine, a pointer to
  ; a routine that disables paging and stores the fact that we
  ; no longer need it, and a pointer to the int 0x15 eax=0xe820
  ; physical memory map (prefixed with its entry count).
  push dword e820map_count
  push dword doneWithPaging
  push dword bios_putchar
  push dword bootinfo
  call c_code
  add esp, 16

  ; Go back into real mode.
  call protToReal
  bits 16

  ; Say that we got to the end of implemented functionality.
  mov si, stumpmsg
  jmp short kaboom

noPAE:
  mov si, noPAEmsg
  ; fall through to kaboom

kaboom:
  call display
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


  ; Build the int 0x15 eax=0xe820 memory map.
  ; Trashes all registers.
build_e820:
  xor ebx, ebx
  mov di, e820map

.fetchAnother:
  mov eax, 0x0000e820
  mov ecx, 20
  mov edx, 0x534d4150
  int 0x15
  jc .e820fail
  cmp eax, 0x534d4150
  jnz .e820fail
  cmp ecx, 20
  jnz .e820fail
  add di, 20
  cmp di, bootinfo
  jnb .e820toobig
  inc dword [e820map_count]
  test ebx, ebx
  jz .done
  jmp short .fetchAnother

.e820fail:
  ; Apparently some buggy BIOSes return error instead of clearing
  ; ebx when we reach the end of the list.
  cmp dword [e820map_count], 0
  jnz .done
  mov si, e820failmsg
  jmp kaboom

.e820toobig:
  mov si, e820toobigmsg
  jmp kaboom

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

  ; Apply the protected mode interrupt descriptor table.
  lidt [idttag]

  ; Bypass the activation of paging if it's no longer necessary.
  mov al, byte [dont_need_paging_anymore]
  test al, al
  jnz .skipPaging

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
.skipPaging:

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


  ; Stub handlers for protected mode exceptions.
  ; Macro arguments: [int number], [0 or 1 for whether an error code is pushed]
  bits 32
%macro define_isr 2
int%1_isr:
%if %2 == 0
  push dword 0  ; as a dummy since the processor doesn't push an error code
%endif
  push dword %1
  jmp isr_common
%endmacro
  define_isr 0x00, 0  ; division by zero
  define_isr 0x01, 0  ; debug exception
  define_isr 0x02, 0  ; non-maskable interrupt
  define_isr 0x03, 0  ; breakpoint
  define_isr 0x04, 0  ; overflow
  define_isr 0x05, 0  ; bound range exceeded
  define_isr 0x06, 0  ; invalid opcode
  define_isr 0x08, 1  ; double fault
  define_isr 0x0a, 1  ; invalid task state segment
  define_isr 0x0b, 1  ; segment not present
  define_isr 0x0c, 1  ; stack segment fault
  define_isr 0x0d, 1  ; general protection fault
  define_isr 0x0e, 1  ; page fault
  define_isr 0x10, 0  ; floating point exception

  ; The common interrupt service routine.
  ; Stack state:  [bottom; top of previous stack]
  ;               previous EFL
  ;               previous CS
  ;               previous EIP
  ;               error code (or placeholder zero)
  ;               exception number
  ;               [top]
isr_common:
  ; Stash all other registers.
  pushad
  push ds
  push es
  push fs
  push gs
  ; stack (top to bottom):
  ; gs fs es ds edi esi ebp esp-0x14 ebx ecx edx eax excno errcode eip cs efl
  ; offsets from esp (hex):
  ; 00 04 08 0c 10  14  18  1c       20  24  28  2c  30    34      38  3c 40

  ; Display the proper error message.
  mov eax, dword [esp+0x30]
  mov esi, dword [error_code_table + 4*eax]
  call bios_putstr
  mov esi, errcode_separator
  call bios_putstr
  mov eax, dword [esp+0x34]
  call bios_puthex

  ; Dump the registers.
%macro dump_reg 2
  mov esi, %1_is
  call bios_putstr
  mov eax, dword [esp+%2]
  call bios_puthex
%endmacro
%macro dump_real_reg 1
  mov esi, %1_is
  call bios_putstr
  mov eax, %1
  call bios_puthex
%endmacro
  dump_reg eax, 0x2c
  dump_reg ebx, 0x20
  dump_reg ecx, 0x24
  dump_reg edx, 0x28
  dump_reg esi, 0x14
  dump_reg edi, 0x10
  dump_reg ebp, 0x18
  mov esi, esp_is
  call bios_putstr
  mov eax, dword [esp+0x1c]
  add eax, 0x14
  call bios_puthex
  dump_reg eip, 0x38
  dump_reg efl, 0x40
  dump_reg  cs, 0x3c
  dump_real_reg ss
  dump_reg  ds, 0x0c
  dump_reg  es, 0x08
  dump_reg  fs, 0x04
  dump_reg  gs, 0x00
  dump_real_reg cr0
  dump_real_reg cr2
  dump_real_reg cr3
  dump_real_reg cr4

  ; Halt the system.
  mov esi, system_halted
  call bios_putstr
  cli
.hltloop:
  hlt
  jmp short .hltloop

  ; Were we to return, though, we'd restore register state...
  pop gs
  pop fs
  pop es
  pop ds
  popad
  ; ...discard the error code and exception number, and then return.
  add esp, 8
  iret


  ; Used only by exception handlers.
  ; Takes address of string in esi, which must be in real mode segment zero.
bios_putstr:
  bits 32
  call protToReal
  bits 16
  call display
  call realToProt
  bits 32
  ret

  ; Used only by exception handlers.
  ; Writes eax out in hex.
bios_puthex:
  bits 32
  push ebp
  mov ebp, esp
  sub esp, 4

  mov ebx, eax
  mov edi, 8

  mov dword [esp], '0'
  call bios_putchar
  mov dword [esp], 'x'
  call bios_putchar

.printloop:
  rol ebx, 4
  mov al, bl
  and al, 0x0f
  cmp al, 0x0a
  jl .usedigit
  add al, 'a' - 0x0a  ; convert to a-f
  jmp short .converted
.usedigit:
  add al, '0'
.converted:
  movzx eax, al
  mov dword [esp], eax
  call bios_putchar
  sub edi, 1
  jnz .printloop

  leave
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


  ; Disable paging and store the fact that we don't need it anymore.
  ; C prototype: void done_with_paging(void);
  ;
  ; We have this because at a specific point the C code doesn't need
  ; paging to be activated anymore.  Paging is only used to bring the
  ; kernel pages together into physically contiguous memory.  We can
  ; do all the rest with just plain old unpaged physical memory access;
  ; in fact, we need it that way once we've reassembled the kernel so
  ; we can move stuff around freely in physical memory to prepare for
  ; boot.  If we don't store the fact that we've reached that point,
  ; though, paging will be activated again as soon as we return from
  ; something that drops to real mode, and the C code will almost
  ; surely attempt to access some unmapped piece of memory, and the
  ; resulting page fault will kill us.
  align 16
doneWithPaging:
  bits 32

  push ebp
  mov ebp, esp

  ; Turn off paging...
  mov eax, cr0
  and eax, 0x7fffffff
  mov cr0, eax

  ; ...reset the TLBs...
  xor eax, eax
  mov cr3, eax

  ; ...and turn off PAE.
  mov eax, cr4
  and al, 0xdf
  mov cr4, eax

  ; Store the fact that we did it, and return.
  mov byte [dont_need_paging_anymore], 1

  leave
  ret


; Variables
align 4
real_stack_pointer dw 0
real_stack_segment dw 0
prot_stack_pointer dd 0
prot_stack_segment dw 0
dont_need_paging_anymore db 0

; Strings
banner db 'WinKexec: Linux bootloader implemented as a Windows device driver',\
  0x0d,0x0a,'Copyright (C) 2008-2009 John Stumpo',0x0d,0x0a,0x0d,0x0a,0x00
noPAEmsg db 'This processor does not support PAE.',0x0d,0x0a,\
  'PAE support is required in order to use WinKexec.',0x0d,0x0a,0x00
e820failmsg db 'The BIOS does not support int 0x15 eax=0xe820.',0x0d,0x0a,\
  'WinKexec requires this function to work.',0x0d,0x0a,0x00
e820toobigmsg db 'The memory table returned by int 0x15 eax=0xe820',0x0d,0x0a,\
  'overflowed the buffer allocated to contain it.',0x0d,0x0a,0x00
stumpmsg db 'This program is a stump.  You can help by expanding it.',\
  0x0d,0x0a,0x00  ; in honor of Kyle
errcode_separator db ', errcode=',0x00
eax_is db 0x0d,0x0a,'eax=',0x00
ebx_is db         '  ebx=',0x00
ecx_is db         '  ecx=',0x00
edx_is db         '  edx=',0x00
esi_is db 0x0d,0x0a,'esi=',0x00
edi_is db         '  edi=',0x00
ebp_is db         '  ebp=',0x00
esp_is db         '  esp=',0x00
eip_is db 0x0d,0x0a,'eip=',0x00
efl_is db         '  efl=',0x00
 cs_is db         '   cs=',0x00
 ss_is db         '   ss=',0x00
 ds_is db 0x0d,0x0a,' ds=',0x00
 es_is db         '   es=',0x00
 fs_is db         '   fs=',0x00
 gs_is db         '   gs=',0x00
cr0_is db 0x0d,0x0a,'cr0=',0x00
cr2_is db         '  cr2=',0x00
cr3_is db         '  cr3=',0x00
cr4_is db         '  cr4=',0x00
system_halted db 0x0d,0x0a,0x0d,0x0a,'System halted.',0x0d,0x0a,0x00
exc0x00_msg db 'Division by zero',0x00
exc0x01_msg db 'Debug exception',0x00
exc0x02_msg db 'Non-maskable interrupt',0x00
exc0x03_msg db 'Breakpoint',0x00
exc0x04_msg db 'Overflow',0x00
exc0x05_msg db 'Bound range exceeded',0x00
exc0x06_msg db 'Invalid opcode',0x00
exc0x08_msg db 'Double fault',0x00
exc0x0a_msg db 'Invalid task state segment',0x00
exc0x0b_msg db 'Segment not present',0x00
exc0x0c_msg db 'Stack segment fault',0x00
exc0x0d_msg db 'General protection fault',0x00
exc0x0e_msg db 'Page fault',0x00
exc0x10_msg db 'Floating point exception',0x00

; The table mapping exception numbers to error messages.
  align 4
error_code_table:
  dd exc0x00_msg
  dd exc0x01_msg
  dd exc0x02_msg
  dd exc0x03_msg
  dd exc0x04_msg
  dd exc0x05_msg
  dd exc0x06_msg
  dd 0
  dd exc0x08_msg
  dd 0
  dd exc0x0a_msg
  dd exc0x0b_msg
  dd exc0x0c_msg
  dd exc0x0d_msg
  dd exc0x0e_msg
  dd 0
  dd exc0x10_msg

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

; The interrupt descriptor table used for protected mode exception handling.
; Since we know that the handler addresses fit entirely in the lower
; 16 bits, we can cheat when forming the entries and not split the words.
%define IDT_BASE_DESCRIPTOR 0x00008e0000080000
  align 8
idtstart:
  dq IDT_BASE_DESCRIPTOR + int0x00_isr
  dq IDT_BASE_DESCRIPTOR + int0x01_isr
  dq IDT_BASE_DESCRIPTOR + int0x02_isr
  dq IDT_BASE_DESCRIPTOR + int0x03_isr
  dq IDT_BASE_DESCRIPTOR + int0x04_isr
  dq IDT_BASE_DESCRIPTOR + int0x05_isr
  dq IDT_BASE_DESCRIPTOR + int0x06_isr
  dq 0
  dq IDT_BASE_DESCRIPTOR + int0x08_isr
  dq 0
  dq IDT_BASE_DESCRIPTOR + int0x0a_isr
  dq IDT_BASE_DESCRIPTOR + int0x0b_isr
  dq IDT_BASE_DESCRIPTOR + int0x0c_isr
  dq IDT_BASE_DESCRIPTOR + int0x0d_isr
  dq IDT_BASE_DESCRIPTOR + int0x0e_isr
  dq 0
  dq IDT_BASE_DESCRIPTOR + int0x10_isr
idtend:

; The pointer to the global descriptor table, used when loading it.
gdttag:
  gdtsize dw (gdtend - gdtstart - 1)
  gdtptr dd gdtstart

; The pointer to the interrupt descriptor table, used when loading it.
idttag:
  idtsize dw (idtend - idtstart - 1)
  idtptr dd idtstart

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

; Space for building the int 0x15 eax=0xe820 memory map.
; Make sure we leave enough space for 32 entries, but the code will
; be willing to go farther (though it will not overwrite bootinfo),
; so this has to be the last thing before we pad up to bootinfo.
e820map_count dd 0
e820map times (20 * 32) db 0x00

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
