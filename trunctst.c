/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: ee1bcffa-82b5-11f1-9319-80ee73e9b8e7
 */

/*****************************************************************************/

/* trunc.c: exercise BDOS 40 (WRITEZF), 99 (TRUNCATE), 37 (DRV_RESET), 98 */

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

static unsigned fails;

/*****************************************************************************/

static void
result (int cond, const char *tag)
{
  puts (cond ? "OK " : "FAIL ");
  puts (tag);
  puts ("\r\n");

  if (!cond)
    {
      fails++;
    }
}

/*****************************************************************************/

static void
fill_name (UBYTE *fcb, const char *n8, const char *t3)
{
  int i;

  for (i = 0; i < 36; i++)
    {
      fcb[i] = 0;
    }

  for (i = 0; i < 8; i++)
    {
      fcb[1 + i] = (UBYTE)(n8[i] ? n8[i] : ' ');
    }

  for (i = 0; i < 3; i++)
    {
      fcb[9 + i] = (UBYTE)(t3[i] ? t3[i] : ' ');
    }
}

/*****************************************************************************/

static void
set_ran (UBYTE *fcb, unsigned long rec)
{
  fcb[33] = (UBYTE)((rec >> 16) & 0xff);
  fcb[34] = (UBYTE)((rec >> 8) & 0xff);
  fcb[35] = (UBYTE)(rec & 0xff);
}

/*****************************************************************************/

static unsigned long
get_ran (UBYTE *fcb)
{
  return ((unsigned long)fcb[33] << 16) | ((unsigned long)fcb[34] << 8)
         | (unsigned long)fcb[35];
}

/*****************************************************************************/

void
_start (void)
{
  UBYTE fcb[36];
  UBYTE dma[128];
  UWORD r;
  int i;
  unsigned long sz;

  fails = 0;
  puts ("\r\nTRUNC (BDOS 40/BDOS 99/BDOS 37/BDOS 98)\r\n");

  /* 98 clean disk - always succeeds here */
  r = bdos (98, 0);
  result (r == 0, "98 clean");

  /* 37 reset A: only */
  r = bdos (37, 1);
  result (r == 0, "37 reset A");

  /* Create TRUNC.DAT */
  fill_name (fcb, "TRUNC   ", "DAT");
  bdos (19, (LONG)(unsigned long)fcb); /* delete if present */
  fill_name (fcb, "TRUNC   ", "DAT");
  r = bdos (22, (LONG)(unsigned long)fcb);
  result (r <= 3, "make");

  fill_name (fcb, "TRUNC   ", "DAT");
  r = bdos (15, (LONG)(unsigned long)fcb);
  result (r <= 3, "open");

  bdos (26, (LONG)(unsigned long)dma);

  /* Sequential write 10 records (0..9) */
  for (i = 0; i < 10; i++)
    {
      int j;

      for (j = 0; j < 128; j++)
        {
          dma[j] = (UBYTE)(0x10 + i);
        }

      dma[0] = (UBYTE)('0' + i);
      r = bdos (21, (LONG)(unsigned long)fcb);

      if (r != 0)
        {
          break;
        }
    }

  result (i == 10, "seq write 10");
  bdos (16, (LONG)(unsigned long)fcb);

  /* Size should be 10 */
  fill_name (fcb, "TRUNC   ", "DAT");
  bdos (35, (LONG)(unsigned long)fcb);
  sz = get_ran (fcb);
  result (sz == 10, "size 10");

  /* Truncate to 4 records */
  fill_name (fcb, "TRUNC   ", "DAT");
  set_ran (fcb, 4);
  r = bdos (99, (LONG)(unsigned long)fcb);
  result (r <= 3, "trunc 4");

  fill_name (fcb, "TRUNC   ", "DAT");
  bdos (35, (LONG)(unsigned long)fcb);
  sz = get_ran (fcb);
  result (sz == 4, "size now 4");

  /* Cannot extend via truncate */
  fill_name (fcb, "TRUNC   ", "DAT");
  set_ran (fcb, 20);
  r = bdos (99, (LONG)(unsigned long)fcb);
  result (r == 255, "no extend");

  /* Read remaining records */
  fill_name (fcb, "TRUNC   ", "DAT");
  r = bdos (15, (LONG)(unsigned long)fcb);
  bdos (26, (LONG)(unsigned long)dma);
  {
    int ok = 1;

    for (i = 0; i < 4; i++)
      {
        r = bdos (20, (LONG)(unsigned long)fcb);

        if (r != 0 || dma[0] != (UBYTE)('0' + i))
          {
            ok = 0;
          }
      }

    r = bdos (20, (LONG)(unsigned long)fcb);
    result (ok && r == 1, "read 4 + EOF");
  }

  bdos (16, (LONG)(unsigned long)fcb);

  /*
   * BDOS 40 WRITEZF: write random rec 32 (new block + zero-fill).
   * 2K blocks = 16 recs/block -> rec 32 starts a new block; 16-31 stay a hole.
   * Then write rec 47 (same block, last sector) so size covers 32-47;
   * recs 33-46 were zero-filled by F_WRITEZF and never rewritten.
   */

  fill_name (fcb, "TRUNC   ", "DAT");
  r = bdos (15, (LONG)(unsigned long)fcb);

  for (i = 0; i < 128; i++)
    {
      dma[i] = 0xAB;
    }

  dma[0] = 'Z';
  set_ran (fcb, 32);
  r = bdos (40, (LONG)(unsigned long)fcb);
  result (r == 0, "40 writezf@32");

  set_ran (fcb, 16);
  r = bdos (33, (LONG)(unsigned long)fcb);
  result (r == 1, "hole@16 EOF");

  set_ran (fcb, 32);

  for (i = 0; i < 128; i++)
    {
      dma[i] = 0;
    }

  r = bdos (33, (LONG)(unsigned long)fcb);
  result (r == 0 && dma[0] == 'Z' && dma[1] == 0xAB, "readzf@32");

  /* Extend size into the zero-filled tail of the same block */
  for (i = 0; i < 128; i++)
    {
      dma[i] = 0xCC;
    }

  dma[0] = 'T';
  set_ran (fcb, 47);
  r = bdos (34,
            (LONG)(unsigned long)fcb); /* normal random write, block exists */
  result (r == 0, "write@47 same blk");

  set_ran (fcb, 33);
  r = bdos (33, (LONG)(unsigned long)fcb);
  {
    int z = (r == 0);

    if (z)
      {
        for (i = 0; i < 128; i++)
          {
            if (dma[i] != 0)
              {
                z = 0;
              }
          }
      }

    result (z, "zf mid@33 zero");
  }

  bdos (16, (LONG)(unsigned long)fcb);

  /* Truncate sparse file back to 2 records */
  fill_name (fcb, "TRUNC   ", "DAT");
  set_ran (fcb, 2);
  r = bdos (99, (LONG)(unsigned long)fcb);
  result (r <= 3, "trunc sparse->2");
  fill_name (fcb, "TRUNC   ", "DAT");
  bdos (35, (LONG)(unsigned long)fcb);
  sz = get_ran (fcb);
  result (sz == 2, "size 2 after sparse trunc");

  puts ("fails=");
  putu (fails);
  puts ("\r\n");

  if (fails)
    {
      bdos (108, (LONG)fails); /* non-zero P_CODE for CCP */
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
