/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 7d6eb67e-82b4-11f1-b464-80ee73e9b8e7
 */

/*****************************************************************************/

/* call cpm_bringup() after low-level load, before using BDOS/CCP or disk. */

/*****************************************************************************/

/*
 * Contains init_ramdisk, dph0/dpb0/xlt/csv/alv setup, bdosinit() + bdos(14,0).
 * Used by both cpm386_init() and test_bdos main!
 */

/*****************************************************************************/

#include "bdosinc.h"
#include "biosdef.h"
#include "bringup.h" /* brings the dpb/dph structs and externs */

/*****************************************************************************/

/*
 * RAM disk image: MUST live after kernel .data/.bss (see linker.ld .ramdisk).
 * If it sits in .rodata amid the kernel image, bios_write to high block
 * numbers overwrites cmd_tbl / GDT / BDOS globals (same linear addresses)
 */

asm (" .section .ramdisk, \"aw\", @progbits\n"
     " .global ramdisk\n"
     " .align 16\n"
     "ramdisk:\n"
     " .incbin \"ramdisk.bin\"\n"
     " .previous\n");

/*****************************************************************************/

extern unsigned char ramdisk [RAMDISK_SIZE]; /* provided by incbin asm above */

/*****************************************************************************/

static UBYTE xlt [26];
static UBYTE dirbuf [128];
static UBYTE csv [64];

/*****************************************************************************/

/*
 * Word-mode alloc (dsm>255) to match cpmtools 4mb-hd directory maps.
 * alv bit vector: (dsm+1+7)/8 ~= 256 bytes for dsm=2047
 */

static UBYTE alv [256];

/*****************************************************************************/

/*
 * dpb0: match cpmtools "4mb-hd" (2k blocks, word block numbers, 256 dirents).
 * Physical image is only RAMDISK_SIZE; used blocks stay in that range
 */

struct dpb dpb0 = {
  32,       /* spt                                            */
  4, 15, 1, /* bsh blm exm (2k blocks)                        */
  0, 2047,  /* dsm: word-mode maps (cpmtools 4mb-hd)          */
  255,      /* drm: 256 directory entries                     */
  0x000F,   /* dir_al: first 4 blocks for directory (typical) */
  64,       /* cks: checksum dir sectors                      */
  0         /* trk_off                                        */
};

/*****************************************************************************/

struct dph dph0 =
{
  xlt,      /* UBYTE* xlt          */
  0, 0,  0, /* hiwater, dum1, dum2 */
  dirbuf,   /* dbufp               */
  &dpb0,    /* dpbp                */
  csv, alv
};

/*****************************************************************************/

void
cpm_bringup (void)
{
  /* ramdisk data pre-populated by incbin; no fill needed */
  extern void bdosinit (void);
  bdosinit ();

  /* select A: */
  extern UWORD bdos (WORD func, LONG info);
  bdos (14, 0);
}

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
