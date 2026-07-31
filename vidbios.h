/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 907659f2-8caf-11f1-ae04-80ee73e9b8e7
 */

/*****************************************************************************/

/* vidbios.h - real mode int 10h thunk */

/*****************************************************************************/

#ifndef VIDBIOS_H
# define VIDBIOS_H

/*****************************************************************************/

/*
 * Calling the video BIOS means leaving protected mode, because VBE and the
 * mode-set services are real mode only.  That is practical here for reasons
 * that are worth writing down, since they are properties of this system and
 * not of x86 generally:
 *
 *   - The kernel image lives at 0x10000-0x85000, entirely below 1MB, so
 *     nothing has to be relocated to be reachable from real mode.
 *   - There is no paging (CR3 and CR0.PG are never touched), so there is no
 *     address translation to tear down and rebuild.
 *   - The real mode interrupt vector table at 0-0x3FF, the BIOS data area at
 *     0x400-0x4FF and the video BIOS ROM at 0xC0000 are never written by the
 *     kernel, so the BIOS is still intact and callable.
 *   - The PIC is fully masked and IF is 0 everywhere, so no interrupt can
 *     arrive in the middle of the transition.
 *
 * The 16-bit half of the trampoline has to run below 0x10000, because a
 * 16-bit code segment only has a 16-bit IP.  It is therefore copied out of
 * the kernel image into low memory at init.
 */

/*****************************************************************************/

/*
 * Low memory reserved for the thunk.  Free after boot: stage 1 and stage 2
 * are dead, and memmap.h only claims 0x600-0x60F.
 *
 * These offsets are duplicated in vidbios.s - keep the two in sync.
 */

# define VIDLOW_CODE 0x1000UL  /* 16-bit trampoline, 512 bytes max      */
# define VIDLOW_DATA 0x1200UL  /* register block + saved machine state  */
# define VIDLOW_STACK 0x2000UL /* real mode SS:SP (grows down to 0x1300)*/
# define VIDLOW_XFER 0x2000UL  /* BIOS transfer buffer                  */
# define VIDLOW_XFER_SIZE 0x2000UL

# define VIDLOW_CODE_MAX 0x200UL

/*****************************************************************************/

/*
 * Register block passed to and from int 10h.  Only the registers the video
 * BIOS actually uses are carried.
 */

struct vid_regs
{
  unsigned short ax;
  unsigned short bx;
  unsigned short cx;
  unsigned short dx;
  unsigned short si;
  unsigned short di;
  unsigned short bp;
  unsigned short es;
  unsigned short ds;
  unsigned short flags; /* out only */
};

/*****************************************************************************/

/*
 * Copy the trampoline into low memory.  Must run before any vid_int10()
 * call; safe to call more than once.
 */

void vidbios_init (void);

/*
 * Execute int 10h with the given registers.  Returns 0 on success, or -1 if
 * the thunk is unavailable or already running (a fault taken inside the
 * thunk must not be able to re-enter it).
 */

int vid_int10 (struct vid_regs *r);

/* Non-zero while the transition is in progress. */
int vid_thunk_busy (void);

/*
 * Current BIOS video mode number from int 10h AH=0Fh, or 0xFFFF if the call
 * could not be made.  This is the read-only smoke test for the thunk.
 */

unsigned vid_bios_mode (void);

/*****************************************************************************/

#endif /* ifndef VIDBIOS_H */

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
