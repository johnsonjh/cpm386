/*
 * CP/M-386 - vgafb.h
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: dcb63dac-8cb7-11f1-8db6-80ee73e9b8e7
 */

/*****************************************************************************/

/* vgafb.h - ring-3 access to the graphics framebuffer selector */

/*****************************************************************************/

#ifndef VGAFB_H
# define VGAFB_H

/*****************************************************************************/

/*
 * A graphics modes framebuffer is reached through its own selector rather
 * than through the flat TPA segment because it lives outside the TPA.  It
 * is 0xA0000 for standard VGA, or wherever the card's linear framebuffer
 * is for a VBE mode.  BDOS 229 hands back the selector in .sel.
 *
 * Every helper below saves and restores ES!
 *
 * Offsets are 32-bit, so a framebuffer larger than 64K works.  The
 * descriptor switches to page granularity above 1MB, which is why a
 * 1600x1200 mode is reachable at all.
 *
 * For anything that redraws a whole frame, render into an ordinary array in
 * the TPA and push it across with one vgafb_blit(): a single rep movsd is
 * far better than a per-pixel far store, and it is how a DOOM-style renderer
 * wants to work anyway.
 */

/*****************************************************************************/

/* Copy n bytes from ordinary memory into the framebuffer at off. */

static inline void
vgafb_blit (unsigned short sel, unsigned long off, const void *src,
            unsigned long n)
{
  unsigned long d = off;
  unsigned long s = (unsigned long)src;
  unsigned long c = n;

  /*
   * EDI, ESI and ECX are read-write, not inputs: rep movsb advances all
   * three.  Declaring them as plain inputs lets the compiler carry on
   * believing they still hold what it put there - and since they are
   * callee saved, it likes to keep loop variables in them, so a caller
   * blitting row by row silently gets a doubled source stride.
   */

  __asm__ volatile ("pushl %%es\n\t"
                    "movw %w3, %%es\n\t"
                    "cld\n\t"
                    "rep movsb\n\t"
                    "popl %%es"
                    : "+D"(d), "+S"(s), "+c"(c)
                    : "r"(sel)
                    : "memory", "cc");
}

/*****************************************************************************/

/* Fill n bytes of the framebuffer at off with a byte value. */

static inline void
vgafb_fill (unsigned short sel, unsigned long off, unsigned char v,
            unsigned long n)
{
  unsigned long d = off;
  unsigned long c = n;

  /* rep stosb advances EDI and decrements ECX; see vgafb_blit above. */
  __asm__ volatile ("pushl %%es\n\t"
                    "movw %w3, %%es\n\t"
                    "cld\n\t"
                    "rep stosb\n\t"
                    "popl %%es"
                    : "+D"(d), "+c"(c)
                    : "a"(v), "r"(sel)
                    : "memory", "cc");
}

/*****************************************************************************/

/* Single 8-bit pixel, for packed 256 colour modes. */

static inline void
vgafb_store8 (unsigned short sel, unsigned long off, unsigned char v)
{
  __asm__ volatile ("pushl %%es\n\t"
                    "movw %w0, %%es\n\t"
                    "movb %b2, %%es:(%1)\n\t"
                    "popl %%es"
                    :
                    : "r"(sel), "r"(off), "q"(v)
                    : "memory");
}

/*****************************************************************************/

static inline unsigned char
vgafb_load8 (unsigned short sel, unsigned long off)
{
  unsigned char v;

  __asm__ volatile ("pushl %%es\n\t"
                    "movw %w1, %%es\n\t"
                    "movb %%es:(%2), %b0\n\t"
                    "popl %%es"
                    : "=q"(v)
                    : "r"(sel), "r"(off)
                    : "memory");

  return v;
}

/*****************************************************************************/

/* 16-bit pixel, for 15 and 16 bits per pixel modes. */

static inline void
vgafb_store16 (unsigned short sel, unsigned long off, unsigned short v)
{
  __asm__ volatile ("pushl %%es\n\t"
                    "movw %w0, %%es\n\t"
                    "movw %w2, %%es:(%1)\n\t"
                    "popl %%es"
                    :
                    : "r"(sel), "r"(off), "r"(v)
                    : "memory");
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
