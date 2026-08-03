; CP/M-386
; Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
; SPDX-License-Identifier: MIT
; scspell-id: db39e440-82b4-11f1-aa93-80ee73e9b8e7

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; mbentry.s - Multiboot 1 header + entry point for cpm386.elf

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

bits 32

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .multiboot
align 4
mb_header_start:
    dd 0x1BADB002                   ; magic
    dd 0x00000003                   ; flags: bit0=align modules, bit1=provide meminfo (and mmap if avail)
    dd 0xE4524FFB                   ; checksum = -(magic + flags)  (must be exact)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; --- Actual entry code
section .text.start
global _start
_start:
    cli

    ; Multiboot passes magic in EAX, info pointer in EBX
    cmp eax, 0x2BADB002
    je .multiboot_path

    ; --- Legacy path (called by stage 1 real-mode boot loader) ---
    ; The loader has already:
    ;   - switched to protected mode
    ;   - set up GDT + segments
    ;   - zeroed BSS
    ;   - set up stack
    ;   - published the memory descriptor at 0x600 (see memmap.h)
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

    ; Zero BSS (same job the boot loader used to do for us)
    extern __bss_start
    extern __bss_end
    mov edi, __bss_start
    mov ecx, __bss_end
    sub ecx, edi
    xor eax, eax
    rep stosb

    ; let C parse the multiboot info and publish the memory descriptor at
    ; 0x600 (see memmap.h) so bios_getmrt sees the same thing on both paths
    extern mb_init_from_multiboot
    push dword [mb_info_ptr]
    call mb_init_from_multiboot
    add esp, 4

    ; Runtime stack directly above the kernel image (incl. ramdisk), in
    ; conventional memory - same placement as the stage 2 loader uses.
    ; The TPA is above 1MB and is validated by the kernel, not here.
    extern __kernel_end
    mov esp, __kernel_end + 0x4000

.common_init:
    ; jump into the existing C bring-up (banner, cpm_bringup, ccp, etc.)
    extern cpm386_init
    call cpm386_init

    ; if ccp ever returns, halt
.hang:
    cli
    hlt
    jmp .hang

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; ---------------------------------------------------------------------
; Data
; ---------------------------------------------------------------------
section .data
align 4
mb_info_ptr:
    dd 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; small bootstrap stack for before we zero BSS and pick real stack
section .bss
align 16
    resb 8192
early_stack_top:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; flat 32-bit GDT (code 0x08, data 0x10)
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
