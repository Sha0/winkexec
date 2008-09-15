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

; Code! (32-bit)
section .text
bits 32

; Import stuff from the C code.
extern _KexecKernel
extern _KexecInitrd
extern _KexecKernelCommandLine

extern __imp__KeBugCheckEx@20
extern __imp__MmAllocateContiguousMemory@8
extern __imp__MmGetPhysicalAddress@4
extern __imp__MmMapIoSpace@16

; The code that goes back into real mode, then into flat protected mode,
; then pulls all the pieces of the kernel back together and boots it.
; The C code calls this with no arguments.  This builds the kernel map
; itself through importing symbols. This function will load the real-mode
; code and kernel map into 16-byte aligned areas between
; 0x00500 and 0x98000.  This does not return; on problems this will
; call KeBugCheckEx() itself.
global _KexecLinuxBoot
_KexecLinuxBoot:
  push ebp
  mov ebp,esp

  ; Make some local variable room.
  sub esp,4

  ; Build up a count of bytes needed in ecx.
  xor ecx,ecx

  ; Do the kernel.
  mov eax,dword [_KexecKernel]  ; Size is the first member of a KEXEC_BUFFER.
  add eax,4095  ; We count any 4096-byte pages (or fractions thereof)
  shr eax,10  ; Every 4096 bytes is described by 4 bytes, so divide by 1024.
  and eax,0xfffffffc  ; Cut it back to a multiple of 4.
  add ecx,eax  ; Put it in.
  add ecx,4  ; Terminator of the kernel page list.

  ; Do the initrd and cmdline.
  mov eax,dword [_KexecInitrd]
  add eax,4095
  shr eax,10
  and eax,0xfffffffc
  add ecx,eax
  add ecx,12  ; Terminator, cmdline pointer, cmdline length.

  ; Grab a chunk of RAM.
  push dword 0  ; High 32 bits of maximum allowable physical address
  push dword 0x00097fff  ; Low 32 bits
  push ecx  ; Number of bytes to allocate.
  call dword [__imp__MmAllocateContiguousMemory@8]  ; Mapping into eax.
  mov edx,1
  test eax,eax
  jz near .bsod  ; possible kexec BSoD #1: allocating kernel map failed
  mov dword [ebp-4],eax  ; Squirrel away a copy for later.

  ; Start building the kernel map.
  mov edi,eax
  ; Start, naturally, with the kernel.
  mov esi,dword [_KexecKernel]  ; Size is the first member.
  mov ebx,dword [_KexecKernel+4]  ; Data is the second member.
  test esi,esi
  jz .kerneldone  ; Skip kernel loop if there is no kernel.

.kernelloop:
  ; Turn virtual address ebx into physical address edx:eax.
  push ebx
  call dword [__imp__MmGetPhysicalAddress@4]
  test edx,edx
  mov edx,2
  jnz near .bsod  ; possible kexec BSoD #2: address above 4GB in kernel
  stosd  ; Write it into the kernel map.
  ; Increment the pointers.
  add ebx,4096
  sub esi,4096
  ; Remember that cmp is merely non-destructive sub!
  jle .kerneldone
  jmp short .kernelloop
.kerneldone:
  ; Write the terminator.
  xor eax,eax
  stosd

  ; Now the initrd.
  ; See comments in the block of code that handles the kernel.
  mov esi,dword [_KexecInitrd]
  mov ebx,dword [_KexecInitrd+4]
  test esi,esi
  jz .initrddone
.initrdloop:
  push ebx
  call dword [__imp__MmGetPhysicalAddress@4]
  test edx,edx
  mov edx,3
  jnz near .bsod  ; possible kexec BSoD #3: address above 4GB in initrd
  stosd
  add ebx,4096
  sub esi,4096
  jle .initrddone
  jmp short .initrdloop
.initrddone:
  xor eax,eax
  stosd

  ; And the kernel command line.
  push dword [_KexecKernelCommandLine+4]  ; Pointer
  call dword [__imp__MmGetPhysicalAddress@4]
  test edx,edx
  mov edx,4
  jnz near .bsod  ; possible kexec BSoD #4: kernel command line above 4GB
  stosd
  mov eax,dword [_KexecKernelCommandLine]  ; Size
  stosd

  ; Find the size of the real-mode code.
  mov edi,_KexecLinuxBootRealModeCodeEnd
  sub edi,_KexecLinuxBootRealModeCodeStart

  ; Now grab some low RAM for the code below us.
  push dword 0  ; High 32 bits of maximum allowable physical address
  push dword 0x00097fff  ; Low 32 bits
  push edi  ; Number of bytes to allocate.
  call dword [__imp__MmAllocateContiguousMemory@8]  ; Mapping into eax.
  mov edx,5
  test eax,eax
  jz near .bsod  ; possible kexec BSoD #5: allocating real mode code failed

  ; Copy the code.
  push eax
  mov ebx,edi
  mov edi,eax
  mov esi,_KexecLinuxBootRealModeCodeStart
  mov ecx,ebx
  rep movsb

  ; Load the offsets we need for quick reference.
  mov eax,dword [ebp-4]  ; Kernel map
  pop edi  ; Real mode code

  ; Turn the virtual addresses into physical ones.
  ; Do the kernel map first.
  push eax
  call dword [__imp__MmGetPhysicalAddress@4]
  mov esi,eax

  ; While the kernel map physical address is in eax and the real mode
  ; code mapping is in edi, patch in the kernel map offset.
  ; Note that this intentionally shifts the real code pointer four bytes up!
  stosd

  ; Now translate the real-mode code pointer.
  push edi
  call dword [__imp__MmGetPhysicalAddress@4]
  mov edi,eax

  ; It looks like we are good to go...
  ; let's kick Windows out, return to real mode, and invoke what hopefully
  ; is the code right below us.

  ; First we have to turn off paging (which is a right mess...)
  ; We need to have everything ready, for we must jump into a valid
  ; segment immediately after clearing bit 31 of cr0.  Thankfully we
  ; can take advantage of prefetching; otherwise we'd be SOL.

  ; Prepare to load a new GDT and IDT.
  mov eax,_KexecLinuxBootGdtStart
  sub eax,_KexecLinuxBootGdtEnd
  mov word [_KexecLinuxBootGdtSize],ax
  push dword [_KexecLinuxBootGdtStart]
  call dword [__imp__MmGetPhysicalAddress@4]
  test edx,edx
  mov edx,6
  jnz .bsod  ; possible kexec BSoD #6: GDT above 4GB
  mov dword [_KexecLinuxBootGdtPointer],eax

  mov eax,_KexecLinuxBootIdtStart
  sub eax,_KexecLinuxBootIdtEnd
  mov word [_KexecLinuxBootIdtSize],ax
  push dword [_KexecLinuxBootIdtStart]
  call dword [__imp__MmGetPhysicalAddress@4]
  test edx,edx
  mov edx,7
  jnz .bsod  ; possible kexec BSoD #7: IDT above 4GB
  mov dword [_KexecLinuxBootIdtPointer],eax

  ; Now we must patch the EIP the jump below us goes to (ugh!)
  ; ...fill in stuff here...

  ; Abandon all interrupts, ye who execute here!
  cli

  ; Load a simple GDT with just the null seg, a code seg, and a data seg.
  lgdt [_KexecLinuxBootGdtTag]
  ; Load a null IDT too.
  lidt [_KexecLinuxBootIdtTag]

  ; Paging, be gone!
  mov eax,cr0
  and eax,0x7fffffff
  mov cr0,eax

  ; Start the stuff in .ftext.
  ; It's only safe to do this if we do all of the following:
  ;  - Set CS and EIP at the same time.
  ;  - Do this immediately after the "mov cr0,eax" that turns paging off.
  ;  - Something else I'm forgetting.
  ; Thus we must lay out the instruction ourselves and patch it to the
  ; physical address that we must jump to.
  db 0xea  ; Long jump opcode
  .neweip dd 0
  .newseg dw 0x0008  ; We don't need to patch this.

  ; Three guesses what this blip of code does...
.bsod:
  push edx
  push dword 0x42424242
  push dword 0x42424242
  push dword 0x42424242
  push dword 0x42424242
  call dword [__imp__KeBugCheckEx@20]
  cli
  hlt

; More code! (Still 32-bit.) We jump here after turning off paging and
; entering flat protected mode; we have it its own section so we don't
; have to worry about straddling a page boundary.
section .ftext  ; .text for flat protected mode
bits 32

_KexecLinuxBootFlatProtectedModeCode:

  ; Reload the segment registers (and stack) so they become flat.
  xor eax,eax
  mov ds,ax
  mov es,ax
  mov fs,ax
  mov gs,ax
  mov ss,ax
  mov esp,0x00097ffe

  ; This code is a stump.  You can help by expanding it.
  cli
  hlt

; Still more code! (16-bit this time.) Call it data since that's what it
; is, as far as the 32-bit code is concerned...
section .rdata
bits 16

; Put us into the read-only data section of kexec.sys.  kexec.sys will
; copy us to real-mode memory and run us via the above routine.
_KexecLinuxBootRealModeCodeStart:
  ; Leave four bytes for the kernel map address.
  ; The code that actually calls us will skip it.
  KernelMap dd 0

  ; Placeholder!
  cli
  hlt

_KexecLinuxBootRealModeCodeEnd:

; We'll put our shiny new GDT and IDT in .rdata since they're, well, data.
_KexecLinuxBootGdtStart:
_KexecLinuxBootGdtEnd:

_KexecLinuxBootIdtStart:
_KexecLinuxBootIdtEnd:

; The GDT and IDT pointers, however, go in .bss since we must make them
; use unknown physical addresses.
section .bss
_KexecLinuxBootGdtTag:
  _KexecLinuxBootGdtSize resw 1
  _KexecLinuxBootGdtPointer resd 1

_KexecLinuxBootIdtTag:
  _KexecLinuxBootIdtSize resw 1
  _KexecLinuxBootIdtPointer resd 1