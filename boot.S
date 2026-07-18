; CP/M-386
; Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
; SPDX-License-Identifier: MIT
; scspell-id: 598338c0-82b4-11f1-9adf-80ee73e9b8e7

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; boot.S - 16-bit floppy boot for CP/M-386
; BIOS loads this at 0000:7C00 with DL = boot drive.
; Loads os.bin (LBA 1..) into linear 0x10000, enters protected mode, jumps to kernel.
;
; Notes:
;  - Reads one sector at a time with 64K-DMA boundary normalization
;  - Uses the BIOS boot drive in DL (not hard-coded to 0)
;  - pmode_init() rebuilds the full GDT/TSS/IDT later
;  - SECTORS_TO_LOAD / kernel_* symbols come from bss.inc (nm of cpm386.elf)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%include "bss.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bits 16
org 0x7c00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7c00
    mov [boot_dl], dl          ; save BIOS boot drive

    mov si, bootmsg
    call print
    call detect_memory

    ; Reset disk system
    mov dl, [boot_dl]
    xor ah, ah
    int 0x13

    ; Load SECTORS_TO_LOAD sectors starting LBA 1 -> 0x10000
    mov ax, 0x1000
    mov es, ax
    xor bx, bx                 ; ES:BX = 0x10000
    mov si, 1                  ; current LBA (0-based; LBA0 = boot sector)
    mov di, SECTORS_TO_LOAD

.load_one:
    test di, di
    jz .loaded

    ; Ensure ES:BX does not cross a 64K DMA boundary for a 512-byte read
    cmp bx, 0xfe00
    jbe .chs
    ; Normalize: linear = ES*16+BX; set ES = linear/16, BX = linear&15
    mov ax, es
    mov dx, bx
    mov cl, 4
    shr dx, cl                 ; bx/16
    add ax, dx
    mov es, ax
    and bx, 0x000f

.chs:
    ; LBA SI -> CHS for 1.44M (18 spt, 2 heads): C=LBA/36, H=(LBA%36)/18, S=(LBA%18)+1
    mov ax, si
    xor dx, dx
    mov cx, 36
    div cx                     ; AX=cylinder, DX=temp
    mov ch, al                 ; cyl low (0..79)
    mov ax, dx
    mov cl, 18
    div cl                     ; AL=head, AH=sector0
    mov dh, al
    mov cl, ah
    inc cl                     ; sector 1..18
    and cl, 0x3f

    mov ax, 0x0201             ; read 1 sector
    mov dl, [boot_dl]
    int 0x13
    jnc .ok_read
    ; one retry after reset
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
    add ax, 0x1000             ; +64K in segment form
    mov es, ax
.adv:
    inc si
    dec di
    jmp .load_one

.loaded:
    cli
    lgdt [gdt_desc]
    mov eax, cr0
    or al, 1
    mov cr0, eax
    jmp 0x08:pm_entry

loaderr:
    mov si, errmsg
    call print
    hlt
    jmp $

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

; Memory top (bytes) into [0x600]
detect_memory:
    pusha
    mov ax, 0xe801
    xor bx, bx
    int 0x15
    jc .try88
    test ax, ax
    jnz .have
    mov ax, cx
    mov bx, dx
.have:
    movzx edx, ax              ; 1K blocks below 16M
    movzx eax, bx              ; 64K blocks above 16M
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
    mov ax, [0x413]            ; KB conventional
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

; ---- data (keep compact) ----
boot_dl db 0
bootmsg db 13,10,"CP/M-386",13,10,0
errmsg  db "Fail",0

align 4
gdt:
    dq 0
    db 0xff,0xff,0,0,0,0x9a,0xcf,0   ; 0x08 code
    db 0xff,0xff,0,0,0,0x92,0xcf,0   ; 0x10 data
gdt_end:
gdt_desc:
    dw gdt_end - gdt - 1
    dd gdt

bits 32
pm_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Zero BSS only (ramdisk is in .ramdisk with data, not BSS)
    mov edi, bss_start
    mov ecx, bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    ; Stack and TPA after full kernel including ramdisk
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

times 510 - ($ - $$) db 0
dw 0xaa55
