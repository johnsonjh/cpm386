; CP/M-386 - layout.inc
; Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
; SPDX-License-Identifier: MIT
; scspell-id: 9683fc20-8a55-11f1-800a-80ee73e9b8e7

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; 16 sectors (8K): stage 2 retains the whole E820 map for merging, and
; stage 1 loads it to 0x7e00, so it must stay clear of 0x10000.
STAGE2_SECTORS equ 16

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Local Variables:
; mode: asm
; indent-tabs-mode: nil
; tab-width: 4
; fill-column: 80
; eval: (setq-local display-fill-column-indicator-column 80)
; eval: (display-fill-column-indicator-mode 1)
; End:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; vim: set ft=asm ts=4 sw=4 tw=0 ai expandtab cc=80 :
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
