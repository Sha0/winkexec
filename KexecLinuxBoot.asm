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

; The code that goes back into real mode, then into flat protected mode,
; then pulls all the pieces of the kernel back together and boots it.
; The C code calls this with the physical address of the real-mode code
; and the kernel map as arguments.  The C must have loaded the real-mode
; code and kernel map into 16-byte aligned areas between
; 0x00500 and 0x98000.
global _KexecLinuxBoot
_KexecLinuxBoot:
  push ebp
  mov ebp,esp

  ; Check the offsets' validity.
  ; First the code...
  mov eax,dword [ebp+8]
  test eax,0x0000000f
  jnz .toast
  cmp eax,0x00000500
  jl .toast
  cmp eax,0x00098000
  jge .toast
  ; ...then the map.
  mov eax,dword [ebp+12]
  test eax,0x0000000f
  jnz .toast
  cmp eax,0x00000500
  jl .toast
  cmp eax,0x00098000
  jge .toast

  jmp short .goodtogo

.toast:
  ; Return to the C - there will be a BSoD shortly...
  leave
  ret

.goodtogo:
  ; It looks like we are good to go...
  ; let's kick Windows out, return to real mode, and invoke what hopefully
  ; is the code right below us.

  ; Abandon all interrupts, ye who execute here!
  cli

  ; Placeholder!
  nop

  ; Return to a quasi-sane state and return to imminent BSoD as long
  ; as this is just a skeleton.
  sti
  leave
  ret

; More code! (16-bit this time.) Call it data since that's what it
; is, as far as the 32-bit code is concerned...
section .rdata
bits 16

; Put us into the read-only data section of kexec.sys.  kexec.sys will
; copy us to real-mode memory and run us via the above routine.

global _KexecLinuxBootRealModeCodeStart
_KexecLinuxBootRealModeCodeStart:
  push bp
  mov bp,sp

  ; Placeholder!
  nop

  mov sp,bp
  pop bp
  ret
global _KexecLinuxBootRealModeCodeEnd
_KexecLinuxBootRealModeCodeEnd: