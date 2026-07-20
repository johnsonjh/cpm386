; CP/M-386
; Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
; SPDX-License-Identifier: MIT
; scspell-id: 7a9ae46c-82b5-11f1-ac87-80ee73e9b8e7

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; pmode.s - ring-3 entry/exit and BDOS int 0x30 trampoline

bits 32

section .text

; -----------------------------------------------------------------------------
; void enter_ring3(uint32_t entry_off, uint32_t user_esp)
;   [esp+4] = user EIP (TPA-relative)
;   [esp+8] = user ESP (TPA-relative)
; Returns only after BDOS(0) via return_to_kernel.
; -----------------------------------------------------------------------------
global enter_ring3
enter_ring3:
    push    ebp
    push    ebx
    push    esi
    push    edi

    ; resume_esp points at the saved regs so return_to_kernel can pop+ret
    mov     [resume_esp], esp

    mov     eax, [esp + 20]     ; entry_off (4 regs + retaddr)
    mov     ebx, [esp + 24]     ; user_esp

    ; Build iretd frame: SS, ESP, EFLAGS, CS, EIP
    ; Keep IF=0: no PIC handlers yet; timer IRQ0 (vec 8) would hang.
    push    dword 0x23          ; SS = user data | RPL3
    push    ebx                 ; ESP
    pushf
    and     dword [esp], 0xFFFFFDFF  ; clear IF
    push    dword 0x1B          ; CS = user code | RPL3
    push    eax                 ; EIP

    ; User data segments before privilege drop
    mov     ax, 0x23
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax

    iretd
    ; not reached

; -----------------------------------------------------------------------------
; return_to_kernel - abandon int-stack, resume enter_ring3 caller (ring 0)
; -----------------------------------------------------------------------------
global return_to_kernel
return_to_kernel:
    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    mov     esp, [resume_esp]
    pop     edi
    pop     esi
    pop     ebx
    pop     ebp
    xor     eax, eax
    ret

; -----------------------------------------------------------------------------
; bdos_irq - IDT vector 0x30 (DPL=3 interrupt gate)
; User ABI: EAX = function, EDX = info/infop (TPA-relative if pointer)
; Return:   EAX = UWORD result
; Ring-0 stack frame from hardware (from ring 3):
;   [esp+0]=eip [esp+4]=cs [esp+8]=eflags [esp+12]=esp [esp+16]=ss
; -----------------------------------------------------------------------------
global bdos_irq
extern bdos_syscall_c

bdos_irq:
    ; Save user GPRs / segments. After these pushes:
    ; [esp+0]=ebp [4]=edi [8]=esi [12]=edx [16]=ecx [20]=ebx [24]=eax
    ; [28]=gs [32]=fs [36]=es [40]=ds
    ; [44]=eip [48]=cs [52]=eflags [56]=uesp [60]=uss   (from ring3)
    push    ds
    push    es
    push    fs
    push    gs
    push    eax
    push    ebx
    push    ecx
    push    edx
    push    esi
    push    edi
    push    ebp

    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax

    mov     eax, [esp + 24]     ; user EAX (func)
    mov     edx, [esp + 12]     ; user EDX (info)
    mov     ecx, [esp + 48]     ; user CS (RPL)

    push    ecx                 ; cs
    push    edx                 ; info
    push    eax                 ; func
    call    bdos_syscall_c
    add     esp, 12
    ; result in EAX - store into saved user eax slot for restore
    mov     [esp + 24], eax

    pop     ebp
    pop     edi
    pop     esi
    pop     edx
    pop     ecx
    pop     ebx
    pop     eax
    pop     gs
    pop     fs
    pop     es
    pop     ds
    iretd

; -----------------------------------------------------------------------------
; CPU exception stubs (vectors 0-31)
; No-error-code: push fake 0, then vector, then common.
; Error-code:    CPU pushed err; push vector, then common.
; Stack on entry to exc_common:
;   [esp+0]=vec [4]=err [8]=eip [12]=cs [16]=eflags
;   [20]=uesp [24]=uss   (only when faulted from ring 3)
; -----------------------------------------------------------------------------
%macro EXC_NOERR 1
exc_stub_%1:
    push    dword 0
    push    dword %1
    jmp     exc_common
%endmacro

%macro EXC_ERR 1
exc_stub_%1:
    push    dword %1
    jmp     exc_common
%endmacro

EXC_NOERR 0   ; #DE
EXC_NOERR 1   ; #DB
EXC_NOERR 2   ; NMI
EXC_NOERR 3   ; #BP
EXC_NOERR 4   ; #OF
EXC_NOERR 5   ; #BR
EXC_NOERR 6   ; #UD
EXC_NOERR 7   ; #NM
EXC_ERR   8   ; #DF
EXC_NOERR 9   ; coprocessor segment (legacy)
EXC_ERR   10  ; #TS
EXC_ERR   11  ; #NP
EXC_ERR   12  ; #SS
EXC_ERR   13  ; #GP
EXC_ERR   14  ; #PF
EXC_NOERR 15  ; reserved
EXC_NOERR 16  ; #MF
EXC_ERR   17  ; #AC
EXC_NOERR 18  ; #MC
EXC_NOERR 19  ; #XM
EXC_NOERR 20  ; #VE
EXC_NOERR 21
EXC_NOERR 22
EXC_NOERR 23
EXC_NOERR 24
EXC_NOERR 25
EXC_NOERR 26
EXC_NOERR 27
EXC_NOERR 28
EXC_NOERR 29
EXC_NOERR 30
EXC_NOERR 31

extern fault_handler_c

; Stack layout after this prologue (struct fault_frame in pmode.c):
;   [esp+ 0] ebp  [ 4] edi  [ 8] esi  [12] edx
;   [esp+16] ecx  [20] ebx  [24] eax
;   [esp+28] gs   [32] fs   [36] es   [40] ds
;   [esp+44] vec  [48] err  [52] eip  [56] cs
;   [esp+60] eflags
;   [esp+64] uesp [68] uss   (only if faulted from ring 3)
exc_common:
    push    ds
    push    es
    push    fs
    push    gs
    push    eax
    push    ebx
    push    ecx
    push    edx
    push    esi
    push    edi
    push    ebp

    mov     ax, 0x10
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax

    push    esp                 ; &fault_frame
    call    fault_handler_c
    add     esp, 4

    ; eax == 1: abort ring-3 program back to enter_ring3 caller
    ; eax == 0: unrecoverable (kernel) fault
    test    eax, eax
    jnz     return_to_kernel

.hang:
    cli
    hlt
    jmp     .hang

; Table of stub addresses for pmode_init (vectors 0..31)
global exc_stub_table
align 4
exc_stub_table:
%assign i 0
%rep 32
    dd exc_stub_%+i
%assign i i+1
%endrep

section .data
align 4
global resume_esp
resume_esp:
    dd 0
