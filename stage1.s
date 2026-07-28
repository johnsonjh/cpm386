; CP/M-386
; Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
; SPDX-License-Identifier: MIT
; scspell-id: cf0d5c12-8a55-11f1-ab00-80ee73e9b8e7

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%include "layout.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bits 16
org 0x7c00

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

STAGE2_SEG equ 0x0000
STAGE2_OFF equ 0x7e00

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    mov [boot_dl], dl

    mov si, bootmsg
    call print

    mov dl, [boot_dl]
    xor ah, ah
    int 0x13                   ; reset disk

    mov ax, STAGE2_SEG
    mov es, ax
    mov bx, STAGE2_OFF
    mov si, 1                  ; LBA
    mov di, STAGE2_SECTORS

.load_one:
    test di, di
    jz .loaded

    mov ax, si
    xor dx, dx
    mov cx, 36
    div cx
    mov ch, al
    mov ax, dx
    mov cl, 18
    div cl
    mov dh, al
    mov cl, ah
    inc cl
    and cl, 0x3f

    mov ax, 0x0201
    mov dl, [boot_dl]
    int 0x13
    jnc .ok_read
    pusha
    xor ah, ah
    mov dl, [boot_dl]
    int 0x13
    popa
    mov ax, 0x0201
    mov dl, [boot_dl]
    int 0x13
    jc loaderr

.ok_read:
    add bx, 512
    inc si
    dec di
    jmp .load_one

.loaded:
    mov dl, [boot_dl]          ; confirm boot drive
    jmp STAGE2_SEG:STAGE2_OFF

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

loaderr:
    mov si, errmsg
    call print
    hlt
    jmp $

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

print:
    lodsb
    test al, al
    jz .pdon
    mov ah, 0x0e
    mov bh, 0
    int 0x10
    jmp print
.pdon:
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

boot_dl db 0
bootmsg db 13,10,"CP/M-386 stage 1 loader 0.1 (", BUILDDATE, ")",13,10,"Loading stage 2 ... ",0
errmsg  db "failed!",13,10,13,10,"CP/M-386 stage 1 boot failure, system halted!",13,10,13,10,0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%assign CODE_SIZE ($ - $$)
%warning info: Stage 1 code size is CODE_SIZE bytes

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

times 510 - ($ - $$) db 0
dw 0xaa55

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
