; CP/M-386
; Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
; SPDX-License-Identifier: MIT
; scspell-id: 800fa5d2-9416-11f1-87d0-80ee73e9b8e7

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; lz4dec.inc - lz4 decompressor

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

lz4_boot_decompress:

.lz4d_next_block:
    mov   ebx, [esi]          ; read 4-byte block size
    add   esi, 4

    test  ebx, ebx            ; end mark?
    jz    .lz4d_done

    ; Check for uncompressed block
    test  ebx, 0x80000000
    jnz   .lz4d_raw

    and   ebx, 0x7FFFFFFF     ; mask off high bit

    mov   ebp, esi
    add   ebp, ebx            ; EBP = absolute end of compressed block

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.lz4d_next_token:
    cmp   esi, ebp
    jae   .lz4d_next_block

    movzx eax, byte [esi]     ; read token byte
    inc   esi
    mov   edx, eax            ; save copy for match length
    shr   eax, 4              ; isolate literal length (high 4 bits)
    jz    .lz4d_skip_lits

    cmp   al, 15
    jne   .lz4d_copy_lits
.lz4d_lit_ext:
    movzx ecx, byte [esi]
    inc   esi
    add   eax, ecx
    cmp   cl, 255
    je    .lz4d_lit_ext

.lz4d_copy_lits:
    mov   ecx, eax            ; ECX = total literal count
    shr   ecx, 2              ; DWORD count
    jz    .lz4d_lit_rem
    rep   movsd               ; transfer 4 bytes at a time

.lz4d_lit_rem:
    mov   ecx, eax
    and   ecx, 3              ; remaining 0..3 bytes
    rep   movsb

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.lz4d_skip_lits:
    cmp   esi, ebp            ; check if block ended after literals
    jae   .lz4d_next_block

    and   edx, 0x0F           ; isolate match length (low 4 bits)

    movzx eax, word [esi]     ; 16-bit match offset
    add   esi, 2

    add   edx, 4              ; baseline minimum match = 4
    cmp   dl, 19              ; 15 + 4 = 19 means extended
    jne   .lz4d_copy_match
.lz4d_match_ext:
    movzx ecx, byte [esi]
    inc   esi
    add   edx, ecx
    cmp   cl, 255
    je    .lz4d_match_ext

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.lz4d_copy_match:
    push  esi                  ; save input stream position
    mov   esi, edi
    sub   esi, eax             ; source = dest - offset

    cmp   eax, 4
    jl    .lz4d_safe_match

    mov   ecx, edx             ; ECX = match length
    shr   ecx, 2               ; DWORD count
    jz    .lz4d_match_rem
    rep   movsd                ; copy 4 bytes at a time

.lz4d_match_rem:
    mov   ecx, edx
    and   ecx, 3               ; remaining 0..3 bytes
    rep   movsb
    jmp   .lz4d_match_done

.lz4d_safe_match:
    mov   ecx, edx             ; small-offset fallback: byte-by-byte
.lz4d_safe_loop:
    lodsb
    stosb
    loop  .lz4d_safe_loop

.lz4d_match_done:
    pop   esi                  ; restore source context
    jmp   .lz4d_next_token

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.lz4d_raw:
    ; Uncompressed block: copy verbatim
    and   ebx, 0x7FFFFFFF
    mov   ecx, ebx
    shr   ecx, 2
    rep   movsd
    mov   ecx, ebx
    and   ecx, 3
    rep   movsb
    jmp   .lz4d_next_block

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.lz4d_done:
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Local Variables:
; mode: asm
; indent-tabs-mode: nil
; tab-width: 4
; fill-column: 80
; eval: (setq-local display-fill-column-indicator-column 80)
; eval: (display-fill-column-indicator-mode 1)
; End:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; vim: set ft=asm ts=4 sw=4 tw=0 ai expandtab cc=80 :
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
