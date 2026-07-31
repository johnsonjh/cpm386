/*
 * CP/M-386
 * Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
 * SPDX-License-Identifier: MIT
 * scspell-id: 83d226d6-8caf-11f1-83e4-80ee73e9b8e7
 */

/*****************************************************************************/

/* vgafont.c - load a console font, or restore the ROM font (BDOS 232) */

/*****************************************************************************/

#include "absaddr.h"

/*****************************************************************************/

typedef unsigned short UWORD;
typedef short WORD;
typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned char UBYTE;

/*****************************************************************************/

#define BDOS_INT 0x30

#define BDOS_OPEN 15
#define BDOS_CLOSE 16
#define BDOS_READ_SEQ 20
#define BDOS_SET_DMA 26
#define BDOS_VID_QUERY 229
#define BDOS_VID_FONT 232

/*****************************************************************************/

/* Must match vidmode.h */

#define VIDR_OK 0x0000
#define VIDR_BADFONT 0xFFFA
#define VIDR_FAILED 0xFFFB
#define VIDR_BADMODE 0xFFFD
#define VIDR_NOHW 0xFFFE
#define VIDR_BADPTR 0xFFFF

/*****************************************************************************/

#define DEF_FCB ((UBYTE *)abs_ptr (0x5C))
#define CMD_TAIL ((UBYTE *)abs_ptr (0x80))

#define GLYPHS 256
#define MAX_HEIGHT 32

/*****************************************************************************/

void _start (void) __attribute__ ((section (".text._start")));

/*****************************************************************************/

/* Must match struct cpm_vidmode in bdosdef.h */
struct vidmode
{
  UWORD index;
  UWORD mode;
  UWORD flags;
  UWORD cols;
  UWORD rows;
  UWORD cell_bytes;
  UWORD width;
  UWORD height;
  UWORD bpp;
  UWORD sel;
  ULONG pitch;
  ULONG fb_size;
  ULONG fb_phys;
};

/* Must match struct cpm_vidfont in bdosdef.h */
struct vidfont
{
  ULONG data;
  UWORD height;
  UWORD count;
  UWORD first;
  UWORD flags;
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

static void
zero (void *p, int n)
{
  int i;

  for (i = 0; i < n; i++)
    {
      ((UBYTE *)p) [i] = 0;
    }
}

/*****************************************************************************/

static UBYTE glyphs [GLYPHS * MAX_HEIGHT];
static UBYTE dma [128];
static UBYTE fcb [36];

/*****************************************************************************/

static void
usage (void)
{
  puts ("Usage: VGAFONT [-h] [DEFAULT | filename]\r\n");
  puts ("  (no args)  show the current font geometry\r\n");
  puts ("  DEFAULT    restore the adapter ROM font\r\n");
  puts ("  filename   load glyphs from a raw font file\r\n");
  puts ("\r\n");
  puts ("A font file is 256 glyphs of equal height, no header:\r\n");
  puts ("  2048 bytes = 8x8, 3584 = 8x14, 4096 = 8x16\r\n");
  puts ("The height must match the current text mode; use TEXTMODE\r\n");
  puts ("to select a mode with the cell height you want first.\r\n");
}

/*****************************************************************************/

static void
report (UWORD r)
{
  switch (r)
    {
    case VIDR_NOHW:
      puts ("No video adapter present\r\n");

      break;

    case VIDR_BADFONT:
      puts ("Font height does not match the current text mode\r\n");

      break;

    case VIDR_BADPTR:
      puts ("BDOS rejected the request\r\n");

      break;

    default:
      puts ("Font load failed\r\n");

      break;
    }
}

/*****************************************************************************/

static unsigned
cur_cell_height (void)
{
  struct vidmode v;

  zero (&v, sizeof (v));

  if (bdos (BDOS_VID_QUERY, (LONG)(ULONG)&v) != VIDR_OK)
    {
      return 0;
    }

  /*
   * The mode reports rows and the plane is 2 bytes per cell; the cell
   * height itself is whatever the file has to match, so derive it from the
   * standard 400 and 350 line text timings.
   */

  if (v.rows == 0)
    {
      return 0;
    }

  if (v.rows == 43)
    {
      return 8; /* 350 lines */
    }

  return (unsigned)(400u / v.rows);
}

/*****************************************************************************/

/* Read the whole file into glyphs[]; returns bytes read, or 0 on error. */

static ULONG
load_file (void)
{
  ULONG got = 0;

  if (bdos (BDOS_OPEN, (LONG)(ULONG)fcb) > 3)
    {
      puts ("No file\r\n");

      return 0;
    }

  fcb [32] = 0; /* sequential from record 0 */

  for (;;)
    {
      ULONG i;

      (void)bdos (BDOS_SET_DMA, (LONG)(ULONG)dma);

      if (bdos (BDOS_READ_SEQ, (LONG)(ULONG)fcb) != 0)
        {
          break;
        }

      for (i = 0; i < 128; i++)
        {
          if (got < sizeof (glyphs))
            {
              glyphs [got++] = dma [i];
            }
        }

      if (got >= sizeof (glyphs))
        {
          break;
        }
    }

  (void)bdos (BDOS_CLOSE, (LONG)(ULONG)fcb);

  return got;
}

/*****************************************************************************/

void
_start (void) /*cppcheck-suppress unusedFunction*/
{
  char tail [128];
  unsigned tlen, i;
  struct vidfont f;
  UWORD r;

  tlen = CMD_TAIL [0];

  if (tlen > 126)
    {
      tlen = 126;
    }

  for (i = 0; i < tlen; i++)
    {
      char c = (char)CMD_TAIL [1 + i];

      tail [i] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
    }

  tail [tlen] = 0;

  i = 0;

  while (tail [i] == ' ' || tail [i] == '\t')
    {
      i++;
    }

  puts ("\r\n");

  if (tail [i] == '-' || tail [i] == '/')
    {
      usage ();
      (void)bdos (0, 0);
    }

  /* No argument: describe what is in use now. */
  if (!tail [i])
    {
      unsigned h = cur_cell_height ();
      struct vidmode v;

      zero (&v, sizeof (v));

      if (bdos (BDOS_VID_QUERY, (LONG)(ULONG)&v) != VIDR_OK)
        {
          puts ("No video adapter present\r\n");
          (void)bdos (0, 0);
        }

      puts ("Text mode ");
      putu (v.mode);
      puts (" (");
      putu (v.cols);
      puts ("x");
      putu (v.rows);
      puts ("), cell 8x");
      putu (h);
      puts (", font file would be ");
      putu ((ULONG)GLYPHS * h);
      puts (" bytes\r\n");
      (void)bdos (0, 0);
    }

  zero (&f, sizeof (f));

  /* DEFAULT restores the ROM font. */
  if (tail [i] == 'D' && tail [i + 1] == 'E' && tail [i + 2] == 'F'
      && tail [i + 3] == 'A' && tail [i + 4] == 'U' && tail [i + 5] == 'L'
      && tail [i + 6] == 'T')
    {
      f.data = 0;
      r = bdos (BDOS_VID_FONT, (LONG)(ULONG)&f);

      if (r != VIDR_OK)
        {
          report (r);
        }
      else
        {
          puts ("ROM font restored\r\n");
        }

      (void)bdos (0, 0);
    }

  /*
   * A filename.  The CCP has already parsed the first argument into the
   * default FCB at 0x5C, so use that rather than re-parsing the tail.
   */

  for (i = 0; i < 36; i++)
    {
      fcb [i] = 0;
    }

  for (i = 0; i < 12; i++)
    {
      fcb [i] = DEF_FCB [i];
    }

  {
    ULONG got = load_file ();
    unsigned h;

    if (got == 0)
      {
        (void)bdos (0, 0);
      }

    h = (unsigned)(got / GLYPHS);

    if (h == 0 || h > MAX_HEIGHT || h * GLYPHS != got)
      {
        puts ("Not a 256 glyph font file (");
        putu (got);
        puts (" bytes)\r\n");
        (void)bdos (0, 0);
      }

    f.data = (ULONG)(unsigned long)glyphs;
    f.height = (UWORD)h;
    f.count = GLYPHS;
    f.first = 0;
    r = bdos (BDOS_VID_FONT, (LONG)(ULONG)&f);

    if (r != VIDR_OK)
      {
        report (r);

        if (r == VIDR_BADFONT)
          {
            puts ("  file is 8x");
            putu (h);
            puts (", mode wants 8x");
            putu (cur_cell_height ());
            puts ("\r\n");
          }

        (void)bdos (0, 0);
      }

    puts ("Loaded 256 glyphs of 8x");
    putu (h);
    puts ("\r\n");
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

/******************************************************************************/
/* vim: set ft=c ts=2 sw=2 tw=0 ai expandtab cc=80 : */
/******************************************************************************/
