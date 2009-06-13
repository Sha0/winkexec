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

; Various utility functions written in assembly for use by the final bit
; of C code to set things up for the Linux boot.

section .text
bits 32

; Disable interrupts.
global _util_cli
_util_cli:
  cli
  ret

; Repeatedly halt the processor in a never-ending loop.
; Does not return.
global _util_hlt
_util_hlt:
  hlt
  jmp short _util_hlt

; Figure out whether PAE is enabled.
; Returns 1 if true or 0 if false.
global _util_pae_enabled
_util_pae_enabled:
  mov eax, cr4
  and eax, 0x00000020
  shr eax, 5
  ret

; Get cr3, which is the physical address of the page directory.
global _util_get_cr3
_util_get_cr3:
  mov eax, cr3
  ret

; Flush from the TLB the page whose address is passed as arg1.
global _util_invlpg
_util_invlpg:
  mov eax, [esp + 4]
  invlpg [eax]
  ret

; Get the current processor number.
; (Because KeGetCurrentProcessorNumber() epically fails in MinGW.)
global _util_current_processor
_util_current_processor:
  movzx eax, byte [fs:0x00000051]
  ret
