; CP/M-386
; Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
; SPDX-License-Identifier: MIT
; scspell-id: 964b0df0-8caf-11f1-8e75-80ee73e9b8e7

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; vidbios.s - protected mode <-> real mode transition for int 10h

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Selectors - keep in sync with pmode.h
SEL_KCODE   equ 0x08
SEL_KDATA   equ 0x10
SEL_RCODE16 equ 0x40
SEL_RDATA16 equ 0x48

; Low memory layout - keep in sync with vidbios.h
VIDLOW_CODE  equ 0x1000
VIDLOW_DATA  equ 0x1200
VIDLOW_STACK equ 0x2000

; Offsets within the data block at VIDLOW_DATA
R_AX     equ VIDLOW_DATA + 0
R_BX     equ VIDLOW_DATA + 2
R_CX     equ VIDLOW_DATA + 4
R_DX     equ VIDLOW_DATA + 6
R_SI     equ VIDLOW_DATA + 8
R_DI     equ VIDLOW_DATA + 10
R_BP     equ VIDLOW_DATA + 12
R_ES     equ VIDLOW_DATA + 14
R_DS     equ VIDLOW_DATA + 16
R_FLAGS  equ VIDLOW_DATA + 18
SAVE_ESP equ VIDLOW_DATA + 32
SAVE_GDT equ VIDLOW_DATA + 40   ; 6 bytes
SAVE_IDT equ VIDLOW_DATA + 48   ; 6 bytes

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .text
bits 32

; -----------------------------------------------------------------------------
; void vid_thunk_call (void)
;
; The register block at VIDLOW_DATA must already be filled in.  Everything
; the C caller owns is preserved; interrupts stay off throughout.
; -----------------------------------------------------------------------------
global vid_thunk_call
vid_thunk_call:
    pushad
    pushfd
    cli

    ; Stash the machine state the transition destroys.
    mov     [SAVE_ESP], esp
    sgdt    [SAVE_GDT]
    sidt    [SAVE_IDT]

    ; Real mode interrupt vector table.  The 386 consults IDTR in real mode
    ; too, so this has to be set before the int 10h or it vectors nowhere.
    lidt    [rm_idt_ptr]

    ; Into the copied trampoline, 16-bit protected mode.  SEL_RCODE16 has
    ; base 0, so the offset is the linear address - which is why the blob
    ; must live below 0x10000.
    jmp     SEL_RCODE16:VIDLOW_CODE

; -----------------------------------------------------------------------------
; Re-entry from real mode.  Reached by a 32-bit far jump once PE is set
; again; the linker resolves this address, so the blob needs no fixups.
; -----------------------------------------------------------------------------
global vid_thunk_return
vid_thunk_return:
    mov     ax, SEL_KDATA
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    mov     esp, [SAVE_ESP]

    lidt    [SAVE_IDT]

    popfd
    popad
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .rodata
align 4
rm_idt_ptr:
    dw 0x03FF                           ; real mode IVT: 256 * 4 bytes
    dd 0x00000000

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; The relocatable blob.
;
; Copied verbatim to VIDLOW_CODE at init.  Every branch inside it is
; relative, so the only address that needs fixing up is the one far jump
; that lands back inside it - computed at assembly time from VIDLOW_CODE.
; Data is reached through absolute low addresses, which are correct both in
; 16-bit protected mode (SEL_RDATA16 has base 0) and in real mode (DS = 0).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .text
bits 16
align 16

global vid_blob_start
vid_blob_start:

    ; Still protected mode, now with 16-bit segments.
    mov     ax, SEL_RDATA16
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax

    ; Leave protected mode.  CS keeps its cached base of 0 until the far
    ; jump below reloads it, so execution continues correctly meanwhile.
    mov     eax, cr0
    and     al, 0xFE
    mov     cr0, eax

    jmp     0x0000:(.real - vid_blob_start + VIDLOW_CODE)

.real:
    xor     ax, ax
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    mov     sp, VIDLOW_STACK

    ; Load the caller's registers.  DS last, since the block is read
    ; through it.
    mov     ax, [R_ES]
    mov     es, ax
    mov     bx, [R_BX]
    mov     cx, [R_CX]
    mov     dx, [R_DX]
    mov     si, [R_SI]
    mov     di, [R_DI]
    mov     bp, [R_BP]
    mov     ax, [R_AX]
    push    word [R_DS]
    pop     ds

    int     0x10

    ; Recover a known DS before touching the block again.
    push    ax
    xor     ax, ax
    mov     ds, ax
    pop     ax

    mov     [R_AX], ax
    mov     [R_BX], bx
    mov     [R_CX], cx
    mov     [R_DX], dx
    mov     [R_SI], si
    mov     [R_DI], di
    mov     [R_BP], bp
    mov     ax, es
    mov     [R_ES], ax
    pushf
    pop     ax
    mov     [R_FLAGS], ax

    ; Back to protected mode.
    cli
    o32 lgdt [SAVE_GDT]

    mov     eax, cr0
    or      al, 0x01
    mov     cr0, eax

    ; 32-bit far jump into the kernel proper.
    jmp     dword SEL_KCODE:vid_thunk_return

global vid_blob_end
vid_blob_end:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%assign CODE_SIZE ($ - $$)
%warning info: vidbios code size is CODE_SIZE bytes

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
