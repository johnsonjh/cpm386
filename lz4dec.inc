; CP/M-386 - lz4dec.inc
; Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
; SPDX-License-Identifier: MIT
; scspell-id: 800fa5d2-9416-11f1-87d0-80ee73e9b8e7

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; lz4dec.inc - lz4 decompressor

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

lz4_decompress:

.lz4d_next_block:
    mov   ecx, [esi]          ; read 4-byte block size
    add   esi, 4

    test  ecx, ecx            ; end mark?
    jz    .lz4d_done

    ; Check for uncompressed block
    test  ecx, 0x80000000
    jnz   .lz4d_raw

    and   ecx, 0x7FFFFFFF     ; block size
    mov   ebx, esi
    add   ebx, ecx            ; EBX = absolute end of compressed block

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.lz4d_next_token:
    cmp   esi, ebx
    jae   .lz4d_next_block

    movzx eax, byte [esi]     ; token
    inc   esi
    mov   edx, eax            ; save full token
    shr   eax, 4              ; literal length (high nibble)
    jz    .lz4d_skip_lits

    cmp   al, 15
    jne   .lz4d_lit_len_ready

.lz4d_lit_ext:
    movzx ecx, byte [esi]
    inc   esi
    add   eax, ecx
    cmp   ecx, 255
    je    .lz4d_lit_ext

.lz4d_lit_len_ready:
    ; EAX = literal length
    mov   ecx, eax
    test  ecx, ecx
    jz    .lz4d_skip_lits

    ; copy literals unrolled in 16-byte chunks then bytes
.lz4d_copy_lits_dwords:
    cmp   ecx, 16
    jb    .lz4d_copy_lits_bytes

    mov   eax, [esi]
    mov   [edi], eax
    mov   eax, [esi+4]
    mov   [edi+4], eax
    mov   eax, [esi+8]
    mov   [edi+8], eax
    mov   eax, [esi+12]
    mov   [edi+12], eax

    add   esi, 16
    add   edi, 16
    sub   ecx, 16
    jmp   .lz4d_copy_lits_dwords

.lz4d_copy_lits_bytes:
    test  ecx, ecx
    jz    .lz4d_skip_lits

.lz4d_copy_lits_bytes_loop:
    mov   al, [esi]
    mov   [edi], al
    inc   esi
    inc   edi
    dec   ecx
    jnz   .lz4d_copy_lits_bytes_loop

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.lz4d_skip_lits:
    cmp   esi, ebx            ; check if block ended after literals
    jae   .lz4d_next_block

    and   edx, 0x0F           ; match length nibble

    movzx eax, word [esi]     ; 16-bit match offset
    add   esi, 2

    add   edx, 4              ; baseline minimum match = 4
    cmp   dl, 19              ; 15 + 4 = 19 means extended
    jne   .lz4d_match_len_ready

.lz4d_match_ext:
    movzx ecx, byte [esi]
    inc   esi
    add   edx, ecx
    cmp   ecx, 255
    je    .lz4d_match_ext

.lz4d_match_len_ready:
    ; EAX = offset, EDX = match length
    push  esi                 ; save input stream position
    mov   esi, edi
    sub   esi, eax            ; source = dest - offset

    mov   ecx, edx
    cmp   eax, 4
    jb    .lz4d_safe_match

    ; offset >= 4: forward copy 4-byte chunks then bytes
.lz4d_copy_match_dwords:
    cmp   ecx, 4
    jb    .lz4d_copy_match_bytes

    mov   eax, [esi]
    mov   [edi], eax
    add   esi, 4
    add   edi, 4
    sub   ecx, 4
    jmp   .lz4d_copy_match_dwords

.lz4d_copy_match_bytes:
    test  ecx, ecx
    jz    .lz4d_match_done_restore

.lz4d_copy_match_bytes_loop:
    mov   al, [esi]
    mov   [edi], al
    inc   esi
    inc   edi
    dec   ecx
    jnz   .lz4d_copy_match_bytes_loop
    jmp   .lz4d_match_done_restore

.lz4d_safe_match:
    ; small-offset / overlapping fallback: byte-by-byte
    test  ecx, ecx
    jz    .lz4d_match_done_restore

.lz4d_safe_match_loop:
    mov   al, [esi]
    mov   [edi], al
    inc   esi
    inc   edi
    dec   ecx
    jnz   .lz4d_safe_match_loop

.lz4d_match_done_restore:
    pop   esi                 ; restore source context
    jmp   .lz4d_next_token

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.lz4d_raw:
    ; Uncompressed block: copy as-is (ECX = size)
    and   ecx, 0x7FFFFFFF
    test  ecx, ecx
    jz    .lz4d_next_block

    ; unrolled 16-byte chunks then bytes
.lz4d_raw_dwords:
    cmp   ecx, 16
    jb    .lz4d_raw_bytes

    mov   eax, [esi]
    mov   [edi], eax
    mov   eax, [esi+4]
    mov   [edi+4], eax
    mov   eax, [esi+8]
    mov   [edi+8], eax
    mov   eax, [esi+12]
    mov   [edi+12], eax

    add   esi, 16
    add   edi, 16
    sub   ecx, 16
    jmp   .lz4d_raw_dwords

.lz4d_raw_bytes:
    test  ecx, ecx
    jz    .lz4d_next_block

.lz4d_raw_bytes_loop:
    mov   al, [esi]
    mov   [edi], al
    inc   esi
    inc   edi
    dec   ecx
    jnz   .lz4d_raw_bytes_loop
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
