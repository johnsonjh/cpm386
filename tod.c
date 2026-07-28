/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: dfacbf7e-82b5-11f1-b8d9-80ee73e9b8e7
 */

/*****************************************************************************/

/* tod.c - Time Of Day */

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
#define BDOS_GET_TOD 105
#define BDOS_SET_TOD 104

/*****************************************************************************/

/* Must match rtc.h / kernel */
struct cpm_datetime
{
  UWORD days;
  UBYTE hour;
  UBYTE min;
  UBYTE sec;
};

/*****************************************************************************/

#define CMD_TAIL ((UBYTE *)abs_ptr (0x80))

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

static int
isdigit (char c)
{
  return c >= '0' && c <= '9';
}

/*****************************************************************************/

static unsigned
u2 (const char *p)
{
  return (unsigned)(p [0] - '0') * 10u + (unsigned)(p [1] - '0');
}

/*****************************************************************************/

/* days since 1978-01-01 (must match kernel rtc_ymd_to_days) */
static int
is_leap (unsigned y)
{
  return y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
}

/*****************************************************************************/

static const UBYTE mdays []
    = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/*****************************************************************************/

static UWORD
ymd_to_days (unsigned y, unsigned m, unsigned d)
{
  unsigned day = 0, yy, mm;

  for (yy = 1978; yy < y; yy++)
    {
      day += is_leap (yy) ? 366u : 365u;
    }

  for (mm = 1; mm < m; mm++)
    {
      day += mdays [mm - 1];

      if (mm == 2 && is_leap (y))
        {
          day++;
        }
    }

  day += (d - 1);

  return (UWORD)day;
}

/*****************************************************************************/

static void
days_to_ymd (UWORD days, unsigned *y, unsigned *m, unsigned *d)
{
  unsigned int yy = 1978, rem = days;

  for (;;)
    {
      unsigned ylen = is_leap (yy) ? 366u : 365u;

      if (rem < ylen)
        {
          break;
        }

      rem -= ylen;
      yy++;
    }

  *y = yy;
  *m = 1;

  for (;;)
    {
      unsigned int dim = mdays [*m - 1];

      if (*m == 2 && is_leap (yy))
        {
          dim = 29;
        }

      if (rem < dim)
        {
          break;
        }

      rem -= dim;
      (*m)++;
    }

  *d = rem + 1;
}

/*****************************************************************************/

static void
put2 (unsigned v)
{
  putch ((char)('0' + (v / 10) % 10));
  putch ((char)('0' + v % 10));
}

/*****************************************************************************/

static void
show_tod (const struct cpm_datetime *dt)
{
  unsigned y, m, d;

  days_to_ymd (dt->days, &y, &m, &d);
  put2 (m);
  putch ('/');
  put2 (d);
  putch ('/');
  put2 (y % 100);
  putch ('\t');
  put2 (dt->hour);
  putch (':');
  put2 (dt->min);
  putch (':');
  put2 (dt->sec);
}

/*****************************************************************************/

/* Parse "MM/DD/YY HH:MM:SS" from command tail; return 0 ok */
static int
parse_set (const char *p, struct cpm_datetime *dt)
{
  unsigned mo, da, yy, hh, mi, ss, y;
  char buf [20];
  int i, n = 0;

  while (*p == ' ' || *p == '\t')
    {
      p++;
    }
  for (i = 0; i < 17 && p [i] && p [i] != '\r' && p [i] != '\n'; i++)
    {
      buf [i] = p [i];
    }

  buf [i] = 0;
  n = i;

  /* MM/DD/YY HH:MM:SS = 17 chars */
  if (n < 17)
    {
      return 1;
    }

  if (buf [2] != '/'
   || buf [5] != '/'
   || buf [8] != ' '
   || buf [11] != ':'
   || buf [14] != ':')
    {
      return 1;
    }

  for (i = 0; i < 17; i++)
    {
      if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14)
        {
          continue;
        }

      if (!isdigit (buf [i]))
        {
          return 1;
        }
    }

  mo = u2 (buf + 0);
  da = u2 (buf + 3);
  yy = u2 (buf + 6);
  hh = u2 (buf + 9);
  mi = u2 (buf + 12);
  ss = u2 (buf + 15);

  /* Original: YY 00-99 => 2000-2099 */
  y = 2000u + yy;

  if (mo < 1 || mo > 12 || da < 1 || hh > 23 || mi > 59 || ss > 59)
    {
      return 1;
    }

  {
    unsigned dim = mdays [mo - 1];

    if (mo == 2 && is_leap (y))
      {
        dim = 29;
      }

    if (da > dim)
      {
        return 1;
      }
  }

  dt->days = ymd_to_days (y, mo, da);
  dt->hour = (UBYTE)hh;
  dt->min = (UBYTE)mi;
  dt->sec = (UBYTE)ss;

  return 0;
}

/*****************************************************************************/

static void
usage (void)
{
  const char m1 [] = "Invalid Date & Time Format\r\n";
  const char m2 [] = "Please retry using:\r\n";
  const char m3 [] = "\"TOD MM/DD/YY HH:MM:SS\"\r\n";
  const char m4 [] = "where YY is 00-99 for 2000-2099, or:\r\n";
  const char m5 [] = "\"TOD P\" for continuous display.";

  puts (m1);
  puts (m2);
  puts (m3);
  puts (m4);
  puts (m5);
}

/*****************************************************************************/

/*
 * After continuous mode stops on a key, eat that key and at most a
 * following CR/LF pair - do not drain the whole typeahead (next cmds).
 */

static void
eat_stop_key (void)
{
  int n = 0;

  while (bdos (11, 0) != 0 && n++ < 3)
    {
      int c = (int)bdos (6, 0xFF) & 0xff;

      if (c != '\r' && c != '\n' && c != 0)
        {
          break;
        }
    }
}

/*****************************************************************************/

static void
exit_ccp (void)
{
  (void)bdos (0, 0);
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  struct cpm_datetime dt;
  const UBYTE *tail = CMD_TAIL;
  char arg [128];
  unsigned tlen;
  unsigned i;
  /* Original: default shows once and quits; P enables continuous update. */
  int continuous = 0;
  const char nl [] = "\r\n";

  /* Copy command tail (length at 0x80) */
  tlen = tail [0];

  if (tlen > 126)
    {
      tlen = 126;
    }

  for (i = 0; i < tlen; i++)
    {
      arg [i] = (char)tail [1 + i];
    }

  arg [tlen] = 0;

  /* Skip leading spaces; check for P or date */
  i = 0;

  while (arg [i] == ' ' || arg [i] == '\t')
    {
      i++;
    }

  if (arg [i] == 'P' || arg [i] == 'p')
    {
      continuous = 1;
      i++;

      while (arg [i] == ' ' || arg [i] == '\t')
        {
          i++;
        }
    }

  if (arg [i] && arg [i] != '\r')
    {
      if (parse_set (&arg [i], &dt) != 0)
        {
          usage ();

          exit_ccp ();
        }

      if (bdos (BDOS_SET_TOD, (LONG)(ULONG)&dt) != 0)
        {
          usage ();

          exit_ccp ();
        }

      continuous = 0; /* after set: show once and return */
    }

  /* Show once (default), or loop until key if TOD P */
  for (;;)
    {
      if (bdos (BDOS_GET_TOD, (LONG)(ULONG)&dt) != 0)
        {
          const char e [] = "RTC read failed\r\n";
          puts (e);

          exit_ccp ();
        }

      show_tod (&dt);

      if (!continuous)
        {
          puts (nl);

          exit_ccp ();
        }

      /* Continuous (P): poll for key, else refresh on same line */
      {
        int spins;

        for (spins = 0; spins < 200000; spins++)
          {
            if (bdos (11, 0) != 0)
              {
                puts (nl);
                eat_stop_key ();

                exit_ccp ();
              }
          }

        putch ('\r'); /* overwrite line on next show */
      }
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
