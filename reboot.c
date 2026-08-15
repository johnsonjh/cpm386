/*
 * CP/M-386 - reboot.c
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 9b0f5eda-82b5-11f1-9931-80ee73e9b8e7
 */

/*****************************************************************************/

/* reboot.c */

/*****************************************************************************/

/*
 * Usage:
 *   REBOOT       cold reboot (BIOS reset flag 0)
 *   REBOOT W     warm reboot (BIOS data area 40:72 = 0x1234)
 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_REBOOT 220

/*****************************************************************************/

#define DEF_FCB ((UBYTE *)abs_ptr (0x5C))

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

static UWORD
bdos (WORD func, LONG info)
{
  UWORD ret = 0;

  (void)func;
  (void)info;

  __asm__ volatile ("int %2"
                    : "=a"(ret)
                    : "a"((unsigned)func),
                      "i"(BDOS_INT),
                      "d"((ULONG)info)
                    : "memory", "cc");

  return ret;
}

/*****************************************************************************/

static void
putch (char c)
{
  (void)bdos (2, (LONG)(unsigned char)c);
}

/*****************************************************************************/

static void
puts (const char *s)
{
  while (*s)
    {
      putch (*s++);
    }
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  UBYTE ch = DEF_FCB [1];
  int warm = 0;

  if (ch == 'W' || ch == 'w')
    {
      warm = 1;
    }

  puts (warm ? "Warm reboot...\r\n" : "Cold reboot...\r\n");

  (void)bdos (BDOS_REBOOT, (LONG)warm);

  puts ("Reboot not supported\r\n");

  (void)bdos (0, 0);
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
