/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 072061b2-8c8b-11f1-b8c8-80ee73e9b8e7
 */

/*****************************************************************************/

/* alvtst.c: exercise BDOS 27 (Get Allocation Vector) against BDOS 31/46 */

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#define BDOS_INT 0x30

/*****************************************************************************/

/* Largest vector we ask for; dsm 2047 needs 256 bytes */
#define ALV_MAX 512
#define SENTINEL 0xA5

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

/* match struct dpb in bdosdef.h (BDOS 31 copies it here) */
struct dpb
{
  UWORD spt;
  UBYTE bsh;
  UBYTE blm;
  UBYTE exm;
  UBYTE dpbdum;
  UWORD dsm;
  UWORD drm;
  UWORD dir_al;
  UWORD cks;
  UWORD trk_off;
};

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
  char b [12];
  int i = 0;

  if (!n)
    {
      putch ('0');

      return;
    }

  while (n && i < 12)
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
      fcb [i] = 0;
    }

  for (i = 0; i < 8; i++)
    {
      fcb [1 + i] = (UBYTE)(n8 [i] ? n8 [i] : ' ');
    }

  for (i = 0; i < 3; i++)
    {
      fcb [9 + i] = (UBYTE)(t3 [i] ? t3 [i] : ' ');
    }
}

/*****************************************************************************/

static UBYTE alv [ALV_MAX];
static struct dpb parm;
static UBYTE dma [128];

/*****************************************************************************/

/*
 * Bit order matches setaloc()/chkaloc() in dskutil.c: byte bitnum >> 3,
 * mask 0x80 >> (bitnum & 7).  A set bit means the block is allocated.
 */

static int
blk_allocated (UWORD blk)
{
  return (alv [blk >> 3] & (0x80 >> (blk & 7))) != 0;
}

/*****************************************************************************/

/* Fetch the vector; returns the length BDOS 27 reported. */
static UWORD
get_alv (void)
{
  int i;

  for (i = 0; i < ALV_MAX; i++)
    {
      alv [i] = SENTINEL;
    }

  return bdos (27, (LONG)(ULONG)alv);
}

/*****************************************************************************/

/* Free blocks according to the vector we were just handed. */
static ULONG
count_free (UWORD dsm)
{
  ULONG n = 0;
  ULONG i;

  for (i = 0; i <= (ULONG)dsm; i++)
    {
      if (!blk_allocated ((UWORD)i))
        {
          n++;
        }
    }

  return n;
}

/*****************************************************************************/

/* BDOS 46 free space, in 128 byte records, via the DMA buffer. */
static ULONG
free_records (void)
{
  int i;

  for (i = 0; i < 128; i++)
    {
      dma [i] = 0;
    }

  (void)bdos (26, (LONG)(ULONG)dma);
  (void)bdos (46, 0);

  return (ULONG)dma [0]
      | ((ULONG)dma [1] << 8)
      | ((ULONG)dma [2] << 16)
      | ((ULONG)dma [3] << 24);
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  UBYTE fcb [36];
  UWORD len, explen;
  ULONG free0, free1, free2;
  ULONG recs;
  int i;

  fails = 0;
  puts ("\r\nALVTST (BDOS 27 vs BDOS 31/46)\r\n");

  (void)bdos (14, 0); /* select A: */

  /* DPB first: it defines the expected vector length and block size. */
  for (i = 0; i < (int)sizeof (parm); i++)
    {
      ((UBYTE *)&parm) [i] = 0;
    }

  (void)bdos (31, (LONG)(ULONG)&parm);
  result (parm.dsm != 0, "31 dpb");

  explen = (UWORD)((parm.dsm >> 3) + 1);
  puts ("  dsm=");
  putu (parm.dsm);
  puts (" blocksize=");
  putu ((ULONG)(parm.blm + 1) * 128UL);
  puts (" alvlen=");
  putu (explen);
  puts ("\r\n");

  result (explen <= ALV_MAX, "alv fits buffer");

  if (explen > ALV_MAX)
    {
      puts ("Aborting: vector larger than test buffer\r\n");
      (void)bdos (0, 0);
    }

  /* Reported length must match the (dsm >> 3) + 1 contract. */
  len = get_alv ();
  result (len == explen, "27 length");

  /* Nothing outside the reported length may be touched. */
  result (alv [explen] == SENTINEL, "27 no overrun");

  /*
   * Block 0 holds the directory on this layout (dir_al reserves the first
   * blocks), so it must never read as free.
   */

  result (blk_allocated (0), "27 dir block allocated");

  /*
   * Cross-check against BDOS 46, which walks the same live vector inside
   * the kernel.  free blocks * records per block must agree exactly.
   */

  free0 = count_free (parm.dsm);
  recs = free_records ();
  puts ("  free blocks=");
  putu (free0);
  puts (" bdos46 records=");
  putu (recs);
  puts ("\r\n");
  result (recs == free0 * (ULONG)(parm.blm + 1), "27 agrees with 46");

  /*
   * The vector must be live, not a snapshot taken at mount: allocating
   * has to shrink it and erasing has to give the blocks back.
   */

  fill_name (fcb, "ALVTST  ", "TMP");
  (void)bdos (19, (LONG)(ULONG)fcb); /* remove any leftover */

  fill_name (fcb, "ALVTST  ", "TMP");
  result (bdos (22, (LONG)(ULONG)fcb) <= 3, "make");

  (void)bdos (26, (LONG)(ULONG)dma);

  /* Write enough records to be certain at least one block is claimed. */
  for (i = 0; i < 16; i++)
    {
      int j;

      for (j = 0; j < 128; j++)
        {
          dma [j] = (UBYTE)i;
        }

      if (bdos (21, (LONG)(ULONG)fcb) != 0)
        {
          break;
        }
    }

  result (i == 16, "write 16 records");
  result (bdos (16, (LONG)(ULONG)fcb) <= 3, "close");

  len = get_alv ();
  free1 = count_free (parm.dsm);
  puts ("  free after write=");
  putu (free1);
  puts ("\r\n");
  result (len == explen, "27 length after write");
  result (free1 < free0, "27 tracks allocation");

  /* And BDOS 46 must have moved by the same amount. */
  recs = free_records ();
  result (recs == free1 * (ULONG)(parm.blm + 1), "27 agrees with 46 (used)");

  fill_name (fcb, "ALVTST  ", "TMP");
  result (bdos (19, (LONG)(ULONG)fcb) <= 3, "delete");

  len = get_alv ();
  free2 = count_free (parm.dsm);
  puts ("  free after delete=");
  putu (free2);
  puts ("\r\n");
  result (free2 == free0, "27 tracks free");

  puts (fails ? "\r\nALVTST: FAILED (" : "\r\nALVTST: passed (");
  putu (fails);
  puts (fails ? " failures)\r\n" : " failures)\r\n");

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
