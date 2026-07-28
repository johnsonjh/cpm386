/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 528ee4e0-830b-11f1-8b63-80ee73e9b8e7
 */

/*****************************************************************************/

/* BDOS 141 Test Program */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned long ULONG;

/*****************************************************************************/

#define BDOS_INT 0x30

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
puts (const char *s)
{
  while (*s)
    {
      (void)bdos (2, (LONG)*s++);
    }
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  const char msg1 [] =
    "Testing BDOS 141 (P_DELAY) with 60 ticks (1 sec)...\r\n";
  const char msg2 [] =
    "Tick!\r\n";
  const char msg3 [] =
    "Done.\r\n";
  int i;

  puts (msg1);

  for (i = 0; i < 3; i++)
    {
      (void)bdos (141, 60); /* 60Hz/60 ticks == 1 second */
      puts (msg2);
    }

  puts (msg3);

  (void)bdos (0, 0);

  for (;;)
    {
      ;
    }
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
