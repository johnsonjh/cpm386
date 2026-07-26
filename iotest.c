/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: cbc83692-82b4-11f1-8d39-80ee73e9b8e7
 */

/*****************************************************************************/

/* iotest.c - exercise CP/M-386 BDOS file APIs from ring 3 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#define BDOS_INT 0x30

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
  char buf[12];
  int i = 0;

  if (!n)
    {
      putch ('0');
      return;
    }

  while (n && i < 12)
    {
      buf[i++] = (char)('0' + (n % 10));
      n /= 10;
    }

  while (i)
    {
      putch (buf[--i]);
    }
}

/*****************************************************************************/

static unsigned fails;

/*****************************************************************************/

static void
result (int cond, char tag)
{
  /* tag is a single letter to avoid .rodata absolute-string issues */
  putch (cond ? 'O' : 'F');
  putch (cond ? 'K' : 'A');
  putch (' ');
  putch (tag);
  putch ('\r');
  putch ('\n');

  if (!cond)
    {
      fails++;
    }
}

/*****************************************************************************/

static void
fill_name (UBYTE *fcb, const char *name8, const char *typ3)
{
  int i;

  for (i = 0; i < 36; i++)
    {
      fcb[i] = 0;
    }

  fcb[0] = 0;

  for (i = 0; i < 8; i++)
    {
      fcb[1 + i] = (UBYTE)(name8[i] ? name8[i] : ' ');
    }

  for (i = 0; i < 3; i++)
    {
      fcb[9 + i] = (UBYTE)(typ3[i] ? typ3[i] : ' ');
    }
}

/*****************************************************************************/

/* CP/M-68K random record: ran0=bits16-23, ran1=8-15, ran2=0-7 */
static void
set_ran (UBYTE *fcb, unsigned long rec)
{
  fcb[33] = (UBYTE)((rec >> 16) & 0xff);
  fcb[34] = (UBYTE)((rec >> 8) & 0xff);
  fcb[35] = (UBYTE)(rec & 0xff);
}

void
_start (void)
{
  UBYTE fcb[36];
  UBYTE dma[128];
  UBYTE dma2[128];
  UWORD r;
  int i;
  unsigned long rec;
  const char banner[] = "\r\nIOTEST (BDOS I/O Test Suite)\r\n";
  const char done[] = "*** done fails=";

  fails = 0;
  puts (banner);

  /* C = CREATE */
  fill_name (fcb, "IOWORK  ", "DAT");
  bdos (19, (LONG)(unsigned long)fcb);
  fill_name (fcb, "IOWORK  ", "DAT");
  r = bdos (22, (LONG)(unsigned long)fcb);
  result (r <= 3, 'C');

  /* O = OPEN */
  fill_name (fcb, "IOWORK  ", "DAT");
  r = bdos (15, (LONG)(unsigned long)fcb);
  result (r <= 3, 'O');

  /* W = sequential WRITE x5 */
  bdos (26, (LONG)(unsigned long)dma);

  for (rec = 0; rec < 5; rec++)
    {
      for (i = 0; i < 128; i++)
        {
          dma[i] = (UBYTE)(0xA0 + rec);
        }

      dma[0] = (UBYTE)('0' + rec);
      r = bdos (21, (LONG)(unsigned long)fcb);

      if (r != 0)
        {
          break;
        }
    }

  result (rec == 5, 'W');

  /* L = cLose */
  r = bdos (16, (LONG)(unsigned long)fcb);
  result (r <= 3, 'L');

  /* R = sequential READ x5 + EOF */
  fill_name (fcb, "IOWORK  ", "DAT");
  r = bdos (15, (LONG)(unsigned long)fcb);
  bdos (26, (LONG)(unsigned long)dma2);

  for (rec = 0; rec < 5; rec++)
    {
      r = bdos (20, (LONG)(unsigned long)fcb);

      if (r != 0 || dma2[0] != (UBYTE)('0' + rec))
        {
          break;
        }
    }

  if (rec == 5)
    {
      r = bdos (20, (LONG)(unsigned long)fcb);
      result (r == 1, 'R');
    }
  else
    {
      result (0, 'R');
    }

  bdos (16, (LONG)(unsigned long)fcb);

  /* S = SEARCH first for IOWORK.DAT */
  fill_name (fcb, "IOWORK  ", "DAT");
  bdos (26, (LONG)(unsigned long)dma);
  r = bdos (17, (LONG)(unsigned long)fcb);

  if (r <= 3)
    {
      const UBYTE *de = dma + (r * 32);
      result (de[1] == 'I' && de[2] == 'O' && de[9] == 'D', 'S');
    }
  else
    {
      result (0, 'S');
    }

  /* N = search Next (wild) at least one match */
  fill_name (fcb, "????????", "???");
  r = bdos (17, (LONG)(unsigned long)fcb);
  {
    int n = 0;

    while (r != 255 && n < 40)
      {
        n++;
        r = bdos (18, 0);
      }
    result (n >= 1, 'N');
  }

  /* P = write random (record 2), Q = read random back */
  fill_name (fcb, "IOWORK  ", "DAT");
  r = bdos (15, (LONG)(unsigned long)fcb);
  bdos (26, (LONG)(unsigned long)dma);

  for (i = 0; i < 128; i++)
    {
      dma[i] = 0x5A;
    }

  dma[0] = 'X';
  set_ran (fcb, 2);
  r = bdos (34, (LONG)(unsigned long)fcb);
  result (r == 0, 'P');

  for (i = 0; i < 128; i++)
    {
      dma[i] = 0;
    }

  set_ran (fcb, 2);
  r = bdos (33, (LONG)(unsigned long)fcb);
  result (r == 0 && dma[0] == 'X' && dma[1] == 0x5A, 'Q');

  /*
   * H = hole: with 2K blocks, 16 records/block.
   * Seq wrote 0-4 -> map[0].
   * Write random record 40 -> map[2].
   * map[1] (recs 16-31) stays unallocated.
   * Random read record 20 must return EOF (1).
   */

  for (i = 0; i < 128; i++)
    {
      dma[i] = 0xEE;
    }

  dma[0] = 'H';
  set_ran (fcb, 40);
  r = bdos (34, (LONG)(unsigned long)fcb);
  result (r == 0, 'Y'); /* write past gap */
  set_ran (fcb, 20);
  r = bdos (33, (LONG)(unsigned long)fcb);
  result (r == 1, 'H'); /* hole EOF */

  /* Z: random R/W still works after probing a hole (fresh pattern) */
  for (i = 0; i < 128; i++)
    {
      dma[i] = 0x11;
    }

  dma[0] = 'Z';
  set_ran (fcb, 40);
  r = bdos (34, (LONG)(unsigned long)fcb);

  for (i = 0; i < 128; i++)
    {
      dma[i] = 0;
    }

  set_ran (fcb, 40);
  r = bdos (33, (LONG)(unsigned long)fcb);
  result (r == 0 && dma[0] == 'Z', 'Z');

  bdos (16, (LONG)(unsigned long)fcb);

  puts (done);
  putu (fails);
  putch ('\r');
  putch ('\n');

  bdos (0, 0);

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
