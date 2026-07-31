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
KSTACK_RESERVE equ 0x4000

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; memory descriptor handed to the kernel - keep sync'd with memmap.h

MEMMAP_ADDR   equ 0x600
MEMMAP_MAGIC  equ 0x334D5043            ; "CPM3"
TPA_MIN_BASE  equ 0x100000
TPA_MAX_TOP   equ 0xE0000000

MEMF_E820     equ 0x0001
MEMF_E801     equ 0x0002
MEMF_88       equ 0x0004
MEMF_A20      equ 0x0020

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

stage2_entry:
    cld
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov [boot_dl], dl

    mov si, bootmsg
    call print

    call enable_a20
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
    jmp haltsys

haltsys:
    cli
    hlt
    jmp haltsys

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

enable_a20:
    call a20_test
    test al, al
    jnz .ok

    mov ax, 0x2401
    int 0x15
    call a20_test
    test al, al
    jnz .ok

    cli
    call a20_kbc
    mov cx, 64
.kbc_wait:
    call a20_test
    test al, al
    jnz .ok
    loop .kbc_wait

    call a20_fast
    mov cx, 64
.fast_wait:
    call a20_test
    test al, al
    jnz .ok
    loop .fast_wait

    sti
    mov si, a20msg
    call print
    jmp haltsys

.ok:
    sti
    or dword [mm_flags], MEMF_A20
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

a20_test:
    push ds
    push es
    push si
    push di
    push bx
    xor ax, ax
    mov es, ax
    mov di, 0x0500
    mov ax, 0xFFFF
    mov ds, ax
    mov si, 0x0510
    mov bh, [es:di]
    mov bl, [ds:si]
    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF
    mov al, [es:di]
    mov [ds:si], bl
    mov [es:di], bh
    cmp al, 0xFF
    je .off
    mov al, 1
    jmp .done
.off:
    xor al, al
.done:
    pop bx
    pop di
    pop si
    pop es
    pop ds
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

a20_kbc:
    push bx
    call kbc_wait_in
    mov al, 0xAD
    out 0x64, al
    call kbc_wait_in
    mov al, 0xD0
    out 0x64, al
    call kbc_wait_out
    in al, 0x60
    mov bl, al
    call kbc_wait_in
    mov al, 0xD1
    out 0x64, al
    call kbc_wait_in
    mov al, bl
    or al, 0x02
    out 0x60, al
    call kbc_wait_in
    mov al, 0xAE
    out 0x64, al
    call kbc_wait_in
    pop bx
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

kbc_wait_in:
    push cx
    xor cx, cx
.w:
    in al, 0x64
    test al, 0x02
    jz .d
    out 0x80, al
    loop .w
.d:
    pop cx
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

kbc_wait_out:
    push cx
    xor cx, cx
.w:
    in al, 0x64
    test al, 0x01
    jnz .d
    out 0x80, al
    loop .w
.d:
    pop cx
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

a20_fast:
    in al, 0x92
    or al, 0x02
    and al, 0xFE
    out 0x92, al
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

E820_SMAP equ 0x534d4150
E820_MAX  equ 48

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

detect_memory:
    pushad

    mov dword [mm_base], TPA_MIN_BASE
    mov dword [mm_top], TPA_MIN_BASE

    call e820_collect
    cmp word [e820_cnt], 0
    je .try_e801

    call e820_merge
    cmp eax, TPA_MIN_BASE
    jbe .try_e801
    mov [mm_top], eax
    or dword [mm_flags], MEMF_E820
    jmp .store

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.try_e801:
    xor cx, cx
    xor dx, dx
    xor bx, bx
    mov ax, 0xE801
    int 0x15
    jc .try_88
    test ax, ax
    jnz .have_801
    mov ax, cx
    mov bx, dx
.have_801:
    test ax, ax
    jz .try_88

    movzx edx, ax
    movzx ebx, bx
    or dword [mm_flags], MEMF_E801
    cmp edx, 15 * 1024
    jb .ext_low
    mov edx, 0x1000000
    shl ebx, 16
    add edx, ebx
    jnc .ext_store
    mov edx, 0xFFFFF000
    jmp .ext_store

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.try_88:
    mov ah, 0x88
    xor al, al
    clc
    int 0x15
    jc .store
    test ax, ax
    jz .store
    movzx edx, ax
    or dword [mm_flags], MEMF_88

.ext_low:
    shl edx, 10
    add edx, TPA_MIN_BASE
    jnc .ext_store
    mov edx, 0xFFFFF000

.ext_store:
    mov [mm_top], edx

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.store:
    mov eax, [mm_top]
    cmp eax, TPA_MAX_TOP
    jbe .capped
    mov eax, TPA_MAX_TOP
    mov [mm_top], eax
.capped:
    mov eax, [mm_base]
    mov [MEMMAP_ADDR + 4], eax
    mov eax, [mm_top]
    mov [MEMMAP_ADDR + 8], eax
    mov eax, [mm_flags]
    mov [MEMMAP_ADDR + 12], eax
    mov eax, MEMMAP_MAGIC
    mov [MEMMAP_ADDR + 0], eax

    popad
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

e820_collect:
    xor ebx, ebx
    mov word [e820_cnt], 0
    mov di, e820_buf

.loop:
    mov eax, 0xE820
    mov edx, E820_SMAP
    mov ecx, 24
    mov dword [di + 20], 1
    int 0x15
    jc .done
    cmp eax, E820_SMAP
    jne .done
    cmp ecx, 20
    jb .done

    cmp ecx, 24
    jb .keep
    test byte [di + 20], 1
    jz .next

.keep:
    inc word [e820_cnt]
    add di, 24
    cmp word [e820_cnt], E820_MAX
    jae .done

.next:
    test ebx, ebx
    jnz .loop

.done:
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

e820_merge:
    mov eax, TPA_MIN_BASE

.again:
    xor dx, dx
    mov cx, [e820_cnt]
    test cx, cx
    jz .done
    mov si, e820_buf

.scan:
    cmp dword [si + 16], 1
    jne .next
    cmp dword [si + 4], 0
    jne .next
    mov ebx, [si]
    cmp ebx, eax
    ja .next

    mov ebp, [si + 8]
    add ebp, ebx
    jc .clamp
    cmp dword [si + 12], 0
    jz .have_end
.clamp:
    mov ebp, 0xFFFFF000
.have_end:
    cmp ebp, eax
    jbe .next
    mov eax, ebp
    mov dx, 1

.next:
    add si, 24
    dec cx
    jnz .scan
    test dx, dx
    jnz .again

.done:
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

boot_dl db 0
bootmsg db "OK.",13,10,"CP/M-386 stage 2 loader 0.1 (", BUILDDATE, ")",13,10,"Loading CP/M-386 ... ",13,10,13,10,0
errmsg  db "CP/M-386 stage 2 boot failed, system halted!",0
a20msg  db 13,10,"CP/M-386 cannot enable the A20 gate, system halted!",13,10,0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

align 4
mm_base  dd 0
mm_top   dd 0
mm_flags dd 0

e820_cnt dw 0
e820_buf times (E820_MAX * 24) db 0

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

    mov esp, kernel_end + KSTACK_RESERVE

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

%if CODE_SIZE > (STAGE2_SECTORS * 512)
  %error "stage 2 exceeds STAGE2_SECTORS; raise it in layout.inc"
%endif

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
