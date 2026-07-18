/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 */

/*****************************************************************************/

/* hd.c - hex dump */

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

/*****************************************************************************/

#define BYTES_PER_LINE 20

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

static char
hexdig (unsigned v)
{
  v &= 0x0f;
  return (char)(v > 9 ? v + 'a' - 10 : v + '0');
}

/*****************************************************************************/

/* Continuous dump state across sequential records. */
static char line[81];
static unsigned col; /* 0..BYTES_PER_LINE-1 bytes filled in line */

/*****************************************************************************/

static void
line_clear (void)
{
  unsigned j;

  for (j = 0; j < 80; j++)
    {
      line[j] = ' ';
    }

  line[80] = 0;
  col = 0;
}

/*****************************************************************************/

static void
line_flush (void)
{
  puts (line);
  putch ('\r');
  putch ('\n');
  line_clear ();
}

/*****************************************************************************/

/* Append raw file bytes to the ongoing 20-column dump. */
static void
hexdump_feed (const UBYTE *buf, unsigned size)
{
  unsigned i;
  unsigned char ch;

  for (i = 0; i < size; i++)
    {
      if (col == 0)
        {
          line_clear ();
        }

      ch = buf[i];
      line[col * 3] = hexdig (ch >> 4);
      line[col * 3 + 1] = hexdig (ch);

      if (ch < 0x7f && ch > (unsigned char)' ')
        {
          line[col + 60] = (char)ch;
        }
      else
        {
          line[col + 60] = '.';
        }

      col++;

      if (col == BYTES_PER_LINE)
        {
          line_flush ();
        }
    }
}

/*****************************************************************************/

static void
hexdump_end (void)
{
  if (col != 0)
    {
      line_flush ();
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
  char usage[] = "Usage: HD filename\r\n";
  char nofile[] = "File not found\r\n";

  if (DEF_FCB[1] == ' ' || DEF_FCB[1] == 0)
    {
      puts (usage);
      bdos (0, 0);
    }

  fill_from_def_fcb (fcb);
  fcb[12] = 0;
  fcb[14] = 0;
  fcb[32] = 0xFF; /* request LRBC on open */
  r = bdos (15, (LONG)(unsigned long)fcb);

  if (r > 3)
    {
      puts (nofile);
      bdos (0, 0);
    }

  lrbc = fcb[32]; /* DOS-PLUS bytes used in last record (0 = full) */
  fcb[32] = 0;    /* sequential from record 0 */

  col = 0;
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
          hexdump_feed (prev, 128);
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

      hexdump_feed (prev, n);
    }

  hexdump_end ();

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
