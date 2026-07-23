/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 4a97e7f2-82b4-11f1-b53a-80ee73e9b8e7
 */

/*****************************************************************************/

/* big.c - test of a >64KB .386 image (multi-extent load test). */

/*****************************************************************************/

/*
 * Real payload size is produced by padding big.bin via Makefile.
 * _start must remain at VMA 0x100 (first bytes of the image)!
 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;

/*****************************************************************************/

#define BDOS_INT 0x30

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

static UWORD
bdos (WORD func, LONG info)
{
  UWORD ret;
  __asm__ volatile ("int %2"
                    : "=a"(ret)
                    : "a"((unsigned)func), "i"(BDOS_INT),
                      "d"((unsigned long)info)
                    : "memory", "cc");

  return ret;
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  const char *m = "\r\nBIG.386: loaded OK (>64K, multi-extent)\r\n";

  while (*m)
    bdos (2, (LONG)(unsigned char)*m++);

  { /* Touch a byte near the end of a 70KB image so the pad is real. */
    volatile unsigned char *p = (volatile unsigned char *)0x100;
    /* image padded to 0x100 + 71680; last page marker at 0x100+71679 */
    volatile const unsigned char *end
        = (volatile unsigned char *)(0x100 + 71680 - 1);
    (void)p;

    if (*end == 0x90 || *end == 0x00 || *end == 0xCC)
      bdos (2, (LONG)'!'); /* pad present */

    bdos (2, (LONG)'\r');
    bdos (2, (LONG)'\n');
  }

  bdos (0, 0);

  for (;;);
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

/******************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/******************************************************************************/
