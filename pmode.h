/*
 * CP/M-386 - pmode.h
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 728f0dc0-82b5-11f1-af78-80ee73e9b8e7
 */

/*****************************************************************************/

/* pmode.h - protected-mode privilege, GDT/IDT/TSS, ring-3 launch */

/*****************************************************************************/

#ifndef PMODE_H
# define PMODE_H

/*****************************************************************************/

/* GDT selectors (RPL 0 unless noted) */
# define SEL_NULL 0x00
# define SEL_KCODE 0x08
# define SEL_KDATA 0x10
# define SEL_UCODE 0x18 /* DPL=3; use with RPL3 => 0x1B */
# define SEL_UDATA 0x20 /* DPL=3; use with RPL3 => 0x23 */
# define SEL_TSS 0x28
# define SEL_UVIDEO 0x30  /* DPL=3 VGA text; use with RPL3 => 0x33 */
# define SEL_UFB 0x38     /* DPL=3 graphics framebuffer (stage 2)  */
# define SEL_RCODE16 0x40 /* 16-bit code, for the real mode thunk  */
# define SEL_RDATA16 0x48 /* 16-bit data, for the real mode thunk  */

/*****************************************************************************/

# define SEL_UCODE_RPL3 0x1B
# define SEL_UDATA_RPL3 0x23
# define SEL_UVIDEO_RPL3 0x33
# define SEL_UFB_RPL3 0x3B

/*****************************************************************************/

/* User-callable BDOS software interrupt */
# define BDOS_INT 0x30

/*****************************************************************************/

/*
 * Install flat kernel + user TPA segments, TSS, IDT (incl. int 0x30).
 * tpa_base/tpa_len define the ring-3 code/data window.
 */

void pmode_init (unsigned long tpa_base, unsigned long tpa_len);

/*****************************************************************************/

/*
 * Enter ring 3 at user-relative entry_off with user-relative stack.
 * Returns only when the program exits via BDOS function 0 (int 0x30).
 */

void enter_ring3 (unsigned long entry_eip, unsigned long user_esp);

/*****************************************************************************/

/*
 * Virtual-8086 task state.  One instance (v86_state, in pmode.s) holds the
 * entire machine state of the V86 disk server between yields, and doubles as
 * the register block for the real-mode call it is asked to make.  The field
 * order is fixed: it is exactly a PUSHAD frame followed by the V86 IRETD
 * frame, so the entry stub can snapshot it with a single REP MOVSD.
 *
 * DO NOT REORDER WITHOUT CHANGING pmode.s.
 */

struct v86_state
{
  unsigned long edi, esi, ebp, esp_unused, ebx, edx, ecx, eax;
  unsigned long eip, cs, eflags, esp, ss, es, ds, fs, gs;
};

extern struct v86_state v86_state;

/* EFLAGS for a V86 task: VM=1, IOPL=3, IF=1, reserved bit 1 set. */
# define V86_EFLAGS 0x00023202UL

/* Software interrupt the V86 task uses to yield back to the kernel. */
# define V86_YIELD_INT 0x31

/*
 * Run the V86 task from v86_state until it executes int V86_YIELD_INT, then
 * return with v86_state updated.  Also returns (with v86_state stale) if the
 * task faults, in which case v86_faulted() is nonzero.
 */

void v86_resume (void);

/* Nonzero if the last v86_resume ended in a fault rather than a yield. */
int v86_faulted (void);
void v86_clear_fault (void);

/*
 * Open the TSS I/O permission bitmap to the V86 task, or close it again.
 * Must be closed whenever a ring-3 program can run: a present bitmap
 * overrides IOPL, and this one allows every port.
 */

void pmode_v86_io (int allow);

/*****************************************************************************/

/* Linear base of current user TPA (for syscall pointer fixup). */
unsigned long pmode_tpa_base (void);
unsigned long pmode_tpa_len (void);

/*****************************************************************************/

/* Nonzero after pmode_init (ring-3 TPA window active). */
int pmode_active (void);

/*****************************************************************************/

/*
 * User VGA text segment (platform.h).  Returns selector|RPL3, or 0 if
 * CPM386_HAS_VGA_TEXT is 0 / not installed.
 */

unsigned short pmode_vga_selector (void);
unsigned long pmode_vga_phys_base (void);
unsigned long pmode_vga_map_size (void);

/* Ring-3 graphics framebuffer (stage 2); 0 selector when none is mapped. */
unsigned short pmode_fb_selector (void);
unsigned long pmode_fb_base (void);
unsigned long pmode_fb_size (void);
void pmode_set_fb (unsigned long base, unsigned long size);

/*****************************************************************************/

#endif

/*****************************************************************************/

/*
 * Local Variables:
 * mode: c
 * indent-tabs-mode: nil
 * tab-width: 2
 * c-basic-offset: 2
 * fill-column: 80
 * eval: (setq-local display-fill-column-indicator-column 80)
 * eval: (display-fill-column-indicator-mode 1)
 * End:
 */

/*****************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/*****************************************************************************/
