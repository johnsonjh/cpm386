; CP/M-386
; Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
; SPDX-License-Identifier: MIT
; scspell-id: cd33f1e4-8a55-11f1-8737-80ee73e9b8e7

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%include "layout.inc"
%include "bss.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bits 16
org 0x7e00

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

KERNEL_LBA equ 1 + STAGE2_SECTORS

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

stage2_entry:
    cld
    mov [boot_dl], dl

    mov si, bootmsg
    call print
    call detect_memory

    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov si, KERNEL_LBA
    mov di, SECTORS_TO_LOAD

.load_one:
    test di, di
    jz .loaded

    cmp bx, 0xfe00
    jbe .chs
    mov ax, es
    mov dx, bx
    mov cl, 4
    shr dx, cl
    add ax, dx
    mov es, ax
    and bx, 0x000f

.chs:
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
    jnc .adv
    mov ax, es
    add ax, 0x1000
    mov es, ax
.adv:
    inc si
    dec di
    jmp .load_one

.loaded:
    mov ax, 0x2401
    int 0x15
    in al, 0x92
    or al, 2
    and al, 0xfe
    out 0x92, al

    cli
    cld
    lgdt [gdt_desc]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:pm_entry

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

E820_SMAP equ 0x534d4150

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

detect_memory:
    pusha
    xor ebx, ebx
    mov byte [e820_run], 0

.e820_loop:
    mov eax, 0xe820
    mov edx, E820_SMAP
    mov ecx, 24
    mov di, e820_buf
    int 0x15
    jc .e820_done
    cmp eax, E820_SMAP
    jne .e820_done
    cmp ecx, 20
    jb .e820_done

    cmp dword [e820_buf+4], 0
    jnz .e820_next

    mov eax, [e820_buf+8]
    add eax, [e820_buf+0]
    jc .e820_clamp
    cmp dword [e820_buf+12], 0
    jz .e820_endok
.e820_clamp:
    mov eax, 0xffffffff
.e820_endok:
    cmp byte [e820_run], 0
    jne .e820_ext

    cmp dword [e820_buf+16], 1
    jne .e820_next
    cmp eax, 0x100000
    jbe .e820_next
    mov byte [e820_run], 1
    mov [e820_top], eax
    jmp .e820_next

.e820_ext:
    mov edx, [e820_buf+0]
    cmp edx, [e820_top]
    jne .e820_done
    cmp dword [e820_buf+16], 1
    jne .e820_done
    mov [e820_top], eax

.e820_next:
    test ebx, ebx
    jnz .e820_loop

.e820_done:
    cmp byte [e820_run], 0
    je .try_e801
    mov ecx, [e820_top]
    jmp .store

.try_e801:
    mov ax, 0xe801
    xor bx, bx
    int 0x15
    jc .try88
    test ax, ax
    jnz .have
    mov ax, cx
    mov bx, dx
.have:
    movzx edx, ax
    movzx eax, bx
    shl eax, 6
    add edx, eax
    jmp .addext
.try88:
    mov ah, 0x88
    int 0x15
    jc .fallback
    movzx edx, ax
.addext:
    test edx, edx
    jz .noext
    mov ecx, 1024
    add ecx, edx
    jmp .scale
.noext:
    mov ax, [0x413]
    movzx ecx, ax
.scale:
    test ecx, ecx
    jnz .ok
.fallback:
    mov ecx, 256
.ok:
    cmp ecx, 0x400000
    jb .kbytes
    mov ecx, 0xffffffff
    jmp .store
.kbytes:
    shl ecx, 10
.store:
    mov [0x600], ecx
    popa
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

boot_dl db 0
bootmsg db "OK.",13,10,"CP/M-386 stage 2 loader 0.1 (", BUILDDATE, ")",13,10,"Loading CP/M-386 ... ",13,10,13,10,0
errmsg  db "CP/M-386 stage 2 boot failed, system halted!",0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

e820_run db 0
e820_top dd 0
e820_buf times 24 db 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 4
gdt:
    dq 0
    db 0xff,0xff,0,0,0,0x9a,0xcf,0
    db 0xff,0xff,0,0,0,0x92,0xcf,0
gdt_end:
gdt_desc:
    dw gdt_end - gdt - 1
    dd gdt

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bits 32
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov edi, bss_start
    mov ecx, bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    mov ebx, kernel_end
    add ebx, 0x4000
    mov ecx, [0x600]
    cmp ebx, ecx
    jb .stok
    mov ebx, ecx
    sub ebx, 0x1000
.stok:
    mov esp, ebx
    add ebx, 0x1000
    cmp ebx, ecx
    jb .tpok
    mov ebx, ecx
    sub ebx, 0x1000
.tpok:
    mov [0x604], ebx

    mov eax, kernel_entry
    call eax

.hang:
    cli
    hlt
    jmp .hang

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%assign CODE_SIZE ($ - $$)
%warning info: Stage 2 code size is CODE_SIZE bytes

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

times (STAGE2_SECTORS * 512) - ($ - $$) db 0

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
