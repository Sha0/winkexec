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

; There are probably infinite ways this code can be improved, especially
; with regard to register usage.  It became the magnificent shambles you
; see below from various things that were tried and failed.  (Especially
; with killing paging; I lost count of the number of triple faults...)

; Code! (32-bit)
section .text
bits 32

; Import stuff from the C code.
extern _KexecKernel
extern _KexecInitrd
extern _KexecKernelCommandLine

; Amazingly, we only use these five things from ntoskrnl.exe
extern __imp__KeBugCheckEx@20
extern __imp__MmAllocateContiguousMemory@12
extern __imp__MmGetPhysicalAddress@4
extern __imp__MmMapIoSpace@16  ; to be sorely abused below
extern __imp__MmUnmapIoSpace@8

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
  ; XXX: Figure out why Vista always BSoDs here
  ; (from the function call failing, we know that much...)
  push dword 0  ; High 32 bits of maximum allowable physical address
  push dword 0x00097fff  ; Low 32 bits
  push ecx  ; Number of bytes to allocate.
  call dword [__imp__MmAllocateContiguousMemory@12]  ; Mapping into eax.
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
  call dword [__imp__MmAllocateContiguousMemory@12]  ; Mapping into eax.
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
  ; We must turn it into a real-mode pointer first (ugh!)
  mov ecx,eax
  and eax,0x000f0000
  shl eax,12
  mov ax,cx
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
  ; segment immediately after clearing bit 31 of cr0.  At least, you
  ; would think that... unfortunately we can't take advantage of
  ; prefetching; we're completely, absolutely SOL unless we pull
  ; some really, really dirty tricks.  (Like the code below.)
  ; We don't even have one prefetched instruction to work with,
  ; so there's no getting around it: we must identity-map .ftext.

  ; Prepare to load a new GDT and IDT.
  mov eax,_KexecLinuxBootGdtEnd
  sub eax,_KexecLinuxBootGdtStart
  mov word [_KexecLinuxBootGdtSize],ax
  push dword _KexecLinuxBootGdtStart
  call dword [__imp__MmGetPhysicalAddress@4]
  test edx,edx
  mov edx,6
  jnz near .bsod  ; possible kexec BSoD #6: GDT above 4GB
  mov dword [_KexecLinuxBootGdtPointer],eax

  mov eax,_KexecLinuxBootIdtEnd
  sub eax,_KexecLinuxBootIdtStart
  mov word [_KexecLinuxBootIdtSize],ax
  push dword _KexecLinuxBootIdtStart
  call dword [__imp__MmGetPhysicalAddress@4]
  test edx,edx
  mov edx,7
  jnz near .bsod  ; possible kexec BSoD #7: IDT above 4GB
  mov dword [_KexecLinuxBootIdtPointer],eax

  ; Now we used to patch the EIP the jump below us goes to (ugh!)
  ; I would have used the MDL method, but I'd sooner die than code
  ; that up in assembly.  Thus, the Nasty Evil Hack(tm) method.
  ; We gutted this, though, except for one side effect that the
  ; code below relies on.
  ; (Addendum: is this entire _project_ not a Nasty Evil Hack(tm)?)
  push dword _KexecLinuxBootFlatProtectedModeCode
  call dword [__imp__MmGetPhysicalAddress@4]
  test edx,edx
  mov edx,9
  jnz near .bsod  ; possible kexec BSoD #9: flat protected mode code above 4GB

  ; Now to mess directly with the page table and identity-map .ftext.
  ; Honestly, this could not be more of a PITA.
  ; (Needless to say, here be dragons!)
  ; To recap the stuff we have squirreled away in registers:
  ; EAX now has the physical address of .ftext.
  ; ESI has the physical address of the kernel map.
  ; EDI has the physical address of the start of the real-mode code.
  ; Come to think of it, we don't really need the kernel map anymore...
  mov esi,eax  ; for safe keeping

  ; XXX: DETECT AND HANDLE PAE!
  ; Doesn't it use larger page table entries?

  ; Abandon all interrupts, ye who execute here!
  cli

  ; Map the page directory.
  mov eax,cr3
  and eax,0xfffff000
  push dword 0
  push dword 0x00001000
  push dword 0
  push eax
  call dword [__imp__MmMapIoSpace@16]
  mov edx,10
  test eax,eax
  jz near .bsod  ; possible kexec BSoD #10: failed to map page directory

  ; Figure out which entry is relevant for us.
  mov ebx,esi
  shr ebx,20
  and ebx,0xfffffffc
  or dword [eax+ebx],0x00000063
  mov ebx,dword [eax+ebx]  ; Page directory entry (with page table pointer)

  ; Unmap the page directory and map the page table we need.
  push dword 0x00001000
  push eax
  call dword [__imp__MmUnmapIoSpace@8]

  ; XXX: ACTUALLY ALLOCATE A CHUNK OF RAM FOR THE PAGE TABLE!
  ; We more than likely just treated 0x00000000 as a page table entry,
  ; so we are about to overwrite four random bytes of the first 4KB of
  ; physical memory!  (Though we should be good as long as we don't hit
  ; an interrupt vector Linux needs to boot.)

  ; Map the page table.
  mov eax,ebx
  and eax,0xfffff000
  push dword 0
  push dword 0x00001000
  push dword 0
  push eax
  call dword [__imp__MmMapIoSpace@16]
  mov edx,11
  test eax,eax
  jz near .bsod  ; possible kexec BSoD #11: failed to map page table

  ; Scribble in an entry identity-mapping .ftext.
  mov ebx,esi
  shr ebx,10
  and ebx,0x00000ffc
  mov edx,esi
  and edx,0xfffff000
  or edx,0x00000063
  mov dword [eax+ebx],edx
  invlpg [esi]  ; Make sure the MMU knows what we did.

  ; Unmap the page table.
  push dword 0x00001000
  push eax
  call dword [__imp__MmUnmapIoSpace@8]

  ; Hop in; we can enter flat protected mode for real once we're there.
  ; (And we can finally get to the good part...)

  ; Load a simple GDT with just the null seg, a code seg, and a data seg.
  lgdt [_KexecLinuxBootGdtTag]
  ; Load a null IDT too.
  lidt [_KexecLinuxBootIdtTag]

  ; Start the stuff in .ftext.
  ; The GDT is in limbo right now, but by not touching CS we can get away
  ; with this.  (This is what makes "unreal" mode possible.)  We pass the
  ; address of the real-mode code in EDI.
  jmp esi

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

; More code! (Still 32-bit.) We identity-map this area and jump here
; so we can turn off paging and enter flat protected mode; we have it
; in its own section so we don't have to worry about straddling a
; page boundary.
section .ftext  ; .text for flat protected mode
bits 32

_KexecLinuxBootFlatProtectedModeCode:

incbin "linuxboot_blobs/flatpmode.bin"

; Still more code! (16-bit this time.) Call it data since that's what it
; is, as far as the 32-bit code is concerned...
section .rdata
bits 16

; Put us into the read-only data section of kexec.sys.  kexec.sys will
; copy us to real-mode memory and run us via the above routine.
_KexecLinuxBootRealModeCodeStart:
incbin "linuxboot_blobs/realmode.bin"
_KexecLinuxBootRealModeCodeEnd:

; We'll put our shiny new GDT and IDT in .rdata since they're, well, data.
_KexecLinuxBootGdtStart:
  ; The obligatory null descriptor.
  dq 0
  ; The code segment.
  dq 0x00cf9a000000ffff
  ; The data segment.
  dq 0x00cf92000000ffff
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