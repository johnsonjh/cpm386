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

/* ES:offset store of one text cell (char|attr<<8) */
static inline void
vga_es_store16 (unsigned short sel, unsigned off, unsigned short cell)
{
  __asm__ volatile ("movw %0, %%es\n\t"
                    "movw %2, %%es:(%1)\n\t"
                    :
                    : "r"(sel), "r"(off), "r"(cell)
                    : "memory");
}

/*****************************************************************************/

/* ES:offset load of one text cell. */
static inline unsigned short
vga_es_load16 (unsigned short sel, unsigned off)
{
  unsigned short cell;

  __asm__ volatile ("movw %1, %%es\n\t"
                    "movw %%es:(%2), %0\n\t"
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

/******************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/******************************************************************************/
