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

section .text
bits 32

; void swap_page(unsigned long p1_lower, unsigned long p1_higher,
;                unsigned long p2_lower, unsigned long p2_higher);
;
; Swaps the 4KB chunks of physical memory pointed to by
; p1_higher:p1_lower and p2_higher:p2_lower.
; Assumes paging with PAE is enabled with the first 1 MB of physical memory
; identity-mapped and the page table for the first 2 MB at 0x00091000.
global _swap_page
_swap_page:
  push ebp
  mov ebp, esp
  push esi
  push edi

  ; Map the pages at 0x00100000 and 0x00101000.
  mov eax, [ebp + 8]
  or eax, 0x00000023
  mov dword [0x00091800], eax
  mov eax, [ebp + 12]
  mov dword [0x00091804], eax
  mov eax, [ebp + 16]
  or eax, 0x00000023
  mov dword [0x00091808], eax
  mov eax, [ebp + 20]
  mov dword [0x0009180c], eax
  invlpg [0x00100000]
  invlpg [0x00100100]

  ; Copy page 1 to the scratch area.
  cld
  mov esi, 0x00100000
  mov edi, 0x00092000
  mov ecx, 1024
  rep movsd

  ; Copy page 2 to page 1.
  mov esi, 0x00101000
  mov edi, 0x00100000
  mov ecx, 1024
  rep movsd

  ; Copy the scratch area to page 2.
  mov esi, 0x00092000
  mov edi, 0x00101000
  mov ecx, 1024
  rep movsd

  ; Unmap the pages.
  mov edi, 0x00091800
  mov ecx, 4
  xor eax, eax
  rep stosd

  pop edi
  pop esi
  leave
  ret
