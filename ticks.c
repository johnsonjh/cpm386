/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 */

/*****************************************************************************/

/* ticks.c: BDOS 225 (GET_TICKS) and 226 (SLEEP_UNTIL) tests */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned long ULONG;

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_GET_TICKS 225
#define BDOS_SLEEP_UNTIL 226

/*****************************************************************************/

struct cpm_ticks
{
  ULONG lo;
  ULONG hi;
  ULONG hz;
};

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

static void
putch (char c)
{
  bdos (2, (LONG)(unsigned char)c);
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

static void
putu32 (ULONG n)
{
  char b[12];
  int i = 0;

  if (!n)
    {
      putch ('0');
      return;
    }

  while (n)
    {
      b[i++] = (char)('0' + (n % 10));
      n /= 10;
    }

  while (i--)
    {
      putch (b[i]);
    }
}

/*****************************************************************************/

static void
ticks_add32 (struct cpm_ticks *t, ULONG delta)
{
  ULONG nlo = t->lo + delta;

  if (nlo < t->lo)
    {
      t->hi++;
    }

  t->lo = nlo;
}

/*****************************************************************************/

void
_start (void)
{
  struct cpm_ticks t;
  UWORD r;
  ULONG i;

  puts ("CP/M-386 high-res ticks (BDOS 225/226)\r\n");

  r = bdos (BDOS_GET_TICKS, (LONG)(unsigned long)&t);

  if (r == 0xFFFF)
    {
      puts ("GET_TICKS failed\r\n");
      bdos (0, 0);
    }

  puts ("hz=");
  putu32 (t.hz);
  puts ("  t0 hi=");
  putu32 (t.hi);
  puts (" lo=");
  putu32 (t.lo);
  puts ("\r\n");

  {
    ULONG half = t.hz / 2;

    ticks_add32 (&t, half);
    puts ("sleep ~0.5s ...\r\n");
    r = bdos (BDOS_SLEEP_UNTIL, (LONG)(unsigned long)&t);

    if (r == 0xFFFF)
      {
        puts ("SLEEP_UNTIL failed\r\n");
        bdos (0, 0);
      }
  }

  r = bdos (BDOS_GET_TICKS, (LONG)(unsigned long)&t);
  (void)r;
  puts ("t1 hi=");
  putu32 (t.hi);
  puts (" lo=");
  putu32 (t.lo);
  puts ("\r\n");

  /* Five short 100 ms sleeps */
  for (i = 0; i < 5; i++)
    {
      ULONG d = t.hz / 10; /* 100 ms */

      puts (".");
      ticks_add32 (&t, d);
      bdos (BDOS_SLEEP_UNTIL, (LONG)(unsigned long)&t);

      /* refresh base for next delta */
      bdos (BDOS_GET_TICKS, (LONG)(unsigned long)&t);
    }

  puts ("\r\ndone\r\n");
  bdos (0, 0);
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
