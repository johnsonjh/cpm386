/*
 * CP/M-386 - tsec.c
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: f375076e-82b5-11f1-bbab-80ee73e9b8e7
 */

/*****************************************************************************/

/* tsec.c - exercise BDOS 155 (T_SECONDS) vs BDOS 105 (T_GET) */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;
typedef unsigned long ULONG;

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_105 105
#define BDOS_155 155

/*****************************************************************************/

struct cpm_datetime
{
  UWORD days;
  UBYTE hour;
  UBYTE min;
  UBYTE sec;
};

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

static void
putu (unsigned n)
{
  char b [8];
  int i = 0;

  if (!n)
    {
      putch ('0');

      return;
    }

  while (n && i < 8)
    {
      b [i++] = (char)('0' + n % 10);
      n /= 10;
    }

  while (i)
    {
      putch (b [--i]);
    }
}

/*****************************************************************************/

static void
put2 (unsigned n)
{
  putch ((char)('0' + (n / 10) % 10));
  putch ((char)('0' + n % 10));
}

/*****************************************************************************/

/* Decode packed BCD nibble-pair to binary 0-99 */
static unsigned
from_bcd (UBYTE v)
{
  return (unsigned)((v >> 4) * 10 + (v & 0x0f));
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  struct cpm_datetime bin, bcd;
  UWORD r;

  puts ("BDOS 105 (binary hms):\r\n");
  r = bdos (BDOS_105, (LONG)(ULONG)&bin);

  if (r)
    {
      puts ("  failed\r\n");
    }
  else
    {
      puts ("  days=");
      putu (bin.days);
      puts ("  ");
      put2 (bin.hour);
      putch (':');
      put2 (bin.min);
      putch (':');
      put2 (bin.sec);
      puts ("\r\n");
    }

  puts ("BDOS 155 (BCD hms):\r\n");
  r = bdos (BDOS_155, (LONG)(ULONG)&bcd);

  if (r)
    {
      puts ("  failed\r\n");
    }
  else
    {
      puts ("  days=");
      putu (bcd.days);
      puts ("  BCD ");
      /* show raw BCD bytes then decoded */
      put2 (from_bcd (bcd.hour));
      putch (':');
      put2 (from_bcd (bcd.min));
      putch (':');
      put2 (from_bcd (bcd.sec));
      puts ("  (raw ");
      putu (bcd.hour);
      putch (',');
      putu (bcd.min);
      putch (',');
      putu (bcd.sec);
      puts (")\r\n");
    }

  if (!r && bin.days == bcd.days && bin.hour == from_bcd (bcd.hour)
      && bin.min == from_bcd (bcd.min) && bin.sec == from_bcd (bcd.sec))
    {
      puts ("OK: 105 and 155 agree\r\n");
    }
  else if (!r)
    {
      puts ("WARN: 105/155 mismatch\r\n");
    }

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
