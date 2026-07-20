; CP/M-386
; Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
; SPDX-License-Identifier: MIT
; scspell-id: db39e440-82b4-11f1-aa93-80ee73e9b8e7

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; mbentry.s - Multiboot 1 header + entry point for cpm386.elf

bits 32

section .multiboot
align 4
mb_header_start:
    dd 0x1BADB002                   ; magic
    dd 0x00000003                   ; flags: bit0=align modules, bit1=provide meminfo (and mmap if avail)
    dd 0xE4524FFB                   ; checksum = -(magic + flags)  (must be exact)

; --- Actual entry code
section .text.start
global _start
_start:
    cli

    ; Multiboot passes magic in EAX, info pointer in EBX
    cmp eax, 0x2BADB002
    je .multiboot_path

    ; --- Legacy path (called by boot.S real-mode loader) ---
    ; The loader has already:
    ;   - switched to protected mode
    ;   - set up GDT + segments
    ;   - zeroed BSS
    ;   - set up stack
    ;   - filled 0x600 (top) and 0x604 (tpa) via real-mode detection
    ; We can just enter the common init.
    jmp .common_init

.multiboot_path:
    ; Save multiboot info pointer
    mov [mb_info_ptr], ebx

    ; Install our own known flat GDT (robustness, same as old loader)
    lgdt [gdt_desc]

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Temporary early stack (BSS not zeroed yet, so use this static area)
    mov esp, early_stack_top

    ; Zero BSS (same job boot.S used to do for us)
    extern __bss_start
    extern __bss_end
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    ; let C parse the multiboot info and fill the legacy memory variables
    ; (0x600 / 0x604) so bios_getmrt + TPA logic works without change
    extern mb_init_from_multiboot
    push dword [mb_info_ptr]
    call mb_init_from_multiboot
    add esp, 4

    ; set up a proper runtime stack after whole kernel (incl. ramdisk)
    extern __kernel_end
    mov ebx, __kernel_end
    add ebx, 0x4000                 ; stack reserve
    mov ecx, [0x600]
    cmp ecx, 0
    jne .have_top
    mov ecx, 0xFFFFFFFF
.have_top:
    cmp ebx, ecx
    jb .stok
    mov ebx, ecx
    sub ebx, 0x1000
.stok:
    mov esp, ebx

.common_init:
    ; jump into the existing C bring-up (banner, cpm_bringup, ccp, etc.)
    extern cpm386_init
    call cpm386_init

    ; if ccp ever returns, halt
.hang:
    cli
    hlt
    jmp .hang

; ---------------------------------------------------------------------
; Data
; ---------------------------------------------------------------------
section .data
align 4
mb_info_ptr:
    dd 0

; small bootstrap stack for before we zero BSS and pick real stack
section .bss
align 16
    resb 8192
early_stack_top:

; flat 32-bit GDT (code 0x08, data 0x10) identical layout to boot.S
section .data
align 4
gdt:
    dq 0                            ; null
    ; code 0x08 : base 0, limit 4G, 32-bit, rx
    db 0xff, 0xff, 0x00, 0x00, 0x00, 0x9a, 0xcf, 0x00
    ; data 0x10 : base 0, limit 4G, 32-bit, rw
    db 0xff, 0xff, 0x00, 0x00, 0x00, 0x92, 0xcf, 0x00
gdt_end:

gdt_desc:
    dw gdt_end - gdt - 1
    dd gdt
