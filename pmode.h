/*
 * CP/M-386
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
# define SEL_UVIDEO 0x30 /* DPL=3 VGA text; use with RPL3 => 0x33 */

/*****************************************************************************/

# define SEL_UCODE_RPL3 0x1B
# define SEL_UDATA_RPL3 0x23
# define SEL_UVIDEO_RPL3 0x33

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

void enter_ring3 (unsigned long entry_off, unsigned long user_esp);

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

/******************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/******************************************************************************/
