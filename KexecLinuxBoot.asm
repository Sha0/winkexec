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
  push dword [_KexecInitrd+4]  ; Pointer
  call dword [__imp__MmGetPhysicalAddress@4]
  test edx,edx
  mov edx,4
  jnz .bsod  ; possible kexec BSoD #4: kernel command line above 4GB
  stosd
  mov eax,dword [_KexecInitrd]  ; Size
  stosd

  ; Find the size of the map.
  mov ebx,edi
  sub ebx,dword [ebp-4]  ; The base of the kernel map

  ; Find the size of the real-mode code.
  mov edi,dword [_KexecLinuxBootRealModeCodeEnd]
  sub edi,dword [_KexecLinuxBootRealModeCodeStart]

  ; Now grab some low RAM for the code below us.
  push dword 0  ; High 32 bits of maximum allowable physical address
  push dword 0x00097fff  ; Low 32 bits
  push ecx  ; Number of bytes to allocate.
  call dword [__imp__MmAllocateContiguousMemory@8]  ; Mapping into eax.
  mov edx,5
  test eax,eax
  jz .bsod  ; possible kexec BSoD #5: allocating real mode code failed

  ; It looks like we are good to go...
  ; let's kick Windows out, return to real mode, and invoke what hopefully
  ; is the code right below us.

  ; Map physical address 0x00000467 so we can
  ; drop something there for the BIOS's pleasure.
  ; (I was hoping not to have to call parts of Windows from this file...
  ; maybe we should add some KeBugCheckEx()es rather than returning
  ; when errors happen above and below.)
  push dword 0
  push dword 4
  push dword 0
  push dword 0x00000467
  call dword [__imp__MmMapIoSpace@16]
  test eax,eax
  jz .bsod

  ; Now mangle the flat address into a real-mode far pointer (ugh!)
  add ebx,4
  mov ecx,ebx
  and ebx,0x000f0000
  shl ebx,12
  mov bx,cx

  ; Write the value...
  mov dword [eax],ebx

  ; Set up for fallback to real mode through processor reset.

  ; Abandon all interrupts, ye who execute here!
  cli

  mov al,0x0f
  out 0x70,al  ; Prepare to access CMOS byte 0x0f
  nop
  nop
  nop
  nop
  mov al,0x07
  out 0x71,al  ; BIOS should just jump to reset vector at 0x00000467.

  ; Do the reset.
  mov al,0xfe
  out 0x64,al
  hlt

.bsod:
  push edx
  push dword 0x42424242
  push dword 0x42424242
  push dword 0x42424242
  push dword 0x42424242
  call dword [__imp__KeBugCheckEx@20]
  cli
  hlt

; More code! (16-bit this time.) Call it data since that's what it
; is, as far as the 32-bit code is concerned...
section .rdata
bits 16

; Put us into the read-only data section of kexec.sys.  kexec.sys will
; copy us to real-mode memory and run us via the above routine.

global _KexecLinuxBootRealModeCodeStart
_KexecLinuxBootRealModeCodeStart:
  ; Leave four bytes for the kernel map address.
  ; The code that actually calls us will skip it.
  KernelMap dd 0

  ; Placeholder!
  cli
  hlt

global _KexecLinuxBootRealModeCodeEnd
_KexecLinuxBootRealModeCodeEnd: