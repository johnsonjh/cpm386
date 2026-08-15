/*
 * CP/M-386 - memmap.h
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 784cf906-8c84-11f1-b9d0-80ee73e9b8e7
 */

/*****************************************************************************/

/* memmap.h - boot loader -> kernel usable memory descriptor */

/*****************************************************************************/

#ifndef MEMMAP_H
# define MEMMAP_H

/*****************************************************************************/

# define MEMMAP_ADDR 0x600UL
# define MEMMAP_MAGIC 0x334D5043UL /* "CPM3" */

/*****************************************************************************/

# define MEMMAP_OFF_MAGIC 0x00 /* MEMMAP_MAGIC when valid     */
# define MEMMAP_OFF_BASE 0x04  /* first usable byte  (>= 1MB) */
# define MEMMAP_OFF_TOP 0x08   /* one past last usable byte   */
# define MEMMAP_OFF_FLAGS 0x0C /* MEMF_* below                */

/*****************************************************************************/

/* Which detection method produced base/top, plus loader-verified state. */

# define MEMF_E820 0x0001    /* int 15h e820h memory map        */
# define MEMF_E801 0x0002    /* int 15h e801h sizes             */
# define MEMF_88 0x0004      /* int 15h ah=88h extended size    */
# define MEMF_MBMMAP 0x0008  /* multiboot full memory map       */
# define MEMF_MBBASIC 0x0010 /* multiboot mem_lower/mem_upper   */
# define MEMF_A20 0x0020     /* loader verified A20 is enabled  */

/*****************************************************************************/

/*
 * The TPA starts at 1MB.  Everything below is either the kernel image
 * (loaded at 64K), VGA aperture, or option/system ROM and/or chipset
 * shadow RAM at 0xC0000-0xFFFFF.
 */

# define TPA_MIN_BASE 0x100000UL

/* Never hand ring 3 anything that could be PCI MMIO. */
# define TPA_MAX_TOP 0xE0000000UL

/* Below this much verified RAM above 1MB there is no point in booting. */
# define TPA_MIN_BYTES 0x40000UL /* 256K */

/*****************************************************************************/

# ifndef __ASSEMBLER__

/*****************************************************************************/

int mem_a20_enabled (void);
unsigned long mem_verify_region (unsigned long base, unsigned long top);
unsigned long mem_get_boot_region (unsigned long *base, unsigned long *top);

/*****************************************************************************/

# endif

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
