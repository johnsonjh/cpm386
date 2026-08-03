/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 5bcb7fde-8caf-11f1-87c5-80ee73e9b8e7
 */

/*****************************************************************************/

/* io.h - port I/O primitives shared by the ring-0 device code */

/*****************************************************************************/

#ifndef IO_H
# define IO_H

/*****************************************************************************/

/*
 * Ring 3 cannot reach these: IOPL is 0 and the TSS carries no I/O permission
 * bitmap (pmode.c sets tss.iomap_base past the TSS limit), so every in/out
 * from a user program faults.  All device access is funnelled through the
 * BDOS by design.
 */

static inline void
outb (unsigned short port, unsigned char val)
{
  __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

/*****************************************************************************/

static inline unsigned char
inb (unsigned short port)
{
  unsigned char ret;

  __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));

  return ret;
}

/*****************************************************************************/

static inline void
outw (unsigned short port, unsigned short val)
{
  __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

/*****************************************************************************/

static inline unsigned short
inw (unsigned short port)
{
  unsigned short ret;

  __asm__ volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));

  return ret;
}

/*****************************************************************************/

/*
 * Short delay for chipset registers that need settling between writes.
 * Port 0x80 is the POST diagnostic latch and is harmless to write.
 */

static inline void
io_delay (void)
{
  __asm__ volatile ("outb %%al, $0x80" : : "a"(0));
}

/*****************************************************************************/

#endif /* ifndef IO_H */

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
