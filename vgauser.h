/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 037cade2-82b6-11f1-a5a7-80ee73e9b8e7
 */

/*****************************************************************************/

/* vgauser.h - ring-3 interface to VGA text memory */

/*****************************************************************************/

#ifndef VGA_USER_H
# define VGA_USER_H

/*****************************************************************************/

# define BDOS_CON_VIDEO 224

/*****************************************************************************/

/* Filled by BDOS 224 */
struct cpm_vga_text
{
  unsigned short sel;        /* GDT selector|RPL3 or 0 */
  unsigned short cols;       /* 80                     */
  unsigned short rows;       /* 25                     */
  unsigned short cell_bytes; /* 2                      */
  unsigned long map_size;    /* bytes mapped (32K)     */
  unsigned long phys_base;   /* physical base          */
};

/*****************************************************************************/

/*
 * ES:offset store of one text cell (char|attr<<8).
 *
 * ES is saved and restored.  It used to be left holding the video
 * selector, so any string operation the program performed afterwards - a
 * struct copy, a rep movs emitted by the compiler - wrote into video
 * memory instead of its own data.
 *
 * The offset is 32-bit so the whole 32K plane is reachable, not just the
 * first 64K worth.
 */

static inline void
/*cppcheck-suppress unusedFunction*/
vga_es_store16 (unsigned short sel, unsigned long off, unsigned short cell)
{
  (void)sel;
  (void)off;
  (void)cell;

  __asm__ volatile ("pushl %%es\n\t"
                    "movw %w0, %%es\n\t"
                    "movw %w2, %%es:(%1)\n\t"
                    "popl %%es"
                    :
                    : "r"(sel), "r"(off), "r"(cell)
                    : "memory");
}

/*****************************************************************************/

/* ES:offset load of one text cell.  ES is saved and restored. */
static inline unsigned short
/*cppcheck-suppress unusedFunction*/
vga_es_load16 (unsigned short sel, unsigned long off)
{
  unsigned short cell = 0;

  (void)sel;
  (void)off;

  __asm__ volatile ("pushl %%es\n\t"
                    "movw %w1, %%es\n\t"
                    "movw %%es:(%2), %w0\n\t"
                    "popl %%es"
                    : "=r"(cell)
                    : "r"(sel), "r"(off)
                    : "memory");

  return cell;
}

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
