/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 */

/*****************************************************************************/

/* od.c - octal dump for CP/M-386 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

#define BDOS_INT 0x30
#define DEF_FCB ((UBYTE *)abs_ptr (0x5C))
#define BYTES_PER_LINE 16

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
octaldump (const UBYTE *buf, unsigned size)
{
  unsigned i, j, num = 0;
  unsigned char line[81];
  unsigned char byte;
  int d1, d2, d3, pos;

  for (i = 0; i < size; i++)
    {
      if (num == 0)
        {
          for (j = 0; j < 80; j++)
            {
              line[j] = ' ';
            }

          line[80] = 0;
        }

      byte = buf[i];
      d1 = byte / 64;
      d2 = (byte % 64) / 8;
      d3 = byte % 8;
      pos = (int)(num * 4);
      line[pos] = (unsigned char)('0' + d1);
      line[pos + 1] = (unsigned char)('0' + d2);
      line[pos + 2] = (unsigned char)('0' + d3);

      if (byte >= 32 && byte < 127)
        {
          line[64 + num] = byte;
        }
      else
        {
          line[64 + num] = '.';
        }

      num++;

      if (num == BYTES_PER_LINE)
        {
          puts ((const char *)line);
          putch ('\r');
          putch ('\n');
          num = 0;
        }
    }

  if (num > 0)
    {
      puts ((const char *)line);
      putch ('\r');
      putch ('\n');
    }
}

/*****************************************************************************/

static void
fill_from_def_fcb (UBYTE *fcb)
{
  int i;

  for (i = 0; i < 36; i++)
    {
      fcb[i] = DEF_FCB[i];
    }
}

/*****************************************************************************/

void
_start (void)
{
  UBYTE fcb[36];
  UBYTE dma[128];
  UBYTE prev[128];
  UWORD r;
  UBYTE lrbc = 0;
  int have = 0;
  int i;
  char usage[] = "Usage: OD filename\r\n";
  char nofile[] = "File not found\r\n";

  if (DEF_FCB[1] == ' ' || DEF_FCB[1] == 0)
    {
      puts (usage);
      bdos (0, 0);
    }

  fill_from_def_fcb (fcb);
  fcb[12] = 0;
  fcb[14] = 0;
  fcb[32] = 0xFF;
  r = bdos (15, (LONG)(unsigned long)fcb);

  if (r > 3)
    {
      puts (nofile);
      bdos (0, 0);
    }

  lrbc = fcb[32];
  fcb[32] = 0;

  bdos (26, (LONG)(unsigned long)dma);

  for (;;)
    {
      r = bdos (20, (LONG)(unsigned long)fcb);

      if (r != 0)
        {
          break;
        }

      if (have)
        {
          octaldump (prev, 128);
        }

      for (i = 0; i < 128; i++)
        {
          prev[i] = dma[i];
        }

      have = 1;
    }

  if (have)
    {
      unsigned n = 128;

      if (lrbc != 0)
        {
          n = lrbc;
        }

      octaldump (prev, n);
    }

  bdos (16, (LONG)(unsigned long)fcb);

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
