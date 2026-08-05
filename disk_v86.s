; CP/M-386
; Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
; SPDX-License-Identifier: MIT
; scspell-id: 201202de-9104-11f1-ba2d-80ee73e9b8e7

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; disk_v86.s - real-mode part of the V86 disk server
;
; This blob is copied verbatim to V86_CODE_ADDR and run as a virtual-8086
; task.  It holds no state and makes no decisions: the kernel owns every
; register through v86_state (pmode.s), so all this has to do is turn that
; state into one call through the real-mode interrupt vector table and hand
; the results back.
;
; The obvious `int 13h` is exactly what must not be used.  In virtual-8086
; mode INT n vectors through the *protected mode* IDT, so it would land in
; the kernel's exception stubs rather than the ROM.  PUSHF plus a far CALL
; through the IVT builds the identical stack frame the BIOS handler's IRET
; expects - FLAGS, CS, IP - without the trap.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Keep in sync with disk.h
V86_YIELD_INT equ 0x31          ; yield back to the kernel
V86_FARPTR    equ 0x00003400    ; kernel-supplied seg:off of the ROM handler

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

section .text
bits 16
align 16

global disk_v86_start
disk_v86_start:

    ; First yield: the task exists and is parked.  Every later resume comes
    ; back here-plus-two with the kernel's registers already loaded.
    int     V86_YIELD_INT

.call:
    pushf                       ; the FLAGS the ROM handler's IRET will pop
    cli                         ; ... left as a real INT would have left them
    call    far [V86_FARPTR]

    ; AX and FLAGS (notably CF) now hold the BIOS result.  Yielding does not
    ; disturb them: the CPU pushes EFLAGS before clearing IF, and the kernel
    ; snapshots the lot.
    int     V86_YIELD_INT

    jmp     .call

global disk_v86_end
disk_v86_end:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

%assign CODE_SIZE (disk_v86_end - disk_v86_start)
%warning info: disk_v86 code size is CODE_SIZE bytes

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
