/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: d09290be-82b4-11f1-95db-80ee73e9b8e7
 */

/*****************************************************************************/

/* lrbc.c - print CP/M file size using Last Record Byte Count */

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

/*****************************************************************************/

/* TPA-relative classic base page (user DS base = TPA) */
#define DEF_FCB ((UBYTE *)abs_ptr (0x5C))
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

static void
putu (ULONG n)
{
  char buf [12];
  int i = 0;

  if (n == 0)
    {
      putch ('0');

      return;
    }

  while (n && i < 12)
    {
      buf [i++] = (char)('0' + (n % 10));
      n /= 10;
    }

  while (i > 0)
    {
      putch (buf [--i]);
    }
}

/*****************************************************************************/

/* Exact byte length from record count + DOS-PLUS LRBC */
static ULONG
exact_size (ULONG records, UBYTE lrbc)
{
  if (records == 0)
    {
      return 0;
    }

  if (lrbc == 0)
    {
      return records * 128UL;
    }

  return (records - 1) * 128UL + (ULONG)lrbc;
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  UBYTE fcb [36];
  UWORD r;
  UBYTE lrbc;
  ULONG records, bytes_alloc, bytes_exact;
  int i, has_value = 0, new_lrbc = 0;

  puts ("\r\nLRBC (Last Record Byte Count)\r\n");

  /* Need a filename in the default FCB (from CCP base-page setup) */
  if (DEF_FCB [1] == ' ' || DEF_FCB [1] == 0)
    {
      puts ("Usage: LRBC filename [value]\r\n");

      (void)bdos (0, 0);
    }

  {
    unsigned char const *p = CMD_TAIL + 1;
    int len = CMD_TAIL [0];

    if (len > 126)
      {
        len = 126;
      }

    while (len > 0 && *p == ' ')
      {
        p++;
        len--;
      }

    while (len > 0 && *p != ' ')
      {
        p++;
        len--;
      }

    while (len > 0 && *p == ' ')
      {
        p++;
        len--;
      }

    if (len > 0)
      {
        int parsed = 0;

        while (len > 0 && *p >= '0' && *p <= '9')
          {
            new_lrbc = new_lrbc * 10 + (*p - '0');
            p++;
            len--;
            parsed = 1;
          }

        while (len > 0 && *p == ' ')
          {
            p++;
            len--;
          }

        if (len > 0 || !parsed || new_lrbc > 128)
          {
            puts ("Invalid LRBC value (must be 0-128)\r\n");

            (void)bdos (0, 0);
          }

        has_value = 1;
      }
  }

  /* Copy default FCB; open with cur_rec=0xFF to fetch LRBC into FCB+32 */
  for (i = 0; i < 36; i++)
    {
      fcb [i] = DEF_FCB [i];
    }

  fcb [12] = 0;    /* extent */
  fcb [14] = 0;    /* s2 */
  fcb [32] = 0xFF; /* request LRBC on open (CP/M Plus / DOS Plus) */

  r = bdos (15, (LONG)(ULONG)fcb);

  if (r > 3)
    {
      puts ("File not found\r\n");

      (void)bdos (0, 0);
    }

  /* After open with 0xFF: cur_rec and s1 hold DOS-PLUS LRBC */
  lrbc = fcb [32];
  fcb [32] = 0; /* reset before any sequential I/O (not used here) */

  /*
   * Function 35: compute file size -> ran0..ran2 = next-record count
   * (CP/M-68K packing: ran0 = bits16-23, ran1 = 8-15, ran2 = 0-7).
   */

  (void)bdos (35, (LONG)(ULONG)fcb);
  records = ((ULONG)fcb [33] << 16)
          | ((ULONG)fcb [34] << 8)
          |  (ULONG)fcb [35];

  if (has_value)
    {
      for (i = 0; i < 36; i++)
        {
          fcb [i] = DEF_FCB [i];
        }

      fcb [6] |= 0x80;
      fcb [32] = (UBYTE)new_lrbc;
      r = bdos (30, (LONG)(ULONG)fcb);

      if (r == 255)
        {
          puts ("Error setting LRBC\r\n");

          (void)bdos (0, 0);
        }
    }

  bytes_alloc = records * 128UL;
  bytes_exact = exact_size (records, has_value ? (UBYTE)new_lrbc : lrbc);

  puts ("File: ");

  for (i = 1; i <= 8 && fcb [i] != ' '; i++)
    {
      putch ((char)(fcb [i] & 0x7f));
    }

  if (fcb [9] != ' ')
    {
      putch ('.');

      for (i = 9; i <= 11 && fcb [i] != ' '; i++)
        {
          putch ((char)(fcb [i] & 0x7f));
        }
    }

  puts ("\r\n");

  puts ("Records (storage): ");
  putu (records);
  puts ("\r\n");

  puts ("Bytes (records*128): ");
  putu (bytes_alloc);
  puts ("\r\n");

  puts ("LRBC (DOS-PLUS, bytes used in last rec): ");
  putu ((ULONG)lrbc);

  if (has_value)
    {
      puts (" -> ");
      putu ((ULONG)new_lrbc);
    }

  puts ("\r\n");

  puts ("Exact size (bytes): ");
  putu (bytes_exact);
  puts ("\r\n");

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

/******************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/******************************************************************************/
