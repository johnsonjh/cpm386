/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 580958e2-830b-11f1-b4a2-80ee73e9b8e7
 */

/*****************************************************************************/

/* BDOS 48 Test Program */

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
  UWORD res;

  puts ("Calling BDOS 48 (DRV_FLUSH) with E=0: ");
  res = bdos (48, 0);

  if (res == 0xFFFF)
    {
      puts ("Error!\r\n");
    }
  else
    {
      puts ("Success.\r\n");
    }

  puts ("Calling BDOS 48 (DRV_FLUSH) with E=255: ");
  res = bdos (48, 255);

  if (res == 0xFFFF)
    {
      puts ("Error!\r\n");
    }
  else
    {
      puts ("Success.\r\n");
    }

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

/*****************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/*****************************************************************************/
