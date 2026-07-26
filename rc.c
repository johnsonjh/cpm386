/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 95e4eeac-82b5-11f1-b94d-80ee73e9b8e7
 */

/*****************************************************************************/

/* rc.c - set / show CP/M program return code (BDOS 108 P_CODe) */

/*****************************************************************************/

/*
 * Usage:
 *   RC nnn     set return code to nnn (decimal) and exit
 *   RC         print current return code (usually 0 at CCP)
 *   RC -h      help
 */

/*****************************************************************************/

/*
 * After "RC 42", CCP prints "Return code 42" (nonzero only).
 * Codes 0xFF00-0xFF7F are "fatal" per CP/M 3; 0xFFFE = ^C terminate.
 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

#define BDOS_INT 0x30
#define BDOS_PCODE 108
#define DEF_FCB ((UBYTE *)abs_ptr (0x5C))
#define CMD_TAIL ((UBYTE *)abs_ptr (0x80))

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
putu (unsigned long n)
{
  char b[12];
  int i = 0;

  if (!n)
    {
      putch ('0');
      return;
    }

  while (n && i < 12)
    {
      b[i++] = (char)('0' + n % 10);
      n /= 10;
    }

  while (i)
    {
      putch (b[--i]);
    }
}

/*****************************************************************************/

static void
help (void)
{
  puts ("Usage: RC [nnn]\r\n");
  puts ("  RC nnn  set BDOS 108 return code and exit\r\n");
  puts ("  RC      show current return code\r\n");
}

/*****************************************************************************/

/* Parse unsigned decimal from default FCB name (first token) or tail. */
static int
parse_u (unsigned *out)
{
  unsigned n = 0, i, tlen;
  char tail[128];
  const char *p;

  /* Prefer command tail (allows multi-digit without FCB 8-char limit) */
  tlen = CMD_TAIL[0];

  if (tlen > 126)
    {
      tlen = 126;
    }

  for (i = 0; i < tlen; i++)
    {
      tail[i] = (char)CMD_TAIL[1 + i];
    }

  tail[tlen] = 0;
  p = tail;

  while (*p == ' ' || *p == '\t')
    {
      p++;
    }

  if (*p == '-' || *p == '/')
    {
      if (p[1] == 'h' || p[1] == 'H')
        {
          help ();

          return -1;
        }
    }

  if (*p >= '0' && *p <= '9')
    {
      int any = 0;

      while (*p >= '0' && *p <= '9')
        {
          n = n * 10u + (unsigned)(*p - '0');
          p++;
          any = 1;
        }

      if (any)
        {
          *out = n;

          return 1;
        }
    }

  /* Fallback: first char(s) of FCB name if digits */
  if (DEF_FCB[1] >= '0' && DEF_FCB[1] <= '9')
    {
      for (i = 1; i <= 8 && DEF_FCB[i] >= '0' && DEF_FCB[i] <= '9'; i++)
        {
          n = n * 10u + (unsigned)(DEF_FCB[i] - '0');
        }

      *out = n;

      return 1;
    }

  return 0; /* no number */
}

/*****************************************************************************/

void
_start (void) /* cppcheck-suppress unusedFunction*/
{
  unsigned code;
  int r = parse_u (&code);

  if (r < 0)
    {
      bdos (0, 0);
    }

  if (r == 1)
    {
      /* Set and exit - CCP will print if nonzero */
      bdos (BDOS_PCODE, (LONG)(UWORD)code);

      bdos (0, 0);
    }

  /* Query current */
  {
    UWORD cur = bdos (BDOS_PCODE, (LONG)0xFFFF);
    puts ("Current return code: ");
    putu ((unsigned long)cur);
    puts ("\r\n");
  }

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
