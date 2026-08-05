/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 3d6b85bc-9104-11f1-9aa7-80ee73e9b8e7
 */

/*****************************************************************************/

/* disk.h - CBIOS disk layer backed forthe V86 server disk server */

/*****************************************************************************/

#ifndef DISK_H
# define DISK_H

/*****************************************************************************/

/*
 * Low memory owned by the V86 disk server.  Everything here has to live
 * below 1 MiB and be reachable as seg:off, and the transfer buffer must not
 * straddle a 64 KiB boundary or the floppy controller's DMA cannot reach it.
 *
 * Already spoken for down here: 0x0000 IVT, 0x0400 BIOS data area, 0x0600
 * the memory descriptor (memmap.h), 0x1000-0x2000 the real mode video thunk
 * (vidbios.s).  The kernel image itself starts at 0x10000 (linker.ld).
 *
 * The addresses are a contract with disk_v86.s - keep the two in sync.
 */

# define V86_CODE_ADDR 0x00003000UL   /* server blob, CS=0x0300 IP=0        */
# define V86_FARPTR_ADDR 0x00003400UL /* seg:off of the ROM handler to call */
# define V86_DAP_ADDR 0x00003410UL    /* EDD disk address packet            */
# define V86_STACK_TOP 0x00004000UL   /* real mode stack, grows down        */
# define V86_BUF_ADDR 0x00004000UL    /* transfer buffer                    */
# define V86_BUF_SIZE 0x00002000UL    /* 8 KiB = 16 physical sectors        */

/* Software interrupt the server uses to yield; also in pmode.h. */
# define V86_YIELD_INT 0x31

/*****************************************************************************/

/*
 * Bring up the V86 server and probe the BIOS drives.  Safe to call more than
 * once; the CBIOS entry points call it lazily on first use.  Returns 0 on
 * success.
 */

int disk_init (void);

/*****************************************************************************/

/* Print what the BIOS reports for each drive.
 * For bring-up and STAT
 */

void disk_report (void);

/*****************************************************************************/

/*
 * Stand in for the BIOS timer tick, so the ROM can time the floppy motor
 * out and stop it.  Does nothing unless a floppy has recently been used.
 * Call from idle paths - console polling, BDOS entry - as often as is
 * convenient; it rate-limits itself.
 */

void disk_poll (void);

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
